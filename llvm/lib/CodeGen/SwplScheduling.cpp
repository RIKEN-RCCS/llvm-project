//=- SwplScheduling.cpp - Scheduling process in SWPL -*- C++ -*--------------=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Classes that processing scheduling in SWPL.
//
//===----------------------------------------------------------------------===//

#include "SwplScheduling.h"
#include "SWPipeliner.h"
#include "SwplCalclIterations.h"
#include "SwplPlan.h"
#include "SwplRegEstimate.h"
#include "SwplScr.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include <cmath>

using namespace llvm;
using namespace ore; // for NV
#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<bool> OptionDumpMrt("swpl-debug-dump-mrt",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionBsearch("swpl-ii-by-Bsearch",cl::init(true), cl::ReallyHidden);

static cl::opt<unsigned> OptionMveLimit("swpl-mve-limit",cl::init(30), cl::ReallyHidden);

static cl::opt<unsigned> OptionBudgetRatioThreshold("swpl-budget-ratio-threshold",
                                                  cl::init(100), cl::ReallyHidden);
static cl::opt<double> OptionBudgetRatioLess("swpl-budget-ratio-less",cl::init(50.0), cl::ReallyHidden);
static cl::opt<double> OptionBudgetRatioMore("swpl-budget-ratio-more",cl::init(25.0), cl::ReallyHidden);

static cl::opt<bool> OptionDumpModuleDdg("swpl-debug-dump-moduloddg",cl::init(false), cl::ReallyHidden);

static cl::opt<unsigned> OptionMaxIreg("swpl-max-ireg",cl::init(0), cl::ReallyHidden);
static cl::opt<unsigned> OptionMaxFreg("swpl-max-freg",cl::init(0), cl::ReallyHidden);
static cl::opt<unsigned> OptionMaxPreg("swpl-max-preg",cl::init(0), cl::ReallyHidden);

static cl::opt<bool> OptionDumpEveryInst("swpl-debug-dump-scheduling-every-inst",cl::init(false), cl::ReallyHidden);


static cl::opt<bool> OptionEnableStageScheduling("swpl-enable-stagescheduling",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionDumpSSProgress("swpl-debug-dump-ss-progress",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionDumpCyclicRoots("swpl-debug-dump-ss-cyclicroots",cl::init(false), cl::ReallyHidden);

namespace llvm{

/// \brief 命令が使用する資源を予約する
/// \details 命令が使用する資源をMrtに記録する。
///          疑似命令など資源を使用しない命令の場合は、Mrtでの予約はしない。
/// \param[in] cycle 命令を配置するcycle
/// \param[in] inst
/// \param[in] pipeline instが使用する資源情報
/// \return なし
///
/// \note どの資源使用パターンかわからないと、Mrtの埋めようが無い。
///       資源使用パターンを引数に追加しても、本関数で資源情報の問い合わせが
///       発生するため、引数で資源情報を貰うこととした。
/// \note 引数pipelineは、資源使用パターンに応じたTmPipeline情報
void SwplMrt::reserveResourcesForInst(unsigned cycle,
                                      const SwplInst& inst,
                                      const StmPipeline & pipeline ) {
  // cycle:1, stage:{ 1, 5, 9 }, resources:{ ID1, ID2, ID3 }
  //   ->     resource ID1  ID2  ID3  ID4
  //      cycle   :  1 予約
  //                 2
  //                 3
  //                 4
  //                 5      予約
  //                 6
  //                 7
  //                 8
  //                 9           予約

  // cycle:1, stage:{ 1, 2, 3 }, resources:{ ID1, ID1, ID1 }
  //   ->     resource ID1  ID2  ID3  ID4
  //      cycle   :  1 予約
  //                 2      予約
  //                 3           予約

  // cycle:1, stage:{ 1, 1, 4 }, resources:{ ID1, ID2, ID3 }
  //   ->     resource ID1  ID2  ID3  ID4
  //      cycle   :  1 予約 予約
  //                 2
  //                 3
  //                 4           予約

  // cycle:1, stage:{ 1, 1, 4, 5 }, resources:{ ID1, ID2, ID3, ID3 }
  //   ->     resource ID1  ID2  ID3  ID4
  //      cycle   :  1 予約 予約
  //                 2
  //                 3
  //                 4           予約
  //                 5           予約

  assert( (pipeline.resources.size()==pipeline.stages.size()) && "Unexpected resource information.");

  // 疑似命令など資源を使用しない命令の場合は、Mrtでの予約はしない。
  if( pipeline.resources.size() == 0 ) {
    return;
  }

  unsigned start_cycle = cycle;
  unsigned modulo_cycle;
  for(unsigned i=0; i<pipeline.resources.size(); i++) {

    modulo_cycle = (start_cycle+pipeline.stages[i]) % iteration_interval;

    // check already been reserved.
    if( table[modulo_cycle]->count(pipeline.resources[i]) != 0 &&
        table[modulo_cycle]->at(pipeline.resources[i]) != &inst ) {
        assert( "Resource that has already been reserved.");
    }

    (*(table[modulo_cycle]))[pipeline.resources[i]]=&inst;
  }
  return;
}

/// \brief 配置に支障のある命令を探す
/// \details 第１引数のslotに第２引数のinstructionが入った場合に、
///          sheduleの支障となるscheduling済みinstructionのSetを返す。
/// \param[in] cycle 命令を配置しようとしているcycle
/// \param[in] inst 配置しようとしている命令
/// \param[in] pipeline 命令が使用する資源情報
/// \return 支障となるInstructionのSet
///
/// \note 返却するポインタの領域は呼出し元で解放する必要がある。
/// \note どの資源使用パターンかわからないとチェックできないため、
///       引数で資源情報を貰うこととした。
/// \note 引数の資源パターンに競合する配置済み命令を探すため、引数instは冗長である。
///       （現在のところ使い道なし）
/// \note ロジックはreserveResourcesForInstとほぼ同じ。
///       予約するのではなく、すでに予約されていれば、そのInstを記録していく。
SwplInstSet* SwplMrt::findBlockingInsts(unsigned cycle,
                                        const SwplInst& inst,
                                        const StmPipeline & pipeline)
{
  SwplInstSet* blocking_insts = new SwplInstSet();
  assert( (pipeline.resources.size()==pipeline.stages.size()) && "Unexpected resource information.");

  unsigned start_cycle = cycle;
  unsigned modulo_cycle;
  for(unsigned i=0; i<pipeline.resources.size(); i++) {

    modulo_cycle = (start_cycle+pipeline.stages[i]) % iteration_interval;

    // 既にリソースを使用している命令があればblocking_instに保持
    if( table[modulo_cycle]->count(pipeline.resources[i]) != 0 ) {
      blocking_insts->insert((*(table[modulo_cycle]))[pipeline.resources[i]]);
    }
  }
  return blocking_insts;
}

/// \brief 命令配置のための 資源の空きがあるかを確認する
/// \details 命令配置のための 資源の空きがあるかを確認する。
///          findBlockingInstsの結果で判断する。
/// \param[in] cycle 命令を配置しようとしているcycle
/// \param[in] inst 配置しようとしている命令
/// \param[in] pipeline 命令が使用する資源情報
/// \retval true 命令配置のための資源が空いている
/// \retval false 命令配置のための資源が空いていない
///
/// \note 引数の資源パターンに競合する配置済み命令を探すため、引数instは冗長である。
///       （現在のところ使い道なし）
bool SwplMrt::isOpenForInst(unsigned cycle,
                            const SwplInst& inst,
                            const StmPipeline & pipeline) {
  SwplInstSet* blocking_insts;
  bool is_open;

  blocking_insts = findBlockingInsts(cycle, inst, pipeline);
  is_open = blocking_insts->empty();
  delete blocking_insts;
  return is_open;
}

/// \brief 指定のinstをMrtから削除する
/// \param[in] inst
/// \return なし
///
/// \note SwplInstはMachineInstrと一対一になるので、
///       Mrt上のinstに該当する情報を削除するという処理となっている。
void SwplMrt::cancelResourcesForInst(const SwplInst& inst) {
  for( unsigned i=0; i<table.size(); i++ ) {
    for(auto itr = table[i]->begin(); itr != table[i]->end();) {
      if(itr->second == &inst) {
        table[i]->erase(itr++);
      }
      else {
        itr++;
      }
    }
  }
  return;
}

/// \brief スケジューリング結果をMRTの枠組で表示する
///
/// ```
///   下記のようなループを例とする
///   DO I=1,1024
///   A(I) =  A(I) + B(I)
///   ENDDO
///
///   例として-Kfast,nosimd,nounroll -W0,-zswpl=dump-mrtで翻訳すると,
///   下記のようにMRT結果が表示される.
///   縦軸はcycle,横軸が命令発行Slotを表しており,左上の命令から順に発行される。
///   命令の括弧内数字は,もとのループの何回転目の命令を利用したかを表示している。
///   これから,A(4) + B(4)のfaddがA(1)=のstoreと同じcycleに発行されている事がわかる.
///
///   0  0  0  0  ffload (6)  0           ffload (6)  add    (6)
///   0  0  0  0  ffstore(1)  ffadd  (4)  add    (6)  sub    (1)
/// ```
/// \param [in] inst_slot_map
/// \param [in] stream 出力stream
/// \return なし
void SwplMrt::dump(const SwplSlots& slots, raw_ostream &stream) {
  unsigned modulo_cycle;
  StmResourceId resource_id;
  unsigned res_max_length = 0;
  unsigned word_width, inst_gen_width, inst_rotation_width;
  unsigned ii = iteration_interval;
  unsigned numresource = SWPipeliner::STM->getNumResource();

  assert(ii==table.size());

  // Resource名の最大長の取得
  // resource_idはA64FXRes::PortKindのenum値に対応する。
  // １～STM->getNumResource()をリソースとして扱う 。
  // ０はP_NULLであり、リソースではない。
  for (resource_id = 1; (unsigned)resource_id <= numresource; ++resource_id) {
    res_max_length = std::max( res_max_length, (unsigned)(strlen(SWPipeliner::STM->getResourceName(resource_id))) );
  }

  inst_gen_width = std::max( getMaxOpcodeNameLength()+1, res_max_length);  // 1 = 'p|f' の分
  inst_rotation_width = ( slots.size() > 0 ) ? 4 : 0;              // 4 = '(n) 'の分
  word_width = inst_gen_width+inst_rotation_width;

  // Resource名の出力
  stream << "\n       ";
  for (resource_id = 1; (unsigned)resource_id <= numresource; ++resource_id) {
    stream << SWPipeliner::STM->getResourceName(resource_id);
    for (unsigned l=strlen(SWPipeliner::STM->getResourceName(resource_id)); l<word_width; l++) {
      stream << " ";
    }
  }
  stream << "\n";
  for (unsigned l=0; l<(resource_id*word_width);l++) {
    stream << "-";
  }
  stream << "\n";

  // 各cycle毎に資源に対応する命令を出力する
  for (modulo_cycle = 0; modulo_cycle < ii; ++modulo_cycle) {
    stream << "       ";
    // resource_idはA64FXRes::PortKindのenum値に対応する。
    // １～STM->getNumResource()をリソースとして扱う 。
    // ０はP_NULLであり、リソースではない。
    for (resource_id = 1; (unsigned)resource_id <= numresource; ++resource_id) {
      auto pr = table[modulo_cycle]->find(resource_id);

      if( pr == table[modulo_cycle]->end() ) {
        stream << "0";
        for(unsigned i=1; i<word_width; i++) {
          stream << " ";
        }
      }
      else {
        // オペコード名と回転数の出力
        printInstMI(stream, pr->second /* inst */, inst_gen_width);
        if( inst_rotation_width != 0 ) {
          printInstRotation( stream, slots, pr->second /* inst */, ii);
        }
      }
    }
    stream << "\n"; // 1 cycle分終了
  }
  return;
}

/// \brief デバッグ向けMRTダンプ
/// \details デバッグ向けMRTダンプ。inst_slot_mapを用いたrotationは出力しない。
void SwplMrt::dump() {
  SwplSlots dummy;
  dbgs() << " iteration_interval = " << iteration_interval << "\n";
  dump( dummy, dbgs() );
  return;
}

/// \brief Mrt中のSwplInstのオペコード名の中で、最長の長さを取得する
/// \return オペコード名の最長の長さ
unsigned SwplMrt::getMaxOpcodeNameLength() {
  unsigned max_length=0;
  for( auto *cl : table ) {
    for( auto &pr : *cl ) {
      max_length = std::max( max_length,
                             (unsigned)(pr.second->getName().size()+1) );
    }
  }
  return max_length;
}

/// \brief MRTを準備する
/// \details 引数のiiからMRTを準備し、SwplMrtのポインタを返す。
/// \param[in] iteration_interval 生成するMRTのii
/// \return SwplMrtのポインタ
SwplMrt* SwplMrt::construct (unsigned iteration_interval) {
  SwplMrt*  mrt = new SwplMrt(iteration_interval);
  unsigned ii = mrt->iteration_interval;
  for(unsigned i=0; i<ii; i++) {
    mrt->table.push_back( (new std::map<StmResourceId, const SwplInst*>()) );
  }
  return mrt;
}

/// \brief MRTを解放する
/// \param[in] mrt 解放するSwplMrtのポインタ
/// \return なし
void SwplMrt::destroy(SwplMrt* mrt) {
  // tableの要素であるmap域の解放
  for(unsigned i=0; i<(mrt->iteration_interval); i++) {
    delete mrt->table[i];
  }
  delete mrt;
  return;
}

/// \brief Instのオペコード名を出力する
/// \details Instのオペコード名を出力する。widthまでの余りは空白を出力する。
/// \param [in] stream 出力stream
/// \param [in] inst 対象の命令
/// \param [in] width オペコード名の出力幅
/// \return なし
void SwplMrt::printInstMI(raw_ostream &stream,
                           const SwplInst* inst, unsigned width) {
  unsigned prechr = 0;
  if( inst->isDefinePredicate() ) {
    stream << 'p';
    prechr = 1;
  } else if(  inst->isFloatingPoint() ) {
    stream << 'f';
    prechr = 1;
  }

  stream << inst->getName();

  assert( prechr + inst->getName().size() <= width );
  for (unsigned l=(prechr+inst->getName().size()); l<width; l++) {
    stream << " ";
  }

  return;
}

/// \brief instの回転数を出力する
/// \param [in] stream 出力stream
/// \param [in] inst_slot_map
/// \param [in] inst 対象の命令
/// \param [in] ii iteration interval
/// \detaile  Instの回転数を '(n) 'の形式で出力する。
void SwplMrt::printInstRotation(raw_ostream &stream,
                                const SwplSlots& slots,
                                const SwplInst* inst, unsigned ii) {
  SwplSlot slot;
  int rotation;

  /* inst_slot_mapからrotationの値を取得する */

  if ((slot = slots.at(inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
    report_fatal_error("instruction not found in Slots.");
  }

  unsigned max_slot = SwplSlot::baseSlot(ii);
  for (auto& t : slots) {
    if (t == SwplSlot::UNCONFIGURED_SLOT) continue;
    if (max_slot < t) max_slot = t;
  }

  unsigned max_cycle = max_slot / SWPipeliner::FetchBandwidth;
  rotation = (max_cycle - slot.calcCycle())/ii + 1;

  // 対応する回転数を表示
  stream << "(" << rotation << ") ";

  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief 配置する命令の優先順位を求める
/// \return priorities ( map ; key=priority, valud=SwplInst* )
///
/// \note 領域の解放は呼出し元の責任で実施する。
SwplInstIntMap* SwplModuloDdg::computePriorities() const {
  SwplInstIntMap* priorities;
  size_t n_body_insts, i;

  priorities = new SwplInstIntMap();
  n_body_insts = loop.getSizeBodyInsts();
  for (i = 0; i < n_body_insts; ++i) {
    const SwplInst& inst = loop.getBodyInst(i);
    int priority = i;
    priorities->insert( std::make_pair( &inst, priority ) );
  }
  return priorities;
}

/// \brief ２命令間のdelayを求める
/// \details calc delay for former_inst to latter_inst
/// \param [in] _former_inst former instruction
/// \param [in] _latter_inst latter instruction
/// \return delay
int SwplModuloDdg::getDelay(const SwplInst& c_former_isnt, const SwplInst& c_latter_inst) const {
  SwplInstEdge* edge;
  int delay;

  edge = graph.findEdge( c_former_isnt, c_latter_inst );
  assert (edge != nullptr);

  assert(modulo_delay_map->find(edge) != modulo_delay_map->end());
  delay =  modulo_delay_map->find(edge)->second;

  if( c_former_isnt.isPrefetch() ) {
    return std::min(delay, (int)iterator_interval);
  } else {
    return delay;
  }
}

/// \brief 該当のループのDebugLocを返す
/// \return DebugLoc of loop.
llvm::DebugLoc SwplModuloDdg::getLoopStartLoc() const {
  return loop.getML()->getStartLoc();
}

/// \brief SwplInstのSuccessorの命令群を返す
/// \param[in] c_inst 対象の命令
/// \return number of Successors.
const SwplInsts& SwplModuloDdg::getSuccessors(const SwplInst& c_inst) const {
  return graph.getSuccessors(const_cast<SwplInst&>(c_inst));
}

/// \brief SwplInstのPredecessorの命令群を返す
/// \param[in] c_inst 対象の命令
/// \return number of predecessors.
const SwplInsts& SwplModuloDdg::getPredecessors(const SwplInst& c_inst) const {
  return graph.getPredecessors(const_cast<SwplInst&>(c_inst));
}

/// \brief SwplModuloDdgを生成する
/// \details SwplModuloDdgの領域を獲得し、引数の情報にて初期設定し、返却する。
/// \param [in] c_ddg データ依存情報
/// \param [in] iterator_interval II
/// \return 生成したSwplModuloDdg
///
/// \note 領域の解放は呼出し元の責任で実施する。
SwplModuloDdg* SwplModuloDdg::construct(const SwplDdg& c_ddg, unsigned iterator_interval) {
  SwplModuloDdg* modulo_ddg = new SwplModuloDdg( c_ddg.getGraph(),
                                                 c_ddg.getLoop() );
  modulo_ddg->iterator_interval = iterator_interval;
  modulo_ddg->modulo_delay_map = c_ddg.getModuloDelayMap(iterator_interval);
  return modulo_ddg;
}

/// \brief SwplModuloDdgを解放する
/// \details SwplModuloDdgの領域を解放する。メンバのmodulo_delay_map域も開放する。
/// \param [in] SwplModuloDdgのポインタ
/// \return なし
void SwplModuloDdg::destroy(SwplModuloDdg* modulo_ddg) {
  delete modulo_ddg->modulo_delay_map;
  delete modulo_ddg;
  return;
}

/// \brief デバッグ向けSwplModuloDdgダンプ
/// \return なし
void SwplModuloDdg::dump() {
  assert( modulo_delay_map != nullptr );

  dbgs() << "-- ModuloDdg dump --\n";
  dbgs() << "iterator_interval : " <<iterator_interval << "\n";
  for( auto *inst : loop.getBodyInsts() ) {
    inst->getMI()->print( dbgs() );

    dbgs() << "\tPipelines\n";
    if(SWPipeliner::STM->isPseudo( *(inst->getMI()) ) ) {
      dbgs() << "\t\tNothing...(pseudo instruction)\n";
    }
    else {
      for( auto *pl : *(SWPipeliner::STM->getPipelines( *(inst->getMI()) ) ) ){
        dbgs() << "\t\t";
        SWPipeliner::STM->print( dbgs(), *pl );
      }
    }

    dbgs() << "\tEdge to\n";
    if( modulo_delay_map->size() == 0 ) {
      dbgs() << "\t\tNothing...\n";
    }
    else {
      for( auto &mp : *modulo_delay_map ) {
        if( mp.first->getInitial() == inst ) {
          dbgs() << "\t\tdelay " << format("%2d : ", mp.second);
          mp.first->getTerminal()->getMI()->print( dbgs() );
        }
      }
    }
  }
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief ひとつのInstをIMSにより配置する
/// \details state->inst_queueから１つのInstを取り出して、IMSにて配置する。
/// \param [in,out] state スケジューリング情報および結果を保持する構造体
/// \retval true 配置処理が適切におこなわれた
/// \retval false 配置処理ができなかった
///
/// \note schedulingがある種の無限ループのような状況になってしまい,
///       使用するSlotが上下限を越えることがある。
///       その場合は本処理結果がfalseとなする。
bool SwplTrialState::tryNext() {
  unsigned begin_cycle;
  unsigned end_cycle;
  SwplSlot slot;
  unsigned ii = iteration_interval;

  // 未配置キュー（state->inst_queue）の先頭のInstを取得。
  // 取り出した時点では、取り出したInstはstate->inst_queueに残る。
  const SwplInst& inst = getInstToBeScheduled();

  if( OptionDumpEveryInst ) {
    dbgs() << "=========================================\n";
    assert( !(inst.isPhi()) );
    inst.getMI()->print(dbgs());
  }

  // 配置可能なlatestのcycleを求める。
  begin_cycle = calculateLatestCycle(inst);
  end_cycle = begin_cycle - ii;
  // begin_sycleからend_cycleに遡って、instを配置するslotを見つける
  SlotInstPipeline SIP = chooseSlot(begin_cycle, end_cycle, inst);

  // 20回転以上またいだ命令が候補になった場合、
  // まともな結果にならないためやめる.
  if ((SwplSlot::baseSlot(ii) - SIP.slot)/(SWPipeliner::FetchBandwidth * ii) > 20) {
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "DBG(SwplTrialState::tryNext) "
             << "gave up scheduling because it was arranged across more than 20 stages.(II="
             << ii
             << ")\n";
    }
    return false;
  }

  // 配置が繰り返し行なわれ,SLOTの上下限を越えた場合
  if (SIP.slot == SWPL_ILLEGAL_SLOT) {
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "DBG(SwplTrialState::tryNext) "
             << "gave up scheduling because the upper and lower limits of SLOT have been exceeded.(II="
             << ii
             << ")\n";
    }
    return false;
  }

  // 命令配置。以下が更新される。
  // ・state->inst_slot_map
  // ・state->inst_last_slot_map
  // ・state->mrt
  // また、state->inst_queueから、配置したInstが取り除かれる。
  scheduleInst(SIP);

  if( OptionDumpEveryInst ) {
    dbgs() << " begin_cycle          : " << begin_cycle << "\n";
    dbgs() << " SIP.slot             : " << SIP.slot << "\n";
    dbgs() << " SIP.slot.calcCycle() : " << SIP.slot.calcCycle() << "\n";
    getMrt()->dump();
    getInstSlotMap()->dump(modulo_ddg.getLoop());
  }

  return true;
}

/// \brief すべての命令が配置済みかを返す
/// \details すべての命令が配置済みかを返す。
///          state->inst_queueにInstが残っていなければ、すべての命令が配置済みとなる。
/// \retval true すべての命令が配置済みである
/// \retval false 配置できていない命令が存在する
bool SwplTrialState::isCompleted() {
  assert(inst_queue);
  return inst_queue->empty();
}

/// \brief SwplTrialStateのMRTをダンプする
/// \param [in] stream 出力stream
/// \return なし
void SwplTrialState::dump( raw_ostream &stream ) {
  stream << "!swp-msg: line ";
  getLoopStartLoc().print(stream);
  stream <<":\n";
  stream << "(mrt\n";
  stream << "  II = " << iteration_interval << "\n";
  mrt->dump(*slots, stream);
  stream <<  ")\n";
  return;
}

/// \brief priority queueの先頭にあるinstを返す
/// \details priority queue（state->inst_queue）の先頭にあるinstを返す。
/// \return priority queue（state->inst_queue）の先頭のinst
const SwplInst& SwplTrialState::getInstToBeScheduled() const {
  auto pair = inst_queue->begin();
  return *(pair->second);
}

/// \brief instのsuccessorかつ配置済みの命令のうち、配置されたcycle－delayの最小値を返す
/// \details instのsuccessorかつ配置済みの命令のうち、
///          "successorが配置されたcycle"－"Instからsuccessorのdelay"が最小のものを返す。
/// \param [in] state スケジューリング情報および結果を保持する構造体
/// \param [in] inst 調査対象の命令
/// \return predecessorが配置されたcycle－Instからsuccessorのdelayの最小値
unsigned SwplTrialState::calculateLatestCycle(const SwplInst& inst) {
  unsigned latest_cycle;

  // 最遅サイクルの初期値としてbase_slotのサイクルを取得
  latest_cycle = SwplSlot::baseSlot(iteration_interval).calcCycle();

  // instのsuccessorの個数分ループ
  for( auto* successor_inst : modulo_ddg.getSuccessors(inst) ) {
    SwplSlot successor_slot;
    unsigned cycle;
    int delay;

    // successor_instが、inst_slot_mapに存在しているか
    // 存在する場合はSlot番号をsuccessor_slotに取得
    if ((successor_slot = slots->at(successor_inst->inst_ix)) == successor_slot.UNCONFIGURED_SLOT) {
      // successor_instが、inst_slot_mapに存在していなければ、次のsuccessorを探す
      if( OptionDumpEveryInst ) {
        dbgs() << "successor_inst : " << successor_inst->getName() << " (not placed)\n";
      }
      continue;
    }

    // inst→successor_instのdelayを取得
    delay = modulo_ddg.getDelay(inst, *successor_inst);

    // 「successor_instが配置されたcycle」－「inst→successor_instのdelay」を取得
    cycle = successor_slot.calcCycle() - delay;
    assert ((int) cycle >= 0);

    if( OptionDumpEveryInst ) {
      dbgs() << "successor_inst : " << successor_inst->getName()
             << " ( cycle:" << successor_slot.calcCycle()
             << ", latecycle:" << cycle
             << ", delay:" << delay
             << " )\n";
    }

    // 「successor_instが配置されたcycle」－「inst→successor_instのdelay」の最小値を保持
    latest_cycle = std::min(latest_cycle, cycle);
  }

  return latest_cycle;
}

/// \brief end_cycle→begin_cycleのサイクル間で命令が配置できるSlotを探す
/// \param [in] begin_cycle 探索を開始するcycle
/// \param [in] end_cycle 探索を終了するcycle（begin_cycleよりも小さい値）
/// \param [in] inst 配置しようとている命令
/// \return SlotInstPipelineブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
SwplTrialState::SlotInstPipeline SwplTrialState::chooseSlot(unsigned begin_cycle, unsigned end_cycle,
                                                            const SwplInst& inst ) {
  //SwplSlot begin_slot, end_slot, slot; // cycle基準とする前
  SwplSlot begin_slot, end_slot;
  //SwplSlot new_schedule_slot; // cycle基準とする前
  unsigned new_schedule_cycle;

  begin_slot = SwplSlot::construct(begin_cycle, SWPipeliner::FetchBandwidth - 1);
  end_slot = SwplSlot::construct(end_cycle, SWPipeliner::FetchBandwidth - 1);
  if (begin_slot == SWPL_ILLEGAL_SLOT || end_slot == SWPL_ILLEGAL_SLOT ){
    return SlotInstPipeline(SWPL_ILLEGAL_SLOT, &(const_cast<SwplInst&>(inst)), nullptr);
  }

  // cycle単位に資源の空きを確認する。
  for (unsigned cycle = begin_cycle; cycle != end_cycle; cycle-- ) {
    const MachineInstr& mi = *(inst.getMI());

    // cycleに空きSlotがあるかを確認
    bool isvirtual = SWPipeliner::STM->isPseudo(mi);
    SwplSlot slot = slots->getEmptySlotInCycle( cycle, iteration_interval, isvirtual );
    if( slot != SWPL_ILLEGAL_SLOT) {
      // 空きslotあり

      if( isvirtual ) {
        // 仮想命令の場合は資源競合の確認はしない
        return SlotInstPipeline(slot, &(const_cast<SwplInst&>(inst)), nullptr );
      }
      else {
        // 資源情報を取得し、MRTに資源競合を問い合わせる。
        // 資源競合がなければ配置可能
        for( auto *pl : *(SWPipeliner::STM->getPipelines(mi)) ) {
          if( mrt->isOpenForInst(cycle, inst, *pl) ) {
            return SlotInstPipeline(slot, &(const_cast<SwplInst&>(inst)), const_cast<StmPipeline *>(pl) );
          }
        }
      }
    }
    // 空きslotがなければ、次のcycleへ。
  }

  // 空いている場所が無い場合
  new_schedule_cycle = getNewScheduleCycle(inst);

  unsigned cycle = std::min(new_schedule_cycle, begin_cycle);

  // cycle内でinstが配置可能なslotを探す
  return latestValidSlot(inst, cycle);
}

/// \brief 前回スケジュールした slot より一つ上のcycleを返す
/// \details 前回スケジュールした slot の一つ上のcycleを返す。
///          もし初めてなら base_slotのcycleを返す。
/// \param [in] inst 配置しようとている命令
/// \return 検出したcycle
unsigned SwplTrialState::getNewScheduleCycle(const SwplInst& inst) {
  SwplSlot slot;

  if (last_slots->at(inst.inst_ix) != SwplSlot::UNCONFIGURED_SLOT) {
    slot = last_slots->at(inst.inst_ix);
    slot = slot -
           SWPipeliner::FetchBandwidth; // FetchBandwidthを引けば、1cycle前のいずれかのslotとなる
  }
  else {
    slot = SwplSlot::baseSlot(iteration_interval);
  }

  return slot.calcCycle();
}

/// \brief 指定cycleから遡って、instが配置可能なcycleを返す
/// \details 指定cycleから（SlotMin側へ）遡って、instが配置可能なcycleを返す。
/// \param [in] inst 対象の命令
/// \param [in] cycle 調査開始cycle
/// \return SlotInstPipelineオブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
SwplTrialState::SlotInstPipeline SwplTrialState::latestValidSlot(const SwplInst& inst, unsigned cycle) {
  unsigned end_cycle = SwplSlot::slotMin().calcCycle();
  unsigned begin_cycle = cycle;

  const MachineInstr& mi = *(inst.getMI());
  bool isvirtual = SWPipeliner::STM->isPseudo(mi);

  // cycle単位に資源の空きを確認する。
  for (; begin_cycle != end_cycle; begin_cycle-- ) {
    SwplSlot slot = slots->getEmptySlotInCycle( begin_cycle, iteration_interval, isvirtual );
    if( slot != SWPL_ILLEGAL_SLOT) {
      if( isvirtual ) {
        return SlotInstPipeline(slot, &(const_cast<SwplInst&>(inst)), nullptr );
      }
      else {
        for( auto *pl : *(SWPipeliner::STM->getPipelines(mi)) ) {
          if( mrt->isOpenForInst(begin_cycle, inst, *pl) ) {
            // slotと資源は結び付いていない。
            //
            // 現時点では、資源が予約できるcycleかつ、inst_slot_map上で競合しないslotを返している。
            // このため、この命令の配置時に、資源競合となる命令は存在しないこととなる。
            return SlotInstPipeline(slot, &(const_cast<SwplInst&>(inst)), const_cast<StmPipeline *>(pl) );
          }
        }
      }
    }
  }

  return SlotInstPipeline(SWPL_ILLEGAL_SLOT, &(const_cast<SwplInst&>(inst)), nullptr);
}

/// \brief Instをslotに配置する
/// \param [in] SIP SlotInstPipelineブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
/// \return なし
void SwplTrialState::scheduleInst(SlotInstPipeline& SIP) {
  assert (inst_queue->begin()->second == SIP.inst);

  inst_queue->erase( inst_queue->begin() ); //pop front

  unsetResourceConstrainedInsts(SIP);
  unsetDependenceConstrainedInsts(SIP);
  setInst(SIP);

  return;
}

/// \brief リソースが競合する命令を未配置にする
/// \details リソースが競合する命令を未配置にする。ここでのリソースとは演算器資源を指す。
/// \param [in] SIP SlotInstPipelineブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
/// \return なし
void SwplTrialState::unsetResourceConstrainedInsts(SlotInstPipeline& SIP) {
  SwplInstSet* blocking_insts;

  if( SIP.pipeline == nullptr ) {
    assert(SWPipeliner::STM->isPseudo(*(SIP.inst->getMI())) ); // 資源情報無し＝仮想命令である
    assert( SIP.slot.calcFetchSlot() <
           SWPipeliner::RealFetchBandwidth ); // 資源情報無し＝slotは仮想命令用である
    // 仮想命令の場合は、競合する資源は無いため、
    // 資源競合によりunsetする命令は無い。
    return;
  }

  blocking_insts = mrt->findBlockingInsts( SIP.slot.calcCycle(), *(SIP.inst), *(SIP.pipeline) );
  for( auto *inst : *blocking_insts ) {
    if( OptionDumpEveryInst ) {
      dbgs() << "unset (ResourceConstrained) : " << inst->getName() << "\n";
    }
    unsetInst( *inst );
  }
  delete blocking_insts;
  return;
}

/// \brief 依存がある命令を未配置にする
/// \param [in] SIP SlotInstPipelineブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
/// \return なし
void SwplTrialState::unsetDependenceConstrainedInsts(SlotInstPipeline& SIP) {
  unsigned cycle;
  cycle = SIP.slot.calcCycle();

  for( auto predecessor_inst : modulo_ddg.getPredecessors( *(SIP.inst) ) ) {
    SwplSlot predecessor_slot;
    unsigned predecessor_cycle;
    int delay;

    if((predecessor_slot = slots->at(predecessor_inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
      continue;
    }
    predecessor_cycle = predecessor_slot.calcCycle();
    delay = modulo_ddg.getDelay(*predecessor_inst, *(SIP.inst));
    assert ((int) (predecessor_cycle + delay) >= 0);
    if (predecessor_cycle + delay > cycle) {
      if( OptionDumpEveryInst ) {
        dbgs() << "unset (DependConstrained  ) : " << predecessor_inst->getName() << "\n";
      }
      unsetInst(*predecessor_inst);
    }
  }
  return;
}

/// \brief instをslotに配置する
/// \details instをslotに配置する。stateの以下フィールドが更新される。
///          - state->inst_slot_map
///          - state->inst_last_slot_map
///          - state->mrt
/// \param [in] SIP SlotInstPipelineブジェクト。instが配置できるslot, inst, 配置可能となったpipeline
/// \return なし
void SwplTrialState::setInst(SlotInstPipeline& SIP) {

  slots->at(SIP.inst->inst_ix) = SIP.slot;
  last_slots->at(SIP.inst->inst_ix) = SIP.slot;

  if( SIP.pipeline == nullptr ) {
    assert(SWPipeliner::STM->isPseudo(*(SIP.inst->getMI())) ); // 資源情報無し＝仮想命令である
    assert( SIP.slot.calcFetchSlot() <
           SWPipeliner::RealFetchBandwidth ); // 資源情報無し＝slotは仮想命令用である
    return;
  }

  mrt->reserveResourcesForInst(SIP.slot.calcCycle(), *(SIP.inst), *(SIP.pipeline) );
  return;
}

/// \brief 配置済みのInstを未配置にする
/// \details 配置済みのInstを未配置にする。以下が実施され、SwplTrialStateのフィールドが更新される。
///          - state->inst_queue：Instを追加（Instは、配置したので、未配置キューから取り除かれている）
///          - state->inst_slot_map：Instを取り除く
///          - state->mrt：Instを取り除く
///
/// \param [in] inst 未配置にするInst
/// \return なし
void SwplTrialState::unsetInst(const SwplInst& inst) {
  int priority;

  // state->prioritiesからInstの優先度を取得
  priority = instPriority( &inst );

  // Instと優先度のPairを、未配置キューに追加
  inst_queue->insert( std::make_pair( priority, &inst ) );

  // 配置済みSlotマップから、Instが配置されたSlotを除外
  slots->at(inst.inst_ix) = SwplSlot::UNCONFIGURED_SLOT;

  // MRTからInstを除外
  mrt->cancelResourcesForInst( inst );
  return;
}

/// \brief Instの優先度を返却する
/// \details state->prioritiesを参照し、Instの優先度を返却する。
/// \param [in] inst 優先度を求めるInst
/// \return Instの優先度
int SwplTrialState::instPriority(const SwplInst* inst) {
  assert( priorities->find(inst) != priorities->end() );
  return priorities->find(inst)->second;
}

/// \brief 該当のループのDebugLocを返す
/// \return DebugLoc of loop.
llvm::DebugLoc SwplTrialState::getLoopStartLoc() {
  return modulo_ddg.getLoopStartLoc();
}

/// \brief SwplTrialStateを生成し、初期設定する
/// \param [in] c_modulo_ddg modulo ddg
/// \return 生成したSwplTrialState
SwplTrialState* SwplTrialState::construct(const SwplModuloDdg& c_modulo_ddg) {
  SwplTrialState* state;
  state = new SwplTrialState(c_modulo_ddg);

  state->iteration_interval = state->modulo_ddg.getIterationInterval();
  state->priorities = state->modulo_ddg.computePriorities();

  state->inst_queue = new SwplInstPrioque();
  for( auto &pair : *(state->priorities) ) {
    state->inst_queue->insert( std::make_pair( pair.second, pair.first ) );
  }

  state->slots = new SwplSlots();
  state->slots->resize(c_modulo_ddg.getLoopBody_ninsts());
  state->last_slots = new SwplSlots();
  state->last_slots->resize(c_modulo_ddg.getLoopBody_ninsts());
  state->mrt = SwplMrt::construct(state->iteration_interval);

  return state;
}

/// \brief SwplTrialState領域を解放する
/// \details SwplTrialState領域を解放する。ただし、inst_slot_mapは使うので解放しない。
/// \param [in] state SwplTrialStateのポインタ
/// \return なし
void SwplTrialState::destroy(SwplTrialState* state) {
  // state->inst_slot_map は使うので destroy しない。
  delete state->priorities;
  delete state->inst_queue;
  delete state->last_slots;
  SwplMrt::destroy(state->mrt);
  delete state;
  return;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief SwplPlanSpecを初期化する
/// \details SwplPlanSpecをSwplDdgの情報他で初期化する。
///          - 初期化に失敗した場合は falseを返却する。
///
/// \param [in] res_mii リソースMII
/// \param [out] existsPragma Is II specified in pragma?
/// \retval true specの初期化成功
/// \retval false specの初期化失敗
///
/// \note 現在、SwplPlanSpec.assumed_iterationは常にASSUMED_ITERATIONS_NONE (-1)
/// \note 現在、SwplPlanSpec.pre_expand_numは常に1
bool SwplPlanSpec::init(unsigned arg_res_mii, bool &existsPragma) {
  existsPragma = false;
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "        : (Iterative Modulo Scheduling"
           << ". ResMII " << arg_res_mii << ". ";
  }

  n_insts = ddg.getLoopBody_ninsts();
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "NumOfBodyInsts " << n_insts << ". ";
  }
  res_mii = arg_res_mii;
  is_itr_count_constant = isIterationCountConstant(loop, &itr_count);
  original_cycle = 0U;

  pre_expand_num = 1;
  assumed_iterations = ASSUMED_ITERATIONS_NONE;

  n_fillable_float_invariants = 0; /* IMS＝fillを許容しないポリシー であるため0 */

  ims_base_info.budget  = getBudget(n_insts);
  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "Budget " << ims_base_info.budget << ". ";
  }
  min_ii = res_mii;

  if(SWPipeliner::nOptionMinIIBase()>0) {
    min_ii = SWPipeliner::nOptionMinIIBase(); // option value (default:65)
  }

  max_ii = SWPipeliner::nOptionMaxIIBase();

  // pragma
  unsigned int ii;
  ii = getInitiationInterval(loop, existsPragma);
  if (existsPragma) {
    min_ii = ii;
    max_ii = ii + 1;

    //analysis
    const auto *ml = loop.getML();
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "InitiationInterval", ml->getStartLoc(), ml->getHeader())
      << "This loop tries to schedule with the InitiationInterval=" << ore::NV("InitiationInterval ", ii) << " specified in the pragma.";
    });
      
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "II " << ii << ". ";
    }
  }

  unable_max_ii = min_ii - 1;

  // Remedies when calculated min_ii is greater than max_ii
  if (min_ii >= max_ii) {
    //analysis
    const auto *ml = loop.getML();
    SWPipeliner::ORE->emit([&]() {
      return MachineOptimizationRemarkAnalysis(DEBUG_TYPE, "MaxII", ml->getStartLoc(), ml->getHeader())
        << "Since the calculated min_ii("
        << ore::NV("min_ii", min_ii)
        << ") is greater than or equal to max_ii("
        << ore::NV("max_ii", max_ii)
        << "), processing continues with max_ii=min_ii+1.";
    });
    max_ii=min_ii+1;
  }

  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "Minimum II = " << min_ii << ".)\n";
  }
  return true;
}

/// \brief ループにおける言種別の数を取得する
/// \param [in,out] num_load load命令数
/// \param [in,out] num_store store命令数
/// \param [in,out] num_float float命令数
///
/// \note  言数はspillを含むscheduleの評価に用いられる
/// \note  store系およびindirectload系はFLA,FLBを利用する為,
///        資源の圧迫率が高い.
void SwplPlanSpec::countLoadStore(unsigned *num_load, unsigned *num_store, unsigned *num_float ) const {
  *num_load  = 0;
  *num_store = 0;
  *num_float = 0;

  for( auto *inst : loop.getBodyInsts() ) {
    if( inst->isStore() ) {
      (*num_store)++;
    } else if( inst->isLoad() ) {
      (*num_load)++;
    }

    if (inst->isFloatingPoint()) {
      (*num_float)++;
    }
  }
  return;
}

/// \brief ループの回転数を取得する
/// \param [in] c_loop 対象となるループ情報
/// \param [in,out] iteration_count 回転数
/// \details ソース情報から、ループの回転数を取得する。
///          回転数が取得できた場合は、itr_countメンバ変数に格納し、trueを返す。
/// \retval true ループ回転数が取得できた
/// \retval false ループ回転数が取得できなかった
bool SwplPlanSpec::isIterationCountConstant(const SwplLoop& c_loop, unsigned* iteration_count) {
  SwplScr swpl_scr( *(const_cast<llvm::MachineLoop*>(c_loop.getML())) );
  SwplTransformedMIRInfo temp_tli;

  *iteration_count = 0;

  /* 制御変数の初期値など、元のループの制御変数に関する情報の取得*/
  if( !swpl_scr.findBasicInductionVariable(temp_tli) ) {
    llvm_unreachable("failed at findBasicInductionVariable.");
    return false;
  }

  /* 回転数が既知の場合は取得した回転数を設定する*/
  if (temp_tli.isIterationCountConstant) {
    *iteration_count = temp_tli.originalKernelIteration;
    return true;
  }
  return false;
}

/// \brief iterative_scheduleの繰返し上限を求める
/// \param [in] n_insts スケジューリング対象の命令数
/// \return iterative_scheduleの繰返し上限
unsigned SwplPlanSpec::getBudget(unsigned n_insts) {
  double ratio;

  /* ループの言数により動的に決定する。*/
  if (n_insts < OptionBudgetRatioThreshold ) { // option value (default:100)
    ratio = OptionBudgetRatioLess; // option value (default:5.0)
  } else {
    ratio = OptionBudgetRatioMore; // option value (default:2.5)
  }
  return (unsigned)(ratio * (double)n_insts);
}

/// \brief Search metadata and get the specified value of pipeline_initiation_interval
/// \details Recursively traverses nested meta-information to obtain the specified value.
/// \param [in] MD Target metadata
/// \param [out] exists True is specified in metadata
/// \return II value
unsigned int getIIMetadata(MDNode *MD, bool &exists) {

  if (MD->isDistinct()) {
    // example) !25 = distinct !{!25, !18, !23, !26, !27, !28}
    for (unsigned i = 1, e = MD->getNumOperands(); i < e; ++i) {
      MDNode *childMD = dyn_cast<MDNode>(MD->getOperand(i));

      if (MD == nullptr)
        continue;

      unsigned int ret = getIIMetadata(childMD, exists);
      if (exists)
        return ret;
    }
  }
  else {
    // example) !28 = !{!"llvm.loop.pipeline.initiationinterval", i32 30}
    MDString *S = dyn_cast<MDString>(MD->getOperand(0));

    if (S == nullptr)
      return 0;

    if (S->getString() == "llvm.loop.pipeline.initiationinterval") {
      assert(MD->getNumOperands() == 2 &&
             "Pipeline initiation interval hint metadata should have two operands.");
      exists = true;
      return mdconst::dyn_extract<ConstantInt>(MD->getOperand(1))->getZExtValue();
    }

    // example) !28 = !{!"llvm.loop.vectorize.followup_all", !29}
    if ((S->getString()).find("followup") != std::string::npos) {
      // empty followup attribute
      // example) !28 = !{!"llvm.loop.vectorize.followup_vectorized"}
      if (MD->getNumOperands() == 1)
       return 0;

      MDNode *childMD = dyn_cast<MDNode>(MD->getOperand(1));
      MDString *secondS = dyn_cast<MDString>(childMD->getOperand(0));

      // example) !28 = !{!"llvm.loop.vectorize.followup_vectorized", !{!"llvm.loop.pipeline.initiationinterval", i32 30}}
      if (secondS != nullptr) {
        if (secondS->getString()=="llvm.loop.pipeline.initiationinterval") {
          assert(childMD->getNumOperands() == 2 &&
                 "Pipeline initiation interval hint metadata should have two operands.");
          exists = true;
          return mdconst::dyn_extract<ConstantInt>(childMD->getOperand(1))->getZExtValue();
        }
      }
      return getIIMetadata(childMD, exists);
    }
  }
  return 0;
}

/// \brief Get the specified value of pipeline_initiation_interval from metadata
/// \param [in] loop Target loop information
/// \param [in,out] exists True is specified in metadata
/// \return II value obtained from metadata
unsigned int SwplPlanSpec::getInitiationInterval(const SwplLoop &loop, bool &exists) {
  exists = false;

  const MachineLoop *ML = loop.getML();
  if (ML == nullptr)
    return 0;

  MachineBasicBlock *LBLK = ML->getHeader();
  if (LBLK == nullptr)
    return 0;

  const BasicBlock *BBLK = LBLK->getBasicBlock();
  if (BBLK == nullptr)
    return 0;

  const Instruction *TI = BBLK->getTerminator();
  if (TI == nullptr)
    return 0;

  MDNode *LoopID = TI->getMetadata(LLVMContext::MD_loop);
  if (LoopID == nullptr)
    return 0;

  assert(LoopID->getNumOperands() > 0 && "requires atleast one operand");
  assert(LoopID->getOperand(0) == LoopID && "invalid loop");

  // Metadata Search
  unsigned int ret = getIIMetadata(LoopID, exists);
  if (exists)
    return ret;

  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief MsResourceResultを初期化する
/// \return なし
///
/// \note 各ms_resource_result毎にget_max_n_regsを制御したいため
///       初期化時にregの上限を変更しやすいような構造体の構成としている.
void SwplMsResourceResult::init() {
  num_necessary_ireg = 0;
  num_necessary_freg = 0;
  num_necessary_preg = 0;
  auto *rk=SWPipeliner::TII->getRegKind(*SWPipeliner::MRI);

  num_max_ireg = (OptionMaxIreg > 0) ? OptionMaxIreg : rk->getNumIntReg();
  num_max_freg = (OptionMaxFreg > 0) ? OptionMaxFreg : rk->getNumFloatReg();
  num_max_preg = (OptionMaxPreg > 0) ? OptionMaxPreg : rk->getNumPredicateReg();
  delete rk;

  setSufficient();
  return;
}

/// \brief レジスタ資源を確認し、is_resource_sufficientを更新する
/// \return なし
void SwplMsResourceResult::setSufficient() {
  setSufficientWithArg( (isIregSufficient() &&
                         isFregSufficient() &&
                         isPregSufficient()) );
  return;
}

/// \brief ireg資源が不足しないか
/// \retval true 不足しない
/// \retval false 不足する
bool SwplMsResourceResult::isIregSufficient() {
  return (num_max_ireg >= num_necessary_ireg);
}

/// \brief freg資源が不足しないか
/// \retval true 不足しない
/// \retval false 不足する
bool SwplMsResourceResult::isFregSufficient() {
  return (num_max_freg >= num_necessary_freg);
}

/// \brief preg資源が不足しないか
/// \retval true 不足しない
/// \retval false 不足する
bool SwplMsResourceResult::isPregSufficient() {
  return (num_max_preg >= num_necessary_preg);
}

/// \brief 必要ireg数の更新
/// \param [in] 必要ireg数
/// \return なし
///
/// \note 更新したireg数でレジスタ資源を確認し、is_resource_sufficientを更新する
void SwplMsResourceResult::setNecessaryIreg(unsigned ireg) {
  num_necessary_ireg = ireg;
  setSufficient();
  return;
}

/// \brief 必要freg数の更新
/// \param [in] 必要ireg数
/// \return なし
///
/// \note 更新したfreg数でレジスタ資源を確認し、is_resource_sufficientを更新する
void SwplMsResourceResult::setNecessaryFreg(unsigned freg) {
  num_necessary_freg = freg;
  setSufficient();
  return;
}

/// \brief 必要preg数の更新
/// \param [in] 必要ireg数
/// \return なし
///
/// \note 更新したpreg数でレジスタ資源を確認し、is_resource_sufficientを更新する
void SwplMsResourceResult::setNecessaryPreg(unsigned preg) {
  num_necessary_preg = preg;
  setSufficient();
  return;
}

/// \brief 不足するireg数の取得
/// \return 不足するireg数
unsigned SwplMsResourceResult::getNumInsufficientIreg() {
  if (isIregSufficient()) {
    return 0;
  }
  return getNecessaryIreg() - getMaxIreg();
}

/// \brief 不足するfreg数の取得
/// \return 不足するfreg数
unsigned SwplMsResourceResult::getNumInsufficientFreg() {
  if (isFregSufficient()) {
    return 0;
  }
  return getNecessaryFreg() - getMaxFreg();
}

/// \brief 不足するpreg数の取得
/// \return 不足するpreg数
unsigned SwplMsResourceResult::getNumInsufficientPreg() {
  if (isPregSufficient()) {
    return 0;
  }
  return getNecessaryPreg() - getMaxPreg();
}

/// \brief レジスタに余裕があるかどうかチェックする
/// \retval true 余裕がある
/// \retval false 余裕がない
///
/// \note SwplMsResult.isEffective()==trueとなるMsResult.MsResourceResultに対して用いる
bool SwplMsResourceResult::isModerate() {
  /* heuristicな条件 */
  if ((num_max_ireg < num_necessary_ireg + loose_margin_ireg) ||
      (num_max_freg < num_necessary_freg + loose_margin_freg) ||
      (num_max_preg < num_necessary_preg + loose_margin_preg) ) {
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief 指定したiiにおけるschedulingを行ない,inst_slot_mapを構成する
/// \details 指定したiiにおけるschedulingを行ない,inst_slot_mapを構成する。
///
/// ```
///   得た結果において資源が十分かどうかは保証していない
///   本処理ではms_resultの下記のmemberが設定される
///      inst_slot_map
///      tried_n_insts
///
/// 以下の翻訳性能改善が考えられる。
/// [1]
///   同一の状態を繰り返し無駄にループを繰り返すことがある。
///   そのため、budgetが余っていても繰り返しが無駄だと判明した時、
///   途中であきらめることが可能であるが、現状でも問題ないため対応しない。
///
///  ex) 空きslotが二つしかなく,かつlatencyを
///      slot間のcycle以上に開ける必要がある場合
///      この場合Slotを求めてどんどん上にSlotを伸ばしてしまう.
///      一定上伸びると何らかの理由で依存のある命令をunsetする
///      もしかしたらbugってる為に掬われているかもしれない
///      要調査.
///
/// [2]
///   ループの途中で必要なregisterを数え、
///   全く足りない場合はschedulingをあきらめた方がよい。
///   schedulingを続け、結果を得たとしても、
///   isHardRegsSufficientでfalseとなり無駄になる可能性が高いため。
/// ```
///
/// \param [in] spec スケジューリング指示情報
/// \retval true スケジューリングに成功した
/// \retval false スケジューリングに失敗した
///
/// \note
///  - spec.ddgおよびms_result->iiから、SwplTrialStateを生成
///  - IMSを実行する関数を呼び出す。スケジューリング結果はSwplTrialState->inst_slot_mapに格納される。
///  - スケジューリング結果(SwplTrialState->inst_slot_map)を、MsResultに保持させる
///  - SwplTrialStateを破棄 ※ただし、inst_slot_map域は残す
bool SwplMsResult::constructInstSlotMapAtSpecifiedII(SwplPlanSpec spec) {
  SwplSlots* p_slots;
  SwplModuloDdg* modulo_ddg;
  SwplTrialState* state;
  bool is_schedule_completed;

  is_schedule_completed = false;
  p_slots = nullptr;
  modulo_ddg = SwplModuloDdg::construct(spec.ddg, ii);
  if( OptionDumpModuleDdg ) {
    modulo_ddg->dump();
  }
  state = SwplTrialState::construct(*modulo_ddg);

  for (unsigned i = 0; i < spec.ims_base_info.budget; ++i) {
    bool trial_is_success = false;
    trial_is_success = state->tryNext();

    /* 適切なtryNextがおこなわれなかった場合 */
    if (!trial_is_success) {
      break;
    }

    // 未配置の命令がある場合
    if (!state->isCompleted()) {
      continue;
    } else {
      /* 命令がすべて配置できた場合 */
      is_schedule_completed = true;
      break;
    }
  }

  if(is_schedule_completed) {
    if (OptionDumpMrt) {
      state->dump( dbgs() );
    }
  }

  p_slots = state->getInstSlotMap();
  tried_n_insts = p_slots->size();
  SwplTrialState::destroy(state);
  SwplModuloDdg::destroy(modulo_ddg);

  if (is_schedule_completed) {
    slots = p_slots;
    return true;
  }  else {
    slots = nullptr;
    /* 不完全なinst_slot_mapのため,解放する*/
    delete p_slots;
    return false;
  }
}

/// \brief inst_slot_mapのレジスタと回転数の情報をms_resultに設定する
/// \param [in] spec スケジューリング指示情報
/// \param [in] limit_reg レジスタ数上限をチェックするか否か
/// \return なし
void SwplMsResult::checkInstSlotMap(const SwplPlanSpec & spec, bool limit_reg) {
  checkHardResource(spec, limit_reg);
  checkIterationCount(spec);
  checkMve(spec);
  return;
}

/// \brief inst_slot_mapに対して資源が足りているかをチェックする
/// \param [in] spec スケジューリング指示情報
/// \param [in] limit_reg レジスタ数上限をチェックするか否か
///
/// \note 現在のところ,整数, predicateレジスタが不足している場合は採用していない
void SwplMsResult::checkHardResource(const SwplPlanSpec & spec, bool limit_reg) {
  if (slots == nullptr || !limit_reg) {
    is_reg_sufficient = true;
    n_insufficient_iregs = 0;
    n_insufficient_fregs = 0;
    n_insufficient_pregs = 0;
    return ;
  }

  /* registerの内容詳細をチェック */
  resource = isHardRegsSufficient(spec);

  n_insufficient_iregs
    = resource.getNumInsufficientIreg();
  n_insufficient_fregs
    = resource.getNumInsufficientFreg();
  n_insufficient_pregs
    = resource.getNumInsufficientPreg();
  is_reg_sufficient
    = resource.isSufficient();

  if (!is_reg_sufficient) {
    /* 整数より浮動小数点不足優先で出力. */
    if ( !resource.isFregSufficient() ) {
      // 浮動小数点数レジスタの場合
      SWPipeliner::Reason = llvm::formatv(
          "Trial at iteration-interval={0}. "
          "This loop cannot be software pipelined because of shortage of floating-point registers.", ii);
    } else if ( !resource.isIregSufficient() ) {
      // 整数レジスタの場合
      SWPipeliner::Reason = llvm::formatv(
          "Trial at iteration-interval={0}. "
          "This loop cannot be software pipelined because of shortage of integer registers.", ii);
    } else if ( !resource.isPregSufficient() ) {
      // predicateレジスタの場合
      SWPipeliner::Reason = llvm::formatv(
          "Trial at iteration-interval={0}. "
          "This loop cannot be software pipelined because of shortage of predicate registers.", ii);
    }

    if ( resource.isIregSufficient() ) {
      unsigned kernel_blocks = slots->calcKernelBlocks(spec.loop, ii);
      evaluateSpillingSchedule(spec, kernel_blocks);
    } else {
      evaluation = -1.0;
    }
  }
  return;
}

/// \brief scheduling結果に対してレジスタが足りるかどうかを判定する
/// \param [in] spec スケジューリング指示情報
/// \return なし
///
/// \note 固定割付けでないレジスタが、整数、浮動小数点数、プレディケートであることが前提の処理である。
/// \note llvm::AArch64::CCRRegClassIDは固定割付けのため処理しない
SwplMsResourceResult SwplMsResult::isHardRegsSufficient(const SwplPlanSpec & spec) {
  unsigned int n_renaming_versions;
  unsigned n_necessary_regs;
  SwplMsResourceResult ms_resource_result;

  ms_resource_result.init();
  n_renaming_versions = slots->calcNRenamingVersions(spec.loop, ii);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(spec.loop, slots, ii,
                                                  llvm::StmRegKind::getIntRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryIreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(spec.loop, slots, ii,
                                                  llvm::StmRegKind::getFloatRegID(),
                                                  n_renaming_versions);
  if(n_necessary_regs > spec.n_fillable_float_invariants) {
    // 必ず成立するはずだが念のため
    n_necessary_regs -= spec.n_fillable_float_invariants;
  }
  ms_resource_result.setNecessaryFreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(spec.loop, slots, ii,
                                                  llvm::StmRegKind::getPredicateRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryPreg(n_necessary_regs);


  if (!(slots->isIccFreeAtBoundary(spec.loop, ii))) {
    ms_resource_result.setSufficientWithArg(false);
  }

  return ms_resource_result;
}

/// \brief 各II毎のinst_slot_mapに対して回転数が足りているかをチェックする
/// \param [in] spec スケジューリング指示情報
/// \return なし。結果は SwplPlanSpec::is_itr_sufficientに格納される。
void SwplMsResult::checkIterationCount(const SwplPlanSpec & spec) {
  required_itr_count = 0;

  /* スケジューリング自体がない場合は,制限にかからない */
  if (slots == nullptr ) {
    is_itr_sufficient = true;
    return;
  }

  /* inst_slot_mapから展開に必要な回転数を取得する*/
  unsigned prolog_blocks = slots->calcPrologBlocks(spec.loop, ii);
  unsigned kernel_blocks = slots->calcKernelBlocks(spec.loop, ii);

  unsigned n_copies  = prolog_blocks + kernel_blocks;
  required_itr_count = n_copies; // insert_move_into_prolog is true.

  /* 回転数が変数の場合 */
  if (!spec.is_itr_count_constant || spec.itr_count <= 0) {
    is_itr_sufficient = SwplCalclIterations::checkIterationCountVariable(spec, *this);
  } else {
    is_itr_sufficient = SwplCalclIterations::checkIterationCountConstant(spec, *this);
  }

  if (is_itr_sufficient == false) {
    SWPipeliner::Reason = llvm::formatv("Trial at iteration-interval={0}."
                                        " This loop is not software pipelined because the iteration count is smaller than {1}"
                                        " and the software pipelining does not improve the performance.",
                                        ii, required_itr_count);
  }
  return;
}

/// \brief kernelの展開数MVEが妥当な数であるかを判定する
/// \param [in] spec スケジューリング指示情報
/// \return なし
void SwplMsResult::checkMve(const SwplPlanSpec & spec) {
  required_mve = 0;

  /* スケジューリング自体がない場合は,制限にかからない */
  if (slots == nullptr ) {
    is_mve_appropriate = true;
    return;
  }

  required_mve = slots->calcKernelBlocks(spec.loop, ii);
  if(required_mve <= OptionMveLimit) {  // option value (default:30)
    is_mve_appropriate = true;
  } else {
    is_mve_appropriate = false;
  }
  return;
}

/// \brief iiでiterative schedulingを行なった結果のdebug用messageを出力する
/// \param [in] spec スケジューリング指示情報
/// \return なし
///
/// \note SwplMsResult.proc_stateによって出力メッセージが一部変わる。
void SwplMsResult::outputDebugMessageForSchedulingResult(SwplPlanSpec spec) {
  unsigned max_freg, max_ireg, max_preg, req_freg, req_ireg, req_preg;
  if (!SWPipeliner::isDebugOutput()) { return ;}
  /* registerの内容詳細をチェック */
  max_freg = 0; max_ireg = 0; max_preg = 0;
  req_freg = 0; req_ireg = 0; req_preg = 0;

  if (slots == nullptr) {
    /* scheduling失敗*/
    dbgs() << "        :  (x) Run out of budget      ";
  } else {
    max_freg = resource.getMaxFreg();
    max_ireg = resource.getMaxIreg();
    max_preg = resource.getMaxPreg();
    req_freg = resource.getNecessaryFreg();
    req_ireg = resource.getNecessaryIreg();
    req_preg = resource.getNecessaryPreg();

    if (!is_itr_sufficient) {
      /* 回転数不足 */
      dbgs() << "        :  (x) Iteration  shortage    ";
    } else if (!is_reg_sufficient) {
      int character_num=0;
      dbgs() << "        :  (x) Register (";
      /* register 不足*/
      if (n_insufficient_iregs > 0) {
        dbgs() << "I";
        character_num+=1;
      }
      if (n_insufficient_fregs > 0) {
        if(character_num>0) {
          dbgs() << ",F";
          character_num+=2;
        } else {
          dbgs() << "F";
          character_num+=1;
        }
      }
      if (n_insufficient_pregs > 0) {
        if(character_num>0) {
          dbgs() << ",P";
          character_num+=2;
        } else {
          dbgs() << "P";
          character_num+=1;
        }
      }

      dbgs() << ") short";
      for(/* character_numの初期値は上で加算 */
          ; character_num<=5;
          ++character_num) {
        dbgs() << " ";
      }
    } else if(!is_mve_appropriate) {
      dbgs() << "        :  (X) Too large mve          ";
    } else if( !(this->isModerate()) ) {
      dbgs() << "        :  (@) Scheduling tight       ";
    } else {
      assert( this->isEffective() );
      /* scheduling成功 */
      dbgs() << "        :  (O) Scheduling succeeds    ";
    }
  }
  const char *statestr="";
  switch (proc_state) {
  case ProcState::ESTIMATE:            statestr="estimation.         "; break;
  case ProcState::RECOLLECT_EFFECTIVE: statestr="recollect effective."; break;
  case ProcState::RECOLLECT_MODERATE:  statestr="recollect moderate. "; break;
  case ProcState::BINARY_SEARCH:       statestr="binary search.      "; break;
  case ProcState::SIMPLY_SEARCH:       statestr="simply search.      "; break;
  default:
    llvm_unreachable("unknown SwplMsResult.proc_state");
  }
  //dbgs() << ((is_called_from_estimate)?"at estimation.   ":"at binary search.")
  dbgs() << "at " << statestr
         << " : (II: " << ii << " in [ " << spec.min_ii << "," << spec.max_ii << "])"
         << " MVE: " << required_mve
         << " Last inst: " << (spec.n_insts - tried_n_insts) << "."
         << " (Itr Org: " << spec.itr_count << ", Req: " << required_itr_count <<")"
         << " (VReg Fp: " << req_freg << "/"<< max_freg
         << ", Int: " << req_ireg << "/"<< max_ireg
         << ", Pre: " << req_preg << "/"<< max_preg <<")"
         << " Eval:"<< evaluation <<".\n";
  return;
}

/// \brief 余裕があるスケジューリングかどうかを判定する
/// \retval true 余裕があるスケジューリングである
/// \retval false 余裕があるスケジューリングでない
///
/// \note 余裕の基準はheuristicである.
/// \note OoO資源をベースにして決定したい
bool SwplMsResult::isModerate() {
  if(isEffective() == false) {
    return false;
  }

  if(resource.isModerate() == false) {
    return false;
  }

  if(required_itr_count > 35) {
    /* 2つのloadを交互におき直して,重なりが異常に大きくなる問題がある.
     * IIを1大きくすると小さくなるため、このようなIIの結果を採用しない */
    return false;
  }

  // \note mveを考慮するか検討

  return true;
}

/// \brief 資源と回転数が十分なスケジュール結果であるかどうか
/// retval true 資源と回転数が十分なスケジュール結果である
/// retval false 資源と回転数が十分なスケジュール結果でない
///
/// \note heuristicな条件は確認しない
bool SwplMsResult::isEffective() {
  if (slots != nullptr &&
      is_itr_sufficient == true &&
      is_reg_sufficient == true &&
      is_mve_appropriate == true ) {
    return true;
  } else {
    return false;
  }
}

/// \brief schedulingに失敗した時のmessageを設定および出力する
/// \details schedulingに失敗した時のmessageを設定および出力する。
///          失敗原因が、Rotation shortage（回転数不足）、レジスタ不足、MVEが大きすぎる、の
///          いずれでもない場合のみ、IODにメッセージIDを設定する。
/// \param [in] spec MachineLoopを取得するためのSwplPlanSpec
/// \return なし
///
/// \note 現在の仕様ではregister不足でschedulingできなかった場合も,
///       inst_slot_mapがnullptrとなっている事に注意すること.
void SwplMsResult::outputGiveupMessageForEstimate(SwplPlanSpec & spec) {
  assert(slots == nullptr ||
         !(is_reg_sufficient &&
           is_itr_sufficient &&
           is_mve_appropriate));

  if (slots != nullptr) {
    /* schedulingに成功したが,レジスタ不足となった場合,
     * iiの上限に達したとき,schedulingをあきらめるメッセージを出す.
     *
     * [messageについて]
     * is_hard_regs_sufficient内でSwpl非適用出力メッセージをIODにcacheするため、
     * giveupした時点のmessageIDで不足レジスタの種類が判別できる。
     */
  } else {
    if (SWPipeliner::isDebugOutput()) {
      if( !is_itr_sufficient) {
        dbgs() << "        : Rotation shortage.[giveup]\n (ii:" << ii << ")\n";
      } else if (!is_reg_sufficient) {
        dbgs() << "        : Register shortage.[giveup]\n (ii:" << ii<< ")\n";
      } else if (!is_mve_appropriate) {
        dbgs() << "        : Too large mve.[giveup]\n (ii:" << ii << ")\n";
      } else {
        dbgs() << "        : Not scheduled.[giveup] (ii:" << ii << ")\n";
      }
    }
    if( !is_itr_sufficient==false && !is_reg_sufficient==false && !is_mve_appropriate==false) {
      // int_slot_mapができない最終原因が、
      // レジスタ不足、回転数不足、MVE制限によるものでない場合
      SWPipeliner::Reason = llvm::formatv(
          "Trial at iteration-interval={0}. "
          "This loop is not software pipelined because no schedule is obtained.", ii);
    }
  }
  return;
}

/// \brief spillがある場合のscheduleの評価を行なう
/// \param [in] spec MachineLoopを取得するためのSwplPlanSpec
/// \param [in] kernel_blocks kernelのブロック数
/// \return なし
///
/// \note heuristicな判定である
/// \note predicate registerは考慮していない
/// \note 現状、命令キャッシュ,live rangeが考慮できていない
///
/// ```
///   spill一つ辺りの影響heuristicに見積もったlatency
///   できるだけ値が安定するように複数の側面で判定する.
///   (1) iiが大きいほど影響は小さい
///   (2) load/storeが多いほど影響が大きい
///   (3) 命令がつまっているほど,影響が大きい
///   (4) spill数が多いほど,命令列の並びを崩しやすい
///   (5) integer命令が多いほど,spillの影響を受けやすい
///
///   ターゲットループ JAXA/HJET/ck-source.f L230
///   cycle短縮率が高く,load/storeが少ないループではspillが目立ちにくい
///   ----------------------------------------------------------
///   peta                                metis
///   201命令 (load系15,store系6)	   161命令 (load系4, store 1)
///   見積もりcycle 262 結果II 76	   見積もりcycle 244 結果II 80
///   freg 342 > 256		           freg 356 > 256
///   ------------------------------------------------------------
/// ```
///
void SwplMsResult::evaluateSpillingSchedule(const SwplPlanSpec & spec, unsigned kernel_blocks) {
  unsigned num_load  = 0;
  unsigned num_store = 0;
  unsigned num_float = 0;

  double   spill_cycle;
  unsigned insuff_fregs;
  unsigned freg_max;
  unsigned const margin_cycle = 5;
  unsigned estimated_total_cycle;

  assert (ii > 0);
  assert (kernel_blocks > 0);
  assert (spec.n_insts  > 0);

  spec.countLoadStore(&num_load, &num_store, &num_float);

  freg_max = resource.getMaxFreg();
  freg_max = (freg_max >= 1)? freg_max:1;
  /* 不足している浮動小数点レジスタ */
  insuff_fregs= resource.getNecessaryFreg() - freg_max;

  spill_cycle = (6.0 + 1.0);

  spill_cycle =  spill_cycle
    * (50.0/(ii + 50.0))                             /* (1) */
    * (1.0 + 1.0*((double)(num_load + num_store))/spec.n_insts) /* (2) */
    * ((double)spec.n_insts
       / ((kernel_blocks + 1)* ii))                  /* (3) */
    * (1.0 + 5.0 * insuff_fregs / freg_max)                     /* (4) */
    * (2.0 - 1.0 * ((double)num_float)/spec.n_insts)            /* (5) */
    * 0.5 ;                                                     /* scaleを適度に補正 */

  assert (spill_cycle > 0);
  estimated_total_cycle = (spill_cycle * insuff_fregs + ii + margin_cycle);

  /* 1.0 以上で,spill込みでもcycle数として得すると判断 */
  evaluation = spec.original_cycle/ (double)estimated_total_cycle;

  return;
}

/// \brief MsResultの生成
/// \details MsResultの生成と初期化
/// \param [in] ii SwplMsResult.iiの初期値
/// \param [in] procstate MsResultの処理状態（ex. binary_search中、など）
/// \return 生成したMsResultのポインタ
///
/// \note 本関数で生成したMsResultは呼出し側で解放する必要がある。
SwplMsResult *SwplMsResult::constructInit(unsigned ii, ProcState procstate ) {
  SwplMsResult * ms_result = new SwplMsResult();

  ms_result->slots = nullptr;
  ms_result->ii = ii;
  ms_result->tried_n_insts = 0;

  ms_result->n_insufficient_iregs = 0;
  ms_result->n_insufficient_fregs = 0;
  ms_result->n_insufficient_pregs = 0;
  ms_result->required_itr_count = 0;
  ms_result->required_mve = 0;
  ms_result->is_reg_sufficient = false;
  ms_result->is_itr_sufficient = false;
  ms_result->is_mve_appropriate = false;

  ms_result->evaluation = 0.0;

  ms_result->proc_state = procstate;

  return ms_result;
}

/// \brief schedulingできるiiを決定し,最終的なscheduling結果を返す
/// \param [in] spec スケジューリング指示情報
/// \return MsResultのポインタ。スケジューリングに失敗した場合はnullptrを返す。
///
/// ```
/// 処理は,大まかに二つの方法でscheduleできるiiを探索する.
///      (1) iiを適当な幅で動的に大きくしていき,schedule可能なiiをおおよそ決定する.
///      (2) schedule可能な最小のiiを探索する為,二分探索を行なう.
///
///   scheduleの制御は下記の方針に従う.
///    - レジスタ不足のスケジューリング結果は評価しない（＝採用しない）。
///
///    - 回転数が静的に既知の場合は,回転数が十分になるようにscheduleする.
///      回転数不足の場合は, iiを拡張して再度scheduleを行なう.
///      これは翻訳時間が増加するが確実に性能向上効果があるため.
///
///    - 一度目のschedule見積もりで完全なscheduleを得られなかった場合は,
///      binary searchを行なわない. これは翻訳性能が増加する事と,
///      回転数やレジスタの影響が大きく,小さなiiを小さくする事の影響が小さい為.
///
///    - EXAにおいてSPARCと実施方針を変更する。
///      レジスタが溢れないことが重要となるため、(1)の探索で解が見つかった場合においても、
///      即時にスケジュールを採用するわけではなく、iiを拡張してレジスタに余裕を持たせた解を探索する.
///      (1)でiiを拡張せずに余裕がある解が見つかった場合に限り、(2)の二分探索を実施する。
///        この時、(2)において許容する解は、余裕のある解のみである。
///      (1)でiiを拡張して余裕がある解が見つかった場合、(2)の二分探索を実施する。
///        この時、(2)において許容する解は、余裕のある解のみである。※１
///      (1)で余裕がある解が見つからなかった場合、余裕が無い解を採用する.
/// ```
///
SwplMsResult *SwplMsResult::calculateMsResult(SwplPlanSpec spec) {
  std::unordered_set<SwplMsResult *> ms_result_candidate; ///< 試行した結果のMsResultを保持する
  SwplMsResult *ms_result = nullptr;
  bool candidate_is_collected = false;
  bool do_binary_search       = true;

  /*******************************************************************
   * [step 0] 入力ループのcycleを見積もる
   *    この値は,register不足のscheduleの評価に使用される。
   *    しかし、is_strongルートに対応しないため、0を設定する。
   *    \note 冗長な呼出となっている.
   ********************************************************************/
  spec.original_cycle = 0;

  /*******************************************************************
   * [step 1] scheduleの候補の収集
   * min_ii からii を増やしながら,inst_slot_mapがnullptrでないms_resultを,
   * HashSetに収集する. ii の増やし方は動的に決定される.
   *******************************************************************/
  candidate_is_collected = collectCandidate(spec, ms_result_candidate, false, ProcState::ESTIMATE);

  /* inst_slot_mapを一つも生成できずにscheduleを失敗した場合 */
  if (!candidate_is_collected) { return nullptr; }

  /******************************************************************
   * [step 2] HashSetからms_resultを選択.
   *    [step 2-1]
   *       register資源と回転数が十分なscheduleがあれば採用する.
   *
   *    [step 2-2]
   *       step 2-1でscheduleが得られなかったら,
   *       回転数不足のscheduleしかない場合, iiを拡張して再度scheduleを行なう.
   *
   *       [step 2-2-1]
   *          拡張したiiで回転数不足の場合はあきらめる.
   *
   ******************************************************************/
  /* [step 2-1] */
  ms_result = getEffectiveSchedule(spec, ms_result_candidate);

  if (ms_result == nullptr) {
    do_binary_search = false;

    /* [step 2-2] */
    if (!isAnyScheduleItrSufficient(ms_result_candidate)) {
      candidate_is_collected =
        recollectCandidateWithExII(spec, ms_result_candidate);
      /* HashSetを解放していない為*/
      assert(candidate_is_collected);
      ms_result = getEffectiveSchedule(spec, ms_result_candidate);

      if (ms_result == nullptr) {
        /* [step 2-2-1] */
        if (!isAnyScheduleItrSufficient(ms_result_candidate)) {
          /* 拡張したiiでも回転数が足りていないscheduleしかない場合はあきらめる. */
          if (SWPipeliner::isDebugOutput()) {
            dbgs() << "!swp-msg :Swpl is not applied because of iteration shortage.\n";
          }
        }
      }
    }
  } else if (ms_result != nullptr &&
             ms_result->isModerate() == false) {
    /* 求めた解に余裕がない場合に、更にレジスタに余裕がある解を求める*/
    assert(ms_result->isEffective());
    candidate_is_collected
      = recollectModerateCandidateWithExII(spec, ms_result_candidate, *ms_result);
    if(candidate_is_collected) {
      ms_result = getModerateSchedule(ms_result_candidate);
      /* 余裕がある解を見つけた場合は、binary searchを実施する */
    } else {
      /* 余裕がある解が見つからない場合は、binary searchはせず、
       * 既に見つけた候補から選択する
       */
      do_binary_search = false;
    }
  }

  /* 不要領域解放の前に, binary searchの対象範囲を取得する*/
  if (do_binary_search && (ms_result != nullptr)) {
    unsigned able_min_ii, unable_max_ii;
    getBinarySearchRange(spec, ms_result_candidate, &able_min_ii, &unable_max_ii);
    assert(unable_max_ii <= spec.unable_max_ii);

    /* binary searchにおいて,
     * min_iiはschedulingできない事がわかっているii
     * max_iiはschedulingできる事がわかっているii
     * として動作する為,下記の通りの設定を行なう.
     */
    spec.min_ii = unable_max_ii;
    spec.max_ii = able_min_ii;
  }

  /******************************************************************
   * [step 3] 不要領域の解放
   ******************************************************************/
  /* 下の処理で解放されないように,採用するms_resultをerase */
  if (ms_result != nullptr) {
    ms_result_candidate.erase(ms_result);
  }

  /* 不要なinst_slot_mapを解放する*/
  for( auto *ms_free : ms_result_candidate) {
    if (ms_free->slots != nullptr) {
      delete ms_free->slots;
    }
    delete ms_free; // MsResult域の解放
  }
  ms_result_candidate.clear();

  /******************************************************************
   * [step 4] collect_candidate の結果の精査
   ******************************************************************/
  if (ms_result == nullptr) {
    return nullptr;
  }

  if (!do_binary_search) {
    return ms_result;
  }

  /* より小さいiiでschedulingできる可能性がない場合 */
  if ( ms_result->isEffective() && (spec.max_ii <= spec.min_ii + 1U) ) {
    return ms_result;
  }

  /**************************************************************
   * [step 5] binary searchによる最小のiiの探索
   * spec.min_iiからspec.max_iiまでの間においてschedulingに成功した場合,
   * schdulingできる最小のiiを二分探索する処理
   * 下記の点に注意する必要がある.
   *
   *   - spec.min_iiはschedulingできない事がわかっているii
   *   - spec.max_iiはschedulingできる事がわかっているii
   *   - 上記が既知のためspec.min_ii,spec.max_iiは,
   *     binary searchでscheduling対象とはならない.
   *
   **************************************************************/

  /* 二分探索による ii 算出の高速化 */
  if ( OptionBsearch ) {
    /*
     * 二分探索の結果, たまたまspec.max_iiがscheduling可能な最小のiiであった場合,
     * ms_result_binaryのinst_slot_mapはnullptrとなる.
     */
    SwplMsResult * ms_result_binary = calculateMsResultByBinarySearch(spec);
    if (ms_result_binary) {
      if(ms_result_binary->slots != nullptr) {
        delete ms_result->slots;
        delete ms_result;
        ms_result = ms_result_binary;
      }
      else {
        delete ms_result_binary;
      }
    }
  } else {
    /* 旧方式の場合は,1ずつiiを変化させて探索 */

    // 現在のms_resultは捨てる。calculateMsResultSimplyは、
    // spec.min_ii～spec.max_iiまでスケジューリング試みるため、
    // 必ず何かしらのms_resultを返却する。
    if (ms_result->slots != nullptr) {
      delete ms_result->slots;
    }
    delete ms_result;

    ms_result = calculateMsResultSimply(spec);
  }

  assert(ms_result->slots != nullptr);
  return ms_result;
}

/// \brief 資源と回転数が十分なscheduleを取得する
/// \param [in] spec スケジューリング指示情報
/// \param [in] ms_result_candidate スケジューリング結果(SwplMsResult*)のHashSet
/// \return ms_result_candidateのうち、資源と回転数が十分なスケジューリング結果のMsResultのポインタ
SwplMsResult *SwplMsResult::getEffectiveSchedule(const SwplPlanSpec & spec,
                                         std::unordered_set<SwplMsResult *>& ms_result_candidate) {
  unsigned able_min_ii;
  SwplMsResult *effective_ms_result;

  able_min_ii = spec.max_ii;
  effective_ms_result = nullptr;

  for( auto *temp : ms_result_candidate) {
    if (temp->isEffective()) {
      if (temp->ii < able_min_ii) {
        able_min_ii = temp->ii;
        effective_ms_result = temp;
      }
    }
  }

  return effective_ms_result;
}

/// \brief 回転数が足りるscheduleが一つでもあるかどうか判定する
/// \param [in] ms_result_candidate スケジューリング結果(MS_RESULT)のHashSet
/// \retval true ms_result_candidateに回転数が足りるscheduleが存在する
/// \retval false ms_result_candidateに回転数が足りるscheduleが存在しない
bool SwplMsResult::isAnyScheduleItrSufficient(std::unordered_set<SwplMsResult *>& ms_result_candidate) {
  for( auto *temp : ms_result_candidate) {
    if (temp != nullptr && temp->is_itr_sufficient) {
      return true;
    }
  }
  return false;
}

/// \brief 資源と回転数が十分で余裕があるscheduleを取得する
/// \details スケジューリング結果のHashSetから、moderate（資源と回転数が十分で余裕がある）な
///          スケジューリング結果を返却する。
/// \param [in] ms_result_candidate スケジューリング結果(MS_RESULT)のHashSet
/// \return moderateなスケジュール結果（MS_RESULT*）
///
/// \note moderateなscheduleは多くとも1つしかHashされていない
SwplMsResult *SwplMsResult::getModerateSchedule(std::unordered_set<SwplMsResult *>& ms_result_candidate) {
#if !defined(NDEBUG)
  {
    unsigned int number_of_moderate_schedule = 0;
    for( auto *temp : ms_result_candidate) {
      if(temp->isModerate()) {
        ++number_of_moderate_schedule;
      }
    }
    assert(number_of_moderate_schedule == 1);
  }
#endif

  for( auto *temp : ms_result_candidate) {
    if(temp->isModerate()) {
      return temp;
    }
  }
  return nullptr;
}

/// \brief effectiveなscheduleが可能な最小のiiと不可能な最大のiiを取得する
/// \details binary searchの範囲となる、effectiveなスケジューリングが可能な最小のiiと不可能な最大のiiを取得する。
/// \param [in] spec スケジューリング指示情報
/// \param [in] ms_result_candidate スケジューリング結果(MS_RESULT)のHashSet
/// \param [out] able_min_ii スケジューリングが可能な最小のiiを格納するポインタ
/// \param [out] unable_max_ii スケジューリングが不可能な最大のiiを格納するポインタ
/// \return なし
void SwplMsResult::getBinarySearchRange(const SwplPlanSpec & spec,
                                    std::unordered_set<SwplMsResult *>& ms_result_candidate,
                                    unsigned *able_min_ii,
                                    unsigned *unable_max_ii ) {
  *able_min_ii   = spec.max_ii;
  *unable_max_ii = spec.unable_max_ii;

  for( auto *temp : ms_result_candidate ) {
    if ( temp->isModerate() ) {
      if (temp->ii < *able_min_ii) {
        *able_min_ii = temp->ii;
      }
    }
    else {
      if (temp->ii > *unable_max_ii) {
	*unable_max_ii = temp->ii;
      }
    }
  }
  return;
}

/// \brief schedulingできる最小のiiを二分探索する
/// \details spec.min_ii～spec.max_iiの範囲で、最小のiiでのスケジューリング結果を返す。
///          スケジューリングを試行するiiは、二分探索敵に決定する。
/// \param [in] spec スケジューリング指示情報
/// \return MsResultのポインタ
///
/// \note
///  - start_ii～end_iiの範囲でii２分探索的に決定してスケジューリングを実施する。
///  - start_iiとend_iiの中間がpoint_ii
///
/// ```
///    collectCandidateによって,spec.min_iiとspec.max_iiの間に
///   schedulingができるiiが見つかった場合に実行される.
///
///   本関数の最初の呼出ではcollectCandidateでiiを増やして探索した結果,
///   schedulingできない最後のiiがstart_ii, schedulingできる最初のiiがend_iiである.
///
///   本処理中ではstart_iiではschedulingができない事,
///   end_iiではschedulingができる事が保証される.
///
///   本関数の再帰呼出の結果,schedulingできるものがなかった場合,
///   collectCandidateで得られたscheduling結果が利用される.
/// ```
///
SwplMsResult *SwplMsResult::calculateMsResultByBinarySearch(SwplPlanSpec spec) {
  SwplMsResult * ms_result;
  SwplMsResult * ms_result_child;
  unsigned point_ii, start_ii, end_ii;

  start_ii = spec.min_ii;
  end_ii   = spec.max_ii;
  /* 次にschedulingするiiはstart_iiとend_iiの中点 */
  point_ii = start_ii + ((end_ii - start_ii)>>1);

  assert(start_ii < end_ii);

  /* 二分探索の終了条件 */
  if ((start_ii == point_ii) || (point_ii == end_ii)) {
    assert (end_ii - start_ii == 1);
    return nullptr;
  }

  /* 中点におけるschedulingの試行 */
  ms_result = SwplMsResult::calculateMsResultAtSpecifiedII(spec, point_ii, ProcState::BINARY_SEARCH);

  /* 探索範囲を二分する*/
  if (ms_result && ms_result->slots == nullptr) {
    /* scheduling不可の最大iiを増加 */
    start_ii = point_ii;
  } else {
    /* scheduling可能の最小iiを減少 */
    end_ii   = point_ii;
  }

  /* 二分探索の再帰呼出*/
  spec.min_ii = start_ii;
  spec.max_ii = end_ii;
  ms_result_child = SwplMsResult::calculateMsResultByBinarySearch(spec);

  /*
   * ms_result_childのinst_slot_mapがnullptrではない時,
   * ms_result_childのiiがend_iiより必ず小さいので採用する
   */
  if(ms_result_child ) {
    if(ms_result_child->slots != nullptr) {
      if (ms_result) {
        if(ms_result->slots != nullptr) {
          delete ms_result->slots;
        }
        delete ms_result;
      }
      ms_result = ms_result_child;
    }
    else {
      delete ms_result_child;
    }
  }

  return ms_result;
}

/// \brief あるiiでschedulingを行ないMsResultを生成する
/// \param [in] spec スケジューリング指示情報
/// \param [in] ii スケジューリングを試みるii
/// \param [in] procstate MsResultの処理状態（ex. binary_search中、など）
/// \return MsResultのポインタ
///
/// \note 本関数は, binary searchにおける各iiで呼ばれる.<br>
///       そのため,資源と回転数が十分のresultのみ返却するものとする.<br>
///       またbinary searchを用いない場合の旧方式で使用される.<br>
SwplMsResult *
SwplMsResult::calculateMsResultAtSpecifiedII(SwplPlanSpec spec, unsigned ii, ProcState procstate) {
  SwplMsResult * ms_result;

  ms_result = SwplMsResult::constructInit(ii, procstate);
  ms_result->constructInstSlotMapAtSpecifiedII(spec);
  ms_result->slots = SwplSSProc::execute(spec.ddg, spec.loop, ii, ms_result->slots, dbgs());
  ms_result->checkInstSlotMap(spec);
  ms_result->outputDebugMessageForSchedulingResult(spec);

  /* binary searchを行なう際は, effective以上のものしか受け入れない*/
  if (!ms_result->isModerate() &&
       ms_result->slots != nullptr) {
    delete ms_result->slots;
    ms_result->slots = nullptr;
  }
  return ms_result;
}

/// \brief iiを1ずつ増やしながらschedulingを行ないMsResultを生成する
/// \details spec.min_ii～spec.max_iiの範囲で、最小のiiでのスケジューリング結果を返す。
///          スケジューリングを試行するiiは１ずつ増やす。
/// \param [in] spec スケジューリング指示情報
/// \return MsResultのポインタ
///
/// \note spec.max_iiでschedulingが可能である事を保証しなければならない.
SwplMsResult *SwplMsResult::calculateMsResultSimply(SwplPlanSpec spec) {
  SwplMsResult * ms_result;
  unsigned ii;

  for (ii = spec.min_ii; ii <= spec.max_ii; ++ii) {
    ms_result = SwplMsResult::calculateMsResultAtSpecifiedII(spec, ii, ProcState::SIMPLY_SEARCH);
    if (ms_result->slots != nullptr) {
      break;
    }
    delete ms_result; // inst_slot_mapが得られなかったMsResultは捨てる
  }
  return ms_result;
}

/// \brief min_iiとmax_iiの間でschedulingできるiiを大まかに探索する
/// \param [in] spec スケジューリング指示情報のポインタ
/// \param [in] ms_result_candidate SwplMsResult*のHashSet
/// \param [in] watch_regs 資源チェックを実施するかのフラグ
/// \param [in] procstate MsResultの処理状態（ex. binary_search中、など）
/// \retval true スケジューリングに成功した
/// \retval false スケジューリングに失敗した
///
/// \note min_ii から ii を増やしながら modulo scheduling および
///       reigster が足りる ii を算出する。ii の増やし方は動的に決定する。<br>
///       ) ex. 基本的に ii で scheduling できたら、ii+1 でも
///             scheduling できるという仮定で本処理が動作する。<br>
///       しかし ながら実際は、isHardRegsSufficient() により、
///       その前提が崩れるケースがある。しかし、この前提が崩れるのは
///       極稀であり、この前提が無いと上手く翻訳性能改善できない。
///       このため稀なケースは無視することにした。
/// \note budgetが尽きるようなIIを選んでいるのでよくない
/// \note specのメンバ変数のうちunable_max_iiのみが更新される
bool SwplMsResult::collectCandidate(
    SwplPlanSpec & spec,
                                std::unordered_set<SwplMsResult *>& ms_result_candidate,
                                bool watch_regs,
                                ProcState procstate) {
  SwplMsResult *ms_result;
  unsigned ii, inc_ii, prev_tried_n_insts;

  prev_tried_n_insts = 0;

  for (ii = spec.min_ii; /* 下で */; ii += inc_ii) {
    ms_result = SwplMsResult::constructInit(ii, procstate);
    ms_result->constructInstSlotMapAtSpecifiedII(spec);
    ms_result->slots = SwplSSProc::execute(spec.ddg, spec.loop, ii, ms_result->slots, dbgs());
    ms_result->checkInstSlotMap(spec);
    ms_result->outputDebugMessageForSchedulingResult(spec);

    /*
     * inst_slot_mapがnullptrではない場合,資源が不足していた場合でも,
     * scheduleが採用される可能性がある為,候補として登録する.
     */
    SwplMsResult ms_result_tmp = *ms_result; // チェックおよび情報出力のためcopy
    if (ms_result->slots != nullptr) {
      ms_result_candidate.insert(ms_result);
    }
    else {
      delete ms_result; // inst_slot_mapが得られなかったMsResultは捨てる
    }

    /* ループ終了条件:
     * (1) 資源と回転数が十分なスケジュール結果を得た場合.
     * (2) 資源と回転数が足りるschedulingを見つけられずiiが上限に達した時.
     * (3) レジスタ不足の時に,iiを増やしても改善が見られない時.
     */
    if (ms_result_tmp.isEffective()) {
      break;
    }
    else {
      spec.unable_max_ii = ii;
      if (ii >= spec.max_ii-1) {
        assert(ii <= spec.max_ii);
        ms_result_tmp.outputGiveupMessageForEstimate(spec);
        break;
      }
      if (watch_regs && !isRegReducible(spec, ms_result_candidate)) {
        if (SWPipeliner::isDebugOutput()) {
          dbgs() << "        : Quit estimation because required regs are not reducible.[giveup]\n";
        }
        break;
      }
    }

    /* inc_iiの決定処理 :ii + inc_ii <= max_ii - 1 が成立する値を返す*/
    inc_ii = ms_result_tmp.getIncII(spec, prev_tried_n_insts);
    prev_tried_n_insts = ms_result_tmp.tried_n_insts;
  }

  return !ms_result_candidate.empty();
}

/// \brief 余裕があるschedulingを大まかに探索する
/// \details min_iiから徐々にiiを増やし、最初に見付けた余裕があschedule結果を、ms_result_candidateに追加する。
/// \param [in,out] spec スケジューリング指示情報のポインタ
/// \param [in,out] ms_result_candidate スケジューリング結果(MS_RESULT)のHashSet
/// \param [in] procstate MsResultの処理状態（ex. binary_search中、など）
/// \retval true スケジューリングに成功した（ms_result_candidateに新しい結果が追加される）
/// \retval false スケジューリングに失敗した
///
/// \note
///   - iiを、spec.min_iiからspec.max_iiまでinc_iiずつ更新しながら、
///     スケジューリングを実施する
///   - iiでのスケジューリングが成功し、かつ、
///     そのスケジューリング結果がMsResult.isModerateの条件を満たす場合は、
///     ms_result_candidateに追加して復帰する
///   - それ以外の場合は、spec.unable_max_iiを更新し、必要に応じて領域開放し、やり直し
bool SwplMsResult::collectModerateCandidate(
    SwplPlanSpec & spec,
                                        std::unordered_set<SwplMsResult *>& ms_result_candidate,
                                        ProcState state) {
  SwplMsResult * ms_result;
  unsigned ii, inc_ii;

  inc_ii = std::max((unsigned)1, (spec.max_ii - spec.min_ii)/5);

  for (ii = spec.min_ii; ii < spec.max_ii; ii += inc_ii) {
    ms_result = SwplMsResult::constructInit(ii, state);
    ms_result->constructInstSlotMapAtSpecifiedII(spec);
    ms_result->slots = SwplSSProc::execute(spec.ddg, spec.loop, ii, ms_result->slots, dbgs());
    ms_result->checkInstSlotMap(spec);
    ms_result->outputDebugMessageForSchedulingResult(spec);

    if(ms_result->isModerate()) {
      ms_result_candidate.insert(ms_result);
      return true;
    }
    else {
      spec.unable_max_ii = ii;
      if (ms_result->slots != nullptr) {
        /* moderateでは無い場合、scheduleが採用される可能性はなく、
         * Hashに入れないため、不要なinst_slot_mapは破棄する */
        delete ms_result->slots;
        ms_result->slots = nullptr;
      }
      delete ms_result;
    }
    // \note inc_iiの決定にはgetIncIIを用いたいが未対応
  }

  return false;
}

/// \brief iiを拡張して、moderateなschedulingを探索する
/// \details 更にレジスタに余裕があるスケジューリングを求めるため、
///          Minii、MaxIIを拡張して、moderateなschedulingの探索を依頼する。
/// \param [in,out] spec スケジューリング指示情報のポインタ
/// \param [in,out] ms_result_candidate スケジューリング結果(MS_RESULT)のHashSet
/// \param [in] ms_result 再試行のベースとなるMsResult
/// \retval true スケジューリングに成功した（ms_result_candidateに新しい結果が追加される）
/// \retval false スケジューリングに失敗した
bool SwplMsResult::recollectModerateCandidateWithExII(
    SwplPlanSpec & spec,
                                                  std::unordered_set<SwplMsResult *>& ms_result_candidate,
    SwplMsResult & ms_result) {
  unsigned old_maxii = spec.max_ii;
  unsigned old_minii = spec.min_ii;
  spec.min_ii = ms_result.ii+1;
  /* 翻訳時間の都合で広くはしすぎない*/
  spec.max_ii = std::max(spec.min_ii, ms_result.ii * 2);
  /* unableはmoderateではないという意味 */
  spec.unable_max_ii = ms_result.ii;

  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "        : Change ii range for moderate schedule search:"
           << "[ " << old_minii   <<", " << old_maxii << " ] -> "
           << "[ " << spec.min_ii << ", " << spec.max_ii <<" ]\n";
  }

  return collectModerateCandidate(spec, ms_result_candidate, ProcState::RECOLLECT_MODERATE);
}

/// \brief iiを拡張してscheduleを再収集する
/// \details スケジューリング指示情報のiiを拡張し、collectCandidate関数を実施して再scheduleする。
/// \note 資源不足の場合に呼び出され、iiを拡張してscheduleを再収集する。
/// \param [in,out] spec スケジューリング指示情報のポインタ
/// \param [in,out] ms_result_candidate SwplMsResult*のHashSet
/// \retval true スケジューリングに成功した
/// \retval false スケジューリングに失敗した
bool SwplMsResult::recollectCandidateWithExII(
    SwplPlanSpec & spec,
                                          std::unordered_set<SwplMsResult *>& ms_result_candidate) {
  unsigned old_maxii = spec.max_ii;
  unsigned old_minii = spec.min_ii;

  spec.min_ii = spec.max_ii;
  spec.max_ii = spec.max_ii * 2;
  assert (spec.min_ii <= spec.max_ii);

  if (SWPipeliner::isDebugOutput()) {
    dbgs() << "        : Extend ii range for iteration shortage:"
           << "[ " << old_minii   << ", " << old_maxii << " ] -> "
           << "[ " << spec.min_ii << ", " << spec.max_ii << " ]\n";
  }

  /*
   * 拡張したiiでスケジュールの再収集を行なう.
   * \note 解放処理を共通にする為,一度目のscheduleはHashSetに入ったままである.
   */
  return SwplMsResult::collectCandidate(spec, ms_result_candidate, true, ProcState::RECOLLECT_EFFECTIVE);
}

/// \brief collectCandidateのために次回転のinc_iiを取得する
/// \details スケジュール結果から、次回スケジューリングのiiを求めるためのinc_iiを取得する。
/// \param [in] spec スケジューリング指示情報
/// \param [in] prev_tried_n_insts 前回の試行でスケジューリングされた命令数
/// \return 算出したinc_ii
///
/// \note  heuristicな手法でinc_iiを決めている.
unsigned SwplMsResult::getIncII(SwplPlanSpec & spec, unsigned prev_tried_n_insts) {
  unsigned inc_ii = 0;

  /* (n_insts - tried_n_insts)>>1 は schedule されなかった命令数の約半分。*/
  inc_ii = ((spec.n_insts - tried_n_insts)>>1);
  if ((prev_tried_n_insts == tried_n_insts) && (inc_ii < 4)) {
    /* 命令の平均 latency 4 τくらい(^^;離す */
    inc_ii = 4;
  }

  /* レジスタ不足の場合 */
  if (!is_reg_sufficient) {
    // \note レジスタ不足数の優先度は、ireg > freg > pregとする
    //       この数値はinc_iiの計算に使用されている為、
    //       従来とschedulingを変えないために暫定的に以下のようにする。
    unsigned n_insufficient_regs_freg_or_preg =
      (n_insufficient_fregs > 0) ? n_insufficient_fregs : n_insufficient_pregs;
    unsigned n_insufficient_regs =
      (n_insufficient_iregs > 0) ? n_insufficient_iregs : n_insufficient_regs_freg_or_preg;

    inc_ii = std::max(inc_ii, n_insufficient_regs);
    /* registerの場合は少しiiを広げると急激に少なくなる場合があるため,
     *  急激に増やさないよう適度に抑える
     */
    inc_ii = std::min(inc_ii, (unsigned)20);
  }

  /* 回転数不足の場合 */
  if (!is_itr_sufficient) {
    // OCT assumed iterationが設定されている場合、
    // 不足するルートを通すため、かならずしも回転数は定数ではない
    inc_ii = std::max(inc_ii, (unsigned)(spec.min_ii* 0.2));
  }

  /* mveが大きすぎる場合 */
  if (!is_mve_appropriate) {
    /* 従来通りのincとする. */
  }

  inc_ii = std::max(std::min(inc_ii, spec.max_ii), (unsigned)1);

  /*
   * 大きなii幅で細かくiiを刻んでも仕方がないため,
   * iiの増やし方をheuristicに調整する
   */
  if (spec.min_ii > 100) {
    inc_ii = (unsigned)std::max((spec.max_ii - spec.min_ii)*0.1, (double)inc_ii);
  }

  /* ii+inc_ii が max_ii-1 を超えてしまうような時は inc_ii を調整する */
  if (ii + inc_ii > spec.max_ii-1) {
    inc_ii = spec.max_ii - 1 - ii;
  }

  assert(inc_ii >= 1);

  return inc_ii;
}

/// \brief iiを減らすとregが減る事が期待できるか否かを判断する
/// \details iiを拡張する事で、register不足が解消する可能性があるかを、
///          既に得られているscheduleからheuristicに判断する。
/// \param [in] spec スケジューリング指示情報
/// \param [in] ms_result_candidate SwplMsResult*のHashSet
/// \retval true iiを減らすとregが減る事が期待できる
/// \retval false iiを減らしてもregが減る事が期待できない
///
/// \note register不足の場合にiiを拡張する事で,register不足が解消する可能性が
///       あるかどうかを、既に得られているscheduleからheuristicに判断する.<br>
///       本機能は翻訳性能改善のために, recollectCandidateWithExIIが
///       無駄に終ると思われる場合に,事前に抑止する事,または
///       recollectCandidateWithExIIでschedulingを行なっている時の
///       中断を目的としている. <br>
///       中断してよい理由として,
///       evaluateSpillingScheduleはiiを増やすほど評価が低くなる為,
///       不足registerが同じである場合はevaluationが増大する事はない.
///       よりよいscheduleが得られると期待できない為,抑止する.
bool SwplMsResult::isRegReducible(
    SwplPlanSpec & spec, std::unordered_set<SwplMsResult *>& ms_result_candidate) {
  unsigned threshold_reduced_reg;

  unsigned n_candidate = 0;
  unsigned able_maxii = spec.min_ii - 1;
  unsigned able_second_maxii = spec.min_ii - 1;
  unsigned maxii_short_freg = 0, maxii_short_ireg = 0, maxii_short_preg = 0;
  unsigned second_maxii_short_freg = 0, second_maxii_short_ireg = 0, second_maxii_short_preg = 0;

  for(auto *temp : ms_result_candidate) {
    assert(temp->isEffective() == false);
    if (temp->slots != nullptr && temp->is_itr_sufficient) {
      if (able_maxii < temp->ii) {
        n_candidate++;
        able_second_maxii = able_maxii;
        second_maxii_short_freg = maxii_short_freg;
        second_maxii_short_ireg = maxii_short_ireg;
        second_maxii_short_preg = maxii_short_preg;

        able_maxii = temp->ii;
        maxii_short_freg = temp->n_insufficient_fregs;
        maxii_short_ireg = temp->n_insufficient_iregs;
        maxii_short_preg = temp->n_insufficient_pregs;
      }
    }
  }

  /* レジスタ不足なscheduleが一つ以下しかない場合は,保守的にregの減少を期待する*/
  if (n_candidate <= 1) {
    return true;
  }

  /* 不足レジスタがない場合も,保守的に必要なregisterが減少できるものとする*/
  if (maxii_short_freg == 0 && maxii_short_ireg == 0 && maxii_short_preg) {
    return true;
  }

  threshold_reduced_reg = std::max((unsigned)((able_maxii - able_second_maxii) * 0.1), (unsigned)1);

  /* iiを増やしたら適度にregisterが減っている場合 */
  return ((second_maxii_short_freg - maxii_short_freg >= threshold_reduced_reg) ||
          (second_maxii_short_ireg - maxii_short_ireg >= threshold_reduced_reg) ||
          (second_maxii_short_preg - maxii_short_preg >= threshold_reduced_reg)
          );
}

/// \brief Execute StageScheduling
/// \return SwplInstSlotHashmap adopt scheduling results
SwplSlots* SwplSSProc::execute(const SwplDdg &ddg,
                         const SwplLoop &loop,
                         unsigned ii,
                         SwplSlots *slots,
                         raw_ostream &stream) {
  auto fname = (loop.getBodyInsts())[0]->getMI()->getMF()->getName();

  if (!OptionEnableStageScheduling)
    return slots;

  if (slots==nullptr) {
    if (OptionDumpSSProgress)
      stream << "*** No StageScheduling: [" << fname << "] no result of SWPL at II=" << ii << ".\n";
    return slots;
  }
  if (slots->calcPrologBlocks(loop, ii)==0) {
    if (OptionDumpSSProgress)
      stream << "*** No StageScheduling: [" << fname << "] loop is Scheduled. But prologue-cycle is 0." << ii << ".\n";
    return slots;
  }

  SwplSSNumRegisters estimateRegBefore(loop, ii, slots);
  if (OptionDumpSSProgress) {
    stream << "*** before SS : dump SwplSSNumRegisters ***\n";
    estimateRegBefore.dump(stream);
  }

  SwplSSCyclicInfo ssCyclicInfo(ddg, loop);
  if (OptionDumpCyclicRoots) {
    ssCyclicInfo.dump(stream);
  }

  SwplSSEdges ssEdges(ddg, loop, ii, *slots);
  ssEdges.updateInCyclic(ssCyclicInfo);
  if (OptionDumpSSProgress) {
    stream << "*** before SS : dump SwplSSEdges ***\n";
    ssEdges.dump(stream, fname);
  }

  // replicate scheduling results
  auto* slots_temp = new SwplSlots;
  *slots_temp = *slots;

  SwplSSMoveinfo acresult;

  // heuristic AC
  SwplSSACProc::execute(ddg, ii, ssEdges, acresult, stream);
  if (OptionDumpSSProgress) {
    stream << "*** dump SwplSSMoveinfo after AC ***\n";
    SwplSSProc::dumpSSMoveinfo(stream, acresult);
  }

  // adjust SwplInstSlotHashmap by SwplSSMoveinfo
  SwplSSProc::adjustSlot(*slots_temp, acresult);
  SwplSSNumRegisters estimateRegAfter(loop, ii, slots_temp);
  if (OptionDumpSSProgress) {
    stream << "*** after SS : dump SwplSSNumRegisters ***\n";
    estimateRegAfter.dump(stream);
    stream << "*** after SS : dump SwplSSEdges ***\n";
    SwplSSEdges TEMPssEdges(ddg, loop, ii, *slots_temp);
    TEMPssEdges.updateInCyclic(ssCyclicInfo);
    TEMPssEdges.dump(stream, fname);
  }

  // Determine whether to adopt or reject StageScheduling results
  // by comparing SwplSSNumRegisters.
  if (!(estimateRegAfter<estimateRegBefore)) {
    // Discard StageScheduling results
    if (OptionDumpSSProgress) {
      stream << "*** StageScheduling results rejected... ***\n";
    }
    delete slots_temp;
    return slots;
  }

  // Adopt StageScheduling results
  if (OptionDumpSSProgress) {
    stream << "*** Adopt StageScheduling results!!! ***\n";
  }
  if (SWPipeliner::isDebugOutput()) {
      dbgs() << "                                                                "
             << "Estimated num of vregs was changed by StageScheduling : ";
      estimateRegBefore.print(dbgs());
      dbgs() << " -> ";
      estimateRegAfter.print(dbgs());
      dbgs() << "\n";
  }
  delete slots;
  return slots_temp;
}

/// \brief dump SwplSSMoveinfo
void SwplSSProc::dumpSSMoveinfo(raw_ostream &stream, const SwplSSMoveinfo &v) {
  if (v.size()==0) {
    stream << "none...\n";
    return;
  }

  for (auto [cinst, movecycle]: v) {
    stream << format("%p", cinst->getMI()) << ":" << cinst->getName() << " : move val=" << movecycle << "\n";
  }
  return;
}

/// \brief update SwplInstSlotHashmap by SwplSSMoveinfo
/// \param [in/out] SwplInstSlotHashmap to be updated
/// \param [in] Instruction placement cycle movement information
/// \return true if SwplInstSlotHashmap changed
bool SwplSSProc::adjustSlot(SwplSlots& slots, SwplSSMoveinfo &v) {
  auto bandwidth = SWPipeliner::FetchBandwidth;
  for (auto [cinst, movecycle]: v) {
    SwplInst *inst = const_cast<SwplInst *>(cinst);
    slots.at(inst->inst_ix).moveSlotIndex(movecycle*bandwidth);
  }
  return true;
}

/// \brief execute AC
/// \param [in] d DDG
/// \param [in] ii InitiationInterval
/// \param [in] e Edge information
/// \param [in/out] Instruction placement cycle movement information
/// \param [in] stream stream of message output
/// \retval true change SwplInstSlotHashmap by StageScheduling-AC
bool SwplSSACProc::execute(const SwplDdg &d,
                           unsigned ii,
                           SwplSSEdges &e,
                           SwplSSMoveinfo &moveinfo,
                           raw_ostream &stream) {
  SwplSSACProc acproc(d, ii, e);
  llvm::SmallVector<SwplSSEdge*, 8> targetedges;
  acproc.getTargetEdges(targetedges);
  for (auto &t: targetedges) {
    acproc.collectPostInsts(*t, moveinfo);
    t->numSkipFactor = 0;
  }
  e.updateCycles(moveinfo);
  return true;
}

/// \brief Collect Edges eligible for AC
/// \param [in/out] v Collection result vector
/// \return num of Collection Edges
unsigned SwplSSACProc::getTargetEdges(llvm::SmallVector<SwplSSEdge*, 8> &v){
  for (auto *e: ssedges.Edges) {
    if ((e->numSkipFactor!=0) && (e->isCyclic()==false)) {
      v.push_back(e);
    }
  }
  return v.size();
}

/// \brief Node search for adjustment by AC
/// \param [in] ddg DDG
/// \param [in/out] explored_list set to store route search results
/// \param [in] out Search information(to) 
/// \param [in] in Search information(from) 
/// \return Nothing
static void node_search(const SwplDdg& ddg,
                        std::set<const SwplInst*> &explored_list,
                        const SwplInst* out, const SwplInst* in) {
  if (explored_list.count(in) > 0) {
    return; // already checked.
  }
  explored_list.insert(in);

  std::set<const SwplInst*> dest;
  for (auto *edge: ddg.getGraph().getEdges()) {
    const std::vector<unsigned> &distances = ddg.getDistancesFor(*edge);
    if (distances.size()==1 && distances[0]==20)
      continue;

    if (edge->getInitial()==edge->getTerminal())
      continue;

    if (edge->getInitial()==in && edge->getTerminal()!=out)
      dest.insert(edge->getTerminal());
    else if (edge->getTerminal()==in && edge->getInitial()!=out)
      dest.insert(edge->getInitial());
    else
      continue;
  }

  if (dest.empty()) {
    return;
  }

  for (auto *d: dest) {
    if (explored_list.count(d) == 0) {
      node_search(ddg, explored_list, in, d);
    }
  }
  return;
}

/// \brief Collect subsequent instructions and movement amount
/// \param [in] e Edge information
/// \param [in/out] Cycle movement per instruction
/// \return Nothing
void SwplSSACProc::collectPostInsts(const SwplSSEdge &e, SwplSSMoveinfo &v) {
  auto pre_i = e.InitialVertex;
  auto post_i = e.TerminalVertex;
  auto sf = e.numSkipFactor;
  long movecycle = (0-sf)*II;

  std::set<const SwplInst*> node_list;
  node_search(ddg, node_list, pre_i, post_i);
  for (auto *p: node_list) {
    if (v.contains(p)) {
      v[p]+=movecycle;
    }
    else {
      v.insert(std::make_pair(p, movecycle));
    }
  }
  return;
}

/// \brief SwplSSEdge constructor
SwplSSEdge::SwplSSEdge(const SwplInst *ini, unsigned inicycle,
                       const SwplInst *term, unsigned termcycle,
                       int delay_in, unsigned distance_in,
                       unsigned ii) : delay(delay_in),
                                      distance(distance_in),
                                      II(ii),
                                      inCyclic(false),
                                      InitialVertex(ini),
                                      InitialCycle(inicycle),
                                      TerminalVertex(term),
                                      TerminalCycle(termcycle),
                                      numSkipFactor(0) {
  long sf_val=0;
  if ((InitialCycle==TerminalCycle) && (InitialVertex->getMI()==TerminalVertex->getMI())) {
    // Edge to self is not subject to StageScheduling
    sf_val = 0;
  }
  else {
    sf_val = (int)(std::floor( ((double)TerminalCycle - InitialCycle - delay) / II )+distance);
  }
  assert(sf_val>=0);
  numSkipFactor = sf_val;
}

/// \brief dump SwplSSEdge
void SwplSSEdge::dump(raw_ostream &stream, StringRef fname) const {
  stream << "[" << fname << "] ";
  stream << "Edge:(";
  stream << format("%p", InitialVertex->getMI()) << ":" << InitialVertex->getName();
  stream << ") to (";
  stream << format("%p", TerminalVertex->getMI()) << ":" << TerminalVertex->getName();
  stream << ")";
  stream << ", inCyclic:" << inCyclic;
  stream << ", SkipFactor<" << numSkipFactor << ">";
  stream << ", p_cycle:" << InitialCycle << ", s_cycle:" << TerminalCycle;
  stream << ", delay:" << delay  << ", distance:" << distance;
  stream << ", II:" << II << "\n";
}

/// \brief SwplSSEdges constructor
/// \details Generate edge information used for adjustment in StageScheduling
///          from DDG, loop, and instruction placement results
SwplSSEdges::SwplSSEdges(const SwplDdg &ddg,
                         const SwplLoop &loop,
                         unsigned ii,
                         SwplSlots& slots) : II(ii) {
  const SwplInstGraph &graph = ddg.getGraph();
  for (auto *inst : loop.getBodyInsts()) {
    SwplSlot p_slot;
    unsigned p_cycle;

    auto begin_slot = slots.findBeginSlot(loop, ii);
    p_slot = slots.getRelativeInstSlot(*inst, begin_slot);
    p_cycle = p_slot.calcCycle();

    for (auto *sucinst: graph.getSuccessors(*inst) ) {

      auto edge = graph.findEdge( *inst, *sucinst );
      assert(edge != nullptr);

      const std::vector<unsigned> &distances = ddg.getDistancesFor(*edge);
      const std::vector<int> &delays = ddg.getDelaysFor(*edge);
      auto distance = distances.begin();
      auto delay = delays.begin();
      auto distance_end = distances.end();

      int max_modulo_delay=INT_MIN;
      int out_delay=INT_MIN;
      unsigned out_distance=INT_MIN;

      for (; distance!=distance_end ; ++distance, ++delay) {
        int delay_val = *delay;
        unsigned distance_val = *distance;

        if (delay_val==0) {
          // The delay is 0 only when the from instruction of edge is a pseudo instruction.
          assert(SWPipeliner::STM->isPseudo(*(edge->getInitial()->getMI())));
          delay_val=1;
        }

        int modulo_delay = delay_val - ii * (int)(distance_val);

        if (max_modulo_delay < modulo_delay) {
          // Record delay and distance when treated as modulo_delay
          max_modulo_delay = modulo_delay;
          out_delay = delay_val;
          out_distance = distance_val;
        }
      }

      SwplSlot s_slot = slots.getRelativeInstSlot(*sucinst, begin_slot);
      unsigned s_cycle = s_slot.calcCycle();

      if (out_distance==20) {
        continue;
      }

      SwplSSEdge* newedge = new SwplSSEdge(inst, p_cycle, sucinst, s_cycle, out_delay, out_distance, ii);
      Edges.push_back(newedge);
    }
  }
}

/// \brief update SwplSSedge.inCyclic with SwplSSCycleInfo
/// \param [in] ci Circulation part information
/// \return Nothing
void SwplSSEdges::updateInCyclic(const SwplSSCyclicInfo & ci) {
  for (auto *edge: Edges) {
    edge->setInCyclic(ci.isInCyclic(edge->InitialVertex,
                                    edge->TerminalVertex));
  }
}

/// \brief update SwplSSedge.InitialCycle/TerminalCycle by SwplSSMoveinfo
/// \param [in/out] Instruction placement cycle movement information
/// \return Nothing
void SwplSSEdges::updateCycles(const SwplSSMoveinfo &v) {
  for (auto [cinst, movecycle]: v) {
    for (auto *edge: Edges) {
      if (edge->InitialVertex==cinst)
        edge->InitialCycle+=movecycle;
      if (edge->TerminalVertex==cinst)
        edge->TerminalCycle+=movecycle;
    }
  }
}

/// \brief dump SwplSSedges
void SwplSSEdges::dump(raw_ostream &stream, StringRef fname) const {
  for (auto *edge: Edges) {
    edge->dump(stream, fname);
  }
}

/// \brief Node search for circular/acyclic determination.
/// \param [in] ddg DDG
/// \param [in/out] explored_list set to store route search results
/// \param [in/out] rootlists vector of circular partial information
/// \param [in] out Search information(to)
/// \param [in] in Search information(from)
/// \param [in/out] rootlist circular partial information
/// \return Nothing
static void cyclic_search( const SwplDdg& ddg,
                         std::set<const SwplInst*> &explored_list,
                         std::vector<std::vector<const SwplInst*> *> &rootlists,
                         const SwplInst* out, const SwplInst* in, std::vector<const SwplInst*> *rootlist ) {
  if (explored_list.count(in) > 0) {
    return; // already checked.
  }
  explored_list.insert(in);

  std::set<const SwplInst*> dest;
  for (auto *edge: ddg.getGraph().getEdges()) {
    const std::vector<unsigned> &distances = ddg.getDistancesFor(*edge);
    if (distances.size()==1 && distances[0]==20)
      continue;

    if (edge->getInitial()==edge->getTerminal())
      continue;

    if (edge->getInitial()==in && edge->getTerminal()!=out)
      dest.insert(edge->getTerminal());
    else if (edge->getTerminal()==in && edge->getInitial()!=out)
      dest.insert(edge->getInitial());
    else
      continue;
  }

  if (dest.empty()) {
    rootlists.push_back(rootlist);
    return;
  }

  for (auto *d: dest) {
    std::vector<const SwplInst*> *reprootlist = new std::vector<const SwplInst*>;
    for (auto *node: *rootlist) {
      reprootlist->push_back(node);
    }
    reprootlist->push_back(d);

    if (explored_list.count(d) > 0) {
      rootlists.push_back(reprootlist);
    }
    else {
      cyclic_search(ddg, explored_list, rootlists,
                  in, d, reprootlist );
    }
  }
  delete rootlist;
  return;
}

/// \brief SwplSSCycleInfo constructor
/// \detail Extract circulating nodes.
/// \param [in] ddg DDG
/// \param [in] loop loop information
/// \return Nothing
SwplSSCyclicInfo::SwplSSCyclicInfo(const SwplDdg& ddg,
                                   const SwplLoop& loop) {
  std::set<const SwplInst*> explored_list;
  std::vector<std::vector<const SwplInst*> *> rootlists;
  for (auto *inst: loop.getBodyInsts()) {
    if (!(explored_list.count(inst) > 0)) {
      std::vector<const SwplInst*> *rootlist = new std::vector<const SwplInst*>;
      rootlist->push_back(inst);
      cyclic_search(ddg,
                  explored_list, rootlists,
                  nullptr, inst,
                  rootlist);
    }
  }

  for (auto *root: rootlists) {
    unsigned size = root->size();
    for (unsigned i=0; i<(size-1); i++) {
      for (unsigned j=(i+1); j<size; j++) {
        if (root->at(i)==root->at(j)) {
          std::vector<const SwplInst*> *cnodes = new std::vector<const SwplInst*>;
          for (unsigned k=i; k<=j; k++) {
            cnodes->push_back(root->at(k));
          }
          cyclic_node_list.push_back(cnodes);
        }
      }
    }
    delete root;
  }
}

/// \brief Check if Edge is in cyclic-part.
/// \retval true edge of ini to term, is in cyclic-part.
/// \retval false edge of ini to term, is not in cyclic-part.
bool SwplSSCyclicInfo::isInCyclic(const SwplInst *ini, const SwplInst *term) const {
  for (auto *nodes: cyclic_node_list) {
    unsigned size=nodes->size();
    if (size < 2) continue;
    for (unsigned i=0; i<(size-1); i++) {
      if ((nodes->at(i)==ini  && nodes->at(i+1)==term) ||
          (nodes->at(i)==term && nodes->at(i+1)==ini ))
        return true;
    }
  }
  return false;
}

/// \brief dump SwplSSCycleInfo
void SwplSSCyclicInfo::dump(raw_ostream &stream) const {
  stream << "*** dump SwplSSSyclicInfo ***\n";
  if (cyclic_node_list.size()==0) {
    stream << "There is no circulation part.\n";
  }
  else {
    for (auto *root: cyclic_node_list){
      stream << "root : { ";
      for (auto *node: *root){
        stream << "(" << format("%p", node->getMI()) << ":" << node->getName() << ") ";
      }
      stream << "}\n";
    }
  }
}

/// \brief SwplSSNumRegisters constructor
SwplSSNumRegisters::SwplSSNumRegisters(const SwplLoop &loop,
                                       unsigned ii,
                                       const SwplSlots *slots) {
  auto n_renaming_versions = slots->calcNRenamingVersions(loop, ii);

  ireg = SwplRegEstimate::calcNumRegs(loop, slots, ii,
                                      llvm::StmRegKind::getIntRegID(),
                                      n_renaming_versions);
  freg = SwplRegEstimate::calcNumRegs(loop, slots, ii,
                                      llvm::StmRegKind::getFloatRegID(),
                                      n_renaming_versions);
  preg = SwplRegEstimate::calcNumRegs(loop, slots, ii,
                                      llvm::StmRegKind::getPredicateRegID(),
                                      n_renaming_versions);
}

/// \brief dump SwplSSNumRegisters
void SwplSSNumRegisters::dump(raw_ostream &stream) const {
  stream << "estimate VReg  Fp: " << freg << ", Int: " << ireg << ", Pre: " << preg <<"\n";
}

/// \brief print num of registers in SwplSSNumRegisters
void SwplSSNumRegisters::print(raw_ostream &stream) const {
  stream << "(VReg  Fp: " << freg << ", Int: " << ireg << ", Pre: " << preg <<")";
}

}
