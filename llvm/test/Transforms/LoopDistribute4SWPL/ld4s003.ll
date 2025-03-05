; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop (2)

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %0, ptr noalias noundef %1, ptr noalias noundef %2, ptr noalias noundef %3, ptr noalias noundef %4, ptr noalias noundef %5, ptr noalias noundef %6, ptr noalias noundef %7, ptr noalias noundef %8) #0 {
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

20:                                               ; preds = %77, %9
  %21 = load i32, ptr %19, align 4
  %22 = load i32, ptr %10, align 4
  %23 = sub nsw i32 %22, 1
  %24 = icmp slt i32 %21, %23
  br i1 %24, label %25, label %80

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
  %36 = load ptr, ptr %17, align 8
  %37 = load i32, ptr %19, align 4
  %38 = sext i32 %37 to i64
  %39 = getelementptr inbounds float, ptr %36, i64 %38
  %40 = load float, ptr %39, align 4
  %41 = call float @llvm.fmuladd.f32(float %30, float %35, float %40)
  %42 = load ptr, ptr %11, align 8
  %43 = load i32, ptr %19, align 4
  %44 = sext i32 %43 to i64
  %45 = getelementptr inbounds float, ptr %42, i64 %44
  store float %41, ptr %45, align 4
  %46 = load ptr, ptr %14, align 8
  %47 = load i32, ptr %19, align 4
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds float, ptr %46, i64 %48
  %50 = load float, ptr %49, align 4
  %51 = fadd float %50, 1.000000e+00
  %52 = load ptr, ptr %12, align 8
  %53 = load i32, ptr %19, align 4
  %54 = sext i32 %53 to i64
  %55 = getelementptr inbounds float, ptr %52, i64 %54
  store float %51, ptr %55, align 4
  %56 = load ptr, ptr %12, align 8
  %57 = load i32, ptr %19, align 4
  %58 = sext i32 %57 to i64
  %59 = getelementptr inbounds float, ptr %56, i64 %58
  %60 = load float, ptr %59, align 4
  %61 = load ptr, ptr %16, align 8
  %62 = load i32, ptr %19, align 4
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, ptr %61, i64 %63
  %65 = load float, ptr %64, align 4
  %66 = load ptr, ptr %18, align 8
  %67 = load i32, ptr %19, align 4
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds float, ptr %66, i64 %68
  %70 = load float, ptr %69, align 4
  %71 = call float @llvm.fmuladd.f32(float %60, float %65, float %70)
  %72 = load ptr, ptr %14, align 8
  %73 = load i32, ptr %19, align 4
  %74 = add nsw i32 %73, 1
  %75 = sext i32 %74 to i64
  %76 = getelementptr inbounds float, ptr %72, i64 %75
  store float %71, ptr %76, align 4
  br label %77

77:                                               ; preds = %25
  %78 = load i32, ptr %19, align 4
  %79 = add nsw i32 %78, 1
  store i32 %79, ptr %19, align 4
  br label %20, !llvm.loop !6

80:                                               ; preds = %20
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
