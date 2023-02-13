//=- AArch64RegAllocLoop.cpp - Register Allocation for SWP -*- C++ -*---------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Register Allocation for SWP.
//
//===----------------------------------------------------------------------===//

#include "AArch64.h"
#include "AArch64TargetTransformInfo.h"
#include "../../CodeGen/SWPipeliner.h"
#include "../../CodeGen/SwplTransformMIR.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_set>

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"
#define UNAVAILABLE_REGS 7

static cl::opt<bool> DebugSwplRegAlloc("swpl-debug-reg-alloc",cl::init(false), cl::ReallyHidden);

// 使ってはいけない物理レジスタのリスト
static std::array<unsigned, UNAVAILABLE_REGS> nousePhysRegs {
    (unsigned)AArch64::SP,   // ゼロレジスタ(XZR)に等しい
    (unsigned)AArch64::FP,   // X29
    (unsigned)AArch64::LR,   // X30
    (unsigned)AArch64::WSP,  // ゼロレジスタ(WZR)に等しい
    (unsigned)AArch64::W29,  // FP(X29)相当
    (unsigned)AArch64::W30   // LR(X30)相当
};


/**
 * @brief  物理レジスタ割り付け対象外の仮想レジスタ一覧を作成する
 * @param  [in]     mi llvm::MachineInstr
 * @param  [in]     mo llvm::MachineOperand
 * @param  [in,out] ex_vreg 割り付け対象外仮想レジスタ
 * @param  [in]     reg getReg()で取得したレジスタ番号
 * @note   制約のある命令は、仮対処として、物理レジスタ割り付け対象外とする。
 */
static void createExcludeVReg(MachineInstr *mi, MachineOperand &mo,
                              std::unordered_set<unsigned> &ex_vreg, unsigned reg) {
  if (!Register::isVirtualRegister(reg)) {
    // 仮想レジスタでない
    return;
  }

  assert(mi);
  if (mo.getSubReg() != 0) {
    // sub-registerを持つレジスタなら除外リストに追加
    if( DebugSwplRegAlloc ) {
      dbgs() << "Excluded subreg.   MachineOperand: ";
      mo.print(dbgs());
      dbgs() << "\n";
    }
    ex_vreg.insert(reg);
  } else if (mo.isTied()) {
    // tied-defなら除外リストに追加
    if( DebugSwplRegAlloc ) {
      dbgs() << "Excluded tied-def.   MachineOperand: ";
      mo.print(dbgs());
      dbgs() << "\n";
    }
    ex_vreg.insert(reg);
  } else {
    // 対象のオペランドなら除外リストに追加
    switch (mi->getOpcode()) {
    case TargetOpcode::EXTRACT_SUBREG:
    case TargetOpcode::INSERT_SUBREG:
    case TargetOpcode::REG_SEQUENCE:
    case TargetOpcode::SUBREG_TO_REG:
      if( DebugSwplRegAlloc ) {
        dbgs() << "Excluded operand: " << mi->getOpcode()
                << ", MachineOperand: ";
        mo.print(dbgs());
        dbgs() << "\n";
      }
      ex_vreg.insert(reg);
      break;
    default:
      break;
    }
  }

  return;
}

/**
 * @brief  仮想レジスタごとに生存区間リストを作成する
 * @param  [in]     mo llvm::MachineOperand
 * @param  [in]     reg getReg()で取得したレジスタ番号
 * @param  [in,out] reginfo 割り当て済みレジスタ情報
 * @param  [in]     num_mi llvm::MachineInstrの通し番号
 * @param  [in]     total_mi llvm::MachineInstrの総数
 * @param  [in]     ex_vreg 割り付け対象外仮想レジスタ
 * @param  [in]     doVReg renaming後の制御変数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 */
static int createLiveRange(MachineOperand &mo, unsigned reg,
                           SwplRegAllocInfoTbl &rai_tbl,
                           int num_mi, int total_mi,
                           std::unordered_set<unsigned> &ex_vreg,
                           Register doVReg) {
  int ret = 0;

  /*
   * 以下に気を付ける
   * - liveinの場合はいきなり参照になる
   * - liveoutの場合
   * - 参照後、ループの冒頭に戻った後での定義関係
   * - 定義のみ(あるのか？)
   * - vectorレジスタ/subreg/複数参照
   * - 物理レジスタ領域の重なり($x0と$w0など)
   */

  // getReg()で取得したレジスタが仮想か物理かを判定する
  if (MCRegister::isPhysicalRegister(reg)) {
    // 物理レジスタである
    // 割り当て済みレジスタ情報を当該物理レジスタで検索
    if( !(rai_tbl.isUsePReg(reg)) ) {
      // 物理レジスタ割り付け前に登場する物理レジスタは使用することができない。
      // 無効な情報として、レジスタ情報になければ新規登録するのみ。
      rai_tbl.addRegAllocInfo(0, 1, total_mi, &mo, reg);
    }
  } else if (Register::isVirtualRegister(reg)) {
    // 仮想レジスタである

    // 除外リストに含まれていない限り、処理続行
    if (ex_vreg.find(reg) == ex_vreg.end()) {

      // 割り当て済みレジスタ情報を当該仮想レジスタで検索
      auto rai = rai_tbl.getWithVReg( reg );

      // TODO: 以下、同じような処理が続くため、関数化した方が良い
      // 当該オペランドが"定義"か
      if (mo.isDef()) {
        if (rai != nullptr) {
          // 同じ仮想レジスタの定義は、複数存在しないことが前提
          // 現在 assertとしているが、想定外としてエラーとすべきか。
          assert( rai->num_def == -1 );

          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          if (reg == doVReg) {
            // doVRegに該当する場合、当該仮想レジスタに割り当てる物理レジスタは
            // 再利用されて欲しくないため、liverangeをMAX値で更新
            rai->updateNumDefNoReuse( num_mi );
          } else {
            rai->updateNumDef( num_mi );
          }
          rai->addMo(&mo);
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, num_mi, -1, &mo );
        }
      }
      // 当該オペランドが"参照"か
      if (mo.isUse()) {
        if (rai != nullptr) {
          if( rai->num_def == -1 ||
              rai->num_use == -1 ||
              rai->num_use > rai->num_def ) {
            // 当該仮想レジスタは登録済みなのでMI通し番号を更新
            if (reg == doVReg) {
              // doVRegに該当する場合、当該仮想レジスタに割り当てる物理レジスタは
              // 再利用されて欲しくないため、liverangeをMAX値で更新
              rai->updateNumUseNoReuse( num_mi );
            } else {
              rai->updateNumUse( num_mi );
            }
          }
          rai->addMo(&mo);
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, -1, num_mi, &mo );
        }
      }
    }
  }  else {
    // do nothing. 仮想レジスタでも物理レジスタでもない
  }

  return ret;
}

/**
 * @brief  生存区間を基に仮想レジスタへ物理レジスタを割り付ける
 * @param  [in,out] reginfo 割り当て済みレジスタ情報
 * @param  [in]     total_mi カーネル部の総MI数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 * @details レジスタ再利用時の「最も過去」とは、「出現した順に古い方から」という意
 */
static int physRegAllocWithLiveRange(SwplRegAllocInfoTbl &rai_tbl, unsigned total_mi) {
  int ret = 0;

  // 割り当て済みレジスタ情報でループ
  for (size_t i = 0; i < rai_tbl.length(); i++) {
    RegAllocInfo *itr_cur =rai_tbl.getWithIdx(i);
    bool allocated = false;

    // 以下の仮想レジスタは、物理レジスタ割り付け対象としない
    // ・無効な仮想レジスタ（「無効な仮想レジスタ」ってなに？）
    // ・ブロック内に定義or参照のどちらかしかない
    // ・参照→定義の順に出現する仮想レジスタ
    if ((itr_cur->vreg == 0) || (!Register::isVirtualRegister(itr_cur->vreg)) ||
        (itr_cur->num_def < 0) || (itr_cur->num_use < 0) ||
        (itr_cur->num_use < itr_cur->num_def) ) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " This vreg is not subject to RA as it may be livein/liveout. ("
               << printReg(itr_cur->vreg, SWPipeliner::TRI) << ")\n";
      }
      continue;
    }

    // 第１候補：空いている物理レジスタから割り付け
    if (!allocated) {
      const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(itr_cur->vreg);
      for (auto preg:*trc) {
        // 物理レジスタ番号取得

        // 使ってはいけない物理レジスタは割付けない
        std::array<unsigned, UNAVAILABLE_REGS>::iterator itr_nousePreg;
        itr_nousePreg = std::find(nousePhysRegs.begin(), nousePhysRegs.end(), preg);        
        if (itr_nousePreg != nousePhysRegs.end()) continue;

        // 当該物理レジスタが割り当て可能なレジスタでなければcontinue
        if ((preg == 0) || (!SWPipeliner::MRI->isAllocatable(preg)))
          continue;

        // 割り当て済みリストに登録されていないなら当該物理レジスタを使用
        if( !(rai_tbl.isUsePReg( preg )) ) {
          itr_cur->preg = preg;
          allocated = true;
          break;
        }
      }
    }

    // 第２候補：空いている物理レジスタがなく割り当てできなかったら、
    // 過去に割り当て済みの物理レジスタの中から
    // 「生存区間終了している」かつ「最も過去に"定義"に割り当てたもの」から再利用
    if (!allocated) {
      unsigned preg = rai_tbl.getReusePReg( itr_cur );
      if( preg!=0 ) {
        itr_cur->preg = preg;
        allocated = true;
      }
    }

    // それでも物理レジスタ割り当てできなかったら異常復帰
    if (!allocated) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " failed to allocate a physical register for virtual register "
               << printReg(itr_cur->vreg) << "\n";
      }
      std::string mistr;
      raw_string_ostream mistream(mistr);

      mistream << "Failed to allocate physical register. VReg="
               << printReg(itr_cur->vreg, SWPipeliner::TRI, 0, SWPipeliner::MRI)
               << ':'
               << printRegClassOrBank(itr_cur->vreg, *SWPipeliner::MRI, SWPipeliner::TRI);
      SWPipeliner::Reason = mistr;
      ret = -1;
      break;
    }
  }

  return ret;
}

/**
 * @brief  レジスタ情報を基にMachineOperand::setReg()を呼び出す
 * @param  [in,out] reginfo 割り当て済みレジスタ情報
 */
static void callSetReg(SwplRegAllocInfoTbl &rai_tbl) {
  for (size_t i = 0; i < rai_tbl.length(); i++) {
    RegAllocInfo *rinfo = rai_tbl.getWithIdx(i);
    // 仮想レジスタと物理レジスタが有効な値でない限り何もしない
    if ((rinfo->vreg == 0) || (rinfo->preg == 0))
      continue;
    // 当該物理レジスタを使用するMachineOperandすべてにsetReg()する
    for (auto *mo : rinfo->vreg_mo) {
      mo->setReg(rinfo->preg);
      mo->setIsRenamable(true);
    }
  }
  return;
}

/**
 * @brief  カーネル部のMachineInstrをprintする
 * @param  [in] MF MachineFunction
 * @param  [in] start kernel部の開始インデックス
 * @param  [in] end kernel部の終了インデックス
 * @param  [in] mis mi_tableの情報
 */
static void dumpKernelInstrs(const MachineFunction &MF,
                             const size_t start, const size_t end,
                             const std::vector<MachineInstr*> mis) {
  for (size_t i = start; i < end; ++i) {
    MachineInstr *mi = mis[i];
    if (mi != nullptr) {
      /*
       * SWPLの結果反映中のMIは、MFから紐づいていないため、
       * mi->print(dbgs())でOpcodeがUNKNOWNで出力されてしまう。
       * よって、結果反映前のMFを用いる。
       */
      mi->print(dbgs(),
                /* IsStandalone= */ true,
                /* SkipOpers= */ false,
                /* SkipDebugLoc= */ false,
                /* AddNewLine= */ true,
                MF.getSubtarget().getInstrInfo() );
    }
  }
}

/**
 * @brief  SWPLpass内における仮想レジスタへの物理レジスタ割り付け
 * @param  [in,out] tmi 命令列変換情報
 * @param  [in]     MF  MachineFunction
 * @note   カーネルループのみが対象。spillが発生する場合は再スケジュール。
 */
bool AArch64InstrInfo::physRegAllocLoop(SwplTransformedMIRInfo *tmi,
                                        const MachineFunction &MF) const {
  const MachineInstr *firstMI = nullptr;
  const MachineInstr *lastMI  = nullptr;
  std::unordered_set<unsigned> exclude_vreg;

  /// kernel部のMachineInstrを対象とする
  /// まず初めに有効な総MI数を数えつつ、割り付け対象としないレジスタのリストを作成する
  unsigned total_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    if (firstMI == nullptr)
      firstMI = mi;
    ++total_mi;

    // MIのオペランド数でループ
    for (MachineOperand& mo:mi->operands()) {
      // レジスタオペランドなら以降の処理継続
      // レジスタ番号 0 はいかなる種類のレジスタも表していない
      unsigned reg = 0;
      if ((!mo.isReg()) || ((reg = mo.getReg()) == 0))
        continue;

      // TODO: 割り付け対象としないレジスタ一覧を作る(仮対処)
      createExcludeVReg(mi, mo, exclude_vreg, reg);
    }
  }

  if( DebugSwplRegAlloc  ) {
    dumpKernelInstrs(MF, tmi->prologEndIndx, tmi->kernelEndIndx, tmi->mis);
  }

  tmi->swplRAITbl = new SwplRegAllocInfoTbl(total_mi);

  unsigned num_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    ++num_mi;
    if (lastMI==nullptr && num_mi==total_mi)
      lastMI = tmi->mis[num_mi];

    // MIのオペランド数でループ
    for (MachineOperand& mo:mi->operands()) {
      // レジスタオペランドなら以降の処理継続
      // レジスタ番号 0 はいかなる種類のレジスタも表していない
      unsigned reg = 0;
      if ((!mo.isReg()) || ((reg = mo.getReg()) == 0))
        continue;

      // 各仮想レジスタの生存区間リストを作成する
      if (createLiveRange(mo, reg, *(tmi->swplRAITbl), num_mi,
                          total_mi, exclude_vreg, tmi->doVReg) != 0) {
        assert(0 && "createLiveRange() failed");
        return false;
      }
    }
  }

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete createLiveRange()\n";
    (*tmi->swplRAITbl).dump();
  }

  // 物理レジスタ情報を割り当てる
  if (physRegAllocWithLiveRange(*(tmi->swplRAITbl), total_mi) != 0) {
    // 割り当て失敗
    return false;
  }

  // setReg()呼び出し
  callSetReg(*(tmi->swplRAITbl));

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete callSetReg()\n";
    (*tmi->swplRAITbl).dump();
    dumpKernelInstrs(MF, tmi->prologEndIndx, tmi->kernelEndIndx, tmi->mis);
  }

  return true;
}

/**
 * @brief  定義点と参照点から生存区間を求める(再利用禁止版)
 * @details 仮想レジスタごとの生存区間を返す。
 *          定義 < 参照の場合、当該仮想レジスタに割り当たる物理レジスタの
 *          再利用を禁止するため、参照点を最大値(total_mi)にする。
 * @retval -1より大きい値 計算した生存区間
 * @retval -1            計算不能
 */
int RegAllocInfo::calcLiveRangeNoReuse() {
  if ((num_def != -1) && (num_use != -1) && (num_def < num_use)) {
    // 定義の時点で参照がある or 参照の時点で定義がある and
    // 定義の後に参照があるケースでは、再利用させないための値を設定する
    num_use = total_mi;
  }
  return calcLiveRange();
}

/**
 * @brief  定義点と参照点から生存区間を求める
 * @details 仮想レジスタごとの生存区間を求める。
 *          定義点、参照点のどちらかが存在しない場合、計算不能として-1を返す。
 * @retval -1より大きい値 計算した生存区間
 * @retval -1            計算不能
 */
int RegAllocInfo::calcLiveRange() {
  if ( num_def != -1 && num_use != -1 ) {
    // 定義の時点で参照がある or 参照の時点で定義がある
    if (num_def < num_use) {
      // 定義の後に参照があるケース
      return (num_use - num_def);
    } else {
      // 参照の後に定義があるケース
      return (total_mi - num_def + num_use);
    }
  }

  // 定義の時点で参照がない or 参照の時点で定義がない
  return -1;
}

/**
 * @brief  生存区間表の行のLiveRangeが重なるかを判定する
 * @param  [in]  reginfo1 比較対象のRegAllocInfo
 * @param  [in]  reginfo2 比較対象のRegAllocInfo
 * @retval true  LiveRangeが重なる
 * @retval false LiveRangeが重ならない
 */
bool SwplRegAllocInfoTbl::isOverlapLiveRange( RegAllocInfo *reginfo1, RegAllocInfo *reginfo2) {
  int def1 = reginfo1->num_def;
  int use1 = reginfo1->num_use;
  
  int def2 = reginfo2->num_def;
  int use2 = reginfo2->num_use;

  for(int i=0; i<(int)total_mi; i++) {
    bool overlap1 = false;
    bool overlap2 = false;
    if(def1>use1) {
      if( i<=use1 || i>=def1 ) overlap1=true;
    }
    else {
      if( def1<=i && i<=use1 ) overlap1=true;
    }
    
    if(def2>use2) {
      if( i<=use2 || i>=def2 ) overlap2=true;
    }
    else {
      if( def2<=i && i<=use2 ) overlap2=true;
    }
      
    if( overlap1 && overlap2 ) {
      return true;
    }
  }
  return false;
}

/**
 * @brief  SwplRegAllocInfoTblにRegAllocInfoエントリを追加する
 * @param  [in] vreg 仮想レジスタ番号
 * @param  [in] num_def 定義番号
 * @param  [in] num_use 参照番号
 * @param  [in] mo llvm::MachineOperand
 * @param  [in] preg 物理レジスタ番号
 */
void SwplRegAllocInfoTbl::addRegAllocInfo( unsigned vreg,
                                           int num_def,
                                           int num_use,
                                           MachineOperand *mo,
                                           unsigned preg) {
  const TargetRegisterClass *trc;

  assert(mo);

  unsigned clsid = 0;
  if( vreg != 0 ) {
    trc = SWPipeliner::MRI->getRegClass(vreg);
    clsid = trc->getID();
  }
  rai_tbl.push_back({vreg, preg, num_def, num_use, -1, clsid, {mo}, total_mi});
  return;
}

/**
 * @brief  テーブルの中から、指定された仮想レジスタに該当するエントリを返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval 非nullptr 指定された仮想レジスタに該当するRegAllocInfoへのポインタ
 * @retval nullptr   指定された仮想レジスタに該当するRegAllocInfoは存在しない
 */
RegAllocInfo* SwplRegAllocInfoTbl::getWithVReg( unsigned vreg ) {
  auto itr =
    find_if(rai_tbl.begin(), rai_tbl.end(),
            [&](RegAllocInfo &info){
              return(info.vreg == vreg);
            });
  if (itr == rai_tbl.end())
    return nullptr;
  
  return &(*itr);
}

/**
 * @brief  テーブルの中から、再利用可能な物理レジスタ番号を返す
 * @param  [in] rai 割り当て対象のRegAllocInfo
 * @retval 非0 再利用可能な物理レジスタ番号
 * @retval 0   再利用可能な物理レジスタは見つからなかった
 */
unsigned SwplRegAllocInfoTbl::getReusePReg( RegAllocInfo* rai ) {
  assert(rai->preg==0);

  // TODO：現状、翻訳性能を考慮しない作りとなっている。
  // TODO：現在は、表の上から見つかったものを返している。
  //       どの行を再利用対象にするかの論理を入れたい。
  // TODO：物理レジスタに生存区間を紐づけたようなデータを活用するような形で、
  //       この検索を早く実施できるようにしたい。

  // レジスタクラスに該当するレジスタごとに、
  // 既に割り当て済みの行を収集し、そのすべての生存区間が重なっていなければ、
  // 再利用可能とする。
  const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(rai->vreg);
  for (auto itr = trc->begin(), itr_end = trc->end();
       itr != itr_end; ++itr) {
    unsigned candidate_preg = *itr;
    std::vector<RegAllocInfo*> ranges;
    
    for (size_t j = 0; j < rai_tbl.size(); j++) {
      RegAllocInfo *wk_reginfo = &(rai_tbl[j]);
      if( isPRegOverlap( candidate_preg, wk_reginfo->preg ) ) {
        ranges.push_back( wk_reginfo );
      }
    }

    bool isoverlap = false;
    for(unsigned i=0; i<ranges.size(); i++) {
      if( isOverlapLiveRange(ranges[i], rai) ) {
        isoverlap = true;
        break;
      }
    }
    
    if(isoverlap == false) {
      return candidate_preg;
    }
  }
  
  return 0;
}

/**
 * @brief  二つの物理レジスタの物理領域が重複するかを判定する
 * @param  [in] preg1 チェック対象の物理レジスタ
 * @param  [in] preg2 チェック対象の物理レジスタ
 * @retval true  重複している
 * @retval false 重複していない
 */
bool SwplRegAllocInfoTbl::isPRegOverlap( unsigned preg1, unsigned preg2 ) {
  if( preg1 == preg2 ) return true;

  auto q = regconstrain[preg1];
  if( std::find(q.begin(), q.end(), preg2) != q.end() ) {
    return true;
  }

  return false;
}

/**
 * @brief  指定された物理レジスタが割り当てに使用されているか否かを判定する
 * @param  [in] preg チェック対象の物理レジスタ
 * @retval true  使用されている
 * @retval false 使用されていない
 */
bool SwplRegAllocInfoTbl::isUsePReg( unsigned preg ) {
  for(unsigned i=0; i<rai_tbl.size(); i++) {
    unsigned usedpreg = rai_tbl[i].preg;
    if( preg == usedpreg ) return true;

    // 物理的に重複するレジスタも、利用済みとして扱う
    if( regconstrain.count(usedpreg) > 0 ) {
      auto q = regconstrain[usedpreg];
      if( std::find(q.begin(), q.end(), preg) != q.end() )
        return true;
    }
  }
  return false;
}

/**
 * @brief  レジスタ割り当て情報 SwplRegAllocInfoTbl のデバッグプリント
 */
void SwplRegAllocInfoTbl::dump() {
  dbgs() << "No.\tvreg\tpreg\tdef\tuse\trange\tclass\tMO\n";
  for (size_t i = 0; i < rai_tbl.size(); i++) {
    RegAllocInfo *ri_p = &(rai_tbl[i]);
    dbgs() << i << "\t"
           << printReg(ri_p->vreg, SWPipeliner::TRI) << "\t" // 割り当て済み仮想レジスタ
           << printReg(ri_p->preg, SWPipeliner::TRI) << "\t" // 割り当て済み物理レジスタ
           << ri_p->num_def << "\t"             // 仮想レジスタの定義番号
           << ri_p->num_use << "\t"             // 仮想レジスタの参照番号
           << ri_p->liverange  << "\t";         // 生存区間(参照番号-定義番号)
    // 仮想レジスタのレジスタクラス
    if (ri_p->vreg > 0) {
      const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(ri_p->vreg);
      const char* reg_class_name = SWPipeliner::TRI->getRegClassName(trc);
      dbgs() << reg_class_name << "\t";
    } else {
      dbgs() << ri_p->vreg_classid  << "\t";
    }
    dbgs() << "[ ";
    for (size_t l = 0; l < ri_p->vreg_mo.size(); l++) {
      // 仮想レジスタのMachineOperand
      MachineOperand *m = ri_p->vreg_mo[l];
      // レジスタオペランドなら出力内容を分かり易くする
      if (m->isReg()) {
        dbgs() << printReg(m->getReg(), SWPipeliner::TRI);
      } else {
        m->print(dbgs());
      }
      dbgs() << " ";
    }
    dbgs() << " ]\n";
  }
  return;
}

SwplRegAllocInfoTbl::SwplRegAllocInfoTbl(unsigned num_of_mi) {
  total_mi=num_of_mi;
  rai_tbl={};

  regconstrain[AArch64::W0]  = { AArch64::X0 };
  regconstrain[AArch64::W1]  = { AArch64::X1 };
  regconstrain[AArch64::W2]  = { AArch64::X2 };
  regconstrain[AArch64::W3]  = { AArch64::X3 };
  regconstrain[AArch64::W4]  = { AArch64::X4 };
  regconstrain[AArch64::W5]  = { AArch64::X5 };
  regconstrain[AArch64::W6]  = { AArch64::X6 };
  regconstrain[AArch64::W7]  = { AArch64::X7 };
  regconstrain[AArch64::W8]  = { AArch64::X8 };
  regconstrain[AArch64::W9]  = { AArch64::X9 };
  regconstrain[AArch64::W10] = { AArch64::X10 };
  regconstrain[AArch64::W11] = { AArch64::X11 };
  regconstrain[AArch64::W12] = { AArch64::X12 };
  regconstrain[AArch64::W13] = { AArch64::X13 };
  regconstrain[AArch64::W14] = { AArch64::X14 };
  regconstrain[AArch64::W15] = { AArch64::X15 };
  regconstrain[AArch64::W16] = { AArch64::X16 };
  regconstrain[AArch64::W17] = { AArch64::X17 };
  regconstrain[AArch64::W18] = { AArch64::X18 };
  regconstrain[AArch64::W19] = { AArch64::X19 };
  regconstrain[AArch64::W20] = { AArch64::X20 };
  regconstrain[AArch64::W21] = { AArch64::X21 };
  regconstrain[AArch64::W22] = { AArch64::X22 };
  regconstrain[AArch64::W23] = { AArch64::X23 };
  regconstrain[AArch64::W24] = { AArch64::X24 };
  regconstrain[AArch64::W25] = { AArch64::X25 };
  regconstrain[AArch64::W26] = { AArch64::X26 };
  regconstrain[AArch64::W27] = { AArch64::X27 };
  regconstrain[AArch64::W28] = { AArch64::X28 };
  // W29-W30は割り付けに使用されない
  regconstrain[AArch64::WSP] = { AArch64::SP  };
  regconstrain[AArch64::WZR] = { AArch64::XZR };
  
  regconstrain[AArch64::X0]  = { AArch64::W0 };
  regconstrain[AArch64::X1]  = { AArch64::W1 };
  regconstrain[AArch64::X2]  = { AArch64::W2 };
  regconstrain[AArch64::X3]  = { AArch64::W3 };
  regconstrain[AArch64::X4]  = { AArch64::W4 };
  regconstrain[AArch64::X5]  = { AArch64::W5 };
  regconstrain[AArch64::X6]  = { AArch64::W6 };
  regconstrain[AArch64::X7]  = { AArch64::W7 };
  regconstrain[AArch64::X8]  = { AArch64::W8 };
  regconstrain[AArch64::X9]  = { AArch64::W9 };
  regconstrain[AArch64::X10] = { AArch64::W10 };
  regconstrain[AArch64::X11] = { AArch64::W11 };
  regconstrain[AArch64::X12] = { AArch64::W12 };
  regconstrain[AArch64::X13] = { AArch64::W13 };
  regconstrain[AArch64::X14] = { AArch64::W14 };
  regconstrain[AArch64::X15] = { AArch64::W15 };
  regconstrain[AArch64::X16] = { AArch64::W16 };
  regconstrain[AArch64::X17] = { AArch64::W17 };
  regconstrain[AArch64::X18] = { AArch64::W18 };
  regconstrain[AArch64::X19] = { AArch64::W19 };
  regconstrain[AArch64::X20] = { AArch64::W20 };
  regconstrain[AArch64::X21] = { AArch64::W21 };
  regconstrain[AArch64::X22] = { AArch64::W22 };
  regconstrain[AArch64::X23] = { AArch64::W23 };
  regconstrain[AArch64::X24] = { AArch64::W24 };
  regconstrain[AArch64::X25] = { AArch64::W25 };
  regconstrain[AArch64::X26] = { AArch64::W26 };
  regconstrain[AArch64::X27] = { AArch64::W27 };
  regconstrain[AArch64::X28] = { AArch64::W28 };
  regconstrain[AArch64::FP ] = { AArch64::W29 };
  regconstrain[AArch64::LR ] = { AArch64::W30 };
  regconstrain[AArch64::SP ] = { AArch64::WSP };
  regconstrain[AArch64::XZR] = { AArch64::WZR };

  regconstrain[AArch64::B0]  = { AArch64::H0,  AArch64::S0,  AArch64::D0,  AArch64::Q0,  AArch64::Z0_HI,  AArch64::Z0  };
  regconstrain[AArch64::B1]  = { AArch64::H1,  AArch64::S1,  AArch64::D1,  AArch64::Q1,  AArch64::Z1_HI,  AArch64::Z1  };
  regconstrain[AArch64::B2]  = { AArch64::H2,  AArch64::S2,  AArch64::D2,  AArch64::Q2,  AArch64::Z2_HI,  AArch64::Z2  };
  regconstrain[AArch64::B3]  = { AArch64::H3,  AArch64::S3,  AArch64::D3,  AArch64::Q3,  AArch64::Z3_HI,  AArch64::Z3  };
  regconstrain[AArch64::B4]  = { AArch64::H4,  AArch64::S4,  AArch64::D4,  AArch64::Q4,  AArch64::Z4_HI,  AArch64::Z4  };
  regconstrain[AArch64::B5]  = { AArch64::H5,  AArch64::S5,  AArch64::D5,  AArch64::Q5,  AArch64::Z5_HI,  AArch64::Z5  };
  regconstrain[AArch64::B6]  = { AArch64::H6,  AArch64::S6,  AArch64::D6,  AArch64::Q6,  AArch64::Z6_HI,  AArch64::Z6  };
  regconstrain[AArch64::B7]  = { AArch64::H7,  AArch64::S7,  AArch64::D7,  AArch64::Q7,  AArch64::Z7_HI,  AArch64::Z7  };
  regconstrain[AArch64::B8]  = { AArch64::H8,  AArch64::S8,  AArch64::D8,  AArch64::Q8,  AArch64::Z8_HI,  AArch64::Z8  };
  regconstrain[AArch64::B9]  = { AArch64::H9,  AArch64::S9,  AArch64::D9,  AArch64::Q9,  AArch64::Z9_HI,  AArch64::Z9  };
  regconstrain[AArch64::B10] = { AArch64::H10, AArch64::S10, AArch64::D10, AArch64::Q10, AArch64::Z10_HI, AArch64::Z10 };
  regconstrain[AArch64::B11] = { AArch64::H11, AArch64::S11, AArch64::D11, AArch64::Q11, AArch64::Z11_HI, AArch64::Z11 };
  regconstrain[AArch64::B12] = { AArch64::H12, AArch64::S12, AArch64::D12, AArch64::Q12, AArch64::Z12_HI, AArch64::Z12 };
  regconstrain[AArch64::B13] = { AArch64::H13, AArch64::S13, AArch64::D13, AArch64::Q13, AArch64::Z13_HI, AArch64::Z13 };
  regconstrain[AArch64::B14] = { AArch64::H14, AArch64::S14, AArch64::D14, AArch64::Q14, AArch64::Z14_HI, AArch64::Z14 };
  regconstrain[AArch64::B15] = { AArch64::H15, AArch64::S15, AArch64::D15, AArch64::Q15, AArch64::Z15_HI, AArch64::Z15 };
  regconstrain[AArch64::B16] = { AArch64::H16, AArch64::S16, AArch64::D16, AArch64::Q16, AArch64::Z16_HI, AArch64::Z16 };
  regconstrain[AArch64::B17] = { AArch64::H17, AArch64::S17, AArch64::D17, AArch64::Q17, AArch64::Z17_HI, AArch64::Z17 };
  regconstrain[AArch64::B18] = { AArch64::H18, AArch64::S18, AArch64::D18, AArch64::Q18, AArch64::Z18_HI, AArch64::Z18 };
  regconstrain[AArch64::B19] = { AArch64::H19, AArch64::S19, AArch64::D19, AArch64::Q19, AArch64::Z19_HI, AArch64::Z19 };
  regconstrain[AArch64::B20] = { AArch64::H20, AArch64::S20, AArch64::D20, AArch64::Q20, AArch64::Z20_HI, AArch64::Z20 };
  regconstrain[AArch64::B21] = { AArch64::H21, AArch64::S21, AArch64::D21, AArch64::Q21, AArch64::Z21_HI, AArch64::Z21 };
  regconstrain[AArch64::B22] = { AArch64::H22, AArch64::S22, AArch64::D22, AArch64::Q22, AArch64::Z22_HI, AArch64::Z22 };
  regconstrain[AArch64::B23] = { AArch64::H23, AArch64::S23, AArch64::D23, AArch64::Q23, AArch64::Z23_HI, AArch64::Z23 };
  regconstrain[AArch64::B24] = { AArch64::H24, AArch64::S24, AArch64::D24, AArch64::Q24, AArch64::Z24_HI, AArch64::Z24 };
  regconstrain[AArch64::B25] = { AArch64::H25, AArch64::S25, AArch64::D25, AArch64::Q25, AArch64::Z25_HI, AArch64::Z25 };
  regconstrain[AArch64::B26] = { AArch64::H26, AArch64::S26, AArch64::D26, AArch64::Q26, AArch64::Z26_HI, AArch64::Z26 };
  regconstrain[AArch64::B27] = { AArch64::H27, AArch64::S27, AArch64::D27, AArch64::Q27, AArch64::Z27_HI, AArch64::Z27 };
  regconstrain[AArch64::B28] = { AArch64::H28, AArch64::S28, AArch64::D28, AArch64::Q28, AArch64::Z28_HI, AArch64::Z28 };
  regconstrain[AArch64::B29] = { AArch64::H29, AArch64::S29, AArch64::D29, AArch64::Q29, AArch64::Z29_HI, AArch64::Z29 };
  regconstrain[AArch64::B30] = { AArch64::H30, AArch64::S30, AArch64::D30, AArch64::Q30, AArch64::Z30_HI, AArch64::Z30 };
  regconstrain[AArch64::B31] = { AArch64::H31, AArch64::S31, AArch64::D31, AArch64::Q31, AArch64::Z31_HI, AArch64::Z31 };

  regconstrain[AArch64::S0]  = { AArch64::B0,  AArch64::H0,  AArch64::D0,  AArch64::Q0,  AArch64::Z0_HI,  AArch64::Z0 };
  regconstrain[AArch64::S1]  = { AArch64::B1,  AArch64::H1,  AArch64::D1,  AArch64::Q1,  AArch64::Z1_HI,  AArch64::Z1 };
  regconstrain[AArch64::S2]  = { AArch64::B2,  AArch64::H2,  AArch64::D2,  AArch64::Q2,  AArch64::Z2_HI,  AArch64::Z2 };
  regconstrain[AArch64::S3]  = { AArch64::B3,  AArch64::H3,  AArch64::D3,  AArch64::Q3,  AArch64::Z3_HI,  AArch64::Z3 };
  regconstrain[AArch64::S4]  = { AArch64::B4,  AArch64::H4,  AArch64::D4,  AArch64::Q4,  AArch64::Z4_HI,  AArch64::Z4 };
  regconstrain[AArch64::S5]  = { AArch64::B5,  AArch64::H5,  AArch64::D5,  AArch64::Q5,  AArch64::Z5_HI,  AArch64::Z5 };
  regconstrain[AArch64::S6]  = { AArch64::B6,  AArch64::H6,  AArch64::D6,  AArch64::Q6,  AArch64::Z6_HI,  AArch64::Z6 };
  regconstrain[AArch64::S7]  = { AArch64::B7,  AArch64::H7,  AArch64::D7,  AArch64::Q7,  AArch64::Z7_HI,  AArch64::Z7 };
  regconstrain[AArch64::S8]  = { AArch64::B8,  AArch64::H8,  AArch64::D8,  AArch64::Q8,  AArch64::Z8_HI,  AArch64::Z8 };
  regconstrain[AArch64::S9]  = { AArch64::B9,  AArch64::H9,  AArch64::D9,  AArch64::Q9,  AArch64::Z9_HI,  AArch64::Z9 };
  regconstrain[AArch64::S10] = { AArch64::B10, AArch64::H10, AArch64::D10, AArch64::Q10, AArch64::Z10_HI, AArch64::Z10 };
  regconstrain[AArch64::S11] = { AArch64::B11, AArch64::H11, AArch64::D11, AArch64::Q11, AArch64::Z11_HI, AArch64::Z11 };
  regconstrain[AArch64::S12] = { AArch64::B12, AArch64::H12, AArch64::D12, AArch64::Q12, AArch64::Z12_HI, AArch64::Z12 };
  regconstrain[AArch64::S13] = { AArch64::B13, AArch64::H13, AArch64::D13, AArch64::Q13, AArch64::Z13_HI, AArch64::Z13 };
  regconstrain[AArch64::S14] = { AArch64::B14, AArch64::H14, AArch64::D14, AArch64::Q14, AArch64::Z14_HI, AArch64::Z14 };
  regconstrain[AArch64::S15] = { AArch64::B15, AArch64::H15, AArch64::D15, AArch64::Q15, AArch64::Z15_HI, AArch64::Z15 };
  regconstrain[AArch64::S16] = { AArch64::B16, AArch64::H16, AArch64::D16, AArch64::Q16, AArch64::Z16_HI, AArch64::Z16 };
  regconstrain[AArch64::S17] = { AArch64::B17, AArch64::H17, AArch64::D17, AArch64::Q17, AArch64::Z17_HI, AArch64::Z17 };
  regconstrain[AArch64::S18] = { AArch64::B18, AArch64::H18, AArch64::D18, AArch64::Q18, AArch64::Z18_HI, AArch64::Z18 };
  regconstrain[AArch64::S19] = { AArch64::B19, AArch64::H19, AArch64::D19, AArch64::Q19, AArch64::Z19_HI, AArch64::Z19 };
  regconstrain[AArch64::S20] = { AArch64::B20, AArch64::H20, AArch64::D20, AArch64::Q20, AArch64::Z20_HI, AArch64::Z20 };
  regconstrain[AArch64::S21] = { AArch64::B21, AArch64::H21, AArch64::D21, AArch64::Q21, AArch64::Z21_HI, AArch64::Z21 };
  regconstrain[AArch64::S22] = { AArch64::B22, AArch64::H22, AArch64::D22, AArch64::Q22, AArch64::Z22_HI, AArch64::Z22 };
  regconstrain[AArch64::S23] = { AArch64::B23, AArch64::H23, AArch64::D23, AArch64::Q23, AArch64::Z23_HI, AArch64::Z23 };
  regconstrain[AArch64::S24] = { AArch64::B24, AArch64::H24, AArch64::D24, AArch64::Q24, AArch64::Z24_HI, AArch64::Z24 };
  regconstrain[AArch64::S25] = { AArch64::B25, AArch64::H25, AArch64::D25, AArch64::Q25, AArch64::Z25_HI, AArch64::Z25 };
  regconstrain[AArch64::S26] = { AArch64::B26, AArch64::H26, AArch64::D26, AArch64::Q26, AArch64::Z26_HI, AArch64::Z26 };
  regconstrain[AArch64::S27] = { AArch64::B27, AArch64::H27, AArch64::D27, AArch64::Q27, AArch64::Z27_HI, AArch64::Z27 };
  regconstrain[AArch64::S28] = { AArch64::B28, AArch64::H28, AArch64::D28, AArch64::Q28, AArch64::Z28_HI, AArch64::Z28 };
  regconstrain[AArch64::S29] = { AArch64::B29, AArch64::H29, AArch64::D29, AArch64::Q29, AArch64::Z29_HI, AArch64::Z29 };
  regconstrain[AArch64::S30] = { AArch64::B30, AArch64::H30, AArch64::D30, AArch64::Q30, AArch64::Z30_HI, AArch64::Z30 };
  regconstrain[AArch64::S31] = { AArch64::B31, AArch64::H31, AArch64::D31, AArch64::Q31, AArch64::Z31_HI, AArch64::Z31 };

  regconstrain[AArch64::D0]  = { AArch64::B0,  AArch64::H0,  AArch64::S0,  AArch64::Q0,  AArch64::Z0_HI,  AArch64::Z0 };
  regconstrain[AArch64::D1]  = { AArch64::B1,  AArch64::H1,  AArch64::S1,  AArch64::Q1,  AArch64::Z1_HI,  AArch64::Z1 };
  regconstrain[AArch64::D2]  = { AArch64::B2,  AArch64::H2,  AArch64::S2,  AArch64::Q2,  AArch64::Z2_HI,  AArch64::Z2 };
  regconstrain[AArch64::D3]  = { AArch64::B3,  AArch64::H3,  AArch64::S3,  AArch64::Q3,  AArch64::Z3_HI,  AArch64::Z3 };
  regconstrain[AArch64::D4]  = { AArch64::B4,  AArch64::H4,  AArch64::S4,  AArch64::Q4,  AArch64::Z4_HI,  AArch64::Z4 };
  regconstrain[AArch64::D5]  = { AArch64::B5,  AArch64::H5,  AArch64::S5,  AArch64::Q5,  AArch64::Z5_HI,  AArch64::Z5 };
  regconstrain[AArch64::D6]  = { AArch64::B6,  AArch64::H6,  AArch64::S6,  AArch64::Q6,  AArch64::Z6_HI,  AArch64::Z6 };
  regconstrain[AArch64::D7]  = { AArch64::B7,  AArch64::H7,  AArch64::S7,  AArch64::Q7,  AArch64::Z7_HI,  AArch64::Z7 };
  regconstrain[AArch64::D8]  = { AArch64::B8,  AArch64::H8,  AArch64::S8,  AArch64::Q8,  AArch64::Z8_HI,  AArch64::Z8 };
  regconstrain[AArch64::D9]  = { AArch64::B9,  AArch64::H9,  AArch64::S9,  AArch64::Q9,  AArch64::Z9_HI,  AArch64::Z9 };
  regconstrain[AArch64::D10] = { AArch64::B10, AArch64::H10, AArch64::S10, AArch64::Q10, AArch64::Z10_HI, AArch64::Z10 };
  regconstrain[AArch64::D11] = { AArch64::B11, AArch64::H11, AArch64::S11, AArch64::Q11, AArch64::Z11_HI, AArch64::Z11 };
  regconstrain[AArch64::D12] = { AArch64::B12, AArch64::H12, AArch64::S12, AArch64::Q12, AArch64::Z12_HI, AArch64::Z12 };
  regconstrain[AArch64::D13] = { AArch64::B13, AArch64::H13, AArch64::S13, AArch64::Q13, AArch64::Z13_HI, AArch64::Z13 };
  regconstrain[AArch64::D14] = { AArch64::B14, AArch64::H14, AArch64::S14, AArch64::Q14, AArch64::Z14_HI, AArch64::Z14 };
  regconstrain[AArch64::D15] = { AArch64::B15, AArch64::H15, AArch64::S15, AArch64::Q15, AArch64::Z15_HI, AArch64::Z15 };
  regconstrain[AArch64::D16] = { AArch64::B16, AArch64::H16, AArch64::S16, AArch64::Q16, AArch64::Z16_HI, AArch64::Z16 };
  regconstrain[AArch64::D17] = { AArch64::B17, AArch64::H17, AArch64::S17, AArch64::Q17, AArch64::Z17_HI, AArch64::Z17 };
  regconstrain[AArch64::D18] = { AArch64::B18, AArch64::H18, AArch64::S18, AArch64::Q18, AArch64::Z18_HI, AArch64::Z18 };
  regconstrain[AArch64::D19] = { AArch64::B19, AArch64::H19, AArch64::S19, AArch64::Q19, AArch64::Z19_HI, AArch64::Z19 };
  regconstrain[AArch64::D20] = { AArch64::B20, AArch64::H20, AArch64::S20, AArch64::Q20, AArch64::Z20_HI, AArch64::Z20 };
  regconstrain[AArch64::D21] = { AArch64::B21, AArch64::H21, AArch64::S21, AArch64::Q21, AArch64::Z21_HI, AArch64::Z21 };
  regconstrain[AArch64::D22] = { AArch64::B22, AArch64::H22, AArch64::S22, AArch64::Q22, AArch64::Z22_HI, AArch64::Z22 };
  regconstrain[AArch64::D23] = { AArch64::B23, AArch64::H23, AArch64::S23, AArch64::Q23, AArch64::Z23_HI, AArch64::Z23 };
  regconstrain[AArch64::D24] = { AArch64::B24, AArch64::H24, AArch64::S24, AArch64::Q24, AArch64::Z24_HI, AArch64::Z24 };
  regconstrain[AArch64::D25] = { AArch64::B25, AArch64::H25, AArch64::S25, AArch64::Q25, AArch64::Z25_HI, AArch64::Z25 };
  regconstrain[AArch64::D26] = { AArch64::B26, AArch64::H26, AArch64::S26, AArch64::Q26, AArch64::Z26_HI, AArch64::Z26 };
  regconstrain[AArch64::D27] = { AArch64::B27, AArch64::H27, AArch64::S27, AArch64::Q27, AArch64::Z27_HI, AArch64::Z27 };
  regconstrain[AArch64::D28] = { AArch64::B28, AArch64::H28, AArch64::S28, AArch64::Q28, AArch64::Z28_HI, AArch64::Z28 };
  regconstrain[AArch64::D29] = { AArch64::B29, AArch64::H29, AArch64::S29, AArch64::Q29, AArch64::Z29_HI, AArch64::Z29 };
  regconstrain[AArch64::D30] = { AArch64::B30, AArch64::H30, AArch64::S30, AArch64::Q30, AArch64::Z30_HI, AArch64::Z30 };
  regconstrain[AArch64::D31] = { AArch64::B31, AArch64::H31, AArch64::S31, AArch64::Q31, AArch64::Z31_HI, AArch64::Z31 };

  regconstrain[AArch64::Q0]  = { AArch64::B0,  AArch64::H0,  AArch64::S0,  AArch64::D0,  AArch64::Z0_HI,  AArch64::Z0 };
  regconstrain[AArch64::Q1]  = { AArch64::B1,  AArch64::H1,  AArch64::S1,  AArch64::D1,  AArch64::Z1_HI,  AArch64::Z1 };
  regconstrain[AArch64::Q2]  = { AArch64::B2,  AArch64::H2,  AArch64::S2,  AArch64::D2,  AArch64::Z2_HI,  AArch64::Z2 };
  regconstrain[AArch64::Q3]  = { AArch64::B3,  AArch64::H3,  AArch64::S3,  AArch64::D3,  AArch64::Z3_HI,  AArch64::Z3 };
  regconstrain[AArch64::Q4]  = { AArch64::B4,  AArch64::H4,  AArch64::S4,  AArch64::D4,  AArch64::Z4_HI,  AArch64::Z4 };
  regconstrain[AArch64::Q5]  = { AArch64::B5,  AArch64::H5,  AArch64::S5,  AArch64::D5,  AArch64::Z5_HI,  AArch64::Z5 };
  regconstrain[AArch64::Q6]  = { AArch64::B6,  AArch64::H6,  AArch64::S6,  AArch64::D6,  AArch64::Z6_HI,  AArch64::Z6 };
  regconstrain[AArch64::Q7]  = { AArch64::B7,  AArch64::H7,  AArch64::S7,  AArch64::D7,  AArch64::Z7_HI,  AArch64::Z7 };
  regconstrain[AArch64::Q8]  = { AArch64::B8,  AArch64::H8,  AArch64::S8,  AArch64::D8,  AArch64::Z8_HI,  AArch64::Z8 };
  regconstrain[AArch64::Q9]  = { AArch64::B9,  AArch64::H9,  AArch64::S9,  AArch64::D9,  AArch64::Z9_HI,  AArch64::Z9 };
  regconstrain[AArch64::Q10] = { AArch64::B10, AArch64::H10, AArch64::S10, AArch64::D10, AArch64::Z10_HI, AArch64::Z10 };
  regconstrain[AArch64::Q11] = { AArch64::B11, AArch64::H11, AArch64::S11, AArch64::D11, AArch64::Z11_HI, AArch64::Z11 };
  regconstrain[AArch64::Q12] = { AArch64::B12, AArch64::H12, AArch64::S12, AArch64::D12, AArch64::Z12_HI, AArch64::Z12 };
  regconstrain[AArch64::Q13] = { AArch64::B13, AArch64::H13, AArch64::S13, AArch64::D13, AArch64::Z13_HI, AArch64::Z13 };
  regconstrain[AArch64::Q14] = { AArch64::B14, AArch64::H14, AArch64::S14, AArch64::D14, AArch64::Z14_HI, AArch64::Z14 };
  regconstrain[AArch64::Q15] = { AArch64::B15, AArch64::H15, AArch64::S15, AArch64::D15, AArch64::Z15_HI, AArch64::Z15 };
  regconstrain[AArch64::Q16] = { AArch64::B16, AArch64::H16, AArch64::S16, AArch64::D16, AArch64::Z16_HI, AArch64::Z16 };
  regconstrain[AArch64::Q17] = { AArch64::B17, AArch64::H17, AArch64::S17, AArch64::D17, AArch64::Z17_HI, AArch64::Z17 };
  regconstrain[AArch64::Q18] = { AArch64::B18, AArch64::H18, AArch64::S18, AArch64::D18, AArch64::Z18_HI, AArch64::Z18 };
  regconstrain[AArch64::Q19] = { AArch64::B19, AArch64::H19, AArch64::S19, AArch64::D19, AArch64::Z19_HI, AArch64::Z19 };
  regconstrain[AArch64::Q20] = { AArch64::B20, AArch64::H20, AArch64::S20, AArch64::D20, AArch64::Z20_HI, AArch64::Z20 };
  regconstrain[AArch64::Q21] = { AArch64::B21, AArch64::H21, AArch64::S21, AArch64::D21, AArch64::Z21_HI, AArch64::Z21 };
  regconstrain[AArch64::Q22] = { AArch64::B22, AArch64::H22, AArch64::S22, AArch64::D22, AArch64::Z22_HI, AArch64::Z22 };
  regconstrain[AArch64::Q23] = { AArch64::B23, AArch64::H23, AArch64::S23, AArch64::D23, AArch64::Z23_HI, AArch64::Z23 };
  regconstrain[AArch64::Q24] = { AArch64::B24, AArch64::H24, AArch64::S24, AArch64::D24, AArch64::Z24_HI, AArch64::Z24 };
  regconstrain[AArch64::Q25] = { AArch64::B25, AArch64::H25, AArch64::S25, AArch64::D25, AArch64::Z25_HI, AArch64::Z25 };
  regconstrain[AArch64::Q26] = { AArch64::B26, AArch64::H26, AArch64::S26, AArch64::D26, AArch64::Z26_HI, AArch64::Z26 };
  regconstrain[AArch64::Q27] = { AArch64::B27, AArch64::H27, AArch64::S27, AArch64::D27, AArch64::Z27_HI, AArch64::Z27 };
  regconstrain[AArch64::Q28] = { AArch64::B28, AArch64::H28, AArch64::S28, AArch64::D28, AArch64::Z28_HI, AArch64::Z28 };
  regconstrain[AArch64::Q29] = { AArch64::B29, AArch64::H29, AArch64::S29, AArch64::D29, AArch64::Z29_HI, AArch64::Z29 };
  regconstrain[AArch64::Q30] = { AArch64::B30, AArch64::H30, AArch64::S30, AArch64::D30, AArch64::Z30_HI, AArch64::Z30 };
  regconstrain[AArch64::Q31] = { AArch64::B31, AArch64::H31, AArch64::S31, AArch64::D31, AArch64::Z31_HI, AArch64::Z31 };

  regconstrain[AArch64::Z0_HI]  = { AArch64::B0,  AArch64::H0,  AArch64::S0,  AArch64::D0,  AArch64::Q0,  AArch64::Z0 };
  regconstrain[AArch64::Z1_HI]  = { AArch64::B1,  AArch64::H1,  AArch64::S1,  AArch64::D1,  AArch64::Q1,  AArch64::Z1 };
  regconstrain[AArch64::Z2_HI]  = { AArch64::B2,  AArch64::H2,  AArch64::S2,  AArch64::D2,  AArch64::Q2,  AArch64::Z2 };
  regconstrain[AArch64::Z3_HI]  = { AArch64::B3,  AArch64::H3,  AArch64::S3,  AArch64::D3,  AArch64::Q3,  AArch64::Z3 };
  regconstrain[AArch64::Z4_HI]  = { AArch64::B4,  AArch64::H4,  AArch64::S4,  AArch64::D4,  AArch64::Q4,  AArch64::Z4 };
  regconstrain[AArch64::Z5_HI]  = { AArch64::B5,  AArch64::H5,  AArch64::S5,  AArch64::D5,  AArch64::Q5,  AArch64::Z5 };
  regconstrain[AArch64::Z6_HI]  = { AArch64::B6,  AArch64::H6,  AArch64::S6,  AArch64::D6,  AArch64::Q6,  AArch64::Z6 };
  regconstrain[AArch64::Z7_HI]  = { AArch64::B7,  AArch64::H7,  AArch64::S7,  AArch64::D7,  AArch64::Q7,  AArch64::Z7 };
  regconstrain[AArch64::Z8_HI]  = { AArch64::B8,  AArch64::H8,  AArch64::S8,  AArch64::D8,  AArch64::Q8,  AArch64::Z8 };
  regconstrain[AArch64::Z9_HI]  = { AArch64::B9,  AArch64::H9,  AArch64::S9,  AArch64::D9,  AArch64::Q9,  AArch64::Z9 };
  regconstrain[AArch64::Z10_HI] = { AArch64::B10, AArch64::H10, AArch64::S10, AArch64::D10, AArch64::Q10, AArch64::Z10 };
  regconstrain[AArch64::Z11_HI] = { AArch64::B11, AArch64::H11, AArch64::S11, AArch64::D11, AArch64::Q11, AArch64::Z11 };
  regconstrain[AArch64::Z12_HI] = { AArch64::B12, AArch64::H12, AArch64::S12, AArch64::D12, AArch64::Q12, AArch64::Z12 };
  regconstrain[AArch64::Z13_HI] = { AArch64::B13, AArch64::H13, AArch64::S13, AArch64::D13, AArch64::Q13, AArch64::Z13 };
  regconstrain[AArch64::Z14_HI] = { AArch64::B14, AArch64::H14, AArch64::S14, AArch64::D14, AArch64::Q14, AArch64::Z14 };
  regconstrain[AArch64::Z15_HI] = { AArch64::B15, AArch64::H15, AArch64::S15, AArch64::D15, AArch64::Q15, AArch64::Z15 };
  regconstrain[AArch64::Z16_HI] = { AArch64::B16, AArch64::H16, AArch64::S16, AArch64::D16, AArch64::Q16, AArch64::Z16 };
  regconstrain[AArch64::Z17_HI] = { AArch64::B17, AArch64::H17, AArch64::S17, AArch64::D17, AArch64::Q17, AArch64::Z17 };
  regconstrain[AArch64::Z18_HI] = { AArch64::B18, AArch64::H18, AArch64::S18, AArch64::D18, AArch64::Q18, AArch64::Z18 };
  regconstrain[AArch64::Z19_HI] = { AArch64::B19, AArch64::H19, AArch64::S19, AArch64::D19, AArch64::Q19, AArch64::Z19 };
  regconstrain[AArch64::Z20_HI] = { AArch64::B20, AArch64::H20, AArch64::S20, AArch64::D20, AArch64::Q20, AArch64::Z20 };
  regconstrain[AArch64::Z21_HI] = { AArch64::B21, AArch64::H21, AArch64::S21, AArch64::D21, AArch64::Q21, AArch64::Z21 };
  regconstrain[AArch64::Z22_HI] = { AArch64::B22, AArch64::H22, AArch64::S22, AArch64::D22, AArch64::Q22, AArch64::Z22 };
  regconstrain[AArch64::Z23_HI] = { AArch64::B23, AArch64::H23, AArch64::S23, AArch64::D23, AArch64::Q23, AArch64::Z23 };
  regconstrain[AArch64::Z24_HI] = { AArch64::B24, AArch64::H24, AArch64::S24, AArch64::D24, AArch64::Q24, AArch64::Z24 };
  regconstrain[AArch64::Z25_HI] = { AArch64::B25, AArch64::H25, AArch64::S25, AArch64::D25, AArch64::Q25, AArch64::Z25 };
  regconstrain[AArch64::Z26_HI] = { AArch64::B26, AArch64::H26, AArch64::S26, AArch64::D26, AArch64::Q26, AArch64::Z26 };
  regconstrain[AArch64::Z27_HI] = { AArch64::B27, AArch64::H27, AArch64::S27, AArch64::D27, AArch64::Q27, AArch64::Z27 };
  regconstrain[AArch64::Z28_HI] = { AArch64::B28, AArch64::H28, AArch64::S28, AArch64::D28, AArch64::Q28, AArch64::Z28 };
  regconstrain[AArch64::Z29_HI] = { AArch64::B29, AArch64::H29, AArch64::S29, AArch64::D29, AArch64::Q29, AArch64::Z29 };
  regconstrain[AArch64::Z30_HI] = { AArch64::B30, AArch64::H30, AArch64::S30, AArch64::D30, AArch64::Q30, AArch64::Z30 };
  regconstrain[AArch64::Z31_HI] = { AArch64::B31, AArch64::H31, AArch64::S31, AArch64::D31, AArch64::Q31, AArch64::Z31 };

  regconstrain[AArch64::Z0]  = { AArch64::B0,  AArch64::H0,  AArch64::S0,  AArch64::D0,  AArch64::Z0_HI,  AArch64::Q0 };
  regconstrain[AArch64::Z1]  = { AArch64::B1,  AArch64::H1,  AArch64::S1,  AArch64::D1,  AArch64::Z1_HI,  AArch64::Q1 };
  regconstrain[AArch64::Z2]  = { AArch64::B2,  AArch64::H2,  AArch64::S2,  AArch64::D2,  AArch64::Z2_HI,  AArch64::Q2 };
  regconstrain[AArch64::Z3]  = { AArch64::B3,  AArch64::H3,  AArch64::S3,  AArch64::D3,  AArch64::Z3_HI,  AArch64::Q3 };
  regconstrain[AArch64::Z4]  = { AArch64::B4,  AArch64::H4,  AArch64::S4,  AArch64::D4,  AArch64::Z4_HI,  AArch64::Q4 };
  regconstrain[AArch64::Z5]  = { AArch64::B5,  AArch64::H5,  AArch64::S5,  AArch64::D5,  AArch64::Z5_HI,  AArch64::Q5 };
  regconstrain[AArch64::Z6]  = { AArch64::B6,  AArch64::H6,  AArch64::S6,  AArch64::D6,  AArch64::Z6_HI,  AArch64::Q6 };
  regconstrain[AArch64::Z7]  = { AArch64::B7,  AArch64::H7,  AArch64::S7,  AArch64::D7,  AArch64::Z7_HI,  AArch64::Q7 };
  regconstrain[AArch64::Z8]  = { AArch64::B8,  AArch64::H8,  AArch64::S8,  AArch64::D8,  AArch64::Z8_HI,  AArch64::Q8 };
  regconstrain[AArch64::Z9]  = { AArch64::B9,  AArch64::H9,  AArch64::S9,  AArch64::D9,  AArch64::Z9_HI,  AArch64::Q9 };
  regconstrain[AArch64::Z10] = { AArch64::B10, AArch64::H10, AArch64::S10, AArch64::D10, AArch64::Z10_HI, AArch64::Q10 };
  regconstrain[AArch64::Z11] = { AArch64::B11, AArch64::H11, AArch64::S11, AArch64::D11, AArch64::Z11_HI, AArch64::Q11 };
  regconstrain[AArch64::Z12] = { AArch64::B12, AArch64::H12, AArch64::S12, AArch64::D12, AArch64::Z12_HI, AArch64::Q12 };
  regconstrain[AArch64::Z13] = { AArch64::B13, AArch64::H13, AArch64::S13, AArch64::D13, AArch64::Z13_HI, AArch64::Q13 };
  regconstrain[AArch64::Z14] = { AArch64::B14, AArch64::H14, AArch64::S14, AArch64::D14, AArch64::Z14_HI, AArch64::Q14 };
  regconstrain[AArch64::Z15] = { AArch64::B15, AArch64::H15, AArch64::S15, AArch64::D15, AArch64::Z15_HI, AArch64::Q15 };
  regconstrain[AArch64::Z16] = { AArch64::B16, AArch64::H16, AArch64::S16, AArch64::D16, AArch64::Z16_HI, AArch64::Q16 };
  regconstrain[AArch64::Z17] = { AArch64::B17, AArch64::H17, AArch64::S17, AArch64::D17, AArch64::Z17_HI, AArch64::Q17 };
  regconstrain[AArch64::Z18] = { AArch64::B18, AArch64::H18, AArch64::S18, AArch64::D18, AArch64::Z18_HI, AArch64::Q18 };
  regconstrain[AArch64::Z19] = { AArch64::B19, AArch64::H19, AArch64::S19, AArch64::D19, AArch64::Z19_HI, AArch64::Q19 };
  regconstrain[AArch64::Z20] = { AArch64::B20, AArch64::H20, AArch64::S20, AArch64::D20, AArch64::Z20_HI, AArch64::Q20 };
  regconstrain[AArch64::Z21] = { AArch64::B21, AArch64::H21, AArch64::S21, AArch64::D21, AArch64::Z21_HI, AArch64::Q21 };
  regconstrain[AArch64::Z22] = { AArch64::B22, AArch64::H22, AArch64::S22, AArch64::D22, AArch64::Z22_HI, AArch64::Q22 };
  regconstrain[AArch64::Z23] = { AArch64::B23, AArch64::H23, AArch64::S23, AArch64::D23, AArch64::Z23_HI, AArch64::Q23 };
  regconstrain[AArch64::Z24] = { AArch64::B24, AArch64::H24, AArch64::S24, AArch64::D24, AArch64::Z24_HI, AArch64::Q24 };
  regconstrain[AArch64::Z25] = { AArch64::B25, AArch64::H25, AArch64::S25, AArch64::D25, AArch64::Z25_HI, AArch64::Q25 };
  regconstrain[AArch64::Z26] = { AArch64::B26, AArch64::H26, AArch64::S26, AArch64::D26, AArch64::Z26_HI, AArch64::Q26 };
  regconstrain[AArch64::Z27] = { AArch64::B27, AArch64::H27, AArch64::S27, AArch64::D27, AArch64::Z27_HI, AArch64::Q27 };
  regconstrain[AArch64::Z28] = { AArch64::B28, AArch64::H28, AArch64::S28, AArch64::D28, AArch64::Z28_HI, AArch64::Q28 };
  regconstrain[AArch64::Z29] = { AArch64::B29, AArch64::H29, AArch64::S29, AArch64::D29, AArch64::Z29_HI, AArch64::Q29 };
  regconstrain[AArch64::Z30] = { AArch64::B30, AArch64::H30, AArch64::S30, AArch64::D30, AArch64::Z30_HI, AArch64::Q30 };
  regconstrain[AArch64::Z31] = { AArch64::B31, AArch64::H31, AArch64::S31, AArch64::D31, AArch64::Z31_HI, AArch64::Q31 };
};

static DenseMap<Register, Register> regmap={
    {AArch64::W0, AArch64::X0},
    {AArch64::W1, AArch64::X1},
    {AArch64::W2, AArch64::X2},
    {AArch64::W3, AArch64::X3},
    {AArch64::W4, AArch64::X4},
    {AArch64::W5, AArch64::X5},
    {AArch64::W6, AArch64::X6},
    {AArch64::W7, AArch64::X7},
    {AArch64::W8, AArch64::X8},
    {AArch64::W9, AArch64::X9},
    {AArch64::W10, AArch64::X10},
    {AArch64::W11, AArch64::X11},
    {AArch64::W12, AArch64::X12},
    {AArch64::W13, AArch64::X13},
    {AArch64::W14, AArch64::X14},
    {AArch64::W15, AArch64::X15},
    {AArch64::W16, AArch64::X16},
    {AArch64::W17, AArch64::X17},
    {AArch64::W18, AArch64::X18},
    {AArch64::W19, AArch64::X19},
    {AArch64::W20, AArch64::X20},
    {AArch64::W21, AArch64::X21},
    {AArch64::W22, AArch64::X22},
    {AArch64::W23, AArch64::X23},
    {AArch64::W24, AArch64::X24},
    {AArch64::W25, AArch64::X25},
    {AArch64::W26, AArch64::X26},
    {AArch64::W27, AArch64::X27},
    {AArch64::W28, AArch64::X28},
    {AArch64::WSP, AArch64::SP},
    {AArch64::WZR, AArch64::XZR},

    {AArch64::X0, AArch64::X0},
    {AArch64::X1, AArch64::X1},
    {AArch64::X2, AArch64::X2},
    {AArch64::X3, AArch64::X3},
    {AArch64::X4, AArch64::X4},
    {AArch64::X5, AArch64::X5},
    {AArch64::X6, AArch64::X6},
    {AArch64::X7, AArch64::X7},
    {AArch64::X8, AArch64::X8},
    {AArch64::X9, AArch64::X9},
    {AArch64::X10, AArch64::X10},
    {AArch64::X11, AArch64::X11},
    {AArch64::X12, AArch64::X12},
    {AArch64::X13, AArch64::X13},
    {AArch64::X14, AArch64::X14},
    {AArch64::X15, AArch64::X15},
    {AArch64::X16, AArch64::X16},
    {AArch64::X17, AArch64::X17},
    {AArch64::X18, AArch64::X18},
    {AArch64::X19, AArch64::X19},
    {AArch64::X20, AArch64::X20},
    {AArch64::X21, AArch64::X21},
    {AArch64::X22, AArch64::X22},
    {AArch64::X23, AArch64::X23},
    {AArch64::X24, AArch64::X24},
    {AArch64::X25, AArch64::X25},
    {AArch64::X26, AArch64::X26},
    {AArch64::X27, AArch64::X27},
    {AArch64::X28, AArch64::X28},
    {AArch64::FP , AArch64::FP},
    {AArch64::LR , AArch64::LR},
    {AArch64::SP , AArch64::SP},
    {AArch64::XZR, AArch64::XZR}
};

void SwplRegAllocInfoTbl::setRangeReg(std::vector<int> *range, RegAllocInfo& regAllocInfo) {
  if (regAllocInfo.num_def < 0) {
    for (int i = 1; i <= regAllocInfo.num_use; i++) {
      range->at(i)++;
    }
  } else if (regAllocInfo.num_use > 0 && regAllocInfo.num_def >= regAllocInfo.num_use) {
    for (int i=1; i<=regAllocInfo.num_use; i++) {
      range->at(i)++;
    }
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)++;
    }
  } else if (regAllocInfo.num_use < 0) {
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)++;
    }
  } else {
    for (int i = regAllocInfo.num_def; i <= regAllocInfo.num_use; i++) {
      range->at(i)++;
    }
  }
}

void SwplRegAllocInfoTbl::countRegs() {
  std::vector<int> ireg;
  std::vector<int> freg;
  std::vector<int> preg;

  ireg.resize(total_mi+1);
  freg.resize(total_mi+1);
  preg.resize(total_mi+1);

  std::unique_ptr<StmRegKind> regKind;
  for (auto &regAllocInfo:rai_tbl) {
    if (regAllocInfo.vreg==0) {
      // SWPL化前から実レジスタが割り付けてある
      regKind.reset(SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, regAllocInfo.preg));
      if (regKind->isCCRegister()) {
        // CCレジスタは計算から除外する
        continue;
      }
      if (regKind->isInteger()) {
        auto r = regmap[regAllocInfo.preg];
        if( std::find(nousePhysRegs.begin(), nousePhysRegs.end(), r) != nousePhysRegs.end() ){
          // レジスタ割付処理で割付禁止レジスタが、割り付けてあるため、計算から除外する
          continue;
        }
      }
    } else {
      if (regAllocInfo.preg == 0) {
        // 実レジスタを割り付けていないので、計算から除外する
        continue;
      }
      regKind.reset(SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, regAllocInfo.vreg));
    }
    if (regKind->isInteger()) {
      setRangeReg(&ireg, regAllocInfo);
    } else if (regKind->isFloating()) {
      setRangeReg(&freg, regAllocInfo);
    } else if (regKind->isPredicate()) {
      setRangeReg(&preg, regAllocInfo);
    } else {
      llvm_unreachable("unnown reg class");
    }
  }
  // レジスタ割付では、なぜか１から始まるので、以下のforでは0番目を処理しているがほんの少し無駄になっている
  for (int i:ireg) num_ireg = std::max(num_ireg, i);
  for (int i:freg) num_freg = std::max(num_freg, i);
  for (int i:preg) num_preg = std::max(num_preg, i);
}

int SwplRegAllocInfoTbl::countIReg() {
  if (num_ireg<0) countRegs();
  return num_ireg;
}

int SwplRegAllocInfoTbl::countFReg()  {
  if (num_freg<0) countRegs();
  return num_freg;
}

int SwplRegAllocInfoTbl::countPReg()  {
  if (num_preg<0) countRegs();
  return num_preg;
}

int SwplRegAllocInfoTbl::availableIRegNumber() const {
  return AArch64::GPR64RegClass.getNumRegs()-UNAVAILABLE_REGS;
}

int SwplRegAllocInfoTbl::availableFRegNumber() const {
  return AArch64::FPR64RegClass.getNumRegs();
}

int SwplRegAllocInfoTbl::availablePRegNumber() const {
  return AArch64::PPR_3bRegClass.getNumRegs();
}