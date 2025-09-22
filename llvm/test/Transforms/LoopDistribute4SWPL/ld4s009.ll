; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop. num of distributied is 2.

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %0, ptr noundef %1, ptr noundef %2, ptr noundef %3, ptr noundef %4, ptr noundef %5, ptr noundef %6, ptr noundef %7, ptr noundef %8) #0 {
  %10 = alloca i32, align 4
  %11 = alloca ptr, align 8
  %12 = alloca ptr, align 8
  %13 = alloca ptr, align 8
  %14 = alloca ptr, align 8
  %15 = alloca ptr, align 8
  %16 = alloca ptr, align 8
  %17 = alloca ptr, align 8
  %18 = alloca ptr, align 8
  %19 = alloca i32, align 4
  store i32 %0, ptr %10, align 4
  store ptr %1, ptr %11, align 8
  store ptr %2, ptr %12, align 8
  store ptr %3, ptr %13, align 8
  store ptr %4, ptr %14, align 8
  store ptr %5, ptr %15, align 8
  store ptr %6, ptr %16, align 8
  store ptr %7, ptr %17, align 8
  store ptr %8, ptr %18, align 8
  store i32 0, ptr %19, align 4
  br label %20

20:                                               ; preds = %63, %9
  %21 = load i32, ptr %19, align 4
  %22 = load i32, ptr %10, align 4
  %23 = sub nsw i32 %22, 2
  %24 = icmp slt i32 %21, %23
  br i1 %24, label %25, label %66

25:                                               ; preds = %20
  %26 = load ptr, ptr %13, align 8
  %27 = load i32, ptr %19, align 4
  %28 = sext i32 %27 to i64
  %29 = getelementptr inbounds float, ptr %26, i64 %28
  %30 = load float, ptr %29, align 4
  %31 = load ptr, ptr %15, align 8
  %32 = load i32, ptr %19, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds float, ptr %31, i64 %33
  %35 = load float, ptr %34, align 4
  %36 = fadd float %30, %35
  %37 = load ptr, ptr %11, align 8
  %38 = load i32, ptr %19, align 4
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, ptr %37, i64 %39
  store float %36, ptr %40, align 4
  %41 = load ptr, ptr %17, align 8
  %42 = load i32, ptr %19, align 4
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds float, ptr %41, i64 %43
  %45 = load float, ptr %44, align 4
  %46 = load ptr, ptr %13, align 8
  %47 = load i32, ptr %19, align 4
  %48 = add nsw i32 %47, 1
  %49 = sext i32 %48 to i64
  %50 = getelementptr inbounds float, ptr %46, i64 %49
  store float %45, ptr %50, align 4
  %51 = load ptr, ptr %12, align 8
  %52 = getelementptr inbounds float, ptr %51, i64 1
  %53 = load float, ptr %52, align 4
  %54 = load ptr, ptr %18, align 8
  %55 = load i32, ptr %19, align 4
  %56 = srem i32 %55, 10
  %57 = sext i32 %56 to i64
  %58 = getelementptr inbounds float, ptr %54, i64 %57
  %59 = load float, ptr %58, align 4
  %60 = fmul float %53, %59
  %61 = load ptr, ptr %12, align 8
  %62 = getelementptr inbounds float, ptr %61, i64 0
  store float %60, ptr %62, align 4
  br label %63

63:                                               ; preds = %25
  %64 = load i32, ptr %19, align 4
  %65 = add nsw i32 %64, 1
  store i32 %65, ptr %19, align 4
  br label %20, !llvm.loop !6

66:                                               ; preds = %20
  ret void
}

attributes #0 = { noinline nounwind  uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 18.1.8"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
