; RUN: llc < %s -O1 -mcpu=a64fx  -fswp  -swpl-debug  --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed=aarch64-swpipeliner --pass-remarks-output=- -o /dev/null 2>&1 | FileCheck %s
;CHECK:canPipelineLoop:NG
;CHECK:Not a single basic block.
;CHECK:This loop cannot be software pipelined because the shape of the loop is not covered by software pipelining
  
; ModuleID = '2912_3.c'
source_filename = "2912_3.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [1000 x double] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [1000 x double] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [1000 x double] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @foo(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %20

4:                                                ; preds = %2
  %5 = zext i32 %0 to i64
  br label %6

6:                                                ; preds = %4, %11
  %7 = phi i64 [ 0, %4 ], [ %18, %11 ]
  %8 = getelementptr inbounds i32, ptr %1, i64 %7
  %9 = load i32, ptr %8, align 4, !tbaa !6
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %20, label %11

11:                                               ; preds = %6
  %12 = getelementptr inbounds [1000 x double], ptr @b, i64 0, i64 %7
  %13 = load double, ptr %12, align 8, !tbaa !10
  %14 = getelementptr inbounds [1000 x double], ptr @c, i64 0, i64 %7
  %15 = load double, ptr %14, align 8, !tbaa !10
  %16 = fadd double %13, %15
  %17 = getelementptr inbounds [1000 x double], ptr @a, i64 0, i64 %7
  store double %16, ptr %17, align 8, !tbaa !10
  %18 = add nuw nsw i64 %7, 1
  %19 = icmp eq i64 %18, %5
  br i1 %19, label %20, label %6, !llvm.loop !12

20:                                               ; preds = %11, %6, %2
  store double 1.000000e+00, ptr @a, align 8, !tbaa !10
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!11, !11, i64 0}
!11 = !{!"double", !8, i64 0}
!12 = distinct !{!12, !13, !14}
!13 = !{!"llvm.loop.mustprogress"}
!14 = !{!"llvm.loop.unroll.disable"}
