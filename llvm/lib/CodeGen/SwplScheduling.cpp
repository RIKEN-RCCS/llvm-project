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
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/CodeGen/SwplTargetMachine.h"
#include <cmath>
#include <queue>

using namespace llvm;
using namespace ore; // for NV
#define DEBUG_TYPE "aarch64-swpipeliner"

static cl::opt<bool> OptionDumpMrt("swpl-debug-dump-mrt",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionDumpLsMrt("ls-debug-dump-mrt",cl::init(false), cl::ReallyHidden);
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
static cl::opt<bool> OptionDumpLsEveryInst("ls-debug-dump-scheduling-every-inst",cl::init(false), cl::ReallyHidden);

static cl::opt<bool> OptionEnableStageScheduling("swpl-enable-stagescheduling",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionDumpSSProgress("swpl-debug-dump-ss-progress",cl::init(false), cl::ReallyHidden);
static cl::opt<bool> OptionDumpCyclicRoots("swpl-debug-dump-ss-cyclicroots",cl::init(false), cl::ReallyHidden);

static llvm::cl::opt<bool> OptionDumpReg("swpl-debug-dump-estimate-reg",llvm::cl::init(false), llvm::cl::ReallyHidden);
static llvm::cl::opt<bool> OptionDumpLsReg("ls-debug-dump-estimate-reg",llvm::cl::init(false), llvm::cl::ReallyHidden);


void SwplMrt::reserveResourcesForInst(unsigned cycle,
                                      const SwplInst& inst,
                                      const StmPipeline & pipeline ) {
  // cycle:1, stage:{ 1, 5, 9 }, resources:{ ID1, ID2, ID3 }
  //   ->     resource ID1     ID2      ID3    ID4
  //      cycle   :  1 reserve
  //                 2
  //                 3
  //                 4
  //                 5         reserve
  //                 6
  //                 7
  //                 8
  //                 9                  reserve

  // cycle:1, stage:{ 1, 2, 3 }, resources:{ ID1, ID1, ID1 }
  //   ->     resource ID1     ID2     ID3    ID4
  //      cycle   :  1 reserve
  //                 2         reserve
  //                 3                 reserve

  // cycle:1, stage:{ 1, 1, 4 }, resources:{ ID1, ID2, ID3 }
  //   ->     resource ID1     ID2     ID3    ID4
  //      cycle   :  1 reserve reserve
  //                 2
  //                 3
  //                 4                 reserve

  // cycle:1, stage:{ 1, 1, 4, 5 }, resources:{ ID1, ID2, ID3, ID3 }
  //   ->     resource ID1     ID2     ID3    ID4
  //      cycle   :  1 reserve reserve
  //                 2
  //                 3
  //                 4                 reserve
  //                 5                 reserve

  assert( (pipeline.resources.size()==pipeline.stages.size()) && "Unexpected resource information.");

  // Instructions that do not use resources,
  // such as pseudo-instructions, are not reserved using Mrt.
  if( pipeline.resources.size() == 0 ) {
    return;
  }

  unsigned start_cycle = cycle;
  unsigned modulo_cycle;
  for(unsigned i=0; i<pipeline.resources.size(); i++) {

    if (iteration_interval == 0) {
      // LS Processing
      modulo_cycle = (start_cycle+pipeline.stages[i]);
    } else {
      // SWPL Processing
      modulo_cycle = (start_cycle+pipeline.stages[i]) % iteration_interval;
    }

    // check already been reserved.
    if( table[modulo_cycle]->count(pipeline.resources[i]) != 0 &&
        table[modulo_cycle]->at(pipeline.resources[i]) != &inst ) {
        assert( "Resource that has already been reserved.");
    }

    (*(table[modulo_cycle]))[pipeline.resources[i]]=&inst;
  }
  return;
}

SwplInstSet* SwplMrt::findBlockingInsts(unsigned cycle,
                                        const SwplInst& inst,
                                        const StmPipeline & pipeline)
{
  SwplInstSet* blocking_insts = new SwplInstSet();
  assert( (pipeline.resources.size()==pipeline.stages.size()) && "Unexpected resource information.");

  unsigned start_cycle = cycle;
  unsigned modulo_cycle;
  for(unsigned i=0; i<pipeline.resources.size(); i++) {

    if (iteration_interval == 0) {
      modulo_cycle = (start_cycle+pipeline.stages[i]);
    } else {
      modulo_cycle = (start_cycle+pipeline.stages[i]) % iteration_interval;
    }

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

void SwplMrt::dump(const SwplSlots& slots, raw_ostream &stream) {
  unsigned modulo_cycle;
  StmResourceId resource_id;
  unsigned res_max_length = 0;
  unsigned word_width, inst_gen_width, inst_rotation_width;
  unsigned ii;
  unsigned numresource = SWPipeliner::STM->getNumResource();

  if (iteration_interval != 0) {
    ii = iteration_interval;
    assert(ii==table.size());
  } else {
    ii = size;
  }

  // Resource名の最大長の取得
  // resource_idはA64FXRes::PortKindのenum値に対応する。
  // １～STM->getNumResource()をリソースとして扱う 。
  // ０はP_NULLであり、リソースではない。
  for (resource_id = 1; (unsigned)resource_id <= numresource; ++resource_id) {
    res_max_length = std::max( res_max_length, (unsigned)(strlen(SWPipeliner::STM->getResourceName(resource_id))) );
  }

  inst_gen_width = std::max( getMaxOpcodeNameLength()+1, res_max_length);  // 1 = 'p|f' の分
  inst_rotation_width = ( iteration_interval!=0 && slots.size()>0 ) ? 4 : 0;  // 4 = '(n) 'の分
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
    mrt->table.push_back( (new llvm::DenseMap<StmResourceId, const SwplInst*>()) );
  }
  return mrt;
}

SwplMrt* SwplMrt::constructForLs (unsigned size_input) {
  SwplMrt*  mrt = new SwplMrt(0);
  mrt->setSize(size_input);
  for(unsigned i=0; i<mrt->getSize(); i++) {
    mrt->table.push_back( (new llvm::DenseMap<StmResourceId, const SwplInst*>()) );
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

  auto MapFindEdge = modulo_delay_map->find(edge);
  assert(MapFindEdge != modulo_delay_map->end());
  delay = MapFindEdge->second;

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

  if (SWPipeliner::isImportDDG()) {
    // Dependency information changed in yaml is used to generate moduloDelayMap in order to
    // only affect SWPL scheduling.
    SwplDdg copied_ddg;
    copied_ddg = c_ddg; //copy
    copied_ddg.importYaml(); //change DistanceMap and DelaysMap by importYaml().
    modulo_ddg->modulo_delay_map = copied_ddg.getModuloDelayMap(iterator_interval);
  }
  else {
    modulo_ddg->modulo_delay_map = c_ddg.getModuloDelayMap(iterator_interval);
  }
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
  auto Find_Inst = priorities->find(inst);
  assert( Find_Inst != priorities->end() );
  return Find_Inst->second;
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


/// \brief スケジューリング結果が要する回転数が、実際の回転数（可変）を満たすかをチェックする
/// \note assumeに対応しないため、常にtrueを返却する
bool SwplCalclIterations::checkIterationCountVariable(const SwplPlanSpec & spec, const SwplMsResult & ms) {
  return true;
}

/// \brief スケジューリング結果が要する回転数が、実際の回転数（固定）を満たすかをチェックする
/// \details スケジューリング結果が要する回転数が、実際の回転数（固定）を満たすかをチェックする
///          実際の命令列上の回転数を優先する.
/// \retval true スケジューリング結果が要する回転数が、実際の回転数（固定）を満たす
/// \retval false スケジューリング結果が要する回転数が、実際の回転数（固定）を満たさない
bool SwplCalclIterations::checkIterationCountConstant(const SwplPlanSpec & spec, const SwplMsResult & ms) {
  return (ms.required_itr_count <= spec.itr_count);
}

/// \brief schedulingする前に、SWPLの最低条件を確認する
/// \param [in] spec スケジューリング指示情報
/// \param [in,out] required_itr 必要なループ回転数
/// \retval true 条件を満して、SWPL可能
/// \retval false 条件を満しておらず、SWPL不可能
/// \remark required_itrに、ソース上必要なループ回転数を設定する.
///         元ループの回転数が2回転以下の時, SWPL不可.
/// \note 2回転の時はschedulingしたいが,
///       現在の実装ではn_copies >=3 を要求する為,できていない.
///       この値(2回転) はis_iteration_count_constantでも利用している為そろえる事.
/// \note 現在、SwplPlanSpec.assumed_iterationは常にASSUMED_ITERATIONS_NONE (-1)
/// \note 現在、SwplPlanSpec.pre_expand_numは常に1
bool SwplCalclIterations::preCheckIterationCount(const SwplPlanSpec & spec, unsigned int *required_itr) {
  *required_itr = 0;

  unsigned int const minimum_n_copies = 3;
  *required_itr = spec.pre_expand_num * minimum_n_copies;

  /* 定数の場合、Pragma 指定よりも、実際のMIR上の数を優先する */
  if(spec.is_itr_count_constant) {
    if (spec.itr_count <= 2) {
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : SWPL is canceled for this loop whose iteration count < 3. \n";
      }
      return false;
    }
    return true; /* MIR上に定数がある場合は、見積値を参照しない. */
  } else {
    /* 見積り値 */
    int assumed_iterations = spec.assumed_iterations;
    if (assumed_iterations >= 0) {
      assert(assumed_iterations <= SwplPlanSpec::ASSUMED_ITERATIONS_MAX);
    }

    if( assumed_iterations >= 0 && *required_itr > (unsigned)assumed_iterations ) {
      if (SWPipeliner::isDebugOutput()) {
        dbgs() << "        : SWPL is canceled for this loop whose assumed iteration count < 3. \n";
      }
      return false;
    }
  }
  return true;
}


/// \brief scheduling結果に対して指定したレジスタがいくつ必要であるかを数える処理
/// \note 必要なレジスタ数を正確に計算する事は、RAでなければできないため、
///       SWPL独自に必要となるレジスタを概算する
unsigned SwplRegEstimate::calcNumRegs(const SwplLoop& loop,
                                      const SwplSlots* slots,
                                      unsigned iteration_interval,
                                      unsigned regclassid,
                                      unsigned n_renaming_versions) {
  RenamedRegVector* vector_renamed_regs;
  unsigned n_immortal_regs, n_mortal_regs, n_interfered_regs,
    n_extended_regs, n_patten_regs;
  unsigned max_regs = 0, margin = 0;

  assert(slots != nullptr);
  assert(n_renaming_versions > 0);

  /* 見積もり対象のレジスタを収集 */
  vector_renamed_regs = new RenamedRegVector();
  collectRenamedRegs(loop, slots, iteration_interval,
                     regclassid, n_renaming_versions, vector_renamed_regs);

  /* predecessor, successor, phi をたどってつながるものは全て
   * 同一の hard reg に割り当てられる。
   * したがって、頭だけ数えなければならない。*/

  n_immortal_regs = getNumImmortalRegs (loop, regclassid);
  n_interfered_regs = getNumInterferedRegs(loop, slots,
                                           iteration_interval, regclassid,
                                           n_renaming_versions);
  n_mortal_regs = getNumMortalRegs (loop, slots,
                                    iteration_interval, regclassid);
  n_patten_regs = getNumPatternRegs(vector_renamed_regs,
                                    iteration_interval,
                                    n_renaming_versions, regclassid);
  n_extended_regs = getNumExtendedRegs (vector_renamed_regs,
                                        slots,
                                        iteration_interval, regclassid,
                                        n_renaming_versions);					      

  max_regs = std::max(max_regs, n_immortal_regs + n_mortal_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_interfered_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_extended_regs);
  max_regs = std::max(max_regs, n_immortal_regs + n_patten_regs);
  const char* regname="other";

  if (llvm::StmRegKind::isInteger(regclassid) && n_renaming_versions >= 3 ) {
    regname = "I";
    margin = 1;
  } else if( llvm::StmRegKind::isFloating(regclassid) && n_renaming_versions >= 3 ) {
    regname = "F";
    margin = 1;
  } else if( llvm::StmRegKind::isPredicate(regclassid) ) { /* versionの制限は無し */
    regname = "P";
    margin = 1;
  }

  max_regs += margin;

  if (OptionDumpReg) {
    dbgs() << "Estimated number of "
           << regname << " registers (classid:" << regclassid << ") :";
    dbgs() << " immortal " << n_immortal_regs
           << ", mortal  " << n_mortal_regs
           << ", interfered "<< n_interfered_regs
           << ", extended "<< n_extended_regs
           << ", pattern "<< n_patten_regs
           << ", margin "<< margin <<",\n"
           << " total    : "<< max_regs << "\n";
  }

  /* vector_renamed_regs, renamed_regを解放する*/
  for( auto *rr : *vector_renamed_regs ) {
    delete rr;
  }
  delete vector_renamed_regs;

  return max_regs;
}

/// \brief RenamedRegの初期化
void SwplRegEstimate::initRenamedReg(SwplRegEstimate::Renamed_Reg* reg) {
  reg->reg = nullptr;
  reg->def_cycle = 0;
  reg->use_cycle = 0;
  reg->is_recurrence = false;
  return;
}

/// \brief RenamedRegへの設定
void SwplRegEstimate::setRenamedReg(SwplRegEstimate::Renamed_Reg* reg,
                                    const SwplReg& sreg,
                                    unsigned def_cycle,
                                    unsigned use_cycle,
                                    unsigned iteration_interval,
                                    bool is_recurrence) {
  assert(reg != nullptr);
  initRenamedReg(reg);
  reg->reg = &(const_cast<SwplReg&>(sreg));
  reg->def_cycle = (int)def_cycle;
  reg->use_cycle = (int)use_cycle;
  reg->is_recurrence = is_recurrence;
  return;
}

/// \brief 干渉を考慮してrenaming version後の変数が要求するレジスタを数える処理
/// \note レジスタ数見積りの下限を求めるために用いる
///
/// ```
///   - 以下では、T = MVE*IIと定義する.
///   また、RAが割り付けるレジスタの意味で、"レジスタ"
///   SwplRegの意味で、"vreg"という言葉を用いる.
///   また、RAが割付ける場合、renaming versiong後ろのそれぞれのvregは,
///   単一のレジスタに割付けられ、live rangeの途中で、二つのレジスタ間で乗り換える
///   様なことはない事を前提としている.
///     
///   - 一次補正
///   ライブレンンジL1がT/2を越えるようなvregが要求するレジスタ数の最小値を求める.
///   L1はT/2を超えるため、renaming versionされた自身のvregは
///   すべてのversionと互いに干渉するため, レジスタをMVE個だけ要求する.
///   また、他のprライブレンジL1がT/2を越えるvregも,
///   T/2以上のvreg同士は常に重なるため, レジスタを共有する事はないため、
///   要求するレジスタを単純に加算すればよい.
///   さらに、vregの空き区間(T-L1)に、同じレジスタを使用できる他のvregは、
///   T/2以上のライブレンジを持たないため、無視している.
///
///   - 二次補正
///   vregの空き区間(T-L1)区間に、埋めるられないライブレンジL2をもつ
///   vregをカウントする。以下の点が、L1のvregと異なる.
///   - L2のライブレンジをもつvregは、自身のすべてのversionとかさなるとは限らない。
///   => 要求するレジスタがMVE個でない場合があり、L2とversion数の関係で定まる.
///   - L2のライブレンジをもつ異なるレジスタ同士が、重なるとは限らない.
///   => 異なるvregの要求レジスタ数同士を単純に加算してはならない.
///
///   一次補正と二次補正の和と、getNumImmortalRegの結果の和が、
///   必要なレジスタ数の下限を与える.
/// ```
///
unsigned SwplRegEstimate::getNumInterferedRegs(const SwplLoop& loop,
                                               const SwplSlots* slots,
                                               unsigned iteration_interval,
                                               unsigned regclassid,
                                               unsigned n_renaming_versions) {
  unsigned counter = 0;
  unsigned maximum_live_range;
  int full_kernel_cycles, live_range;
  int maximum_gap;
  bool is_recurrence;

  assert(slots != nullptr);
  assert(n_renaming_versions > 0);

  full_kernel_cycles = n_renaming_versions * iteration_interval;
  maximum_gap = -1;
  maximum_live_range = 0;

  /* 一次補正のため,ライブレンジがT/2を越えるvregの重なりを数える
   * 同時に、二次補正に必要な情報を収集する
   */
  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();
    is_recurrence = inst->isRecurrence();

    for( auto *reg : inst->getDefRegs() ) {
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( !(reg->getUseInsts().empty()) ) {
        last_use_cycle
          = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }

      assert(last_use_cycle >= def_cycle);
      live_range = (int)(last_use_cycle-def_cycle+1);

      if(is_recurrence == true && n_renaming_versions == 2) {
        /* rucurrenceのlive rangeは、1cycle重ねて冗長に表現している.
         * n_renaming_versions == 2の場合に、閾値T/2を超えるかどうかに影響をあたえる.
         * ひとつのSwplRegで隙間なく一つのレジスタをを占有できる.
         * 但し、最適化が動くとその限りではない。
         * countとしては、+1するように特別扱いする.
         */
        counter += reg->getRegSize();
      } else if(live_range*2 > full_kernel_cycles) {
        /* T/2をこえるvregのうち、非生存区間の最大長 */
        if(maximum_gap < full_kernel_cycles - live_range) {
          maximum_gap = full_kernel_cycles - live_range;
          assert(maximum_gap >= 0);
        }
        /* 一次補正 */
        counter += n_renaming_versions * reg->getRegSize();
      } else {
        /* T/2をこえないregisterのうち、最大のライブレンジ */
        if(maximum_live_range < (last_use_cycle - def_cycle + 1)) {
          maximum_live_range = (last_use_cycle - def_cycle + 1);
        }
      }
    }
  }

  /**
   * T/2を越えないライブレンンジL2をもつレジスタのうち、
   * T/2を越えるレジスタの隙間(T-L1)にいれられないレジスタをカウントする.
   * 具体的には,versioningされたレジスタを表現するために必要なレジスタ数(total_color)を計算する
   * 各々のversion(0,1,2,...,n_renaming_version-1)に、
   * 0,1,2,...,total_color-1を順番に塗っていき,
   * 最終versionと,それとlive rangeが重なるversionのレジスタの色が異なればよい.
   *
   * \note 本実装では、以下に対応できていない
   *  - modulo cycleまでは考慮できていない
   *  - この条件に該当するレジスタ同士の干渉は考えていない
   *  - tuple型を考慮できていない
   */
  if(maximum_gap != -1 && maximum_gap < (signed)maximum_live_range) {
    int total_color, last_color;

    /* liverangeがのびているstage数だけ少なくとも、レジスタが必要 */
    int stage = (maximum_live_range + iteration_interval -1)/iteration_interval;

    for(total_color = stage; total_color <= (signed)n_renaming_versions; ++total_color) {
      /* 最終versionのcolor */
      last_color = (n_renaming_versions-1)%total_color;
      /* 最終versionのcolorが、0-(stage-1)のcolorなら重なる */
      if(last_color < (stage - 1)%total_color ) {
        continue;
      }
    }
    counter += total_color;
  }
  return counter;
}

/// \brief Processing to count unrenamed registers
/// \note unrenamed registers is livein register or defined by φ, and following successor leads to itself
unsigned SwplRegEstimate::getNumImmortalRegs(const SwplLoop& loop, unsigned regclassid ) {
  unsigned counter;
  llvm::SmallSet<int, 20> regset;

  counter = 0;
  for( auto *inst : loop.getBodyInsts() ) {
    for( auto *reg : inst->getUseRegs() ) {
      if ( reg->isRegNull() ) {
        continue;
      }
      if( !(reg->isSameKind(regclassid)) ) {
        continue;
      }
      if ( reg->isStack() ) {
        continue;
      }
      auto *def = reg->getDefInst();
      if (def!=nullptr && !def->isInLoop()) {
        if (regset.contains(reg->getReg())) continue;
        regset.insert(reg->getReg());
        counter += reg->getRegSize();
      }
    }
  }

  for( auto *inst : loop.getPhiInsts() ) {
    SwplReg *reg, *successor;
    reg = &(inst->getDefRegs(0));
    assert ( !(reg->isRegNull()) );
    if ( !reg->isSameKind(regclassid) ) {
      continue;
    }
  
    for (successor = reg->getSuccessor();
         successor != nullptr;
         successor = successor->getSuccessor()) {
      if (successor == reg) {
        if (regset.contains(reg->getReg())) continue;
        regset.insert(reg->getReg());
        counter += reg->getRegSize();
        break;
      }
    }
  }
  return counter;
}

/// \brief renamingされるレジスタを数える
/// \note rename される reg （body で定義されていて、predecessor が無いもの）。
///       それぞれ live_range を計算し、重複分もそのまま予約する。
unsigned SwplRegEstimate::getNumMortalRegs(const SwplLoop& loop, 
                                           const SwplSlots* slots,
                                           unsigned iteration_interval,
                                           unsigned regclassid) {
  unsigned max_counter;
  std::vector<int> reg_counters(iteration_interval, 0);

  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();

    for( auto *reg : inst->getDefRegs() ) {
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( reg->isUsed() ) {
        last_use_cycle = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }
      incrementCounters (reg->getRegSize(),
                         &reg_counters, iteration_interval,
                         def_cycle, last_use_cycle);
    }
  }

  /* branch 用の icc をカウントする。
   * icc が iteration_interval のブロックを跨って使用されているかを、
   * チェックする場所が他にない。*/
  if( llvm::StmRegKind::isCC( regclassid ) ) {
    incrementCounters (1, &reg_counters, iteration_interval,
                        0, 0);
  }

  max_counter = findMaxCounter(&reg_counters, iteration_interval);
  return max_counter;
}

/// \brief regclassid毎にレジスタ見積りの数え上げを行う
/// \note 従来の見積もりと同様であり、SwplSlotをcycleに換算しているため、
///       過剰な重なりを生成する場合がある.結果、spillが出ない安全サイドに倒している.
///       Slot単位でも見積もりが可能であり、より正確になるはずあるが、
///       暫定的にcycle単位で処理をおこなう.
void SwplRegEstimate::collectRenamedRegs(const SwplLoop& loop, 
                                         const SwplSlots* slots,
                                         unsigned iteration_interval,
                                         unsigned regclassid,
                                         unsigned n_renaming_versions,
                                         RenamedRegVector* vector_renamed_regs) {
#ifndef NDEBUG
  // assertでのみ使用
  unsigned full_kernel_cycles;
  full_kernel_cycles = n_renaming_versions * iteration_interval;
#endif

  for( auto* inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_cycle;
    bool is_recurrence;
  
    assert( inst->isPhi()==false);
  
    def_slot = slots->at(inst->inst_ix);
    def_cycle = def_slot.calcCycle();

    is_recurrence = inst->isRecurrence();

    for( auto *reg : inst->getDefRegs() ) {
      SwplRegEstimate::Renamed_Reg *renamed_reg;
      unsigned last_use_cycle;
   
      if (isCountedReg(*reg, regclassid) == false) {
        continue;
      }
    
      if ( !(reg->getUseInsts().empty()) ) {
        last_use_cycle
          = slots->calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      }  else {
        last_use_cycle = def_cycle;
      }
      assert(last_use_cycle >= def_cycle);
      assert(last_use_cycle - def_cycle + 1 <= full_kernel_cycles);

      renamed_reg = new SwplRegEstimate::Renamed_Reg();
      setRenamedReg(renamed_reg, *reg, def_cycle,
                    last_use_cycle, iteration_interval,
                    is_recurrence);

      vector_renamed_regs->push_back(renamed_reg);
    }
  }
  return;
}

/// \brief reg2をiteration_intervalずつずらして、cycleの位置を正規化する
void SwplRegEstimate::normalizeDefUseCycle(SwplRegEstimate::Renamed_Reg *reg1,
                                           SwplRegEstimate::Renamed_Reg *reg2,
                                           int *def_cycle_reg1, int *use_cycle_reg1,
                                           int *def_cycle_reg2, int *use_cycle_reg2,
                                           unsigned iteration_interval) {
  int base_cycle;

  base_cycle = reg1->def_cycle - 1;
			    
  *def_cycle_reg1 = 1; /* reg1->def_cycle - base_cycle */
  *use_cycle_reg1 = reg1->use_cycle - base_cycle;
  *def_cycle_reg2 = reg2->def_cycle - base_cycle;
  *use_cycle_reg2 = reg2->use_cycle - base_cycle;

  /* def_cycle_reg2をdef_cycle_reg1と同じversionにする正規化処理 */
  if(*def_cycle_reg2 < *def_cycle_reg1) {
    while(*def_cycle_reg2 < *def_cycle_reg1) {
      *def_cycle_reg2 += iteration_interval;
      *use_cycle_reg2 += iteration_interval;
    }
  } else if (*def_cycle_reg2 >= *def_cycle_reg1 + (signed)iteration_interval) {
    while(*def_cycle_reg2 >= *def_cycle_reg1 + (signed)iteration_interval) {
      *def_cycle_reg2 -= iteration_interval;
      *use_cycle_reg2 -= iteration_interval;
    }
  }
  assert(*def_cycle_reg1 <= *def_cycle_reg2);
  assert(*def_cycle_reg2 <= (signed)iteration_interval);
  return;
}
				      
/// \brief reg1のuse後方の空領域に、reg2を配置できるかどうかを判定し、
///        可能ならreg1->use_cycleからの最小のoffset値を返す. 不可能なら0を返却する.
/// \retval reg2を配置可能なreg1->use_cycleからの最小のoffset値
unsigned SwplRegEstimate::getOffsetAllocationForward(SwplRegEstimate::Renamed_Reg *reg1,
                                                     SwplRegEstimate::Renamed_Reg *reg2,
                                                     unsigned iteration_interval,
                                                     unsigned n_renaming_versions) {
  int version;
  int dead_cycle_reg1, live_cycle_reg2;
  int full_kernel_cycles;
  int offset_value = 0;
  int def_cycle_reg1, use_cycle_reg1, def_cycle_reg2, use_cycle_reg2;

  full_kernel_cycles = n_renaming_versions * iteration_interval;

  /* def_cycle_reg1 <= def_cycle_reg2をみたすように正規化 */
  normalizeDefUseCycle(reg1, reg2,
                       &def_cycle_reg1, &use_cycle_reg1,
                       &def_cycle_reg2, &use_cycle_reg2,
                       iteration_interval);

  assert(use_cycle_reg1 <= full_kernel_cycles &&
         use_cycle_reg2 <= full_kernel_cycles + (signed)iteration_interval);

  dead_cycle_reg1 = full_kernel_cycles - (use_cycle_reg1 - def_cycle_reg1 + 1);
  live_cycle_reg2 = use_cycle_reg2 - def_cycle_reg2 + 1;

  if(dead_cycle_reg1 < live_cycle_reg2) {
    /* どのようにずらしても隙間に入らないため */
    return offset_value;
  }

  /* reg2のcycleをiiずつversion回だけずらして、重なりを判定 */
  for(version=0; version < (signed)n_renaming_versions; ++version) {
    int renamed_def_cycle_reg2, renamed_use_cycle_reg2;
    renamed_def_cycle_reg2 = def_cycle_reg2 + version * iteration_interval;
    renamed_use_cycle_reg2 = use_cycle_reg2 + version * iteration_interval;

    /* reg1がdeadである領域に,reg2がおさまっていれば良い */
    if (use_cycle_reg1 < renamed_def_cycle_reg2 &&
        renamed_use_cycle_reg2 <= full_kernel_cycles) {
      offset_value = renamed_def_cycle_reg2 - use_cycle_reg1;
      assert(offset_value >= 1);
      assert(offset_value <= (signed)iteration_interval);
      /* 条件に最初に適合するoffsetが最小のoffsetのためreturn */
      return offset_value;
    }
  }
  return offset_value;
}

/// \brief reg1の空領域に、reg2を配置できるかどうかを
///        modulo cycleとrenamingを考慮して判定し、
///        可能ならreg1->use_cycleからの最小のoffset値を返す
///        不可能なら0を返却する.
/// \retval reg2を配置可能なreg1->use_cycleからの最小のoffset値
unsigned SwplRegEstimate::getOffsetAllocationBackward(SwplRegEstimate::Renamed_Reg *reg1,
                                                      SwplRegEstimate::Renamed_Reg *reg2,
                                                      unsigned iteration_interval,
                                                      unsigned n_renaming_versions) {
  unsigned version;
  int dead_cycle_reg1, live_cycle_reg2;
  int full_kernel_cycles;
  int def_cycle_reg1, use_cycle_reg1, def_cycle_reg2, use_cycle_reg2;
  int offset_value = 0;

  full_kernel_cycles = n_renaming_versions * iteration_interval;  

  /* def_cycle_reg2 <= def_cycle_reg1をみたすように正規化 */
  normalizeDefUseCycle(reg2, reg1,
                       &def_cycle_reg2, &use_cycle_reg2,
                       &def_cycle_reg1, &use_cycle_reg1,
                       iteration_interval);

  dead_cycle_reg1 = full_kernel_cycles - (use_cycle_reg1 - def_cycle_reg1 + 1);
  live_cycle_reg2 = use_cycle_reg2 - def_cycle_reg2 + 1;

  if(dead_cycle_reg1 < live_cycle_reg2) {
    /* どのようにずらしても隙間に入らないため */
    return offset_value;
  }

  /* reg2のcycleをiiずつversion回だけ前にずらして、重なりを判定 */
  for(version=0; version < n_renaming_versions; ++version) {
    int renamed_def_cycle_reg2, renamed_use_cycle_reg2;
    renamed_def_cycle_reg2 = def_cycle_reg2 - version * iteration_interval;
    renamed_use_cycle_reg2 = use_cycle_reg2 - version * iteration_interval;

    /* reg1がdeadである領域に,reg2がおさまっていれば良い */
    if (use_cycle_reg1 - full_kernel_cycles + 1 <= renamed_def_cycle_reg2 &&
        renamed_use_cycle_reg2  < def_cycle_reg1) {
      offset_value = def_cycle_reg1 - renamed_use_cycle_reg2;
      assert(offset_value >= 1);
      assert(offset_value <= (signed)iteration_interval);
      /* 条件に最初に適合するoffsetが最小のoffsetのためreturn */
      return offset_value;
    }
  }
  return offset_value;
}

/// \brief renamed_regのlive rangeを前方に延す処理
/// \note 詳細はextendRenamedRegLivesForwardを参照
void SwplRegEstimate::extendRenamedRegLivesBackward(RenamedRegVector* vector_renamed_regs,
                                                    unsigned iteration_interval,
                                                    unsigned n_renaming_versions) {
  int const init_use_offset = iteration_interval + 1;
  int full_kernel_cycles = n_renaming_versions * iteration_interval;

  for( auto *reg1 : *vector_renamed_regs ) {
    int min_use_offset;
    min_use_offset = init_use_offset;

    if (reg1->is_recurrence && n_renaming_versions == 2) { continue; }
  
    /* reg2を配置するすきまがないため */
    if (reg1->use_cycle - reg1->def_cycle + 1 == full_kernel_cycles) { continue; }
    
    for( auto *reg2 : *vector_renamed_regs ) {
      /* 自身との重なりをみるため、indx,indx2は同じ場合もチェックする */

      /* 空き領域に配置可能か */
      int use_offset = getOffsetAllocationBackward(reg1, reg2, iteration_interval,
                                                   n_renaming_versions);
      if(use_offset != 0) {
        /* reg1->use_cycleより大きくかつ, 最もreg1->use_cycleに近い値を取得 */
        if(min_use_offset > use_offset) {
          min_use_offset = use_offset;
          if(min_use_offset == 1) {
            /* 最小のoffsetであるため */
            break;
          }
        }
      }
    }
  
    if(min_use_offset == 1) {
      /* use直後で、別のrenamed_regで使うことが可能なためuseは延ばせない. */
      continue;
    } else{
      int old_live_range, new_live_range;
      old_live_range = reg1->use_cycle - reg1->def_cycle + 1;
    
      if(min_use_offset == init_use_offset) {
        /* forwardの場合と同様に延す */
        if( old_live_range * n_renaming_versions > full_kernel_cycles *(n_renaming_versions/2) ) {
          new_live_range = full_kernel_cycles;
        } else {
          new_live_range = ((old_live_range + iteration_interval - 1)/iteration_interval)*iteration_interval;
        }
      } else {
        new_live_range = old_live_range + min_use_offset - 1;
      }
      reg1->def_cycle = reg1->use_cycle - (new_live_range - 1);
    
    }
  }
  return;
}

/// \brief live rangeの空き領域において, 他のrenamed_regを
///        配置できないcycleをみつけ、live rangeを延ばす
///
/// ```
///   例として、ii=3,n_renaming_versions=3の場合を考える.
///   reg1は、6-9cycleが空いており、reg2(version=0),
///   reg3(version=2)を埋めることが可能である.
///   したがって、RAでreg1とreg2またはreg1とreg2は同じレジスタを
///   割り付けることが可能である.
///   ここで、reg1の6cycle目は、reg2,reg3のいずれにおいても
///   使用されることがないため、reg1のuse_cycleを後ろへ延ばす.
///   reg2の9cycle目も同様に、use_cycleを延す.
///   互いにすべての配置を比較して影響を与えない範囲に限定して、
///   use_cycleを延すため、regの順序は問わない.
///
///   1<------->9
///   reg1 *** **- ---  def:1, use:5
///   reg2 --- --- **-  def:7, use:8
///   reg3 -** --- ---  def:2, use:3
///   122 110 110 => 342 
///
///   1<------->9
///   reg1 *** *** ---  def:1, use:6
///   reg2 --- --- ***  def:7, use:8
///   reg3 -** --- ---  def:2, use:3
///   122 111 111 => 344
///
///   さらに、def側にlive rangeを延すことを考えると
///   reg3の1cycle目を延すことが可能である。
///   後ろ側へは、extendRenamedRegLivesBackwardで実施する.
///
///   1<------->9
///   reg1 *** *** ---  def:1, use:6
///   reg2 --- --- ***  def:7, use:8
///   reg3 *** --- ---  def:2, use:3
///   222 111 111 => 444
/// ```
/// 
void SwplRegEstimate::extendRenamedRegLivesForward(RenamedRegVector* vector_renamed_regs,
                                                   unsigned iteration_interval,
                                                   unsigned n_renaming_versions) {
  int const init_def_offset = iteration_interval + 1;
  int full_kernel_cycles = n_renaming_versions * iteration_interval;

  for( auto *reg1 : *vector_renamed_regs ) {
    int min_def_offset;
    min_def_offset = init_def_offset;

    if (reg1->is_recurrence && n_renaming_versions == 2) { continue; }
  
    /* reg2を配置するすきまがないため */
    if (reg1->use_cycle - reg1->def_cycle + 1 == full_kernel_cycles) { continue; }
    
    for( auto *reg2 : *vector_renamed_regs ) {
      /* 自身との重なりをみるため、indx,indx2は同じ場合もチェックする */

      /* 空き領域に配置可能か */
      int def_offset = getOffsetAllocationForward(reg1, reg2, iteration_interval,
                                                  n_renaming_versions);
      if(def_offset != 0) {
        /* reg1->use_cycleより大きくかつ, 最もreg1->use_cycleに近い値を取得 */
        if(min_def_offset > def_offset) {
          min_def_offset = def_offset;
          if(min_def_offset == 1) {
            /* 最小のoffsetであるため */
            break;
          }
        }
      }
    }
  
    if(min_def_offset == 1) {
      /* use直後で、別のrenamed_regで使うことが可能なためuseは延ばせない. */
      continue;
    } else{
      int old_live_range, new_live_range;
      old_live_range = reg1->use_cycle - reg1->def_cycle + 1;
    
      if(min_def_offset == init_def_offset) {
        /* 自身も含めて他のrenamed_regが隙間に配置不可能であるため、live rangeを延す.
         * iteration_intervalを超えてlive rangeを延すと、reg1 == reg2の場合に、
         * 新しいversionとの重なりが生成されてしまう.
         */
        if( old_live_range * n_renaming_versions > full_kernel_cycles *(n_renaming_versions/2) ) {
          /* reg1==reg2の場合に,他のversionと既に重なっている場合,限界まで延す */
          new_live_range = full_kernel_cycles;
        } else {
          /* 自身との過剰な重なりを作らないために、iteration_intervalを超えない範囲まで延す */
          new_live_range = ((old_live_range + iteration_interval - 1)/iteration_interval)*iteration_interval;
        }
      } else {
        new_live_range = old_live_range + min_def_offset - 1;
      }
      reg1->use_cycle = reg1->def_cycle + new_live_range - 1;
    
    }
  }
  return;
}

/// \brief reg_counterのでliveなcycleをincrementする
/// \note  begin_cycle, end_cycle は両方カウントされる。
///        cycle のどこで使われるか判らないから。
void SwplRegEstimate::incrementCounters(int reg_size, std::vector<int>* reg_counters,
                                        unsigned iteration_interval,
                                        unsigned begin_cycle, unsigned end_cycle) {
  unsigned cycle;
  for (cycle = begin_cycle; cycle <= end_cycle; ++cycle) {
    int counter;
    unsigned modulo_cycle;

    modulo_cycle = cycle % iteration_interval;
    counter = (*reg_counters)[modulo_cycle];
    (*reg_counters)[modulo_cycle] = (counter + reg_size);
  }
  return;
}

/// \brief 各cycle毎のbusy数の最大値を見積もり結果として取得する
unsigned SwplRegEstimate::getEstimateResult(RenamedRegVector* vector_renamed_regs,
                                            std::vector<int>* reg_counters,
                                            unsigned iteration_interval) {
  unsigned indx;
  unsigned max_counter, counter;

  for( auto *rreg : *vector_renamed_regs ) {
    incrementCounters (rreg->reg->getRegSize(),
                        reg_counters,
                        iteration_interval,
                        rreg->def_cycle, rreg->use_cycle);
  }

  max_counter = 0;
  for (indx=0; indx < iteration_interval; ++indx) {
    counter = (*reg_counters)[indx];
    if(max_counter < counter) { max_counter = counter; }
  }
  return max_counter;

}

/// \brief live_range_mask以上のlive rangeをもつレジスタの数を求める
unsigned SwplRegEstimate::countLivesWithMask(RenamedRegVector* vector_renamed_regs,
                                             unsigned live_range_mask) {
  unsigned count_lives = 0;

  /* 最大、最小のcycleを収集 */
  for( auto *rreg : *vector_renamed_regs ) {
    unsigned live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
    if(live_range >= live_range_mask) {
      count_lives += rreg->reg->getRegSize();
    }
  }

  return count_lives;
}

/// \brief live_range_mask以上のlive rangeをもつレジスタの最大重なり数を求める
unsigned SwplRegEstimate::countOverlapsWithMask(RenamedRegVector* vector_renamed_regs,
                                                unsigned live_range_mask,
                                                unsigned n_renaming_versions) {
  unsigned max_cycle, min_cycle;
  unsigned pattern_max_counter;

  min_cycle = SwplSlot::slotMax().calcCycle();
  max_cycle = SwplSlot::slotMin().calcCycle();

  /* 最大、最小のcycleを収集 */
  for( auto *rreg : *vector_renamed_regs ) {
    unsigned live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
    if(live_range <= live_range_mask || rreg->is_recurrence == true) { continue; }
    if((signed)max_cycle < rreg->use_cycle) { max_cycle = rreg->use_cycle; }
    if((signed)min_cycle > rreg->def_cycle) { min_cycle = rreg->def_cycle; }
  }

  if (max_cycle <= min_cycle) {
    /* 該当するregが無いため中断*/
    return 0;
  }

  /* 各renamed_regのcycle毎の生存数を積み上げる*/
  std::vector<int> tmp_reg_counters( (max_cycle - min_cycle + 1), 0 ); /* cycle毎の生存数 */
  for( auto *rreg : *vector_renamed_regs ) {
    int live, live_range;
    live_range = rreg->use_cycle - rreg->def_cycle + 1;
  
    if(live_range <= (signed)live_range_mask || rreg->is_recurrence == true) { continue; }
  
    for (live=rreg->def_cycle - min_cycle; live < live_range; ++ live) {
      tmp_reg_counters[live] = (tmp_reg_counters[live] + 1);
    }
  }

  pattern_max_counter = 0;      
  for( auto counter : tmp_reg_counters ) {
    if((signed)pattern_max_counter < counter) { pattern_max_counter = counter; }
  }

  return pattern_max_counter;
}

/// \brief iiに対するlive rangeの重なりから、必要レジスタ数をパターンマッチ
/// \note 必要レジスタ数はheuristicな条件で決めている
/// \note OoOが豊富な場合、SWPLの重なりを緩くしても問題ないため、
///       大きなMVEに対して過剰に見積もる事は許容される
unsigned SwplRegEstimate::getNumPatternRegs (RenamedRegVector* vector_renamed_regs,
                                             unsigned iteration_interval,
                                             unsigned n_renaming_versions, unsigned regclassid) {
  unsigned live_range_mask;
  unsigned live_overlaps;
  unsigned pattern_max_counter = 0;

  /* live_range > ii/2のケース
   * ii/2を越える場合に、SwplRegはレジスタを共有できないため、
   * renamed_regの全versionに1つのレジスタを割り当てることを前提に, レジスタ数が予測できる.
   * ただし、SwplRegが複数のレジスタを割り当てられる場合があり、
   * その場合に対しては、過剰な見積もりになっている.
   * 例えば、2/3*iiのrenamed_regが3つで、互いに1/3*iiずれている場合、
   * 2つのレジスタで割り付ける事ができる.
   * \note 効果があるケースが見つかっていないため不要と思われる
   */
  {
    live_range_mask = (iteration_interval+1)/2;
    pattern_max_counter = countLivesWithMask (vector_renamed_regs, live_range_mask);
  }

  /* live_range > iiのケース
   * 自身の隣接するversionと重なるため、renamed_regに対して複数レジスタが必要になる
   * ただし、別のrenamed_regと共有する事が可能であるため、2で割る
   */
  {
    live_range_mask = iteration_interval;
    live_overlaps = countOverlapsWithMask (vector_renamed_regs, live_range_mask,
                                           n_renaming_versions);

    /* 証明できていないが、傾向はあっている思われる*/
    if(n_renaming_versions%2 == 1) {
      /* 奇数の場合、一つのrenamed_regに対し少なくとも3個が必要であり、
       * 互いに共有できる場合があるため.
       */
      pattern_max_counter = std::max(pattern_max_counter,
                                     3 * ((live_overlaps+1)/2));
    } else {
      /* 奇数の場合、一つのrenamed_regに対し少なくとも2個が必要*/
      pattern_max_counter = std::max(pattern_max_counter,
                                     2 * ((live_overlaps+1)/2));
    }

    /* MVE==5の場合は、spillがおきやすい傾向にあるため多めに見積る */
    if( llvm::StmRegKind::isPredicate(regclassid) ) {
      if (n_renaming_versions == 5) {
        pattern_max_counter = std::max(pattern_max_counter,
                                       3 * live_overlaps);
      }
    }
  }

  /* live_range > (2*ii) のケース
   * MVEが4以下の場合は, pattern以外の見積もりで十分と判断した
   */
  {
    if(n_renaming_versions >= 5) {
      live_range_mask = iteration_interval * 2;
      live_overlaps = countOverlapsWithMask (vector_renamed_regs, live_range_mask,
                                             n_renaming_versions);
      switch (n_renaming_versions) {
      case 5:
        /* 前後2version重なり、全体が重なりあうため、5個レジスタが必要 
         * live_overlapsが少数の場合は、妥当な見積もりと思われる
         */
        pattern_max_counter = std::max(pattern_max_counter,
                                       5 * live_overlaps );
        break;
      default:
        // \note MVE=5以外の場合、明確な数は不明
        break;
      }
    }
  }

  /* tmp_reg_countersは、呼出し元で解放 */
  return pattern_max_counter;
}

/// \brief 干渉と割付けを意識してlive rangeを伸ばしながら見積もりを実施する
/// \note getNumMortalRegsよりかならず大きい結果を返却する
unsigned SwplRegEstimate::getNumExtendedRegs (RenamedRegVector* vector_renamed_regs,
                                              const SwplSlots* slots,
                                              unsigned iteration_interval,
                                              unsigned regclassid,
                                              unsigned n_renaming_versions) {
  std::vector<int> reg_counters( iteration_interval, 0 );
  int max_counter;

  /* 見積もりのためlive rangeの延長 */
  extendRenamedRegLivesForward(vector_renamed_regs,
                               iteration_interval, n_renaming_versions);
  extendRenamedRegLivesBackward(vector_renamed_regs,
                                iteration_interval, n_renaming_versions);
  max_counter = getEstimateResult(vector_renamed_regs,
                                  &reg_counters, iteration_interval);
  return max_counter;
}

/// \brief SwplRegが見積り時の対象となるかどうか
/// \note getNumMortalRegs、getNumInterferedRegsで用いる
bool SwplRegEstimate::isCountedReg(const SwplReg& reg, unsigned regclassid) {
  if ( reg.isRegNull() ) {
    return false;
  }
  if( !reg.isSameKind(regclassid) ) {
    return false;
  }
  if (reg.getPredecessor() != nullptr) {
    /* 既に割り当てられた reg を使うだけなので、計算しない。*/
    return false;
  }
  if ( reg.isStack() ) {
    return false;
  }
  return true;
}

/// \brief int_Vector内の最大の値を取得する
int SwplRegEstimate::findMaxCounter(std::vector<int>* reg_counters, unsigned iteration_interval) {
  int max_counter;
  unsigned modulo_cycle;

  /* Find max */
  max_counter = 0;
  for (modulo_cycle = 0; modulo_cycle < iteration_interval; ++modulo_cycle) {
    int counter;
    counter = (*reg_counters)[modulo_cycle];
    max_counter = std::max(max_counter, counter);
  }
  return max_counter;
}

#define ceil_div(x, y) (((x) - 1) / (y) + 1)

bool SwplPlan::existsPragma = false;

/// \brief instが配置されたslot番号（begin_slot起点）を返す
/// \details SwplPlan.slotsに配置されたinstを探し、
///          begin_slot起点のslot番号を返す
/// \param [in] c_inst
/// \return instが配置されたslot番号（begin_slot起点）
unsigned SwplPlan::relativeInstSlot(const SwplInst& c_inst) const {
  return slots.getRelativeInstSlot(c_inst, begin_slot);
}

/// \brief 命令が配置された範囲のCycle数を返す
/// \return 命令が配置された範囲のCycle数
int SwplPlan::getTotalSlotCycles() {
  return (end_slot - begin_slot) / SWPipeliner::FetchBandwidth;
}

/// \brief SwplPlanをダンプする
/// \param [in] stream 出力stream
/// \return なし
void SwplPlan::dump(raw_ostream &stream) {
  stream << "(plan " << format("%p",this) <<":\n";
  stream << "  iteration_interval  = " << iteration_interval <<"\n";
  stream << "  n_iteration_copies  = " << n_iteration_copies << "\n";;
  stream << "  n_renaming_versions = " << n_renaming_versions <<"\n";
  stream << "  begin_slot = " << begin_slot << "\n";
  stream << "  end_slot   = " << end_slot <<"\n";
  stream << "  total_cycles  = " << total_cycles << "\n";
  stream << "  prolog_cycles = " << prolog_cycles << "\n";
  stream << "  kernel_cycles = " << kernel_cycles << "\n";
  stream << "  epilog_cycles = " << epilog_cycles << "\n";

  dumpInstTable(stream);
  stream << ")\n";
  return;
}

/// \brief dump SwplPlan for LS
/// \param [in] stream output stream
/// \return なし
void SwplPlan::dumpForLS(raw_ostream &stream) {
  stream << "(plan " << format("%p",this) <<":\n";
  stream << "  begin_slot = " << begin_slot << "\n";
  stream << "  end_slot   = " << end_slot <<"\n";
  stream << "  total_cycles  = " << total_cycles << "\n";

  assert(iteration_interval==total_cycles);
  assert(n_iteration_copies==1);
  assert(n_renaming_versions==1);
  assert(prolog_cycles==0);
  assert(kernel_cycles==total_cycles);
  assert(epilog_cycles==0);

  dumpInstTable(stream);
  stream << ")\n";
  return;
}

void SwplPlan::dumpInstTable(raw_ostream &stream) {
  size_t table_size = (size_t)end_slot - (size_t)begin_slot;
  std::vector<SwplInst*> table(table_size, nullptr);

  for( auto *inst : loop.getBodyInsts() ) {
    unsigned slot;
    slot = slots.getRelativeInstSlot(*inst, begin_slot);
    table[slot] = inst;
  }
  stream << "\n\t( MachineInstr* : OpcodeName )\n";

  for (unsigned i = 0; i < table_size; ++i) {
    SwplInst* inst;
    SwplSlot slot;

    slot = begin_slot + i;
    if(slot.calcFetchSlot() == 0) {
      stream << "\n";
    }
    if (prolog_cycles != 0) {
      if(i!=0 && i%(SWPipeliner::FetchBandwidth * iteration_interval) == 0 ) { // new line by II
        stream << "\n";
      }
    }

    inst = table[i];

    if (inst == nullptr) {
      stream << "\t--" << slot.calcFetchSlot() << "--\t";
    } else {
      stream << "\t(";
      stream << format("%p", inst->getMI());
      stream << ":" << inst->getName();
      stream << ")\t";
    }
  }
  stream << "\n";

  return;
}

/// \brief registerが足りているかを判定する
/// \details renaming_versionsでregisterが足りるかを判定する
/// \param [in] c_loop ループ情報
/// \param [in] slots スケジューリング結果
/// \param [in] iteration_interval iteration interval
/// \param [in] n_renaming_versions renaming versions
/// \retval true 足りる
/// \retval false 足りない
///
/// \note 固定割付けでないレジスタが、整数、浮動小数点数、プレディケートであることが前提の処理である。
/// \note CCRegは固定割付けのため処理しない。
bool SwplPlan::isSufficientWithRenamingVersions(const SwplLoop& c_loop,
                                                const SwplSlots& slots,
                                                unsigned iteration_interval,
                                                unsigned n_renaming_versions) {
  SwplMsResourceResult ms_resource_result;
  unsigned n_necessary_regs;

  ms_resource_result.init();

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getIntRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryIreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getFloatRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryFreg(n_necessary_regs);

  n_necessary_regs = SwplRegEstimate::calcNumRegs(c_loop, &slots, iteration_interval,
                                                  llvm::StmRegKind::getPredicateRegID(),
                                                  n_renaming_versions);
  ms_resource_result.setNecessaryPreg(n_necessary_regs);

  if ( !(slots.isIccFreeAtBoundary(c_loop, iteration_interval)) ) {
    ms_resource_result.setSufficientWithArg(false);
  }

  return ms_resource_result.isModerate();
}

/// \brief SwplPlanを取得し、スケジューリング結果からPlanの要素を設定する
/// \details SwplPlanを取得し、スケジューリング結果からSwplPlanの要素を設定する。
///          SwplPlanは、schedulingが完了した後, transform mirへ渡す情報となる。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] slots スケジューリング結果
/// \param [in] min_ii 計算されたMinII
/// \param [in] ii スケジュールで採用されたII
/// \param [in] resource スケジュールで利用する資源情報
/// \return スケジューリング結果を設定したSwplPlanを返す。
SwplPlan* SwplPlan::construct(const SwplLoop& c_loop,
                              SwplSlots& slots,
                              unsigned min_ii,
                              unsigned ii,
                              const SwplMsResourceResult& resource) {
  SwplPlan* plan = new SwplPlan(c_loop); //plan->loop =loop;
  size_t prolog_blocks, kernel_blocks;

  plan->minimum_iteration_interval = min_ii;
  plan->iteration_interval = ii;
  plan->slots = slots;

  prolog_blocks = slots.calcPrologBlocks(plan->loop, ii);
  kernel_blocks = slots.calcKernelBlocks(plan->loop, ii);
  plan->n_iteration_copies = prolog_blocks + kernel_blocks;
  plan->n_renaming_versions
    = slots.calcNRenamingVersions(plan->loop, ii);

  plan->begin_slot = slots.findBeginSlot(plan->loop, ii);
  plan->end_slot = SwplSlot::baseSlot(ii) + 1;

  plan->prolog_cycles = prolog_blocks * ii;
  plan->kernel_cycles = kernel_blocks * ii;
  plan->epilog_cycles = plan->prolog_cycles;
  plan->total_cycles = (plan->prolog_cycles
                        + plan->kernel_cycles
                        + plan->epilog_cycles);

  plan->num_necessary_freg = resource.getNecessaryFreg();
  plan->num_max_freg = resource.getMaxFreg();
  plan->num_necessary_ireg = resource.getNecessaryIreg();
  plan->num_max_ireg = resource.getMaxIreg();
  plan->num_necessary_preg = resource.getNecessaryPreg();
  plan->num_max_preg = resource.getMaxPreg();

  return plan;
}

/// \brief SwplPlanの解放
/// \param [in] plan SwplPlanオブジェクトのポインタ
/// \return なし
void SwplPlan::destroy(SwplPlan* plan) {
  delete plan;
  return;
}

/// \brief SwplDdgを元にschedulingを試行し,SwplPlanを生成する
/// \details selectPlan関数がTRY_SCHEDULE_SUCCESSで復帰した場合、
///          SwplPlan::construct関数の結果を返す。
/// \param [in] ddg データ依存情報
/// \retval schedule結果(SwplPlan)
/// \retval nullptr schedule結果が得られない場合
SwplPlan* SwplPlan::generatePlan(SwplDdg& ddg)
{
  SwplSlots slots;
  slots.resize(ddg.getLoopBody_ninsts());
  unsigned ii, min_ii, itr;
  SwplMsResourceResult resource;

  TryScheduleResult rslt =
    selectPlan(ddg,
               slots, &ii, &min_ii, &itr, resource);

  switch(rslt) {
  case TryScheduleResult::TRY_SCHEDULE_SUCCESS:
    return SwplPlan::construct( *(ddg.getLoop()), slots, min_ii, ii, resource);

  case TryScheduleResult::TRY_SCHEDULE_FAIL:
    return nullptr;

  case TryScheduleResult::TRY_SCHEDULE_FEW_ITER: {
    SWPipeliner::Reason =
        llvm::formatv("This loop is not software pipelined because the iteration count is smaller than "
                      "{0} and "
                      "the software pipelining does not improve the performance.", itr);
    return nullptr;
  }
  }
  llvm_unreachable("select_plan returned an unknown state.");
  return nullptr;
}

SwplPlan* SwplPlan::generateLsPlan(const LsDdg& lsddg, const SwplScr::UseMap &LiveOutReg)
{
  LSListScheduling lsproc(lsddg, LiveOutReg);
  SwplSlots* slots = lsproc.getScheduleResult();

  // Generate a plan and set information such as scheduling results.
  const SwplLoop &currentLoop = lsddg.getLoop();
  auto lsplan = new SwplPlan(currentLoop);
  lsplan->slots = *slots;
  lsplan->n_iteration_copies=1;
  lsplan->n_renaming_versions=1;
  unsigned fb = SWPipeliner::FetchBandwidth;
  lsplan->begin_slot=lsproc.getEarliestSlot();
  lsplan->end_slot= (slots->findLastSlot(currentLoop) / fb + 1) * fb;
  auto total_cycles = (lsplan->end_slot.calcCycle()) - (lsplan->begin_slot.calcCycle());
  lsplan->total_cycles=total_cycles;
  lsplan->iteration_interval=total_cycles;
  lsplan->prolog_cycles=0;
  lsplan->kernel_cycles=total_cycles;
  lsplan->epilog_cycles=0;
  lsplan->num_max_freg = SWPipeliner::LsMaxFReg;
  lsplan->num_max_ireg = SWPipeliner::LsMaxIReg;
  lsplan->num_max_preg = SWPipeliner::LsMaxPReg;
  lsplan->num_necessary_freg=lsproc.getNumNecessaryFreg();
  lsplan->num_necessary_ireg=lsproc.getNumNecessaryIreg();
  lsplan->num_necessary_preg=lsproc.getNumNecessaryPreg();
  return lsplan;
}


/// \brief StmのResource情報により制限されるmin IIを計算する
/// \details resource min IIは以下の手順で求める。
///          1. 演算器数分のカウンタを用意（0.0で初期化）
///          2. スケジューリング対象の命令ごとに以下を実施
///            - 命令が使用する資源パターンごとに以下を実施
///              - 利用する演算器のカウンタに"1/命令が使用する資源パターン数"を加算
///          3. 演算器ごとのカウンタの最大（小数点切り上げ）が
///             Resource情報により制限されるmin IIとなる
///
/// \param [in] c_loop ループを構成する命令の情報
/// \return 算出したresource_ii
///
/// \note LLVM版では非パイプライン情報は取得できない。
///       現実装では、非パイプライン命令もパイプライン命令と同様に扱って計算している
unsigned SwplPlan::calculateResourceII(const SwplLoop& c_loop) {
  int max_counter;
  unsigned numresource;

  // 資源ごとの利用カウントをresource_appearsで保持する。
  // resource_appearsは、STM->getNumResource()+1分確保し、
  // resource_appears[A64FXRes::PortKind]が資源ごとの利用カウントとなる。
  // 要素[0]はA64FXRes::PortKind::P_NULLに該当する要素とし、使用しない。
  numresource = SWPipeliner::STM->getNumResource();
  std::vector<float> resource_appears(numresource+1, 0.0);

  for (auto *inst : c_loop.getBodyInsts()) {
    // 資源情報の生成と資源パターン数の取得
    const auto *pipes = SWPipeliner::STM->getPipelines( *(inst->getMI()) );
    unsigned num_pattern = pipes->size();

    for(auto *pipeline : *pipes ) {
      for(unsigned i=0; i<pipeline->resources.size(); i++) {
        StmResourceId resource = pipeline->resources[i];

        // どの資源パターンが使用されるかはわからないため、
        // n_appearsには"1.0/資源利用のパターン数"の値を足しこむ
        resource_appears[resource] += (1.0 / num_pattern);
      }
    }
  }
  max_counter = 0;
  for(auto val : resource_appears ) {
    int count = std::ceil(val); // 小数点切り上げ
    max_counter = std::max( max_counter, count );
  }
  return max_counter;
}

/// \brief 資源により制限されるmin IIを計算する
/// \details 以下から求めたIIの最大を返す。
///           - 1cycleに単位のfetch slot数の制約から求めたmin II
///           - TmのResource情報により制限されるmin II
///
/// \param [in] c_loop ループを構成する命令の情報
/// \return ResII
unsigned SwplPlan::calcResourceMinIterationInterval(const SwplLoop& c_loop) {
  unsigned fetch_constrained_ii;
  size_t n_body_insts;
  unsigned res_ii;

  n_body_insts = c_loop.getSizeBodyRealInsts();
  assert (n_body_insts != 0);

  /* 1cycleにつきfetch slot数しか命令は発行できない事による制約 */
  fetch_constrained_ii = ceil_div(n_body_insts, SWPipeliner::RealFetchBandwidth );

  // メモリポート数による制約は、
  // calculateResorceIIで計算されるmemory unitの制約の方が厳しい為、
  // 考慮しない。
  // Tradコードにも「実質的には不要な処理」とコメントがある。
  res_ii = calculateResourceII(c_loop);
  return std::max(fetch_constrained_ii, res_ii);
}

/// \brief 指定のアルゴリズムでスケジュールを試行する
/// \details スケジューリング指示情報(PLAN_SPEC)を生成し、
///          calculateMsResultによるスケジューリングを依頼する。
///
/// \param [in] c_ddg 対象の DDG
/// \param [in] res_mii ddg に対して計算された ResMII
/// \param [out] slots スケジュールしたスロットの動的配列
/// \param [out] selected_ii スケジュールで採用された II
/// \param [out] calculated_min_ii 計算された MinII
/// \param [out] required_itr ソフトパイプルートを通るのに必要なイテレート数
/// \param [out] resource スケジュールで利用する資源情報
/// \retval TRY_SCHEDULE_SUCCESS スケジュール成功。全ての出力引数を利用できる。
/// \retval TRY_SCHEDULE_FAIL スケジュール失敗。全ての出力引数は未定義。
/// \retval TRY_SCHEDULE_FEW_ITER ループのイテレート数不足によりソフトパイプ適用不可。
///         required_itr のみ利用できる。
TryScheduleResult SwplPlan::trySchedule(const SwplDdg& c_ddg,
                                        unsigned res_mii,
                                        SwplSlots** slots,
                                        unsigned* selected_ii,
                                        unsigned* calculated_min_ii,
                                        unsigned* required_itr,
                                        SwplMsResourceResult* resource) {
  SwplPlanSpec spec(c_ddg);
  if( !(spec.init(res_mii, existsPragma)) ){
    // SwplPlanSpec::init()復帰値は現在trueのみ。将来的にfalseがくる可能性を考えifを残す
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  }

  *calculated_min_ii = spec.min_ii;

  if( !(SwplCalclIterations::preCheckIterationCount(const_cast<const SwplPlanSpec &>(spec), required_itr)) ) {
    return TryScheduleResult::TRY_SCHEDULE_FEW_ITER;
  }

  SwplMsResult *ms_result = SwplMsResult::calculateMsResult(spec);

  if (ms_result != nullptr && ms_result->slots != nullptr) {
    *slots = ms_result->slots;
    *selected_ii = ms_result->ii;
    *resource = ms_result->resource;
    delete ms_result;
    return TryScheduleResult::TRY_SCHEDULE_SUCCESS;
  } else {
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  }

}

/// \brief IMSでスケジューリングを行う
///
/// \param [in] c_ddg 対象の DDG
/// \param [out] rslt_slots スケジュール結果であるスロットの動的配列
/// \param [out] selected_ii スケジュールで採用された II
/// \param [out] calculated_min_ii 計算された MinII
/// \param [out] required_itr ソフトパイプルートを通るのに必要なイテレート数
/// \param [out] resource スケジュールで利用する資源情報
/// \retval TRY_SCHEDULE_SUCCESS スケジュール成功。全ての出力引数を利用できる。
/// \retval TRY_SCHEDULE_FAIL スケジュール失敗。全ての出力引数は未定義。
/// \retval TRY_SCHEDULE_FEW_ITER ループのイテレート数不足によりソフトパイプ適用不可。
///                               required_itr のみ利用できる。
TryScheduleResult SwplPlan::selectPlan(const SwplDdg& c_ddg,
                                       SwplSlots& rslt_slots,
                                       unsigned* selected_ii,
                                       unsigned* calculated_min_ii,
                                       unsigned* required_itr,
                                       SwplMsResourceResult& resource) {
  unsigned res_mii = calcResourceMinIterationInterval( c_ddg.getLoop() );

  SwplSlots* slots_tmp;
  unsigned ii_tmp, min_ii_tmp, itr;
  SwplMsResourceResult resource_tmp;

  switch(trySchedule(c_ddg,
                     res_mii,
                     &slots_tmp,
                     &ii_tmp,
                     &min_ii_tmp,
                     &itr,
                     &resource_tmp)) {
  case TryScheduleResult::TRY_SCHEDULE_SUCCESS:
    rslt_slots = *slots_tmp; // copy
    delete slots_tmp;
    *selected_ii = ii_tmp;
    *calculated_min_ii = min_ii_tmp;
    *required_itr = itr;
    resource = resource_tmp; // copy
    return TryScheduleResult::TRY_SCHEDULE_SUCCESS;
  case TryScheduleResult::TRY_SCHEDULE_FAIL:
    return TryScheduleResult::TRY_SCHEDULE_FAIL;
  case TryScheduleResult::TRY_SCHEDULE_FEW_ITER:
    *required_itr = itr;
    return TryScheduleResult::TRY_SCHEDULE_FEW_ITER;
  }
  llvm_unreachable("unknown TryScheduleResult");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief プロローグを構成するblock数を返す
/// \details プロローグを構成するblock数を返す。
///          block数は、IIで区切られたサイクルの数。ステージ数と同意。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return プロローグを構成するblock数
size_t SwplSlots::calcPrologBlocks(const SwplLoop& c_loop,
                                                 unsigned iteration_interval) {
  return (calcFlatScheduleBlocks(c_loop, iteration_interval) - 1);
}

/// \brief slotsにて、命令が配置されている最初のSlot番号を返す
/// \details first_slotsにて、命令が配置されている最初のSlot番号を返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \return 検出したスロット番号
///
/// \note c_loop.BodyInstsの命令がすべて配置済みでなければならない。
SwplSlot SwplSlots::findFirstSlot(const SwplLoop& c_loop) {
  SwplSlot first_slot = SwplSlot::slotMax();

  for (auto &slot : *this) {
    if (slot == SwplSlot::UNCONFIGURED_SLOT) continue;
    first_slot = (first_slot < slot) ? first_slot : slot;
  }
  return first_slot;
}

/// \brief last_slotsにて、命令が配置されている最後のSlot番号を返す
/// \details slotsにて、命令が配置されている最後のSlot番号を返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \return 検出したスロット番号
///
/// \note c_loop.BodyInstsの命令がすべて配置済みでなければならない。
SwplSlot SwplSlots::findLastSlot(const SwplLoop& c_loop) {
  SwplSlot last_slot = SwplSlot::slotMin();

  for (auto &slot : *this) {
    if (slot == SwplSlot::UNCONFIGURED_SLOT) continue;
    last_slot = (last_slot > slot) ? last_slot : slot;
  }
  return last_slot;
}

/// \brief calcFlatScheduleBlocks
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return スケジューリング結果のunroll前のブロック数
size_t SwplSlots::calcFlatScheduleBlocks(const SwplLoop& c_loop,
                                                   unsigned iteration_interval) {
  SwplSlot first_slot, last_slot;
  first_slot = findFirstSlot(c_loop);
  last_slot = findLastSlot(c_loop);
  return (last_slot.calcBlock(iteration_interval) + 1
          - first_slot.calcBlock(iteration_interval));
}

/// \brief カーネルがunrollされる数(renaming version)を返す
/// \details カーネルがunrollされる数(renaming version)を返す
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return renaming version
size_t SwplSlots::calcKernelBlocks(const SwplLoop& c_loop,
                                                 unsigned iteration_interval) {
  size_t n_renaming_versions;

  n_renaming_versions = calcNRenamingVersions(c_loop, iteration_interval);
  return n_renaming_versions;
}

/// \brief instが配置されたslot番号（begin_slot起点）を返す
/// \param [in] c_inst
/// \param [in] begin_slot
/// \return instが配置されたslot番号（begin_slot起点）
unsigned SwplSlots::getRelativeInstSlot(const SwplInst& c_inst,
                                                  const SwplSlot& begin_slot) const {
  assert(this->at(c_inst.inst_ix) != SwplSlot::UNCONFIGURED_SLOT);
  return this->at(c_inst.inst_ix) - begin_slot;
}

/// \brief registerのlive rangeとiiの関係からrenaming versionを求める
/// \details  body で定義されていて、predecessor が無い regについて live_range を計算し、
///           最大値を求める。
///           live rangeが跨るblockの数だけ、regのrenamingを行なう必要がある。
///           現状はn_renaming_versionsがkernelに重なる回転数と一致する仕組みとなっている。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return renaming versions数
size_t SwplSlots::calcNRenamingVersions(const SwplLoop& c_loop,
                                                  unsigned iteration_interval) const {
  size_t max_live_cycles;
  size_t necessary_n_renaming_versions;

  max_live_cycles = 1;
  for (auto* def_inst : c_loop.getBodyInsts()) {
    SwplSlot def_slot;
    size_t def_cycle;

    assert(this->at(def_inst->inst_ix) != SwplSlot::UNCONFIGURED_SLOT);
    def_slot = this->at(def_inst->inst_ix);
    def_cycle = def_slot.calcCycle();
    for( auto *reg : def_inst->getDefRegs() ) {
      size_t live_cycles, last_use_cycle;

      if ( reg->isRegNull() ) {
        continue;
      }
      if ( reg->getPredecessor() != nullptr) {
        /* 既に割り当てられた reg を使うだけなので、計算しない。*/
        continue;
      }
      if ( !reg->isUsed() ) {
        continue;
      }
      last_use_cycle = calcLastUseCycleInBodyWithInheritance(*reg, iteration_interval);
      assert (def_cycle <= last_use_cycle);

      live_cycles = last_use_cycle - def_cycle + 1UL;
      assert (live_cycles >= 1);

      max_live_cycles = std::max(max_live_cycles, live_cycles);
    }
  }
  necessary_n_renaming_versions = ceil_div(max_live_cycles, iteration_interval);
  assert (necessary_n_renaming_versions >= 1);
  return necessary_n_renaming_versions;
}

/// \brief 最初のInstが配置されているblockの先頭Slotを返す
/// \details 最初のInstが配置されているblockの先頭Slotを返す。
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \return 最初のInstが配置されているblockの先頭Slot番号
SwplSlot SwplSlots::findBeginSlot(const SwplLoop& c_loop,
                                unsigned iteration_interval) {
  SwplSlot first_slot;
  unsigned begin_block;

  first_slot = findFirstSlot(c_loop);
  begin_block = first_slot.calcBlock(iteration_interval);
  return SwplSlot::constructFromBlock(begin_block, iteration_interval);
}

/// \brief icc が iteration_interval のブロックを跨って使用されていないかをチェックする
/// \param [in] c_loop ループを構成する命令の情報
/// \param [in] iteration_interval II
/// \retval true ブロックを跨って使用されていない
/// \retval false ブロックを跨って使用されている
bool SwplSlots::isIccFreeAtBoundary(const SwplLoop& loop,
                                             unsigned iteration_interval) const {
  for( auto *inst : loop.getBodyInsts() ) {
    SwplSlot def_slot;
    unsigned def_block;

    if ((def_slot = this->at(inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
      report_fatal_error("inst not found in slots.");
    }
    def_block = def_slot.calcBlock(iteration_interval);

    for( auto *reg : inst->getDefRegs() ) {
      unsigned last_use_cycle, last_use_block;

      if( reg->isRegNull() ) {
        continue;
      }
      if( !reg->isIntegerCCRegister() ) {
        continue;
      }
      if( !reg->isUsed() ) {
        continue;
      }
      last_use_cycle = calcLastUseCycleInBody(*reg, iteration_interval);

      last_use_block = last_use_cycle / iteration_interval;
      assert (last_use_block >= def_block);
      if (last_use_block != def_block) { /* F1686 */
        return false;
      }
    }
  }
  return true;
}

/// \brief reg が最後に使われる cycle を返す
/// \details reg が最後に使われる cycle を返す。
///          regがsuccessorを持つ場合は遡って調べる。
/// \param [in] reg 調査対象のreg
/// \param [in] iteration_interval II
/// \return regが最後に使われる cycle
unsigned SwplSlots::calcLastUseCycleInBodyWithInheritance(const SwplReg& reg,
                                                                    unsigned iteration_interval) const {
  const SwplReg* successor_reg;

  successor_reg = reg.getSuccessor();
  if (successor_reg != nullptr) {
    return calcLastUseCycleInBodyWithInheritance (*successor_reg,
                                                  iteration_interval);
  }
  return calcLastUseCycleInBody(reg, iteration_interval);
}

/// \brief reg が最後に使われる slot を返す
/// \details phi で使われる場合は、次のイタレーションで最後に使われる slot に
///          iteration_interval を足して、比べる。
/// \param [in] reg 調査対象のreg
/// \param [in] iteration_interval II
/// \return regが最後に使われる cycle
unsigned SwplSlots::calcLastUseCycleInBody(const SwplReg& reg,
                                                     unsigned iteration_interval) const {
  unsigned last_use_cycle;
  const SwplInst* def_inst;

  /* last_use_cycle の初期値を設定。 */
  def_inst = reg.getDefInst();
  if (def_inst->isBodyInst()) {
    SwplSlot def_slot;

    if ((def_slot = this->at(def_inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
      report_fatal_error("instruction not found in Slots.");
    }
    last_use_cycle = def_slot.calcCycle();
  } else {
    last_use_cycle = SwplSlot::slotMin().calcCycle();
  }

  for (auto *use_inst : reg.getUseInsts() ) {
    unsigned use_cycle;

    if( use_inst->isPhi()) {
      unsigned next_use_cycle;

      assert( !((reg.getDefInst())->isPhi()) );
      SwplReg& next_reg = use_inst->getDefRegs(0);
      assert ( !reg.isRegNull() );
      next_use_cycle = calcLastUseCycleInBodyWithInheritance(next_reg,
                                                             iteration_interval);
      use_cycle = next_use_cycle + iteration_interval;
    } else if( use_inst->isBodyInst() ) {
      SwplSlot use_slot;

      if ((use_slot = this->at(use_inst->inst_ix)) == SwplSlot::UNCONFIGURED_SLOT) {
        report_fatal_error("instruction not found in Slots.");
      }
      use_cycle = use_slot.calcCycle();
    } else {
      llvm_unreachable("unexpected use_inst.");
      continue;
    }
    last_use_cycle = std::max(last_use_cycle, use_cycle);
  }
  return last_use_cycle;
}

/// \brief slots中の最大最小slotを求める。
/// \param [out] max_slot 最大slot
/// \param [out] max_slot 最小slot
/// \return なし
void SwplSlots::getMaxMinSlot(SwplSlot& max_slot, SwplSlot& min_slot) {
  assert(this->size()!=0);
  SwplSlot max=0;
  SwplSlot min=SwplSlot::slotMax();

  for (auto& mp : *this) {
    if (mp == SwplSlot::UNCONFIGURED_SLOT) continue;
    if (max < mp) max=mp;
    if (min > mp) min=mp;
  }
  max_slot = max;
  min_slot = min;
  return;
}

/// \brief cycleにおける空きslotを返す
/// \param [in] cycle 検索対象のcycle
/// \param [in] iteration_interval II
/// \param [in] isvirtual 仮想命令用の空きSlotを探すのであればtrue
/// \return cycleのうち、命令が配置されていないslot
SwplSlot SwplSlots::getEmptySlotInCycle( unsigned cycle,
                                                   unsigned iteration_interval,
                                                   bool isvirtual ) {
  // InstSlotHashmapを２次元配列で表したイメージは以下のとおり。
  // Real命令と仮想命令によって、配置可能なSlotが異なる
  //
  //         |----仮想命令用slot-----|----Real命令用slot----|
  // cycle 1  slot  slot  slot  slot  slot  slot  slot  slot
  // cycle 2  slot  slot  slot  slot  slot  slot  slot  slot
  //       :  slot  slot  slot  slot  slot  slot  slot  slot
  // cycle n  slot  slot  slot  slot  slot  slot  slot  slot
  //         |<-------------------------------------------->| SWPipeliner::FetchBandwidth
  //                                 |<-------------------->| SWPipeliner::RealFetchBandwidth

  unsigned bandwidth = SWPipeliner::FetchBandwidth;
  unsigned realbandwidth = SWPipeliner::RealFetchBandwidth;

  std::vector<bool> openslot(bandwidth);
  for(unsigned i=0; i<bandwidth; i++) {
    // isvirtualがfalseの場合は、opnslotの初期値を以下のようにする。
    // ・0～(bandwidth - realbandwidth - 1)の要素をfalse
    // ・(bandwidth - realbandwidth)～(bandwidth-1)の要素をtrue
    // isvirtualがtrueの場合は逆
    openslot[i] = (i<(bandwidth - realbandwidth)) ? isvirtual : !isvirtual;
  }

  unsigned target_modulo_cycle = cycle % iteration_interval;

  for (auto& mp : *this) {
    if (mp == SwplSlot::UNCONFIGURED_SLOT) continue;
    unsigned modulo_cycle = mp.calcCycle() % iteration_interval;
    if(target_modulo_cycle == modulo_cycle) {
      openslot[mp.calcFetchSlot()] = false; // 使用済み
    }
  }
  for( unsigned idx=0; idx<bandwidth; idx++) {
    if( openslot[idx] == true ) {
      return SwplSlot::construct(cycle, 0) + idx;
    }
  }
  return SWPL_ILLEGAL_SLOT;
}


SwplSlot SwplSlots::getLsEmptySlotInCycle( unsigned cycle, bool isvirtual ) {
  // getEmptySlotInCycle modified for LS.
  // See comment of getEmptySlotInCycle about slot configuration.

  unsigned bandwidth = SWPipeliner::FetchBandwidth;
  unsigned realbandwidth = SWPipeliner::RealFetchBandwidth;

  std::vector<bool> openslot(bandwidth);
  for(unsigned i=0; i<bandwidth; i++) {
    openslot[i] = (i<(bandwidth - realbandwidth)) ? isvirtual : !isvirtual;
  }

  for (auto& mp : *this) {
    if (mp == SwplSlot::UNCONFIGURED_SLOT) continue;
    unsigned placed_cycle = mp.calcCycle();
    if(cycle == placed_cycle) {
      openslot[mp.calcFetchSlot()] = false; // used
    }
  }
  for( unsigned idx=0; idx<bandwidth; idx++) {
    if( openslot[idx] == true ) {
      return SwplSlot::construct(cycle, 0) + idx;
    }
  }
  return SWPL_ILLEGAL_SLOT;
}

llvm::SmallVector<unsigned, 32> SwplSlots::getInstIdxInSlotOrder() {
  unsigned size  = this->size();

  llvm::SmallVector<std::array<unsigned,2>, 32> instix_slotidx; // vector of [inst_ix, slot.slot_index]
  unsigned instix=0;
  for (auto v: *this) {
    std::array<unsigned,2> ixsx;
    ixsx[0]=instix;
    ixsx[1]=v.slot_index;
    instix_slotidx.push_back(ixsx);
    instix+=1;
  }

  // sort by slot_index
  unsigned range = size-1;
  while (range!=0) {
    for (unsigned i=0; i<range; i++) {
      if (instix_slotidx[i][1] > instix_slotidx[i+1][1]) // compare slot_index
        std::swap(instix_slotidx[i], instix_slotidx[i+1]);
    }
    range-=1;
  }

  // by slot_index, listing inst_ix
  llvm::SmallVector<unsigned, 32> ret;
  for (auto v: instix_slotidx) {
    ret.push_back(v[0]);
  }
  return ret;
}

/// \brief デバッグ用ダンプ
void SwplSlots::dump(const SwplLoop& c_loop) {
  if( this->size() == 0 ) {
      dbgs() << "Nothing...\n";
      return;
  }

  SwplSlot max_slot, min_slot;
  getMaxMinSlot(max_slot, min_slot);

  unsigned max_cycle, min_cycle;
  max_cycle = max_slot.calcCycle();
  min_cycle = min_slot.calcCycle();

  max_slot = SwplSlot::construct(max_cycle, 0) + SWPipeliner::FetchBandwidth;
  min_slot = SwplSlot::construct(min_cycle, 0);

  size_t table_size = (size_t)max_slot - (size_t)min_slot;
  std::vector<SwplInst*> table(table_size, nullptr);
  int ix = 0;

  for(auto* inst : c_loop.getBodyInsts()) {
    if (this->at(ix) != SwplSlot::UNCONFIGURED_SLOT)
      table[(this->at(ix) - min_slot)] = inst;
    ix++;
  }

  for (unsigned i = 0; i < table_size; ++i) {
    SwplInst* inst;
    SwplSlot slot;

    slot = min_slot + i;
    if (slot.calcFetchSlot() == 0) {
      dbgs() << "\n\t(" << slot << ")";
    }

    inst = table[i];

    dbgs() << "\t";
    if (inst == nullptr) {
      dbgs() << "--" << slot.calcFetchSlot() << "--\t";
    } else {
      dbgs() << inst->getName() << "\t" ;
    }
  }
  dbgs() << "\n";
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief slotのcycleを返す
/// \details cycleに換算した値を返す
/// \return slotに該当するcycle
unsigned SwplSlot::calcCycle() {
  return slot_index / SWPipeliner::FetchBandwidth;
}

/// \brief slotのFetchSlotを返す
/// \details FetchSlot = 各cycleの何番目のslotか
/// \return FetchSlot
unsigned SwplSlot::calcFetchSlot() {
  return slot_index % SWPipeliner::FetchBandwidth;
}

/// \brief slotが何番目のblockであるかを返す
/// \param [in] iteration_interval II
/// \return 何番目のblockか
unsigned SwplSlot::calcBlock(unsigned iteration_interval)
{
  return calcCycle() / iteration_interval;
}

/// \brief base slotを返す
/// \param [in] iteration_interval II
/// \return base slot
SwplSlot SwplSlot::baseSlot(unsigned iteration_interval) {
  unsigned middle_block;
  SwplSlot middle_slot;

  middle_slot = SwplSlot::slotMax() / 2 + SwplSlot::slotMin () / 2;
  middle_block = middle_slot.calcBlock(iteration_interval);
  return SwplSlot::constructFromBlock(middle_block, iteration_interval) - 1;
}

/// \brief SwplSlotを生成する
/// \param [in] cycle cycle
/// \param [in] fetch_slot fetch slot
/// \return SwplSlot
SwplSlot SwplSlot::construct(unsigned cycle, unsigned fetch_slot) {
  if( ( SwplSlot::slotMin().calcCycle() > cycle ) ||
      ( cycle > SwplSlot::slotMax().calcCycle() ) ||
      ( fetch_slot >= SWPipeliner::FetchBandwidth )
       ) {
    if (SWPipeliner::isDebugOutput()) {
      dbgs() << "!swp-msg: Used cycle for scheduling is out of the range.\n";
    }
    return SWPL_ILLEGAL_SLOT;
  }
  return cycle * SWPipeliner::FetchBandwidth + fetch_slot;
}

/// \brief block番号からSwplSlotを生成する
/// \param [in] block block
/// \param [in] iteration_interval II
/// \return SwplSlot
SwplSlot SwplSlot::constructFromBlock(unsigned block, unsigned iteration_interval) {
  return SwplSlot::construct(block * iteration_interval, 0);
}

/// \brief slotの最大を返す
/// \return 最大slot
SwplSlot SwplSlot::slotMax() { return 1500000; }

/// \brief slotの最小を返す
/// \return 最小slot
SwplSlot SwplSlot::slotMin() { return  500000; }

SwplSlots* LSListScheduling::getScheduleResult() {
  setPriorityOrder(Ready);

  slots->resize(Ready.size());

  lsmrt = createLsMrt();

  // Scheduling in Ready order.
  for (auto [inst, edges] : Ready) {
    SwplSlot slot = getPlacementSlot(inst, edges);
    (*slots)[inst->inst_ix].slot_index = slot.slot_index;

    if (OptionDumpLsEveryInst) {
      lsmrt->dump(*slots, dbgs());
      slots->dump(lsddg.getLoop());
    }
  }

  if (OptionDumpLsMrt) {
    dbgs() << "!ls-msg: line ";
    lsddg.getLoop().getML()->getStartLoc().print(dbgs());
    dbgs() <<":\n";
    dbgs() << "(mrt\n";
    lsmrt->dump(*slots, dbgs());
    dbgs() <<  ")\n";  
  }

  calcVregs();

  return slots;
}

void LSListScheduling::setPriorityOrder(llvm::MapVector<const SwplInst*, SwplInstEdges> &Ready) {
  SwplInstEdges edges = lsddg.getGraph().getEdges();
  const SwplInsts insts = lsddg.getGraph().getVertices();
  llvm::MapVector<const SwplInst*, unsigned> inst_pre_map; // set of SwplInst and num of pre-insts
  llvm::MapVector<const SwplInst*, SwplInstEdges*> wReady;

  // set of SwplInst and SwplInstEdges.
  for (auto* i : insts) {
    auto es = new SwplInstEdges;
    wReady.insert(std::make_pair(i, es));
  }
  // Insert edge when there is a preceding instruction.
  for (auto* e : edges) {
    wReady[e->getTerminal()]->push_back(e);
  }

  // Count the number of preceding instructions.
  for (auto [f, s] : wReady) {
    inst_pre_map.insert(std::make_pair(f,s->size()));
  }

  // topological sort.
  std::queue<const SwplInst*> Q;
  for (auto [f, s] : inst_pre_map) {
    if (s == 0) {
      Q.push(f);
    }
  }
  while (!Q.empty()) {
    auto q = Q.front();
    Q.pop();
    Ready.insert(std::make_pair(q, *(wReady[q])));
    for (auto [f, s] : wReady) {
      for (auto* edge : *s) {
        if (edge->getInitial() == q) {
          inst_pre_map[f] -= 1;
          if (inst_pre_map[f] == 0) {
            Q.push(f);
          }
        }
      }
    }
  }

  for (auto [f, s] : wReady) {
    delete s;
  }
}

SwplSlot LSListScheduling::getPlacementSlot(const SwplInst* inst, SwplInstEdges &edges) {
  unsigned earliest_cycle = min_cycle;  // Earliest cycle that can be placed

  if (OptionDumpLsEveryInst) {
    dbgs() << "=========================================\n";
    inst->getMI()->print(dbgs());
    if (edges.size() == 0) {
      dbgs() << "pred : none\n";
    }
  }

  // Find the cycle that can be placed fastest from multiple edges
  for (auto* edge : edges) {
    auto s = (*slots)[edge->getInitial()->inst_ix];
    auto ini_cycle = s.calcCycle();
    auto d = lsddg.getDelay(*edge);
    if (d == 0)
      d = 1;  // It should be a pseudo-instruction
      
    if (OptionDumpLsEveryInst) {
      dbgs() << "pred : " << edge->getInitial()->getName() << "(placed=" <<  ini_cycle << ", delay=" << d << ")\n";
    }
    earliest_cycle = std::max(earliest_cycle, ini_cycle + d);
  }

  unsigned attempted_cycle = earliest_cycle;
  bool ispseudo = SWPipeliner::STM->isPseudo(*(inst->getMI()));
  SwplSlot placeable_slot;
  const StmPipeline *pipeline;
  while (1) {
    // Check if cycle slot is free.
    placeable_slot = slots->getLsEmptySlotInCycle(attempted_cycle, ispseudo);
    if (placeable_slot != SWPL_ILLEGAL_SLOT) {
      // Check for resource contention.
      if (ispseudo) {
        pipeline = nullptr;
        break;
      }

      const StmPipeline *pl = findPlacablePipeline(inst, attempted_cycle);
      if (pl != nullptr) {
        pipeline = pl;
        break;
      }
    }

    attempted_cycle += 1;
  }

  // Make a resource reservation.
  if (pipeline != nullptr)
    lsmrt->reserveResourcesForInst(attempted_cycle - min_cycle, *inst, *pipeline);

  if (OptionDumpLsEveryInst) {
    dbgs() << "earliest cycle : " << earliest_cycle << "\n";
    dbgs() << "placed cycle   : " << attempted_cycle << "\n";
    dbgs() << "placed slot    : " << placeable_slot << "\n";
  }

  return placeable_slot;
}

SwplMrt* LSListScheduling::createLsMrt() {
  const SwplInsts insts = lsddg.getGraph().getVertices();

  unsigned length_mrt = 0;  /// MRT length.
  for (auto* i : insts) {
    // Get instruction latency
    int latency_R = SWPipeliner::STM->computeRegFlowDependence(i->getMI(), nullptr);
    int latency_M = SWPipeliner::STM->computeMemFlowDependence(i->getMI(), nullptr);
    int latency = std::max(latency_R, latency_M);
    assert(latency >= 0);
    if (latency == 0) {
      latency += 1;   // Zero is pseudo. Pseudo are treated as latency=1.
    }
    length_mrt += (unsigned)latency;
  }

  // Create LsMRT
  return SwplMrt::constructForLs(length_mrt);
}

const StmPipeline* LSListScheduling::findPlacablePipeline(const SwplInst* inst, unsigned cycle) {
  const MachineInstr& mi = *(inst->getMI());

  // Check for resource contention when placed in a cycle.
  for (auto *pl : *(SWPipeliner::STM->getPipelines(mi))) {
    if (lsmrt->isOpenForInst(cycle - min_cycle, *inst, *pl))
      return pl;
  }

  return nullptr; // not found
}

void LSListScheduling::calcVregs() {
  auto loopinsts=lsddg.getLoopBodyInsts();

  auto regmap=lsddg.getLoop().getOrgReg2NewReg();
  llvm::SmallSet<Register, 8> liveoutconsiders;
  for (auto &t:LiveOutRegs) {
    liveoutconsiders.insert(regmap[t.first]);
  }

  // Get inst_ix in placement order.
  auto order = slots->getInstIdxInSlotOrder();

  // Collect def-reg/uses-reg in the order in which instructions are placed.
  using DefsAndUses = std::array<llvm::SmallSet<Register,4>, 2>;  // [0] as def-regs, [1] as use-regs
  llvm::SmallVector<DefsAndUses, 32> regs_placement_order; // DefsAndUses are [0] as def, [1] as use
  llvm::SmallSet<Register, 32>  alldefs; // all def regs
  llvm::SmallSet<Register, 32>  alluses; // all use regs
  for (auto o: order) {
    auto mi = loopinsts[o]->getMI();
    DefsAndUses defuses;

    for (auto& op: mi->operands()){
      if (!op.isReg()) continue;

      if (op.isDef()) {
        defuses[0].insert(op.getReg());
        alldefs.insert(op.getReg());
      }
      else if (op.isUse()) {
        defuses[1].insert(op.getReg());
        alluses.insert(op.getReg());
      }
    }
    regs_placement_order.push_back(defuses);
  }
  unsigned numregs=regs_placement_order.size();

  if (OptionDumpLsReg) {
    dbgs() << "======= LS estimate regs details =======\n";
    dbgs() << "LiveOutReg(";
    for (auto r: liveoutconsiders) {
      dbgs() << printReg(r, SWPipeliner::TRI);
    }
    dbgs() << "):\n";

    slots->dump(lsddg.getLoop());
    dbgs() << "sorted inst_ix order :";
    for (auto idx: order) {
      dbgs() << idx << " ";
    }
    dbgs() << "\n";

    dbgs() << "--- dump sorted order ---\n";
    for (unsigned idx=0; idx<numregs; idx++) {
      auto swpli = loopinsts[order[idx]];
      auto mi = swpli->getMI();
      dbgs() << " " << idx << "\t" << swpli->inst_ix << " : ";
      mi->print(dbgs());

      dbgs() << "\t\tDefs : ";
      for (auto r: regs_placement_order[idx][0]) {
        dbgs() << printReg(r, SWPipeliner::TRI) << " ";
      }
      dbgs() << "\n";
      dbgs() << "\t\tUses : ";
      for (auto r: regs_placement_order[idx][1]) {
        dbgs() << printReg(r, SWPipeliner::TRI) << " ";
      }
      dbgs() << "\n";
    }
  }

  // Update estimateRegCounter with register live-range.
  estimateIregCounter.resize(numregs);
  estimateFregCounter.resize(numregs);
  estimatePregCounter.resize(numregs);
  unsigned lastidx = numregs-1;
  for (unsigned i=0; i<numregs; i++) {
    auto &defs = regs_placement_order[i][0]; // defs of instructions where inst_ix is i
    auto &uses = regs_placement_order[i][1]; // uses of instructions where inst_ix is i

    // about def
    for (auto r: defs) {
      std::pair<int, int> range(-1, -1); // update range

      //このdefをuseしている命令が存在する
      if (alluses.count(r)>0) {
        if (uses.count(r)>0) {
          range=std::make_pair(0, lastidx); // 対象命令自身がuseしている
          liveoutconsiders.erase(r);
        }
        else {
          int c=-1;
          for (unsigned j=0; j<i; j++) {
            if (regs_placement_order[j][1].count(r)>0)
              c = j;
          }
          if (c!=-1) {
            range=std::make_pair(i, c); // 対象命令より早い時間にuseしている命令が存在する
            liveoutconsiders.erase(r);
          }
          else {
            for (unsigned j=i+1; j<numregs; j++) {
              if (regs_placement_order[j][1].count(r)>0)
                c = j;
            }
            if (c!=-1)
              range=std::make_pair(i, c);// 対象命令より遅い時間にuseしている命令が存在する
          }
        }

      }
      else {
        range=std::make_pair(i, lastidx); // このdefをuseしている命令が存在しない
        liveoutconsiders.erase(r);
      }
      auto [ormore, orless] = range;
      updateRegisterCounter(r, ormore, orless); // update estimate reg counter
    }

    // about use
    for (auto r: uses) {
      std::pair<int, int> range(-1, -1); // update range

      if (alldefs.count(r)==0) {
        // このuseをdefしている命令が存在しない
        int c=-1;
        for (int j=i-1; j>=0; j--) {
          if (regs_placement_order[j][1].count(r)>0) {
            c = j;
            break;
          }
        }
        if (c!=-1)
          range=std::make_pair(c+1, i); //  該当命令より前にuseしている命令が存在する場合
        else
          range=std::make_pair(0, i); // 該当命令より前にuseしている命令が存在しない場合

        auto [ormore, orless] = range;
        updateRegisterCounter(r, ormore, orless); // update estimate reg counter
      }

      if (liveoutconsiders.count(r)>0) {
        // このuseがLiveout考慮レジスタ群に存在する
        if (i!=lastidx)
          updateRegisterCounter(r, i+1, lastidx); // update estimate reg counter

        liveoutconsiders.erase(r);
      }
    }
  }
  return;
}

void LSListScheduling::updateRegisterCounter(Register r, int ormore, int orless) {
  int size=estimateIregCounter.size();
  assert(ormore>=0 && orless>=0);
  assert(ormore<=size && orless<size);

  auto rk = SWPipeliner::TII->getRegKind(*SWPipeliner::MRI, r);
  unsigned rkid, units;
  std::tie(rkid, units) = SWPipeliner::TII->getRegKindId(*SWPipeliner::MRI, r);

  llvm::SmallVector<unsigned, 32> *buf;
  if (rk->isInteger()) buf=&estimateIregCounter;
  else if (rk->isFloating()) buf=&estimateFregCounter;
  else if (rk->isPredicate()) buf=&estimatePregCounter;
  else return; // Not counted.

  if (ormore<=orless) {
    for (int i=ormore; i<=orless; i++) {
      (*buf)[i]+=units;
    }
  }
  else {
    for (int i=0; i<=orless; i++) {
      (*buf)[i]+=units;
    }
    for (int i=ormore; i<size; i++) {
      (*buf)[i]+=units;
    }
  }

  if (OptionDumpLsReg) {
    dbgs() << "updateRegisterCounter(" << printReg(r, SWPipeliner::TRI) << ", "
           << ormore << ", " << orless << ") "
           << "regclass:" << printRegClassOrBank(r, *SWPipeliner::MRI, SWPipeliner::TRI)
           << ", size:"<< units << "\n";
    dbgs() << "--\tireg\tfref\tpreg\n";
    for (unsigned i=0; i<estimateFregCounter.size(); i++) {
      dbgs() << "[" << i
             << "]\t" << estimateIregCounter[i]
             << ",\t" << estimateFregCounter[i]
             << ",\t" << estimatePregCounter[i]
             << "\n";
    }
  }
}
