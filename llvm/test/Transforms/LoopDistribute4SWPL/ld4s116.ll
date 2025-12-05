; RUN: opt %s -O1 -S -mcpu=a64fx -pass-remarks=loop-distribute4swpl -pass-remarks-missed=loop-distribute4swpl -pass-remarks-analysis=loop-distribute4swpl -enable-loop-distribute4swpl -distribute4swpl-limit-freg=1 -distribute4swpl-limit-ireg=1 -print-after=loop-distribute4swpl 2>&1 | FileCheck %s
; CHECK: for.cond.cleanup.loopexit:                        ; preds = %for.body
; CHECK-NEXT:  %add8.lcssa = phi float [ %add8, %for.body ]
; CHECK-NEXT:  br label %for.cond.cleanup
; CHECK-NOT:  poison

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4)
define dso_local float @target(i32 noundef %n, ptr noalias nocapture noundef %A, ptr noalias nocapture noundef readonly %B, ptr noalias nocapture noundef writeonly %C, ptr noalias nocapture noundef readonly %D, ptr noalias nocapture noundef readonly %E, ptr noalias nocapture noundef readonly %F, ptr noalias nocapture noundef readonly %G, ptr noalias nocapture noundef readnone %H) local_unnamed_addr #0 {
entry:
  %cmp30 = icmp sgt i32 %n, 0
  br i1 %cmp30, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext nneg i32 %n to i64
  %load_initial = load float, ptr %A, align 4
  br label %for.body.ldist1

for.body.ldist1:                                  ; preds = %for.body.ldist1, %for.body.preheader
  %store_forwarded = phi float [ %load_initial, %for.body.preheader ], [ %2, %for.body.ldist1 ]
  %indvars.iv.ldist1 = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next.ldist1, %for.body.ldist1 ]
  %arrayidx2.ldist1 = getelementptr inbounds nuw float, ptr %B, i64 %indvars.iv.ldist1
  %0 = load float, ptr %arrayidx2.ldist1, align 4, !tbaa !6
  %arrayidx4.ldist1 = getelementptr inbounds nuw float, ptr %F, i64 %indvars.iv.ldist1
  %1 = load float, ptr %arrayidx4.ldist1, align 4, !tbaa !6
  %2 = tail call float @llvm.fmuladd.f32(float %store_forwarded, float %0, float %1)
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1
  %arrayidx6.ldist1 = getelementptr inbounds nuw float, ptr %A, i64 %indvars.iv.next.ldist1
  store float %2, ptr %arrayidx6.ldist1, align 4, !tbaa !6
  %exitcond.not.ldist1 = icmp eq i64 %indvars.iv.next.ldist1, %wide.trip.count
  br i1 %exitcond.not.ldist1, label %for.body.ldist2, label %for.body.ldist1, !llvm.loop !10

for.body.ldist2:                                  ; preds = %for.body.ldist1, %for.body.ldist2
  %indvars.iv.ldist2 = phi i64 [ %indvars.iv.next.ldist2, %for.body.ldist2 ], [ 0, %for.body.ldist1 ]
  %indvars.iv.next.ldist2 = add nuw nsw i64 %indvars.iv.ldist2, 1
  %arrayidx10.ldist2 = getelementptr inbounds nuw float, ptr %D, i64 %indvars.iv.ldist2
  %3 = load float, ptr %arrayidx10.ldist2, align 4, !tbaa !6
  %arrayidx12.ldist2 = getelementptr inbounds nuw float, ptr %E, i64 %indvars.iv.ldist2
  %4 = load float, ptr %arrayidx12.ldist2, align 4, !tbaa !6
  %arrayidx14.ldist2 = getelementptr inbounds nuw float, ptr %G, i64 %indvars.iv.ldist2
  %5 = load float, ptr %arrayidx14.ldist2, align 4, !tbaa !6
  %6 = tail call float @llvm.fmuladd.f32(float %3, float %4, float %5)
  %arrayidx16.ldist2 = getelementptr inbounds nuw float, ptr %C, i64 %indvars.iv.ldist2
  store float %6, ptr %arrayidx16.ldist2, align 4, !tbaa !6
  %exitcond.not.ldist2 = icmp eq i64 %indvars.iv.next.ldist2, %wide.trip.count
  br i1 %exitcond.not.ldist2, label %for.body, label %for.body.ldist2, !llvm.loop !14

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %tmp1.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add8, %for.body ]
  ret float %tmp1.0.lcssa

for.body:                                         ; preds = %for.body.ldist2, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.ldist2 ]
  %tmp1.031 = phi float [ %add8, %for.body ], [ 0.000000e+00, %for.body.ldist2 ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %7 = trunc i64 %indvars.iv to i32
  %8 = add i32 %n, %7
  %conv = sitofp i32 %8 to float
  %add8 = fadd float %tmp1.031, %conv
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop !16
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+complxnum,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+perfmon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 20.1.8 ()"}
!6 = !{!7, !7, i64 0}
!7 = !{!"float", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = distinct !{!10, !11, !12, !13}
!11 = !{!"llvm.loop.mustprogress"}
!12 = !{!"llvm.loop.unroll.disable"}
!13 = !{!"llvm.loop.distributed4swpl", i32 3, i32 2}
!14 = distinct !{!14, !11, !12, !15}
!15 = !{!"llvm.loop.distributed4swpl", i32 3, i32 1}
!16 = distinct !{!16, !11, !12, !17}
!17 = !{!"llvm.loop.distributed4swpl", i32 3, i32 3}
