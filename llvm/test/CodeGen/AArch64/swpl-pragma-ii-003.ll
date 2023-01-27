; RUN: llc < %s -O1 -mcpu=a64fx -fswp -swpl-debug -swpl-maxii=10 --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed=aarch64-swpipeliner  -o /dev/null 2>&1 | FileCheck %s
;CHECK:Iterative Modulo Scheduling. ResMII 4. NumOfBodyInsts 12. Budget 60. II 5. Minimum II = 4.
;CHECK:Loop is software pipelined. (ii=5
; ModuleID = '2912.c'
source_filename = "2912.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0) local_unnamed_addr #0 {
  %2 = icmp sgt i32 %0, 0
  br i1 %2, label %3, label %5

3:                                                ; preds = %1
  %4 = zext i32 %0 to i64
  br label %6

5:                                                ; preds = %6, %1
  ret void

6:                                                ; preds = %3, %6
  %7 = phi i64 [ 0, %3 ], [ %14, %6 ]
  %8 = getelementptr inbounds [100 x double], ptr @b, i64 0, i64 %7
  %9 = load double, ptr %8, align 8, !tbaa !6
  %10 = getelementptr inbounds [100 x double], ptr @c, i64 0, i64 %7
  %11 = load double, ptr %10, align 8, !tbaa !6
  %12 = fadd double %9, %11
  %13 = getelementptr inbounds [100 x double], ptr @a, i64 0, i64 %7
  store double %12, ptr %13, align 8, !tbaa !6
  %14 = add nuw nsw i64 %7, 1
  %15 = icmp eq i64 %14, %4
  br i1 %15, label %5, label %6, !llvm.loop !10
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!7, !7, i64 0}
!7 = !{!"double", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = distinct !{!10, !11, !12, !13}
!11 = !{!"llvm.loop.mustprogress"}
!12 = !{!"llvm.loop.unroll.disable"}
!13 = !{!"llvm.loop.pipeline.initiationinterval", i32 5}
