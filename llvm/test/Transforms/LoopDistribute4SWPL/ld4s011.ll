; RUN: opt -S -O1 -enable-loop-distribute4swpl -distribute4swpl-limit-freg=5 -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop (2)

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

@aa1 = external global [100 x float], align 4
@cc1 = external global [100 x float], align 4
@bb1 = external global [100 x float], align 4
@dd1 = external global [100 x float], align 4
@aa2 = external global [100 x float], align 4
@cc2 = external global [100 x float], align 4
@bb2 = external global [100 x float], align 4
@dd2 = external global [100 x float], align 4
@aa3 = external global [100 x float], align 4
@cc3 = external global [100 x float], align 4
@bb3 = external global [100 x float], align 4
@dd3 = external global [100 x float], align 4

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %n) #0 {
entry:
  %n.addr = alloca i32, align 4
  %i = alloca i32, align 4
  %t = alloca float, align 4
  store i32 %n, ptr %n.addr, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32, ptr %i, align 4
  %idxprom = sext i32 %2 to i64
  %arrayidx = getelementptr inbounds [100 x float], ptr @aa1, i64 0, i64 %idxprom
  %3 = load float, ptr %arrayidx, align 4
  %4 = load i32, ptr %i, align 4
  %idxprom1 = sext i32 %4 to i64
  %arrayidx2 = getelementptr inbounds [100 x float], ptr @cc1, i64 0, i64 %idxprom1
  %5 = load float, ptr %arrayidx2, align 4
  %6 = load i32, ptr %i, align 4
  %idxprom3 = sext i32 %6 to i64
  %arrayidx4 = getelementptr inbounds [100 x float], ptr @bb1, i64 0, i64 %idxprom3
  %7 = load float, ptr %arrayidx4, align 4
  %8 = call float @llvm.fmuladd.f32(float %5, float %7, float %3)
  %9 = load i32, ptr %i, align 4
  %idxprom5 = sext i32 %9 to i64
  %arrayidx6 = getelementptr inbounds [100 x float], ptr @dd1, i64 0, i64 %idxprom5
  store float %8, ptr %arrayidx6, align 4
  %10 = load i32, ptr %i, align 4
  %idxprom7 = sext i32 %10 to i64
  %arrayidx8 = getelementptr inbounds [100 x float], ptr @aa2, i64 0, i64 %idxprom7
  %11 = load float, ptr %arrayidx8, align 4
  %12 = load i32, ptr %i, align 4
  %idxprom9 = sext i32 %12 to i64
  %arrayidx10 = getelementptr inbounds [100 x float], ptr @cc2, i64 0, i64 %idxprom9
  %13 = load float, ptr %arrayidx10, align 4
  %14 = load i32, ptr %i, align 4
  %idxprom11 = sext i32 %14 to i64
  %arrayidx12 = getelementptr inbounds [100 x float], ptr @bb2, i64 0, i64 %idxprom11
  %15 = load float, ptr %arrayidx12, align 4
  %16 = call float @llvm.fmuladd.f32(float %13, float %15, float %11)
  %17 = load i32, ptr %i, align 4
  %idxprom13 = sext i32 %17 to i64
  %arrayidx14 = getelementptr inbounds [100 x float], ptr @dd2, i64 0, i64 %idxprom13
  store float %16, ptr %arrayidx14, align 4
  %18 = load i32, ptr %i, align 4
  %idxprom15 = sext i32 %18 to i64
  %arrayidx16 = getelementptr inbounds [100 x float], ptr @aa3, i64 0, i64 %idxprom15
  %19 = load float, ptr %arrayidx16, align 4
  %20 = load i32, ptr %i, align 4
  %idxprom17 = sext i32 %20 to i64
  %arrayidx18 = getelementptr inbounds [100 x float], ptr @cc3, i64 0, i64 %idxprom17
  %21 = load float, ptr %arrayidx18, align 4
  %add = fadd float %19, %21
  %22 = load i32, ptr %i, align 4
  %idxprom19 = sext i32 %22 to i64
  %arrayidx20 = getelementptr inbounds [100 x float], ptr @bb3, i64 0, i64 %idxprom19
  %23 = load float, ptr %arrayidx20, align 4
  %add21 = fadd float %add, %23
  store float %add21, ptr %t, align 4
  %24 = load i32, ptr %i, align 4
  %idxprom22 = sext i32 %24 to i64
  %arrayidx23 = getelementptr inbounds [100 x float], ptr @aa3, i64 0, i64 %idxprom22
  %25 = load float, ptr %arrayidx23, align 4
  %26 = load i32, ptr %i, align 4
  %idxprom24 = sext i32 %26 to i64
  %arrayidx25 = getelementptr inbounds [100 x float], ptr @cc3, i64 0, i64 %idxprom24
  %27 = load float, ptr %arrayidx25, align 4
  %mul = fmul float %25, %27
  %28 = load i32, ptr %i, align 4
  %idxprom26 = sext i32 %28 to i64
  %arrayidx27 = getelementptr inbounds [100 x float], ptr @bb3, i64 0, i64 %idxprom26
  %29 = load float, ptr %arrayidx27, align 4
  %30 = load float, ptr %t, align 4
  %31 = call float @llvm.fmuladd.f32(float %mul, float %29, float %30)
  %32 = load i32, ptr %i, align 4
  %idxprom29 = sext i32 %32 to i64
  %arrayidx30 = getelementptr inbounds [100 x float], ptr @dd3, i64 0, i64 %idxprom29
  store float %31, ptr %arrayidx30, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %33 = load i32, ptr %i, align 4
  %inc = add nsw i32 %33, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
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
