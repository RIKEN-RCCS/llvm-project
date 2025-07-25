; RUN: llc < %s -mcpu=a64fx -O1  -fswp  -disable-hwloop  -debug-aarch64tti --pass-remarks-filter=hardware-loops  -o - 2>&1 | FileCheck %s
; CHECK-NOT: DBG
; ModuleID = '2901-2-13.c'
source_filename = "2901-2-13.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@A = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4
@B = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %7

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !6
  %6 = zext i32 %0 to i64
  br label %10

7:                                                ; preds = %10, %2
  %8 = phi double [ 0.000000e+00, %2 ], [ %19, %10 ]
  %9 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, double noundef %8)
  ret i32 0

10:                                               ; preds = %4, %10
  %11 = phi i64 [ 0, %4 ], [ %20, %10 ]
  %12 = phi double [ 0.000000e+00, %4 ], [ %19, %10 ]
  %13 = getelementptr inbounds i8, ptr %5, i64 %11
  %14 = load i8, ptr %13, align 1, !tbaa !10
  %15 = zext i8 %14 to i32
  %16 = trunc i64 %11 to i32
  %17 = mul nsw i32 %16, %15
  %18 = sitofp i32 %17 to double
  %19 = fadd double %12, %18
  %20 = add nuw nsw i64 %11, 1
  %21 = icmp eq i64 %20, %6
  br i1 %21, label %7, label %10, !llvm.loop !11
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
