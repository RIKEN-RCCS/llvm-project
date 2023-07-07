; RUN: llc < %s -O1 -mcpu=a64fx  -fswp  -swpl-debug  --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed=aarch64-swpipeliner  -o /dev/null 2>&1 | FileCheck %s
;CHECK:canPipelineLoop:OK
; ModuleID = '2912_2.c'
source_filename = "2912_2.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @foo(i32 noundef %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %1, %5
  %3 = phi i64 [ 0, %1 ], [ %6, %5 ]
  br label %8

4:                                                ; preds = %5
  ret void

5:                                                ; preds = %8
  %6 = add nuw nsw i64 %3, 1
  %7 = icmp eq i64 %6, 100
  br i1 %7, label %4, label %2, !llvm.loop !6

8:                                                ; preds = %2, %8
  %9 = phi i64 [ 0, %2 ], [ %16, %8 ]
  %10 = getelementptr inbounds [100 x [100 x double]], ptr @b, i64 0, i64 %3, i64 %9
  %11 = load double, ptr %10, align 8, !tbaa !9
  %12 = getelementptr inbounds [100 x [100 x double]], ptr @c, i64 0, i64 %3, i64 %9
  %13 = load double, ptr %12, align 8, !tbaa !9
  %14 = fadd double %11, %13
  %15 = getelementptr inbounds [100 x [100 x double]], ptr @a, i64 0, i64 %3, i64 %9
  store double %14, ptr %15, align 8, !tbaa !9
  %16 = add nuw nsw i64 %9, 1
  %17 = icmp eq i64 %16, 100
  br i1 %17, label %5, label %8, !llvm.loop !13
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.mustprogress"}
!8 = !{!"llvm.loop.unroll.disable"}
!9 = !{!10, !10, i64 0}
!10 = !{!"double", !11, i64 0}
!11 = !{!"omnipotent char", !12, i64 0}
!12 = !{!"Simple C/C++ TBAA"}
!13 = distinct !{!13, !7, !8}
