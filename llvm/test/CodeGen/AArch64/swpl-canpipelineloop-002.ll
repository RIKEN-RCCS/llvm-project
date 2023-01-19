; RUN: llc < %s -mcpu=a64fx -O0 -swpl-debug -fswp --pass-remarks-missed=aarch64-swpipeliner --pass-remarks-filter=aarch64-swpipeliner --pass-remarks-output=- -debug-aarch64tti -o /dev/null 2>&1 | FileCheck --allow-empty %s
;CHECK-NOT: canPipelineLoop

; ModuleID = '2912.c'
source_filename = "2912.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local global [100 x double] zeroinitializer, align 8
@c = dso_local global [100 x double] zeroinitializer, align 8
@a = dso_local global [100 x double] zeroinitializer, align 8

; Function Attrs: noinline nounwind optnone uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  br label %4

4:                                                ; preds = %21, %1
  %5 = load i32, ptr %3, align 4
  %6 = load i32, ptr %2, align 4
  %7 = icmp slt i32 %5, %6
  br i1 %7, label %8, label %24

8:                                                ; preds = %4
  %9 = load i32, ptr %3, align 4
  %10 = sext i32 %9 to i64
  %11 = getelementptr inbounds [100 x double], ptr @b, i64 0, i64 %10
  %12 = load double, ptr %11, align 8
  %13 = load i32, ptr %3, align 4
  %14 = sext i32 %13 to i64
  %15 = getelementptr inbounds [100 x double], ptr @c, i64 0, i64 %14
  %16 = load double, ptr %15, align 8
  %17 = fadd double %12, %16
  %18 = load i32, ptr %3, align 4
  %19 = sext i32 %18 to i64
  %20 = getelementptr inbounds [100 x double], ptr @a, i64 0, i64 %19
  store double %17, ptr %20, align 8
  br label %21

21:                                               ; preds = %8
  %22 = load i32, ptr %3, align 4
  %23 = add nsw i32 %22, 1
  store i32 %23, ptr %3, align 4
  br label %4, !llvm.loop !6

24:                                               ; preds = %4
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
