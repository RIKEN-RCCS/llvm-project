; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks-missed=loop-dist -distribute4swpl-limit-ireg=1  < %s |& FileCheck %s
; CHECK: loop not distributed:

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

@aa1 = external global [100 x float], align 4
@cc1 = external global [100 x float], align 4
@bb1 = external global [100 x float], align 4
@dd1_org = external global [100 x float], align 4
@aa2 = external global [100 x float], align 4
@cc2 = external global [100 x float], align 4
@dd2_org = external global [100 x float], align 4

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  br label %4

4:                                                ; preds = %41, %1
  %5 = load i32, ptr %3, align 4
  %6 = load i32, ptr %2, align 4
  %7 = icmp slt i32 %5, %6
  br i1 %7, label %8, label %44

8:                                                ; preds = %4
  %9 = load i32, ptr %3, align 4
  %10 = sext i32 %9 to i64
  %11 = getelementptr inbounds [100 x float], ptr @aa1, i64 0, i64 %10
  %12 = load float, ptr %11, align 4
  %13 = load i32, ptr %3, align 4
  %14 = sext i32 %13 to i64
  %15 = getelementptr inbounds [100 x float], ptr @cc1, i64 0, i64 %14
  %16 = load float, ptr %15, align 4
  %17 = load i32, ptr %3, align 4
  %18 = sext i32 %17 to i64
  %19 = getelementptr inbounds [100 x float], ptr @bb1, i64 0, i64 %18
  %20 = load float, ptr %19, align 4
  %21 = call float @llvm.fmuladd.f32(float %16, float %20, float %12)
  %22 = load i32, ptr %3, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds [100 x float], ptr @dd1_org, i64 0, i64 %23
  store float %21, ptr %24, align 4
  %25 = load i32, ptr %3, align 4
  %26 = sext i32 %25 to i64
  %27 = getelementptr inbounds [100 x float], ptr @aa2, i64 0, i64 %26
  %28 = load float, ptr %27, align 4
  %29 = load i32, ptr %3, align 4
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds [100 x float], ptr @cc2, i64 0, i64 %30
  %32 = load float, ptr %31, align 4
  %33 = load i32, ptr %3, align 4
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds [100 x float], ptr @bb1, i64 0, i64 %34
  %36 = load float, ptr %35, align 4
  %37 = call float @llvm.fmuladd.f32(float %32, float %36, float %28)
  %38 = load i32, ptr %3, align 4
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds [100 x float], ptr @dd2_org, i64 0, i64 %39
  store float %37, ptr %40, align 4
  br label %41

41:                                               ; preds = %8
  %42 = load i32, ptr %3, align 4
  %43 = add nsw i32 %42, 1
  store i32 %43, ptr %3, align 4
  br label %4, !llvm.loop !6

44:                                               ; preds = %4
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

attributes #0 = { noinline nounwind  uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

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
