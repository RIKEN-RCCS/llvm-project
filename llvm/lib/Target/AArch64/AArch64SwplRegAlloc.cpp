//=- AArch64SwplRegAlloc.cpp - Register Allocation for SWP -*- C++ -*---------------=//
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
#include "llvm/CodeGen/SwplTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-swpipeliner"
#define UNAVAILABLE_REGS 7
#define PRIO_ASC_ORDER 0
#define PRIO_UNUSE 1

static cl::opt<bool> DebugSwplRegAlloc("swpl-debug-reg-alloc",cl::init(false), cl::ReallyHidden);
static cl::opt<int> SwplRegAllocPrio("swpl-reg-alloc-prio",cl::init(PRIO_ASC_ORDER), cl::ReallyHidden);
static cl::opt<bool> EnableSetrenamable("swpl-enable-reg-alloc-setrenamable",cl::init(false), cl::ReallyHidden);

// 使ってはいけない物理レジスタのリスト
static DenseSet<unsigned> nousePhysRegs {
    (unsigned)AArch64::XZR,  // ゼロレジスタ
    (unsigned)AArch64::SP,   // スタックポインタ
    (unsigned)AArch64::FP,   // フレームポインタ(X29)
    (unsigned)AArch64::LR,   // リンクレジスタ(X30)
    (unsigned)AArch64::WZR,  // ゼロレジスタ
    (unsigned)AArch64::WSP,  // スタックポインタ
    (unsigned)AArch64::W29,  // フレームポインタ(X29)相当
    (unsigned)AArch64::W30   // リンクレジスタ(X30)相当
};


/**
 * @brief エピローグ部の仮想レジスタリストを作成する
 * @param [in]     mi llvm::MachineInstr
 * @param [in]     num_mi llvm::MachineInstrの通し番号
 * @param [in,out] ekri_tbl カーネルループ外のレジスタ情報の表
 */
static void createVRegListEpi(MachineInstr *mi, unsigned num_mi,
                              SwplExcKernelRegInfoTbl &ekri_tbl) {
  assert(mi);

  // MIのオペランド数でループ
  for (MachineOperand& mo:mi->operands()) {
    // レジスタオペランド かつ レジスタ番号が 0 でない かつ
    // 仮想レジスタであればリストに加える
    unsigned reg = 0;
    if ((!mo.isReg()) || ((reg = mo.getReg()) == 0) ||
        (!Register::isVirtualRegister(reg)))
      continue;

    // 割り当て済みレジスタ情報を当該仮想レジスタで検索
    auto ri = ekri_tbl.getWithVReg(reg);

    // TODO: 以下、同じような処理が続くため、関数化した方が良い
    // 当該オペランドが"定義"か
    if (mo.isDef()) {
      if (ri != nullptr) {
        if( ri->num_def == -1 ||
            ri->num_use == -1 ) {
          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          ri->updateNumDef(num_mi);
        }
      } else {
        // 当該仮想レジスタは登録されていないため新規登録
        ekri_tbl.addExcKernelRegInfo(reg, num_mi, -1);
      }
    }
    // 当該オペランドが"参照"か
    if (mo.isUse()) {
      if (ri != nullptr) {
        if( ri->num_def == -1 ||
            ri->num_use == -1 ||
            ri->num_use > ri->num_def ) {
          // 当該仮想レジスタは登録済みなのでMI通し番号を更新
          ri->updateNumUse(num_mi);
        }
      } else {
        // 当該仮想レジスタは登録されていないため新規登録
        ekri_tbl.addExcKernelRegInfo(reg, -1, num_mi);
      }
    }
  }

  return;
}

/**
 * @brief Create a COPY from a virtual register to a physical register
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out] p_post_mis COPY instruction for prolog post
 * @param [in]     vreg virtual register number
 * @param [in]     preg physical register number
 */
static void addCopyMIpre(MachineFunction &MF,
                         DebugLoc dl,
                         std::vector<MachineInstr*> &p_post_mis,
                         unsigned vreg, unsigned preg) {
  assert(vreg && preg);
  if( DebugSwplRegAlloc ) {
    dbgs() << " Copy " << printReg(vreg, SWPipeliner::TRI)
           << " to " << printReg(preg, SWPipeliner::TRI) << "\n";
  }

  // Create COPY from virtual register to physical register
  llvm::MachineInstr *copy =
      BuildMI(MF, dl, SWPipeliner::TII->get(TargetOpcode::COPY))
      .addReg(preg, RegState::Define).addReg(vreg);
  p_post_mis.push_back(copy);

  return;
}

/**
 * @brief Create a COPY from a physical register to a virtual register
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out] e_pre_mis COPY instruction for epilog pre
 * @param [in]     vreg virtual register number
 * @param [in]     preg physical register number
 */
static void addCopyMIpost(MachineFunction &MF,
                          DebugLoc dl,
                          std::vector<MachineInstr*> &e_pre_mis,
                          unsigned vreg, unsigned preg) {
  assert(vreg && preg);
  if( DebugSwplRegAlloc ) {
    dbgs() << " Copy " << printReg(preg, SWPipeliner::TRI)
           << " to " << printReg(vreg, SWPipeliner::TRI) << "\n";
  }

  // Create COPY from physical register to virtual register
  llvm::MachineInstr *copy =
      BuildMI(MF, dl, SWPipeliner::TII->get(TargetOpcode::COPY))
      .addReg(vreg, RegState::Define).addReg(preg);
  e_pre_mis.push_back(copy);

  return;
}

/**
 * @brief Create SWPLIVEIN instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    livein_mi MI for livein
 * @param [in]     copy_mis COPY instruction for livein/liveout
 * @param [in]     vreg_loc Location of physical register for COPY instruction
 */
static void createSwpliveinMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **livein_mi,
                          std::vector<MachineInstr*> &copy_mis,
                          unsigned vreg_loc) {

  // Create SWPLIVEIN instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEIN));
  for(auto *copy:copy_mis){
    mib.addReg(copy->getOperand(vreg_loc).getReg(), RegState::ImplicitDefine);
  }
  *livein_mi = mib.getInstr();
  return;
}

/**
 * @brief Add registers to the SWPLIVEOUT instruction
 * @param [in,out] mib MachineInstrBuilder for SWPLIVEOUT
 * @param [in]     copy_mis COPY instruction for livein/liveout
 * @param [in]     vreg_loc Location of physical register for COPY instruction
 * @param [in,out] added_regs Registers already added to the SWPLIVEOUT instruction
 */
static void addRegSwpliveoutMI(MachineInstrBuilder &mib,
                              std::vector<MachineInstr*> &copy_mis,
                              unsigned vreg_loc,
                              std::vector<Register> &added_regs) {
  for (auto *copy:copy_mis) {
    Register reg = copy->getOperand(vreg_loc).getReg();
    bool added = false;

    // Already added registers?
    for (auto &added_reg:added_regs) {
      if (reg == added_reg){
        added = true;
        break;
      }    
    }

    if(!added){
      mib.addReg(reg, RegState::Implicit);
      added_regs.push_back(reg);
    }  
  }
  return;
}

/**
 * @brief Create Prolog SWPLIVEOUT instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    liveout_mi MI for liveout
 * @param [in]     livein_copy_mis COPY instruction for livein
 */
static void createPrologSwpliveoutMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **liveout_mi,
                          std::vector<MachineInstr*> &livein_copy_mis) {

  // Create SWPLIVEOUT instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEOUT));
  std::vector<Register> added_regs;
  addRegSwpliveoutMI(mib, livein_copy_mis, 0, added_regs);
  *liveout_mi = mib.getInstr();
  return;
}

/**
 * @brief Create Kernel SWPLIVEOUT instruction
 * @param [in]     MF MachineFunction
 * @param [in]     dl DebugLoc
 * @param [in,out]    liveout_mi MI for liveout
 * @param [in]     livein_copy_mis COPY instruction for livein
 * @param [in]     liveout_copy_mis COPY instruction for liveout
 */
static void createKernelSwpliveoutMI(MachineFunction &MF,
                          DebugLoc dl,
                          MachineInstr **liveout_mi,
                          std::vector<MachineInstr*> &livein_copy_mis,
                          std::vector<MachineInstr*> &liveout_copy_mis) {

  // Create SWPLIVEOUT instruction
  MachineInstrBuilder mib = BuildMI(MF, dl, SWPipeliner::TII->get(AArch64::SWPLIVEOUT));
  std::vector<Register> added_regs;
  // The livein register is added to extend the live range.
  addRegSwpliveoutMI(mib, livein_copy_mis, 0, added_regs);
  addRegSwpliveoutMI(mib, liveout_copy_mis, 1, added_regs);
  *liveout_mi = mib.getInstr();
  return;
}

/**
 * @brief  Create software pipelining pseudo instructions.
 * @param  [in,out] tmi Information for the SwplTransformMIR
 * @param  [in]     MF  MachineFunction
 */
void AArch64InstrInfo::createSwplPseudoMIs(SwplTransformedMIRInfo *tmi,MachineFunction &MF) const {
  // Get DebugLoc from kernel start MI
  DebugLoc firstDL;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    firstDL = mi->getDebugLoc();
    break;
  }
  
  // Generate SWPLIVEIN, SWPLIVEOUT instructions
  createPrologSwpliveoutMI(MF, firstDL, &tmi->prolog_liveout_mi, tmi->livein_copy_mis);
  createSwpliveinMI(MF, firstDL, &tmi->kernel_livein_mi, tmi->livein_copy_mis, 0);
  createKernelSwpliveoutMI(MF, firstDL, &tmi->kernel_liveout_mi, tmi->livein_copy_mis, tmi->liveout_copy_mis);
  createSwpliveinMI(MF, firstDL, &tmi->epilog_livein_mi, tmi->liveout_copy_mis, 1);
}

/**
 * @brief  Whether it is a software pipelining pseudo instruction
 * @param  [in]  MI MachineInstr
 * @retval true Software pipelining pseudo instruction
 * @retval false Not software pipelining pseudo instruction
 */
bool AArch64InstrInfo::isSwplPseudoMI(MachineInstr &MI) const {
  unsigned op = MI.getOpcode();
  return (op == AArch64::SWPLIVEIN || op == AArch64::SWPLIVEOUT);
}

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
  if ((!Register::isVirtualRegister(reg)) ||
      (ex_vreg.find(reg) != ex_vreg.end())) {
    // 仮想レジスタでない or 除外リストに追加済み なら何もしない
    return;
  }

  // 対象のオペランドなら除外リストに追加
  assert(mi);
  switch (mi->getOpcode()) {
  case TargetOpcode::EXTRACT_SUBREG:
  case TargetOpcode::INSERT_SUBREG:
  case TargetOpcode::REG_SEQUENCE:
  case TargetOpcode::SUBREG_TO_REG:
    if( DebugSwplRegAlloc ) {
      dbgs() << "Excluded operand: " << mi->getOpcode()
             << ", reg=" << printReg(reg, SWPipeliner::TRI) << "\n";
    }
    ex_vreg.insert(reg);
    break;
  default:
    break;
  }
}

/**
 * @brief  仮想レジスタごとに生存区間リストを作成する
 * @param  [in]     mi llvm::MachineInstr
 * @param  [in]     idx moのindex
 * @param  [in]     mo llvm::MachineOperand
 * @param  [in]     reg getReg()で取得したレジスタ番号
 * @param  [in,out] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in]     num_mi llvm::MachineInstrの通し番号
 * @param  [in]     total_mi llvm::MachineInstrの総数
 * @param  [in]     ex_vreg 割り付け対象外仮想レジスタ
 * @param  [in]     doVReg renaming後の制御変数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 */
static int createLiveRange(MachineInstr *mi, unsigned idx,
                           MachineOperand &mo, unsigned reg,
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
      rai_tbl.addRegAllocInfo(0, 1, total_mi, &mo, 0, reg);
    }
  } else if (Register::isVirtualRegister(reg)) {
    // 仮想レジスタである

    // 除外リストに含まれていない限り、処理続行
    if (ex_vreg.find(reg) == ex_vreg.end()) {

      // 割り当て済みレジスタ情報を当該仮想レジスタで検索
      auto rai = rai_tbl.getWithVReg( reg );

      assert(mi);
      unsigned tied_vreg = 0;
      // TODO: 以下、同じような処理が続くため、関数化した方が良い
      // 当該オペランドが"定義"か
      if (mo.isDef()) {
        // 参照オペランドとtiedならそのオペランドを取得
        unsigned tied_idx;
        if ((mo.isTied()) && (mi->isRegTiedToUseOperand(idx, &tied_idx))) {
          MachineOperand &tied_mo = mi->getOperand(tied_idx);
          if (ex_vreg.find(tied_mo.getReg()) != ex_vreg.end()) {
            // tiedの相手が除外リストに含まれるなら自分も除外リストへ
            if (DebugSwplRegAlloc) {
              dbgs() << "Excluded register. current reg="
                    << printReg(reg, SWPipeliner::TRI) << ", tied(excluded)="
                    << printReg(tied_mo.getReg(), SWPipeliner::TRI) << "\n";
            }
            ex_vreg.insert(reg);
            return ret;
          }
          if ((tied_mo.isReg()) && (tied_mo.getReg()) && (tied_mo.isTied())) {
            // レジスタオペランド かつ レジスタ値が0より大きい かつ
            // tiedフラグが立っているならtied仮想レジスタとして使用する
            tied_vreg = tied_mo.getReg();
          }
        }

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
          if (tied_vreg != 0) {
            rai->updateTiedVReg(tied_vreg);
          }
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, num_mi, -1, &mo, tied_vreg );
        }
      }
      // 当該オペランドが"参照"か
      if (mo.isUse()) {
        // 定義オペランドとtiedならそのオペランドを取得
        unsigned tied_idx;
        if ((mo.isTied()) && (mi->isRegTiedToDefOperand(idx, &tied_idx))) {
          MachineOperand &tied_mo = mi->getOperand(tied_idx);
          if (ex_vreg.find(tied_mo.getReg()) != ex_vreg.end()) {
            // tiedの相手が除外リストに含まれるなら自分も除外リストへ
            if (DebugSwplRegAlloc) {
              dbgs() << "Excluded register. current reg="
                    << printReg(reg, SWPipeliner::TRI) << ", tied(excluded)="
                    << printReg(tied_mo.getReg(), SWPipeliner::TRI) << "\n";
            }
            ex_vreg.insert(reg);
            return ret;
          }
          if ((tied_mo.isReg()) && (tied_mo.getReg()) && (tied_mo.isTied())) {
            // レジスタオペランド かつ レジスタ値が0より大きい かつ
            // tiedフラグが立っているならtied仮想レジスタとして使用する
            tied_vreg = tied_mo.getReg();
          }
        }

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
          if (tied_vreg != 0) {
            rai->updateTiedVReg(tied_vreg);
          }
        } else {
          // 当該仮想レジスタは登録されていないため新規登録
          rai_tbl.addRegAllocInfo( reg, -1, num_mi, &mo, tied_vreg );
        }
      }
    }
  }  else {
    // do nothing. 仮想レジスタでも物理レジスタでもない
  }

  return ret;
}

/**
 * @brief Liverangeの延長処理
 * @param [in,out] tmi 命令列変換情報
 */
static void extendLiveRange(SwplTransformedMIRInfo *tmi) {
  assert(tmi);
  for (size_t i = 0; i < tmi->swplRAITbl->length(); i++) {
    RegAllocInfo *rinfo = tmi->swplRAITbl->getWithIdx(i);
    // 後に呼び出すcallSetReg()内でカーネル終端にCOPY命令を追加する
    // 条件に合致したレジスタはnum_useを当該COPY命令に設定して
    // liverangeを伸ばす
    if ((rinfo->vreg > 0) && (rinfo->preg > 0) &&                         // vreg および preg が 0 より大きい
        ((rinfo->num_def > -1) &&                                         // (定義がある) &&
         ((rinfo->num_use == -1) || (rinfo->num_def < rinfo->num_use)) && // (参照がないor定義<参照である) &&
         ((unsigned)rinfo->num_use < rinfo->total_mi)) &&                 // (参照がカーネル終端未満である)
        (tmi->swplEKRITbl->isUseFirstVRegInExcK(rinfo->vreg))) {          // エピローグで参照から始まっている
      /*
       *   bb.5.for.body1: (kernel loop)
       *     $x2(%11) = xxx $x1(%10), 1
       *     $x3(%12) = xxx $x2(%11), 1
       *     |
       *     | この間で$x2が再利用されないよう、liverangeを伸ばす
       *     |
       *     %13 = COPY $x2(%11)    <- callSetReg()にてliveout向けCOPYを追加する
       *   bb.11.for.body1:
       *     %14 = ADD %13, 1
       */
      if( DebugSwplRegAlloc ) {
        dbgs() << "num_use: " << rinfo->num_use << " to " << rinfo->total_mi << "\n";
      }
      rinfo->updateNumUseNoReuse(rinfo->total_mi);
    }
  }
  return;
}

/**
 * @brief  レジスタ情報からレジスタ割り付け対象外であるかをチェックする
 * @param  [in] rai 割り当て済みレジスタ情報
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 * @retval true レジスタ割り付け対象でない
 * @retval false レジスタ割り付け対象である
 */
static bool checkNoPhysRegAlloc(RegAllocInfo *rai, SwplExcKernelRegInfoTbl &ekri_tbl) {
  assert(rai);
  /*
   * 以下の1～3のいずれかに該当する仮想レジスタは、物理レジスタ割り付け対象としない
   * 1. 仮想レジスタ番号が 0
   * 2. isVirtualRegister()の返却値がfalseである
   * 3. カーネル内に定義しかない かつ カーネル外にて参照から始まっていない
   *   上記3.の例）
   *   bb.5.for.body1: (kernel loop)
   *     %10 = ADD $x1, 1  <- %10はカーネル内外で参照されないため、
   *                          割り付ける意味がなく、対象としない
   *   bb.11.for.body1: (epilogue)
   *     %10 = ADD %11, 1
   */
  if ((rai->vreg == 0) || (!Register::isVirtualRegister(rai->vreg)) ||
      ((rai->num_def > -1) && (rai->num_use == -1) &&
       (!ekri_tbl.isUseFirstVRegInExcK(rai->vreg)))) {
    return true;
  }
  return false;
}

/**
 * @brief  自分およびtied相手がレジスタ割り付け対象外であるかの判定
 * @param  [in] own 割り当て済みレジスタ情報
 * @param  [in] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 * @retval true レジスタ割り付け対象でない
 * @retval false レジスタ割り付け対象である
 */
static bool isNoPhysRegAlloc(RegAllocInfo *own, SwplRegAllocInfoTbl &rai_tbl,
                             SwplExcKernelRegInfoTbl &ekri_tbl) {
  bool ret = false;
  // 自分が割り付け対象かを調べる
  ret |= checkNoPhysRegAlloc(own, ekri_tbl);

  RegAllocInfo* tied;
  if ((own->tied_vreg != 0) && (tied = rai_tbl.getWithVReg(own->tied_vreg))) {
    if( DebugSwplRegAlloc ) {
      dbgs() << " tied-register. [current](vreg: "
             << printReg(own->vreg, SWPipeliner::TRI) << ", tied_vreg: "
             << printReg(own->tied_vreg, SWPipeliner::TRI) << "), [tied](vreg: "
             << printReg(tied->vreg, SWPipeliner::TRI) << ", tied_vreg: "
             << printReg(tied->tied_vreg, SWPipeliner::TRI) << ", preg: "
             << printReg(tied->preg, SWPipeliner::TRI) << ")\n";
    }
    assert(tied->tied_vreg == own->vreg);
    // tied相手が存在すれば、tied相手に合わせて自分も割り付け対象外となるか否かを調べる
    ret |= checkNoPhysRegAlloc(tied, ekri_tbl);
    // tied相手によって既に自分にも物理レジスタ割り当て済みなら自分は割り当て処理しない
    ret |= ((tied->preg > 0) && (own->preg > 0) && (tied->preg == own->preg));
  }

  return ret;
}

/**
 * @brief  生存区間を基に仮想レジスタへ物理レジスタを割り付ける
 * @param  [in] preg 物理レジスタ
 * @param  [in] rai 割り当て済みレジスタ情報
 * @param  [in] ekri_tbl カーネル外のレジスタ情報の表
 */
static void assignPReg(unsigned preg, RegAllocInfo *rai, SwplRegAllocInfoTbl &rai_tbl) {
  assert(rai);
  rai->preg = preg;
  if (rai->tied_vreg != 0) {
    RegAllocInfo *tied = rai_tbl.getWithVReg(rai->tied_vreg);
    if (tied) tied->preg = preg;
  }
  return;
}

/**
 * @brief  生存区間を基に仮想レジスタへ物理レジスタを割り付ける
 * @param  [in,out] rai_tbl 割り当て済みレジスタ情報の表
 * @param  [in]     ekri_tbl カーネル外のレジスタ情報の表
 * @param  [in]     total_mi カーネル部の総MI数
 * @retval 0 正常終了
 * @retval 0以外 異常終了
 * @details レジスタ再利用時の「最も過去」とは、「出現した順に古い方から」という意
 */
static int physRegAllocWithLiveRange(SwplRegAllocInfoTbl &rai_tbl,
                                     SwplExcKernelRegInfoTbl &ekri_tbl,
                                     unsigned total_mi) {
  int ret = 0;

  // 割り当て済みレジスタ情報でループ
  for (size_t i = 0; i < rai_tbl.length(); i++) {
    RegAllocInfo *itr_cur =rai_tbl.getWithIdx(i);
    bool allocated = false;

    // レジスタ割り付け対象か
    if (isNoPhysRegAlloc(itr_cur, rai_tbl, ekri_tbl)) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " Following register will be not allocated a physcal register. (vreg: "
               << printReg(itr_cur->vreg, SWPipeliner::TRI) << ", preg: "
               << printReg(itr_cur->preg, SWPipeliner::TRI) << ")\n";
      }
      continue;
    }

    // オプション指定があった場合は、空いている物理レジスタから優先して割り付け
    if ((!allocated) && (SwplRegAllocPrio == PRIO_UNUSE)) {
      const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(itr_cur->vreg);
      for (auto &preg:*trc) {
        // 物理レジスタ番号取得

        // 使ってはいけない物理レジスタは割付けない
        if (nousePhysRegs.contains(preg)) continue;

        // 当該物理レジスタが割り当て可能なレジスタでなければcontinue
        if ((preg == 0) || (!SWPipeliner::MRI->isAllocatable(preg)))
          continue;

        // 割り当て済みリストに登録されていないなら当該物理レジスタを使用
        if( !(rai_tbl.isUsePReg( preg )) ) {
          assignPReg(preg, itr_cur, rai_tbl);
          allocated = true;
          break;
        }
      }
    }

    // 仮想レジスタのレジスタクラスに該当する物理レジスタ群を取得。
    // 当該物理レジスタをキーにレジスタ管理情報の表を見ていき、
    // 「生存区間が自分と重ならない」ものが見つかり次第、当該物理レジスタを割り付け。
    if (!allocated) {
      unsigned preg = rai_tbl.getReusePReg( itr_cur );
      if( preg!=0 ) {
        assignPReg(preg, itr_cur, rai_tbl);
        allocated = true;
      }
    }

    // それでも物理レジスタ割り当てできなかったら異常復帰
    if (!allocated) {
      if( DebugSwplRegAlloc ) {
        dbgs() << " failed to allocate a physical register for virtual register "
               << printReg(itr_cur->vreg) << "\n";
        rai_tbl.dump();
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
 * @param  [in]     MF MachineFunction
 * @param  [in]     pre_dl カーネルpre処理のデバッグ情報の位置
 * @param  [in]     post_dl カーネルpost処理のデバッグ情報の位置
 * @param  [in,out] tmi 命令列変換情報
 */
static void callSetReg(MachineFunction &MF,
                       DebugLoc pre_dl,
                       DebugLoc post_dl,
                       SwplTransformedMIRInfo *tmi) {
  assert(tmi);
  for (size_t i = 0; i < tmi->swplRAITbl->length(); i++) {
    RegAllocInfo *rinfo = tmi->swplRAITbl->getWithIdx(i);
    // 仮想レジスタと物理レジスタが有効な値でない限り何もしない
    if ((rinfo->vreg == 0) || (rinfo->preg == 0))
      continue;

    /*
     * Dealing with registers that span the MBB
     *   bb.10.for.body1:
     *     %11 = COPY %10
     *     $x1 = COPY %11             <- Add COPY defining $x1 (livein)
     *     SWPLIVEOUT implicit $x1
     *   bb.5.for.body1: (kernel loop)
     *     SWPLIVEIN implicit-def $x1
     *     $x2(%12) = ADD $x1(%11), 1
     *     SWPLIVEOUT implicit $x2
     *   bb.11.for.body1:
     *     SWPLIVEIN implicit-def $x2
     *     %12 = COPY $x2(%12)        <- Add COPY defining %12 (liveout)
     *     %13 = ADD %12, 1
     */
    if ((rinfo->num_def == -1) || (rinfo->num_def > rinfo->num_use))
      addCopyMIpre(MF, pre_dl, tmi->livein_copy_mis, rinfo->vreg, rinfo->preg);    // livein
    if (((rinfo->num_def > -1) && (tmi->swplEKRITbl->isUseFirstVRegInExcK(rinfo->vreg)))
        || SWPipeliner::currentLoop->containsLiveOutReg(rinfo->vreg))
      addCopyMIpost(MF, post_dl, tmi->liveout_copy_mis, rinfo->vreg, rinfo->preg); // liveout
    // 当該物理レジスタを使用するMachineOperandすべてにsetReg()する
    for (auto *mo : rinfo->vreg_mo) {
      auto preg = rinfo->preg;
      auto subRegIdx = mo->getSubReg();
      if (subRegIdx != 0) {
        preg = SWPipeliner::TRI->getSubReg(rinfo->preg, subRegIdx);
        mo->setSubReg(0);
      }
      mo->setReg(preg);
      mo->setIsRenamable(EnableSetrenamable);
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
 * @retval true  物理レジスタ割り付け成功
 * @retval false 物理レジスタ割り付け失敗
 * @note   カーネルループのみが対象。
 */
bool AArch64InstrInfo::SwplRegAlloc(SwplTransformedMIRInfo *tmi,
                                    MachineFunction &MF) const {
  DebugLoc firstDL;
  DebugLoc lastDL;
  std::unordered_set<unsigned> exclude_vreg;

  /// epilogue部の仮想レジスタのリストを作成する
  tmi->swplEKRITbl = new SwplExcKernelRegInfoTbl();
  unsigned num_mi = 0;
  for (size_t i = tmi->kernelEndIndx; i < tmi->epilogEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi != nullptr) {
      ++num_mi;
      createVRegListEpi(mi, num_mi, *(tmi->swplEKRITbl));
    }
  }

  /// kernel部のMachineInstrを対象とする
  /// まず初めに有効な総MI数を数えつつ、割り付け対象としないレジスタのリストを作成する
  unsigned total_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    if (total_mi == 0)
      firstDL = mi->getDebugLoc();
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

  num_mi = 0;
  for (size_t i = tmi->prologEndIndx; i < tmi->kernelEndIndx; ++i) {
    MachineInstr *mi = tmi->mis[i];
    if (mi == nullptr)
      continue;
    ++num_mi;
    if (num_mi==total_mi)
      lastDL = mi->getDebugLoc();

    // MIのオペランド数でループ
    for (unsigned opIdx = 0, e = mi->getNumOperands(); opIdx != e; ++opIdx) {
      MachineOperand &mo = mi->getOperand(opIdx);
      // レジスタオペランドなら以降の処理継続
      // レジスタ番号 0 はいかなる種類のレジスタも表していない
      unsigned reg = 0;
      if ((!mo.isReg()) || ((reg = mo.getReg()) == 0))
        continue;

      // 各仮想レジスタの生存区間リストを作成する
      if (createLiveRange(mi, opIdx, mo, reg, *(tmi->swplRAITbl), num_mi,
                          total_mi, exclude_vreg, tmi->doVReg) != 0) {
        assert(0 && "createLiveRange() failed");
        return false;
      }
    }
  }

  extendLiveRange(tmi);

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete createLiveRange()\n";
    (*tmi->swplRAITbl).dump();
  }

  // 物理レジスタ情報を割り当てる
  if (physRegAllocWithLiveRange(*(tmi->swplRAITbl),
                                *(tmi->swplEKRITbl), total_mi) != 0) {
    // 割り当て失敗
    return false;
  }

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete physRegAllocWithLiveRange()\n";
  }

  // setReg()呼び出し
  callSetReg(MF, firstDL, lastDL, tmi);

  if( DebugSwplRegAlloc ) {
    dbgs() << "complete callSetReg()\n";
    (*tmi->swplRAITbl).dump();
    (*tmi->swplEKRITbl).dump();
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
 * @brief  Determine if the LiveRange of rows in the Survival Interval Table overlap.
 * @param  [in]  reginfo1 RegAllocInfo for comparison
 * @param  [in]  reginfo2 RegAllocInfo for comparison
 * @retval true  LiveRange overlap
 * @retval false LiveRange does not overlap
 */
bool SwplRegAllocInfoTbl::isOverlapLiveRange( RegAllocInfo *reginfo1, RegAllocInfo *reginfo2) {
  int def1 = reginfo1->num_def;
  int use1 = reginfo1->num_use;
  
  int def2 = reginfo2->num_def;
  int use2 = reginfo2->num_use;

  if(SWPipeliner::STM->isEnableProEpiCopy()) {
    // // livein case, make sure that the register is not destroyed.
    if (def1<0 || def2<0) return true;
  }

  for(int i=1; i<=(int)total_mi; i++) {    // num_def/num_use starts counting from 1
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
 * @brief  SwplExcKernelRegInfoTblにExcKernelRegInfoエントリを追加する
 * @param  [in] vreg 仮想レジスタ番号
 * @param  [in] num_def 定義番号
 * @param  [in] num_use 参照番号
 */
void SwplExcKernelRegInfoTbl::addExcKernelRegInfo(
                                     unsigned vreg,
                                     int num_def,
                                     int num_use) {
  ekri_tbl.push_back({vreg, num_def, num_use});
  return;
}

/**
 * @brief  テーブルの中から、指定された仮想レジスタに該当するエントリを返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval 非nullptr 指定された仮想レジスタに該当するExcKernelRegInfoへのポインタ
 * @retval nullptr   指定された仮想レジスタに該当するExcKernelRegInfoは存在しない
 */
ExcKernelRegInfo* SwplExcKernelRegInfoTbl::getWithVReg(unsigned vreg) {
  std::vector<ExcKernelRegInfo>::iterator itr =
    find_if(ekri_tbl.begin(), ekri_tbl.end(),
            [&](ExcKernelRegInfo &info){
              return(info.vreg == vreg);
            });
  if (itr == ekri_tbl.end())
    return nullptr;

  return &(*itr);
}

/**
 * @brief  指定された仮想レジスタがカーネル外で参照から始まっている行の有無を返す
 * @param  [in] vreg 仮想レジスタ番号
 * @retval true 指定された仮想レジスタが参照から始まっている行あり
 * @retval false 指定された仮想レジスタが参照から始まっている行なし
 */
bool SwplExcKernelRegInfoTbl::isUseFirstVRegInExcK(unsigned vreg) {
  std::vector<ExcKernelRegInfo>::iterator itr =
    find_if(ekri_tbl.begin(), ekri_tbl.end(),
            [&](ExcKernelRegInfo &info){
              return((info.vreg == vreg) && (info.num_use > -1) &&
                     ((info.num_def == -1) ||           // 参照のみか
                      (info.num_def > info.num_use)));  // 定義>参照か
            });
  if (itr == ekri_tbl.end())
    return false;

  return true;
}

/**
 * @brief  カーネル外レジスタ情報 SwplExcKernelRegInfoTbl のデバッグプリント
 */
void SwplExcKernelRegInfoTbl::dump() {
  dbgs() << "Virtual registers of the epilogue.\n"
         << "No.\tvreg\tdef\tuse\n";
  for (size_t i = 0; i < ekri_tbl.size(); i++) {
    ExcKernelRegInfo *ri_p = &(ekri_tbl[i]);
    dbgs() << i << "\t"
           << printReg(ri_p->vreg, SWPipeliner::TRI) << "\t" // 仮想レジスタ
           << ri_p->num_def << "\t"      // 仮想レジスタの定義番号
           << ri_p->num_use << "\n";     // 仮想レジスタの参照番号
  }
  return;
}

/**
 * @brief  SwplRegAllocInfoTblにRegAllocInfoエントリを追加する
 * @param  [in] vreg 仮想レジスタ番号
 * @param  [in] num_def 定義番号
 * @param  [in] num_use 参照番号
 * @param  [in] mo llvm::MachineOperand
 * @param  [in] tied_vreg tiedの場合、その相手となる仮想レジスタ
 * @param  [in] preg 物理レジスタ番号
 */
void SwplRegAllocInfoTbl::addRegAllocInfo( unsigned vreg,
                                           int num_def,
                                           int num_use,
                                           MachineOperand *mo,
                                           unsigned tied_vreg,
                                           unsigned preg) {
  const TargetRegisterClass *trc;

  assert(mo);

  unsigned clsid = 0;
  if( vreg != 0 ) {
    trc = SWPipeliner::MRI->getRegClass(vreg);
    clsid = trc->getID();
  }
  rai_tbl.push_back({vreg, preg, num_def, num_use, -1, tied_vreg, clsid, {mo}, total_mi});
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

  // tied相手のレジスタ情報を取得する
  RegAllocInfo *tied = nullptr;
  if (rai->tied_vreg != 0)
    tied = getWithVReg(rai->tied_vreg);

  // レジスタクラスに該当するレジスタごとに、
  // 既に割り当て済みの行を収集し、そのすべての生存区間が重なっていなければ、
  // 再利用可能とする。
  const TargetRegisterClass *trc = SWPipeliner::MRI->getRegClass(rai->vreg);
  for (auto itr = trc->begin(), itr_end = trc->end();
       itr != itr_end; ++itr) {
    unsigned candidate_preg = *itr;
    std::vector<RegAllocInfo*> ranges;

    // 当該物理レジスタが割り当て可能なレジスタでなければcontinue
    if ((nousePhysRegs.contains(candidate_preg)) || (candidate_preg == 0) ||
        (!SWPipeliner::MRI->isAllocatable(candidate_preg))) {
      continue;
    }

    for (size_t j = 0; j < rai_tbl.size(); j++) {
      RegAllocInfo *wk_reginfo = &(rai_tbl[j]);
      if( isPRegOverlap( candidate_preg, wk_reginfo->preg ) ) {
        ranges.push_back( wk_reginfo );
      }
    }

    bool isoverlap = false;
    for(unsigned i=0; i<ranges.size(); i++) {
      if ((isOverlapLiveRange(ranges[i], rai)) ||
          ((tied) && (isOverlapLiveRange(ranges[i], tied)))) {
         // 「自分のliverange」もしくは「tied相手のliverange」のどちらかがチェック対象と重なる
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
  return SWPipeliner::TRI->regsOverlap(preg1, preg2);
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
    // 物理的に重複するレジスタは"使用されている"
    if(isPRegOverlap(usedpreg, preg))
      return true;
  }
  return false;
}

/**
 * @brief  レジスタ割り当て情報 SwplRegAllocInfoTbl のデバッグプリント
 */
void SwplRegAllocInfoTbl::dump() {
  dbgs() << "Information of the registers of the kernel. MI=" << total_mi << "\n"
         << "No.\tvreg\tpreg\tdef\tuse\trange\ttied\tclass\tMO\n";
  for (size_t i = 0; i < rai_tbl.size(); i++) {
    RegAllocInfo *ri_p = &(rai_tbl[i]);
    dbgs() << i << "\t"
           << printReg(ri_p->vreg, SWPipeliner::TRI) << "\t" // 割り当て済み仮想レジスタ
           << printReg(ri_p->preg, SWPipeliner::TRI) << "\t" // 割り当て済み物理レジスタ
           << ri_p->num_def << "\t"             // 仮想レジスタの定義番号
           << ri_p->num_use << "\t"             // 仮想レジスタの参照番号
           << ri_p->liverange  << "\t"          // 生存区間(参照番号-定義番号)
           << printReg(ri_p->tied_vreg, SWPipeliner::TRI) << "\t";  // tied仮想レジスタ
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
        dbgs() << printReg(m->getReg(), SWPipeliner::TRI, m->getSubReg());
      } else {
        m->print(dbgs());
      }
      dbgs() << " ";
    }
    dbgs() << " ]\n";
  }
  return;
}

/**
 * @brief SwplRegAllocInfoTblの初期化関数
 * @param [in] num_of_mi MIの総数
 */
SwplRegAllocInfoTbl::SwplRegAllocInfoTbl(unsigned num_of_mi) {
  total_mi=num_of_mi;
  rai_tbl={};
};

/**
 * @brief レジスタ割り付け情報からliverangeを求める
 * @param [in] range        liverange
 * @param [in] regAllocInfo レジスタ割り付け情報
 */
void SwplRegAllocInfoTbl::setRangeReg(std::vector<int> *range, RegAllocInfo& regAllocInfo, unsigned unitNum) {
  if (regAllocInfo.num_def < 0) {
    for (int i = 1; i <= regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
  } else if (regAllocInfo.num_use > 0 && regAllocInfo.num_def >= regAllocInfo.num_use) {
    for (int i=1; i<=regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)+=unitNum;
    }
  } else if (regAllocInfo.num_use < 0) {
    for (unsigned i = regAllocInfo.num_def; i <= total_mi ; i++) {
      range->at(i)+=unitNum;
    }
  } else {
    for (int i = regAllocInfo.num_def; i <= regAllocInfo.num_use; i++) {
      range->at(i)+=unitNum;
    }
  }
}

/**
 * @brief Integer/Floating-point/Predicateレジスタに割り当てた最大レジスタ数を数える
 */
void SwplRegAllocInfoTbl::countRegs() {
  std::vector<int> ireg;
  std::vector<int> freg;
  std::vector<int> preg;

  ireg.resize(total_mi+1);
  freg.resize(total_mi+1);
  preg.resize(total_mi+1);

  unsigned rk, units;

  for (auto &regAllocInfo:rai_tbl) {
    if (regAllocInfo.vreg==0) {
      // SWPL化前から実レジスタが割り付けてある
      std::tie(rk, units) = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, regAllocInfo.preg);
      if (rk == StmRegKind::getCCRegID() || rk == 0) {
        // CCレジスタは計算から除外する
        continue;
      }
      if (rk == StmRegKind::getIntRegID()) {
        if (nousePhysRegs.contains(regAllocInfo.preg)) continue;
        // レジスタ割付処理で割付禁止レジスタが、割り付けてあるため、計算から除外する
      }
    } else {
      if (regAllocInfo.preg == 0) {
        // 実レジスタを割り付けていないので、計算から除外する
        continue;
      }
      std::tie(rk, units) = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, regAllocInfo.vreg);
    }
    if (rk == StmRegKind::getIntRegID()) {
      setRangeReg(&ireg, regAllocInfo, units);
    } else if (rk == StmRegKind::getFloatRegID()) {
      setRangeReg(&freg, regAllocInfo, units);
    } else if (rk == StmRegKind::getPredicateRegID()) {
      setRangeReg(&preg, regAllocInfo, units);
    } else {
      llvm_unreachable("unnown reg class");
    }
  }
  // レジスタ割付では、なぜか１から始まるので、以下のforでは0番目を処理しているがほんの少し無駄になっている
  for (int i:ireg) num_ireg = std::max(num_ireg, i);
  for (int i:freg) num_freg = std::max(num_freg, i);
  for (int i:preg) num_preg = std::max(num_preg, i);
}

/**
 * @brief  Integerレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countIReg() {
  if (num_ireg < 0) countRegs();
  return num_ireg;
}

/**
 * @brief  Floating-pointレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countFReg()  {
  if (num_freg < 0) countRegs();
  return num_freg;
}

/**
 * @brief  Predicateレジスタに割り当てた最大レジスタ数を返す
 * @retval 0以上の値 割り当てた最大レジスタ数
 */
int SwplRegAllocInfoTbl::countPReg()  {
  if (num_preg < 0) countRegs();
  return num_preg;
}

/**
 * @brief  割り当て可能なIntegerレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availableIRegNumber() const {
  // UNAVAILABLE_REGがXおよびWを定義しているため、数は半分にしている
  return AArch64::GPR64RegClass.getNumRegs()-(UNAVAILABLE_REGS/2);
}

/**
 * @brief  割り当て可能なFloating-pointレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availableFRegNumber() const {
  return AArch64::FPR64RegClass.getNumRegs();
}

/**
 * @brief  割り当て可能なPredicateレジスタの数を返す
 * @retval 0以上の値 利用可能なレジスタ数
 */
int SwplRegAllocInfoTbl::availablePRegNumber() const {
  return AArch64::PPR_3bRegClass.getNumRegs();
}
