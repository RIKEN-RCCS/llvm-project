; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop (2)

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
  %sub = sub nsw i32 %1, 2
  %cmp = icmp slt i32 %0, %sub
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load ptr, ptr %aa1.addr, align 8
  %3 = load i32, ptr %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds float, ptr %2, i64 %idxprom
  %4 = load float, ptr %arrayidx, align 4
  %5 = load ptr, ptr %bb1.addr, align 8
  %6 = load i32, ptr %i, align 4
  %idxprom1 = sext i32 %6 to i64
  %arrayidx2 = getelementptr inbounds float, ptr %5, i64 %idxprom1
  %7 = load float, ptr %arrayidx2, align 4
  %add = fadd float %4, %7
  %8 = load ptr, ptr %dd1.addr, align 8
  %9 = load i32, ptr %i, align 4
  %idxprom3 = sext i32 %9 to i64
  %arrayidx4 = getelementptr inbounds float, ptr %8, i64 %idxprom3
  store float %add, ptr %arrayidx4, align 4
  %10 = load ptr, ptr %cc1.addr, align 8
  %11 = load i32, ptr %i, align 4
  %idxprom5 = sext i32 %11 to i64
  %arrayidx6 = getelementptr inbounds float, ptr %10, i64 %idxprom5
  %12 = load float, ptr %arrayidx6, align 4
  %13 = load ptr, ptr %aa1.addr, align 8
  %14 = load i32, ptr %i, align 4
  %add7 = add nsw i32 %14, 2
  %idxprom8 = sext i32 %add7 to i64
  %arrayidx9 = getelementptr inbounds float, ptr %13, i64 %idxprom8
  store float %12, ptr %arrayidx9, align 4
  %15 = load ptr, ptr %bb2.addr, align 8
  %16 = load i32, ptr %i, align 4
  %idxprom10 = sext i32 %16 to i64
  %arrayidx11 = getelementptr inbounds float, ptr %15, i64 %idxprom10
  %17 = load float, ptr %arrayidx11, align 4
  %18 = load ptr, ptr %cc2.addr, align 8
  %19 = load i32, ptr %i, align 4
  %idxprom12 = sext i32 %19 to i64
  %arrayidx13 = getelementptr inbounds float, ptr %18, i64 %idxprom12
  %20 = load float, ptr %arrayidx13, align 4
  %add14 = fadd float %17, %20
  %21 = load ptr, ptr %dd2.addr, align 8
  %22 = load i32, ptr %i, align 4
  %idxprom15 = sext i32 %22 to i64
  %arrayidx16 = getelementptr inbounds float, ptr %21, i64 %idxprom15
  store float %add14, ptr %arrayidx16, align 4
  %23 = load ptr, ptr %aa3.addr, align 8
  %24 = load i32, ptr %i, align 4
  %idxprom17 = sext i32 %24 to i64
  %arrayidx18 = getelementptr inbounds float, ptr %23, i64 %idxprom17
  %25 = load float, ptr %arrayidx18, align 4
  %26 = load ptr, ptr %cc3.addr, align 8
  %27 = load i32, ptr %i, align 4
  %idxprom19 = sext i32 %27 to i64
  %arrayidx20 = getelementptr inbounds float, ptr %26, i64 %idxprom19
  %28 = load float, ptr %arrayidx20, align 4
  %add21 = fadd float %25, %28
  %29 = load ptr, ptr %bb3.addr, align 8
  %30 = load i32, ptr %i, align 4
  %idxprom22 = sext i32 %30 to i64
  %arrayidx23 = getelementptr inbounds float, ptr %29, i64 %idxprom22
  %31 = load float, ptr %arrayidx23, align 4
  %add24 = fadd float %add21, %31
  store float %add24, ptr %t, align 4
  %32 = load ptr, ptr %aa3.addr, align 8
  %33 = load i32, ptr %i, align 4
  %idxprom25 = sext i32 %33 to i64
  %arrayidx26 = getelementptr inbounds float, ptr %32, i64 %idxprom25
  %34 = load float, ptr %arrayidx26, align 4
  %35 = load ptr, ptr %cc3.addr, align 8
  %36 = load i32, ptr %i, align 4
  %idxprom27 = sext i32 %36 to i64
  %arrayidx28 = getelementptr inbounds float, ptr %35, i64 %idxprom27
  %37 = load float, ptr %arrayidx28, align 4
  %mul = fmul float %34, %37
  %38 = load ptr, ptr %bb3.addr, align 8
  %39 = load i32, ptr %i, align 4
  %idxprom29 = sext i32 %39 to i64
  %arrayidx30 = getelementptr inbounds float, ptr %38, i64 %idxprom29
  %40 = load float, ptr %arrayidx30, align 4
  %41 = load float, ptr %t, align 4
  %42 = call float @llvm.fmuladd.f32(float %mul, float %40, float %41)
  %43 = load ptr, ptr %dd3.addr, align 8
  %44 = load i32, ptr %i, align 4
  %idxprom32 = sext i32 %44 to i64
  %arrayidx33 = getelementptr inbounds float, ptr %43, i64 %idxprom32
  store float %42, ptr %arrayidx33, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %45 = load i32, ptr %i, align 4
  %inc = add nsw i32 %45, 1
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
