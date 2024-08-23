; RUN: llc < %s -mcpu=a64fx -O1 -fswp --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: hardware-loop created

; ModuleID = '2901-2-04.c'
source_filename = "2901-2-04.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %10

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !6
  %6 = load i8, ptr %5, align 1, !tbaa !10
  %7 = zext i8 %6 to i32
  %8 = zext i32 %0 to i64
  %9 = zext i32 %0 to i64
  br label %13

10:                                               ; preds = %17, %2
  %11 = phi i32 [ 0, %2 ], [ %28, %17 ]
  %12 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %11)
  ret i32 0

13:                                               ; preds = %4, %17
  %14 = phi i64 [ 0, %4 ], [ %18, %17 ]
  %15 = phi i32 [ 0, %4 ], [ %28, %17 ]
  %16 = mul nsw i32 %15, %7
  br label %20

17:                                               ; preds = %20
  %18 = add nuw nsw i64 %14, 1
  %19 = icmp eq i64 %18, %8
  br i1 %19, label %10, label %13, !llvm.loop !11

20:                                               ; preds = %13, %20
  %21 = phi i64 [ %14, %13 ], [ %29, %20 ]
  %22 = phi i32 [ %16, %13 ], [ %28, %20 ]
  %23 = getelementptr inbounds i8, ptr %5, i64 %21
  %24 = load i8, ptr %23, align 1, !tbaa !10
  %25 = zext i8 %24 to i32
  %26 = mul nsw i32 %22, %25
  %27 = trunc i64 %21 to i32
  %28 = sub nsw i32 %26, %27
  %29 = add nuw nsw i64 %21, 1
  %30 = icmp eq i64 %29, %9
  br i1 %30, label %17, label %20, !llvm.loop !14
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!7, !7, i64 0}
!7 = !{!"any pointer", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!8, !8, i64 0}
!11 = distinct !{!11, !12, !13}
!12 = !{!"llvm.loop.mustprogress"}
!13 = !{!"llvm.loop.unroll.disable"}
!14 = distinct !{!14, !12, !13}
