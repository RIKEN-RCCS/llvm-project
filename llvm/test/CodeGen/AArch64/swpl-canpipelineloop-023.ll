; RUN: llc < %s -O1 -mcpu=a64fx  -fswp  -swpl-debug  -o /dev/null 2>&1 | FileCheck %s
;CHECK:not found (BCC || SUBSXri)
; ModuleID = '2912_inf_loop.c'
source_filename = "2912_inf_loop.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8

; Function Attrs: nofree norecurse noreturn nosync nounwind uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0, i32 noundef %1) local_unnamed_addr #0 {
  %3 = sext i32 %1 to i64
  %4 = getelementptr inbounds [100 x double], ptr @b, i64 0, i64 %3
  %5 = load double, ptr %4, align 8, !tbaa !6
  %6 = getelementptr inbounds [100 x double], ptr @c, i64 0, i64 %3
  %7 = load double, ptr %6, align 8, !tbaa !6
  %8 = fadd double %5, %7
  %9 = getelementptr inbounds [100 x double], ptr @a, i64 0, i64 %3
  store double %8, ptr %9, align 8, !tbaa !6
  br label %10

10:                                               ; preds = %10, %2
  br label %10, !llvm.loop !10
}

attributes #0 = { nofree norecurse noreturn nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

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
!10 = distinct !{!10, !11}
!11 = !{!"llvm.loop.unroll.disable"}
