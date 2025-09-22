; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop. num of distributied is 2.

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %n, ptr noalias noundef %dd1, ptr noalias noundef %dd2, ptr noalias noundef %aa1, ptr noalias noundef %aa2, ptr noalias noundef %bb1, ptr noalias noundef %bb2, ptr noalias noundef %cc1, ptr noalias noundef %cc2, ptr noalias noundef %dd3, ptr noalias noundef %aa3, ptr noalias noundef %bb3, ptr noalias noundef %cc3) #0 {
entry:
  %n.addr = alloca i32, align 4
  %dd1.addr = alloca ptr, align 8
  %dd2.addr = alloca ptr, align 8
  %aa1.addr = alloca ptr, align 8
  %aa2.addr = alloca ptr, align 8
  %bb1.addr = alloca ptr, align 8
  %bb2.addr = alloca ptr, align 8
  %cc1.addr = alloca ptr, align 8
  %cc2.addr = alloca ptr, align 8
  %dd3.addr = alloca ptr, align 8
  %aa3.addr = alloca ptr, align 8
  %bb3.addr = alloca ptr, align 8
  %cc3.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %t = alloca float, align 4
  store i32 %n, ptr %n.addr, align 4
  store ptr %dd1, ptr %dd1.addr, align 8
  store ptr %dd2, ptr %dd2.addr, align 8
  store ptr %aa1, ptr %aa1.addr, align 8
  store ptr %aa2, ptr %aa2.addr, align 8
  store ptr %bb1, ptr %bb1.addr, align 8
  store ptr %bb2, ptr %bb2.addr, align 8
  store ptr %cc1, ptr %cc1.addr, align 8
  store ptr %cc2, ptr %cc2.addr, align 8
  store ptr %dd3, ptr %dd3.addr, align 8
  store ptr %aa3, ptr %aa3.addr, align 8
  store ptr %bb3, ptr %bb3.addr, align 8
  store ptr %cc3, ptr %cc3.addr, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %sub = sub nsw i32 %1, 1
  %cmp = icmp slt i32 %0, %sub
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load ptr, ptr %aa3.addr, align 8
  %3 = load i32, ptr %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds float, ptr %2, i64 %idxprom
  %4 = load float, ptr %arrayidx, align 4
  %5 = load ptr, ptr %cc3.addr, align 8
  %6 = load i32, ptr %i, align 4
  %idxprom1 = sext i32 %6 to i64
  %arrayidx2 = getelementptr inbounds float, ptr %5, i64 %idxprom1
  %7 = load float, ptr %arrayidx2, align 4
  %add = fadd float %4, %7
  %8 = load ptr, ptr %bb3.addr, align 8
  %9 = load i32, ptr %i, align 4
  %idxprom3 = sext i32 %9 to i64
  %arrayidx4 = getelementptr inbounds float, ptr %8, i64 %idxprom3
  %10 = load float, ptr %arrayidx4, align 4
  %add5 = fadd float %add, %10
  store float %add5, ptr %t, align 4
  %11 = load ptr, ptr %aa3.addr, align 8
  %12 = load i32, ptr %i, align 4
  %idxprom6 = sext i32 %12 to i64
  %arrayidx7 = getelementptr inbounds float, ptr %11, i64 %idxprom6
  %13 = load float, ptr %arrayidx7, align 4
  %14 = load ptr, ptr %cc3.addr, align 8
  %15 = load i32, ptr %i, align 4
  %idxprom8 = sext i32 %15 to i64
  %arrayidx9 = getelementptr inbounds float, ptr %14, i64 %idxprom8
  %16 = load float, ptr %arrayidx9, align 4
  %mul = fmul float %13, %16
  %17 = load ptr, ptr %bb3.addr, align 8
  %18 = load i32, ptr %i, align 4
  %idxprom10 = sext i32 %18 to i64
  %arrayidx11 = getelementptr inbounds float, ptr %17, i64 %idxprom10
  %19 = load float, ptr %arrayidx11, align 4
  %20 = load float, ptr %t, align 4
  %21 = call float @llvm.fmuladd.f32(float %mul, float %19, float %20)
  %22 = load ptr, ptr %dd3.addr, align 8
  %23 = load i32, ptr %i, align 4
  %idxprom13 = sext i32 %23 to i64
  %arrayidx14 = getelementptr inbounds float, ptr %22, i64 %idxprom13
  store float %21, ptr %arrayidx14, align 4
  %24 = load ptr, ptr %aa1.addr, align 8
  %25 = load i32, ptr %i, align 4
  %idxprom15 = sext i32 %25 to i64
  %arrayidx16 = getelementptr inbounds float, ptr %24, i64 %idxprom15
  %26 = load float, ptr %arrayidx16, align 4
  %27 = load ptr, ptr %bb1.addr, align 8
  %28 = load i32, ptr %i, align 4
  %idxprom17 = sext i32 %28 to i64
  %arrayidx18 = getelementptr inbounds float, ptr %27, i64 %idxprom17
  %29 = load float, ptr %arrayidx18, align 4
  %30 = load ptr, ptr %cc1.addr, align 8
  %31 = load i32, ptr %i, align 4
  %idxprom20 = sext i32 %31 to i64
  %arrayidx21 = getelementptr inbounds float, ptr %30, i64 %idxprom20
  %32 = load float, ptr %arrayidx21, align 4
  %33 = call float @llvm.fmuladd.f32(float %26, float %29, float %32)
  %34 = load ptr, ptr %dd1.addr, align 8
  %35 = load i32, ptr %i, align 4
  %idxprom22 = sext i32 %35 to i64
  %arrayidx23 = getelementptr inbounds float, ptr %34, i64 %idxprom22
  store float %33, ptr %arrayidx23, align 4
  %36 = load ptr, ptr %aa2.addr, align 8
  %37 = load i32, ptr %i, align 4
  %idxprom24 = sext i32 %37 to i64
  %arrayidx25 = getelementptr inbounds float, ptr %36, i64 %idxprom24
  %38 = load float, ptr %arrayidx25, align 4
  %add26 = fadd float %38, 1.000000e+00
  %39 = load ptr, ptr %dd2.addr, align 8
  %40 = load i32, ptr %i, align 4
  %idxprom27 = sext i32 %40 to i64
  %arrayidx28 = getelementptr inbounds float, ptr %39, i64 %idxprom27
  store float %add26, ptr %arrayidx28, align 4
  %41 = load ptr, ptr %dd2.addr, align 8
  %42 = load i32, ptr %i, align 4
  %idxprom29 = sext i32 %42 to i64
  %arrayidx30 = getelementptr inbounds float, ptr %41, i64 %idxprom29
  %43 = load float, ptr %arrayidx30, align 4
  %44 = load ptr, ptr %bb2.addr, align 8
  %45 = load i32, ptr %i, align 4
  %idxprom31 = sext i32 %45 to i64
  %arrayidx32 = getelementptr inbounds float, ptr %44, i64 %idxprom31
  %46 = load float, ptr %arrayidx32, align 4
  %47 = load ptr, ptr %cc2.addr, align 8
  %48 = load i32, ptr %i, align 4
  %idxprom34 = sext i32 %48 to i64
  %arrayidx35 = getelementptr inbounds float, ptr %47, i64 %idxprom34
  %49 = load float, ptr %arrayidx35, align 4
  %50 = call float @llvm.fmuladd.f32(float %43, float %46, float %49)
  %51 = load ptr, ptr %aa2.addr, align 8
  %52 = load i32, ptr %i, align 4
  %add36 = add nsw i32 %52, 1
  %idxprom37 = sext i32 %add36 to i64
  %arrayidx38 = getelementptr inbounds float, ptr %51, i64 %idxprom37
  store float %50, ptr %arrayidx38, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %53 = load i32, ptr %i, align 4
  %inc = add nsw i32 %53, 1
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
