; RUN: opt %s -O1 -S -mcpu=a64fx -pass-remarks=loop-distribute4swpl -pass-remarks-missed=loop-distribute4swpl -pass-remarks-analysis=loop-distribute4swpl -enable-loop-distribute4swpl -distribute4swpl-limit-freg=1 -distribute4swpl-limit-ireg=1 -print-after=loop-distribute4swpl 2>&1 | FileCheck %s
; CHECK: for.cond.cleanup.loopexit:                        ; preds = %for.body
; CHECK-NEXT:  %mul13.lcssa = phi float [ %mul13, %for.body ]
; CHECK-NEXT:  br label %for.cond.cleanup
; CHECK-NOT:  poison

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4)
define dso_local float @target(i32 noundef %n, ptr noalias nocapture noundef %A, ptr noalias nocapture noundef writeonly %B, ptr noalias nocapture noundef writeonly %C, ptr noalias nocapture noundef readonly %D, ptr noalias nocapture noundef readonly %E, ptr noalias nocapture noundef readonly %F) local_unnamed_addr #0 {
entry:
  %cmp31 = icmp sgt i32 %n, 0
  br i1 %cmp31, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext nneg i32 %n to i64
  %load_initial = load float, ptr %A, align 4
  br label %for.body.ldist1

for.body.ldist1:                                  ; preds = %for.body.ldist1, %for.body.preheader
  %store_forwarded = phi float [ %load_initial, %for.body.preheader ], [ %mul.ldist1, %for.body.ldist1 ]
  %indvars.iv.ldist1 = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next.ldist1, %for.body.ldist1 ]
  %arrayidx2.ldist1 = getelementptr inbounds nuw float, ptr %D, i64 %indvars.iv.ldist1
  %0 = load float, ptr %arrayidx2.ldist1, align 4, !tbaa !6
  %mul.ldist1 = fmul float %store_forwarded, %0
  %arrayidx4.ldist1 = getelementptr inbounds nuw float, ptr %B, i64 %indvars.iv.ldist1
  store float %mul.ldist1, ptr %arrayidx4.ldist1, align 4, !tbaa !6
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1
  %arrayidx8.ldist1 = getelementptr inbounds nuw float, ptr %A, i64 %indvars.iv.next.ldist1
  store float %mul.ldist1, ptr %arrayidx8.ldist1, align 4, !tbaa !6
  %exitcond.not.ldist1 = icmp eq i64 %indvars.iv.next.ldist1, %wide.trip.count
  br i1 %exitcond.not.ldist1, label %for.body, label %for.body.ldist1, !llvm.loop !10

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %tmp1.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %mul13, %for.body ]
  ret float %tmp1.0.lcssa

for.body:                                         ; preds = %for.body.ldist1, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.ldist1 ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx10 = getelementptr inbounds nuw float, ptr %E, i64 %indvars.iv
  %1 = load float, ptr %arrayidx10, align 4, !tbaa !6
  %arrayidx12 = getelementptr inbounds nuw float, ptr %F, i64 %indvars.iv
  %2 = load float, ptr %arrayidx12, align 4, !tbaa !6
  %mul13 = fmul float %1, %2
  %arrayidx15 = getelementptr inbounds nuw float, ptr %C, i64 %indvars.iv
  store float %mul13, ptr %arrayidx15, align 4, !tbaa !6
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop !14
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+complxnum,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+perfmon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }

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
!13 = !{!"llvm.loop.distributed4swpl", i32 2, i32 1}
!14 = distinct !{!14, !11, !12, !15}
!15 = !{!"llvm.loop.distributed4swpl", i32 2, i32 2}
