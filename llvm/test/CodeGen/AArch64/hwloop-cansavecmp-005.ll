; RUN: llc < %s -mcpu=a64fx -O1 -fswp --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: canSaveCmo

; ModuleID = '2901-3-05.c'
source_filename = "2901-3-05.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = tail call i32 @sub() #3
  %4 = icmp sgt i32 %3, 0
  br i1 %4, label %5, label %16

5:                                                ; preds = %2
  %6 = icmp sgt i32 %0, 0
  %7 = zext i32 %0 to i64
  %8 = zext i32 %0 to i64
  br label %9

9:                                                ; preds = %5, %19
  %10 = phi i32 [ 0, %5 ], [ %21, %19 ]
  %11 = phi i32 [ 0, %5 ], [ %20, %19 ]
  br i1 %6, label %12, label %19

12:                                               ; preds = %9
  %13 = load ptr, ptr %1, align 8, !tbaa !6
  %14 = load i8, ptr %13, align 1, !tbaa !10
  %15 = zext i8 %14 to i64
  br label %24

16:                                               ; preds = %19, %2
  %17 = phi i32 [ 0, %2 ], [ %20, %19 ]
  %18 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %17)
  ret i32 0

19:                                               ; preds = %34, %9
  %20 = phi i32 [ %11, %9 ], [ %36, %34 ]
  %21 = add nuw nsw i32 %10, 1
  %22 = tail call i32 @sub() #3
  %23 = icmp slt i32 %21, %22
  br i1 %23, label %9, label %16, !llvm.loop !11

24:                                               ; preds = %12, %34
  %25 = phi i64 [ 0, %12 ], [ %37, %34 ]
  %26 = phi i32 [ %11, %12 ], [ %36, %34 ]
  %27 = add nuw nsw i64 %25, %15
  %28 = trunc i64 %27 to i32
  %29 = add nsw i32 %26, %28
  %30 = getelementptr inbounds i8, ptr %13, i64 %25
  %31 = load i8, ptr %30, align 1, !tbaa !10
  %32 = zext i8 %31 to i32
  %33 = mul nsw i32 %29, %32
  br label %39

34:                                               ; preds = %39
  %35 = trunc i64 %27 to i32
  %36 = add nsw i32 %47, %35
  %37 = add nuw nsw i64 %25, 1
  %38 = icmp eq i64 %37, %7
  br i1 %38, label %19, label %24, !llvm.loop !14

39:                                               ; preds = %24, %39
  %40 = phi i64 [ %25, %24 ], [ %48, %39 ]
  %41 = phi i32 [ %33, %24 ], [ %47, %39 ]
  %42 = getelementptr inbounds i8, ptr %13, i64 %40
  %43 = load i8, ptr %42, align 1, !tbaa !10
  %44 = zext i8 %43 to i32
  %45 = mul nsw i32 %41, %44
  %46 = trunc i64 %40 to i32
  %47 = sub nsw i32 %45, %46
  %48 = add nuw nsw i64 %40, 1
  %49 = icmp eq i64 %48, %8
  br i1 %49, label %34, label %39, !llvm.loop !15
}

declare i32 @sub(...) local_unnamed_addr #1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #3 = { nounwind }

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
!15 = distinct !{!15, !12, !13}
