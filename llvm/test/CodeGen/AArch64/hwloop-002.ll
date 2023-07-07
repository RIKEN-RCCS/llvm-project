; RUN: llc < %s -mcpu=a64fx -O1 -fswp --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: (reason=Multiple BasicBlocks)

; ModuleID = '2901-2-02.c'
source_filename = "2901-2-02.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %21

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !6
  %6 = load i8, ptr %5, align 1, !tbaa !10
  %7 = zext i8 %6 to i32
  br label %11

8:                                                ; preds = %11
  %9 = mul nsw i32 %14, %7
  %10 = icmp sgt i32 %9, 1024
  br i1 %10, label %17, label %11, !llvm.loop !11

11:                                               ; preds = %4, %8
  %12 = phi i32 [ 0, %4 ], [ %9, %8 ]
  %13 = phi i32 [ 0, %4 ], [ %15, %8 ]
  %14 = add nsw i32 %12, %13
  %15 = add nuw nsw i32 %13, 1
  %16 = icmp eq i32 %15, %0
  br i1 %16, label %17, label %8, !llvm.loop !11

17:                                               ; preds = %8, %11
  %18 = phi i32 [ 0, %11 ], [ 1, %8 ]
  %19 = phi i32 [ %14, %11 ], [ %9, %8 ]
  %20 = icmp slt i32 %15, %0
  br label %21

21:                                               ; preds = %17, %2
  %22 = phi i1 [ %3, %2 ], [ %20, %17 ]
  %23 = phi i32 [ 0, %2 ], [ %18, %17 ]
  %24 = phi i32 [ 0, %2 ], [ %19, %17 ]
  br i1 %22, label %27, label %25

25:                                               ; preds = %21
  %26 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %24)
  br label %27

27:                                               ; preds = %21, %25
  %28 = phi i32 [ 0, %25 ], [ %23, %21 ]
  ret i32 %28
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
