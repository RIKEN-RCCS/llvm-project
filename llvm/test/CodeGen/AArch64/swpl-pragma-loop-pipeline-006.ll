; RUN: llc < %s -O2 -mcpu=a64fx -fswp -swpl-debug --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: Specified Swpl disable by option/pragma.

; ModuleID = './pragma-loop-pipeline-006.c'
source_filename = "./pragma-loop-pipeline-006.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @pipeline_enable(i32 noundef %length, ptr nocapture noundef readonly %value) local_unnamed_addr #0 !dbg !8 {
entry:
  %cmp6 = icmp sgt i32 %length, 0, !dbg !12
  br i1 %cmp6, label %for.body.lr.ph, label %for.cond.cleanup, !dbg !13

for.body.lr.ph:                                   ; preds = %entry
  %0 = load ptr, ptr %value, align 8, !tbaa !14
  %1 = load i8, ptr %0, align 1, !tbaa !18
  %conv = zext i8 %1 to i32
  %2 = add nsw i32 %length, -1, !dbg !13
  %xtraiter = and i32 %length, 7, !dbg !13
  %3 = icmp ult i32 %2, 7, !dbg !13
  br i1 %3, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.lr.ph.new, !dbg !13

for.body.lr.ph.new:                               ; preds = %for.body.lr.ph
  %4 = add i32 %length, -8, !dbg !13
  %5 = lshr i32 %4, 3, !dbg !13
  %6 = add nuw nsw i32 %5, 1, !dbg !13
  %xtraiter11 = and i32 %6, 3, !dbg !13
  %7 = icmp ult i32 %4, 24, !dbg !13
  br i1 %7, label %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa, label %for.body.lr.ph.new.new, !dbg !13

for.body.lr.ph.new.new:                           ; preds = %for.body.lr.ph.new
  %unroll_iter23 = and i32 %6, 1073741820, !dbg !13
  br label %for.body, !dbg !13

for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa: ; preds = %for.body, %for.body.lr.ph.new
  %add.7.lcssa.ph = phi i32 [ undef, %for.body.lr.ph.new ], [ %add.7.3, %for.body ]
  %inc.7.lcssa.ph = phi i32 [ undef, %for.body.lr.ph.new ], [ %inc.7.3, %for.body ]
  %i.08.unr19 = phi i32 [ 0, %for.body.lr.ph.new ], [ %inc.7.3, %for.body ]
  %sum.07.unr20 = phi i32 [ 0, %for.body.lr.ph.new ], [ %add.7.3, %for.body ]
  %lcmp.mod.not34 = icmp eq i32 %xtraiter11, 0, !dbg !13
  br i1 %lcmp.mod.not34, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.epil12, !dbg !13

for.body.epil12:                                  ; preds = %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa, %for.body.epil12
  %i.08.epil13 = phi i32 [ %inc.7.epil, %for.body.epil12 ], [ %i.08.unr19, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ]
  %sum.07.epil14 = phi i32 [ %add.7.epil, %for.body.epil12 ], [ %sum.07.unr20, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ]
  %epil.iter18 = phi i32 [ %epil.iter18.next, %for.body.epil12 ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ]
  %mul.epil15 = mul nsw i32 %sum.07.epil14, %conv, !dbg !19
  %add.epil16 = add nsw i32 %mul.epil15, %i.08.epil13, !dbg !20
  %inc.epil17 = or i32 %i.08.epil13, 1, !dbg !21
  %mul.1.epil = mul nsw i32 %add.epil16, %conv, !dbg !19
  %add.1.epil = add nsw i32 %mul.1.epil, %inc.epil17, !dbg !20
  %inc.1.epil = or i32 %i.08.epil13, 2, !dbg !21
  %mul.2.epil = mul nsw i32 %add.1.epil, %conv, !dbg !19
  %add.2.epil = add nsw i32 %mul.2.epil, %inc.1.epil, !dbg !20
  %inc.2.epil = or i32 %i.08.epil13, 3, !dbg !21
  %mul.3.epil = mul nsw i32 %add.2.epil, %conv, !dbg !19
  %add.3.epil = add nsw i32 %mul.3.epil, %inc.2.epil, !dbg !20
  %inc.3.epil = or i32 %i.08.epil13, 4, !dbg !21
  %mul.4.epil = mul nsw i32 %add.3.epil, %conv, !dbg !19
  %add.4.epil = add nsw i32 %mul.4.epil, %inc.3.epil, !dbg !20
  %inc.4.epil = or i32 %i.08.epil13, 5, !dbg !21
  %mul.5.epil = mul nsw i32 %add.4.epil, %conv, !dbg !19
  %add.5.epil = add nsw i32 %mul.5.epil, %inc.4.epil, !dbg !20
  %inc.5.epil = or i32 %i.08.epil13, 6, !dbg !21
  %mul.6.epil = mul nsw i32 %add.5.epil, %conv, !dbg !19
  %add.6.epil = add nsw i32 %mul.6.epil, %inc.5.epil, !dbg !20
  %inc.6.epil = or i32 %i.08.epil13, 7, !dbg !21
  %mul.7.epil = mul nsw i32 %add.6.epil, %conv, !dbg !19
  %add.7.epil = add nsw i32 %mul.7.epil, %inc.6.epil, !dbg !20
  %inc.7.epil = add nuw nsw i32 %i.08.epil13, 8, !dbg !21
  %epil.iter18.next = add i32 %epil.iter18, 1, !dbg !13
  %epil.iter18.cmp.not = icmp eq i32 %epil.iter18.next, %xtraiter11, !dbg !13
  br i1 %epil.iter18.cmp.not, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.epil12, !dbg !13, !llvm.loop !22

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa, %for.body.epil12, %for.body.lr.ph
  %add.lcssa.ph = phi i32 [ undef, %for.body.lr.ph ], [ %add.7.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ], [ %add.7.epil, %for.body.epil12 ]
  %i.08.unr = phi i32 [ 0, %for.body.lr.ph ], [ %inc.7.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ], [ %inc.7.epil, %for.body.epil12 ]
  %sum.07.unr = phi i32 [ 0, %for.body.lr.ph ], [ %add.7.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa ], [ %add.7.epil, %for.body.epil12 ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0, !dbg !13
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil, !dbg !13

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa
  %mul.epil = mul nsw i32 %sum.07.unr, %conv, !dbg !19
  %add.epil = add nsw i32 %mul.epil, %i.08.unr, !dbg !20
  %epil.iter.cmp.not = icmp eq i32 %xtraiter, 1, !dbg !13
  br i1 %epil.iter.cmp.not, label %for.cond.cleanup, label %for.body.epil.1, !dbg !13, !llvm.loop !24

for.body.epil.1:                                  ; preds = %for.body.epil
  %inc.epil = add nuw nsw i32 %i.08.unr, 1, !dbg !21
  %mul.epil.1 = mul nsw i32 %add.epil, %conv, !dbg !19
  %add.epil.1 = add nsw i32 %mul.epil.1, %inc.epil, !dbg !20
  %epil.iter.cmp.not.1 = icmp eq i32 %xtraiter, 2, !dbg !13
  br i1 %epil.iter.cmp.not.1, label %for.cond.cleanup, label %for.body.epil.2, !dbg !13, !llvm.loop !24

for.body.epil.2:                                  ; preds = %for.body.epil.1
  %inc.epil.1 = add nuw nsw i32 %i.08.unr, 2, !dbg !21
  %mul.epil.2 = mul nsw i32 %add.epil.1, %conv, !dbg !19
  %add.epil.2 = add nsw i32 %mul.epil.2, %inc.epil.1, !dbg !20
  %epil.iter.cmp.not.2 = icmp eq i32 %xtraiter, 3, !dbg !13
  br i1 %epil.iter.cmp.not.2, label %for.cond.cleanup, label %for.body.epil.3, !dbg !13, !llvm.loop !24

for.body.epil.3:                                  ; preds = %for.body.epil.2
  %inc.epil.2 = add nuw nsw i32 %i.08.unr, 3, !dbg !21
  %mul.epil.3 = mul nsw i32 %add.epil.2, %conv, !dbg !19
  %add.epil.3 = add nsw i32 %mul.epil.3, %inc.epil.2, !dbg !20
  %epil.iter.cmp.not.3 = icmp eq i32 %xtraiter, 4, !dbg !13
  br i1 %epil.iter.cmp.not.3, label %for.cond.cleanup, label %for.body.epil.4, !dbg !13, !llvm.loop !24

for.body.epil.4:                                  ; preds = %for.body.epil.3
  %inc.epil.3 = add nuw nsw i32 %i.08.unr, 4, !dbg !21
  %mul.epil.4 = mul nsw i32 %add.epil.3, %conv, !dbg !19
  %add.epil.4 = add nsw i32 %mul.epil.4, %inc.epil.3, !dbg !20
  %epil.iter.cmp.not.4 = icmp eq i32 %xtraiter, 5, !dbg !13
  br i1 %epil.iter.cmp.not.4, label %for.cond.cleanup, label %for.body.epil.5, !dbg !13, !llvm.loop !24

for.body.epil.5:                                  ; preds = %for.body.epil.4
  %inc.epil.4 = add nuw nsw i32 %i.08.unr, 5, !dbg !21
  %mul.epil.5 = mul nsw i32 %add.epil.4, %conv, !dbg !19
  %add.epil.5 = add nsw i32 %mul.epil.5, %inc.epil.4, !dbg !20
  %epil.iter.cmp.not.5 = icmp eq i32 %xtraiter, 6, !dbg !13
  br i1 %epil.iter.cmp.not.5, label %for.cond.cleanup, label %for.body.epil.6, !dbg !13, !llvm.loop !24

for.body.epil.6:                                  ; preds = %for.body.epil.5
  %inc.epil.5 = add nuw nsw i32 %i.08.unr, 6, !dbg !21
  %mul.epil.6 = mul nsw i32 %add.epil.5, %conv, !dbg !19
  %add.epil.6 = add nsw i32 %mul.epil.6, %inc.epil.5, !dbg !20
  br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.body.epil, %for.body.epil.1, %for.body.epil.2, %for.body.epil.3, %for.body.epil.4, %for.body.epil.5, %for.body.epil.6, %for.cond.cleanup.loopexit.unr-lcssa, %entry
  %sum.0.lcssa = phi i32 [ 0, %entry ], [ %add.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %add.epil, %for.body.epil ], [ %add.epil.1, %for.body.epil.1 ], [ %add.epil.2, %for.body.epil.2 ], [ %add.epil.3, %for.body.epil.3 ], [ %add.epil.4, %for.body.epil.4 ], [ %add.epil.5, %for.body.epil.5 ], [ %add.epil.6, %for.body.epil.6 ], !dbg !29
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %sum.0.lcssa), !dbg !30
  ret i32 0, !dbg !31

for.body:                                         ; preds = %for.body, %for.body.lr.ph.new.new
  %i.08 = phi i32 [ 0, %for.body.lr.ph.new.new ], [ %inc.7.3, %for.body ]
  %sum.07 = phi i32 [ 0, %for.body.lr.ph.new.new ], [ %add.7.3, %for.body ]
  %niter24 = phi i32 [ 0, %for.body.lr.ph.new.new ], [ %niter24.next.3, %for.body ]
  %mul = mul nsw i32 %sum.07, %conv, !dbg !19
  %add = add nsw i32 %mul, %i.08, !dbg !20
  %inc = or i32 %i.08, 1, !dbg !21
  %mul.1 = mul nsw i32 %add, %conv, !dbg !19
  %add.1 = add nsw i32 %mul.1, %inc, !dbg !20
  %inc.1 = or i32 %i.08, 2, !dbg !21
  %mul.2 = mul nsw i32 %add.1, %conv, !dbg !19
  %add.2 = add nsw i32 %mul.2, %inc.1, !dbg !20
  %inc.2 = or i32 %i.08, 3, !dbg !21
  %mul.3 = mul nsw i32 %add.2, %conv, !dbg !19
  %add.3 = add nsw i32 %mul.3, %inc.2, !dbg !20
  %inc.3 = or i32 %i.08, 4, !dbg !21
  %mul.4 = mul nsw i32 %add.3, %conv, !dbg !19
  %add.4 = add nsw i32 %mul.4, %inc.3, !dbg !20
  %inc.4 = or i32 %i.08, 5, !dbg !21
  %mul.5 = mul nsw i32 %add.4, %conv, !dbg !19
  %add.5 = add nsw i32 %mul.5, %inc.4, !dbg !20
  %inc.5 = or i32 %i.08, 6, !dbg !21
  %mul.6 = mul nsw i32 %add.5, %conv, !dbg !19
  %add.6 = add nsw i32 %mul.6, %inc.5, !dbg !20
  %inc.6 = or i32 %i.08, 7, !dbg !21
  %mul.7 = mul nsw i32 %add.6, %conv, !dbg !19
  %add.7 = add nsw i32 %mul.7, %inc.6, !dbg !20
  %inc.7 = or i32 %i.08, 8, !dbg !21
  %mul.125 = mul nsw i32 %add.7, %conv, !dbg !19
  %add.126 = add nsw i32 %mul.125, %inc.7, !dbg !20
  %inc.127 = or i32 %i.08, 9, !dbg !21
  %mul.1.1 = mul nsw i32 %add.126, %conv, !dbg !19
  %add.1.1 = add nsw i32 %mul.1.1, %inc.127, !dbg !20
  %inc.1.1 = or i32 %i.08, 10, !dbg !21
  %mul.2.1 = mul nsw i32 %add.1.1, %conv, !dbg !19
  %add.2.1 = add nsw i32 %mul.2.1, %inc.1.1, !dbg !20
  %inc.2.1 = or i32 %i.08, 11, !dbg !21
  %mul.3.1 = mul nsw i32 %add.2.1, %conv, !dbg !19
  %add.3.1 = add nsw i32 %mul.3.1, %inc.2.1, !dbg !20
  %inc.3.1 = or i32 %i.08, 12, !dbg !21
  %mul.4.1 = mul nsw i32 %add.3.1, %conv, !dbg !19
  %add.4.1 = add nsw i32 %mul.4.1, %inc.3.1, !dbg !20
  %inc.4.1 = or i32 %i.08, 13, !dbg !21
  %mul.5.1 = mul nsw i32 %add.4.1, %conv, !dbg !19
  %add.5.1 = add nsw i32 %mul.5.1, %inc.4.1, !dbg !20
  %inc.5.1 = or i32 %i.08, 14, !dbg !21
  %mul.6.1 = mul nsw i32 %add.5.1, %conv, !dbg !19
  %add.6.1 = add nsw i32 %mul.6.1, %inc.5.1, !dbg !20
  %inc.6.1 = or i32 %i.08, 15, !dbg !21
  %mul.7.1 = mul nsw i32 %add.6.1, %conv, !dbg !19
  %add.7.1 = add nsw i32 %mul.7.1, %inc.6.1, !dbg !20
  %inc.7.1 = or i32 %i.08, 16, !dbg !21
  %mul.228 = mul nsw i32 %add.7.1, %conv, !dbg !19
  %add.229 = add nsw i32 %mul.228, %inc.7.1, !dbg !20
  %inc.230 = or i32 %i.08, 17, !dbg !21
  %mul.1.2 = mul nsw i32 %add.229, %conv, !dbg !19
  %add.1.2 = add nsw i32 %mul.1.2, %inc.230, !dbg !20
  %inc.1.2 = or i32 %i.08, 18, !dbg !21
  %mul.2.2 = mul nsw i32 %add.1.2, %conv, !dbg !19
  %add.2.2 = add nsw i32 %mul.2.2, %inc.1.2, !dbg !20
  %inc.2.2 = or i32 %i.08, 19, !dbg !21
  %mul.3.2 = mul nsw i32 %add.2.2, %conv, !dbg !19
  %add.3.2 = add nsw i32 %mul.3.2, %inc.2.2, !dbg !20
  %inc.3.2 = or i32 %i.08, 20, !dbg !21
  %mul.4.2 = mul nsw i32 %add.3.2, %conv, !dbg !19
  %add.4.2 = add nsw i32 %mul.4.2, %inc.3.2, !dbg !20
  %inc.4.2 = or i32 %i.08, 21, !dbg !21
  %mul.5.2 = mul nsw i32 %add.4.2, %conv, !dbg !19
  %add.5.2 = add nsw i32 %mul.5.2, %inc.4.2, !dbg !20
  %inc.5.2 = or i32 %i.08, 22, !dbg !21
  %mul.6.2 = mul nsw i32 %add.5.2, %conv, !dbg !19
  %add.6.2 = add nsw i32 %mul.6.2, %inc.5.2, !dbg !20
  %inc.6.2 = or i32 %i.08, 23, !dbg !21
  %mul.7.2 = mul nsw i32 %add.6.2, %conv, !dbg !19
  %add.7.2 = add nsw i32 %mul.7.2, %inc.6.2, !dbg !20
  %inc.7.2 = or i32 %i.08, 24, !dbg !21
  %mul.331 = mul nsw i32 %add.7.2, %conv, !dbg !19
  %add.332 = add nsw i32 %mul.331, %inc.7.2, !dbg !20
  %inc.333 = or i32 %i.08, 25, !dbg !21
  %mul.1.3 = mul nsw i32 %add.332, %conv, !dbg !19
  %add.1.3 = add nsw i32 %mul.1.3, %inc.333, !dbg !20
  %inc.1.3 = or i32 %i.08, 26, !dbg !21
  %mul.2.3 = mul nsw i32 %add.1.3, %conv, !dbg !19
  %add.2.3 = add nsw i32 %mul.2.3, %inc.1.3, !dbg !20
  %inc.2.3 = or i32 %i.08, 27, !dbg !21
  %mul.3.3 = mul nsw i32 %add.2.3, %conv, !dbg !19
  %add.3.3 = add nsw i32 %mul.3.3, %inc.2.3, !dbg !20
  %inc.3.3 = or i32 %i.08, 28, !dbg !21
  %mul.4.3 = mul nsw i32 %add.3.3, %conv, !dbg !19
  %add.4.3 = add nsw i32 %mul.4.3, %inc.3.3, !dbg !20
  %inc.4.3 = or i32 %i.08, 29, !dbg !21
  %mul.5.3 = mul nsw i32 %add.4.3, %conv, !dbg !19
  %add.5.3 = add nsw i32 %mul.5.3, %inc.4.3, !dbg !20
  %inc.5.3 = or i32 %i.08, 30, !dbg !21
  %mul.6.3 = mul nsw i32 %add.5.3, %conv, !dbg !19
  %add.6.3 = add nsw i32 %mul.6.3, %inc.5.3, !dbg !20
  %inc.6.3 = or i32 %i.08, 31, !dbg !21
  %mul.7.3 = mul nsw i32 %add.6.3, %conv, !dbg !19
  %add.7.3 = add nsw i32 %mul.7.3, %inc.6.3, !dbg !20
  %inc.7.3 = add nuw nsw i32 %i.08, 32, !dbg !21
  %niter24.next.3 = add i32 %niter24, 4, !dbg !13
  %niter24.ncmp.3 = icmp eq i32 %niter24.next.3, %unroll_iter23, !dbg !13
  br i1 %niter24.ncmp.3, label %for.cond.cleanup.loopexit.unr-lcssa.loopexit.unr-lcssa, label %for.body, !dbg !13, !llvm.loop !32
}

; Function Attrs: nofree nounwind
declare dso_local noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { nofree nounwind "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-006.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "56e088c4cb7c8944c19f91107daad627")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)"}
!8 = distinct !DISubprogram(name: "pipeline_enable", scope: !9, file: !9, line: 3, type: !10, scopeLine: 3, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!9 = !DIFile(filename: "./pragma-loop-pipeline-006.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "56e088c4cb7c8944c19f91107daad627")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 8, column: 20, scope: !8)
!13 = !DILocation(line: 8, column: 5, scope: !8)
!14 = !{!15, !15, i64 0}
!15 = !{!"any pointer", !16, i64 0}
!16 = !{!"omnipotent char", !17, i64 0}
!17 = !{!"Simple C/C++ TBAA"}
!18 = !{!16, !16, i64 0}
!19 = !DILocation(line: 9, column: 12, scope: !8)
!20 = !DILocation(line: 10, column: 12, scope: !8)
!21 = !DILocation(line: 8, column: 30, scope: !8)
!22 = distinct !{!22, !23, !28}
!23 = !{!"llvm.loop.unroll.disable"}
!24 = distinct !{!24, !25}
!25 = distinct !{!25, !13, !26, !27, !23, !28}
!26 = !DILocation(line: 11, column: 5, scope: !8)
!27 = !{!"llvm.loop.mustprogress"}
!28 = !{!"llvm.loop.pipeline.disable", i1 true}
!29 = !DILocation(line: 0, scope: !8)
!30 = !DILocation(line: 12, column: 2, scope: !8)
!31 = !DILocation(line: 13, column: 2, scope: !8)
!32 = distinct !{!32, !25}
