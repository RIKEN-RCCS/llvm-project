; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: (reason=CALL or ASM exists)

; ModuleID = '2901-2-08.c'
source_filename = "2901-2-08.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %6

4:                                                ; preds = %2
  %5 = zext i32 %0 to i64
  br label %9

6:                                                ; preds = %9, %2
  %7 = phi i32 [ 0, %2 ], [ %22, %9 ]
  %8 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %7)
  ret i32 0

9:                                                ; preds = %4, %9
  %10 = phi i64 [ 0, %4 ], [ %25, %9 ]
  %11 = phi i32 [ 0, %4 ], [ %22, %9 ]
  %12 = load ptr, ptr %1, align 8, !tbaa !6
  %13 = getelementptr inbounds i8, ptr %12, i64 %10
  %14 = load i8, ptr %13, align 1, !tbaa !10
  %15 = zext i8 %14 to i32
  %16 = trunc i64 %10 to i32
  %17 = add nsw i32 %16, %0
  %18 = trunc i64 %10 to i32
  %19 = shl i32 %18, 1
  %20 = mul i32 %19, %17
  %21 = mul i32 %20, %15
  %22 = add nsw i32 %21, %11
  %23 = trunc i64 %10 to i32
  %24 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %23)
  %25 = add nuw nsw i64 %10, 1
  %26 = icmp eq i64 %25, %5
  br i1 %26, label %6, label %9, !llvm.loop !11
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn uwtable vscale_range(1,16)
define dso_local i32 @sub(i32 noundef %0, i32 noundef %1) local_unnamed_addr #1 {
  %3 = shl nsw i32 %0, 1
  %4 = mul nsw i32 %3, %1
  ret i32 %4
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { mustprogress nofree norecurse nosync nounwind readnone willreturn uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

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
