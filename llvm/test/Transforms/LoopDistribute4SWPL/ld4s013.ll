; RUN: opt -S -O1 -enable-loop-distribute4swpl -pass-remarks=loop-dist < %s |& FileCheck %s
; CHECK: distributed loop (2)

; ModuleID = 'target.c'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @target(i32 noundef %n, ptr noalias noundef %dd1, ptr noalias noundef %dd2, ptr noalias noundef %aa1, ptr noalias noundef %aa2, ptr noalias noundef %bb1, ptr noalias noundef %bb2, ptr noalias noundef %cc1, ptr noalias noundef %cc2) #0 {
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
  %i = alloca i32, align 4
  store i32 %n, ptr %n.addr, align 4
  store ptr %dd1, ptr %dd1.addr, align 8
  store ptr %dd2, ptr %dd2.addr, align 8
  store ptr %aa1, ptr %aa1.addr, align 8
  store ptr %aa2, ptr %aa2.addr, align 8
  store ptr %bb1, ptr %bb1.addr, align 8
  store ptr %bb2, ptr %bb2.addr, align 8
  store ptr %cc1, ptr %cc1.addr, align 8
  store ptr %cc2, ptr %cc2.addr, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %sub = sub nsw i32 %1, 1
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
  %8 = load ptr, ptr %cc1.addr, align 8
  %9 = load i32, ptr %i, align 4
  %idxprom3 = sext i32 %9 to i64
  %arrayidx4 = getelementptr inbounds float, ptr %8, i64 %idxprom3
  %10 = load float, ptr %arrayidx4, align 4
  %11 = call float @llvm.fmuladd.f32(float %4, float %7, float %10)
  %12 = load ptr, ptr %dd1.addr, align 8
  %13 = load i32, ptr %i, align 4
  %idxprom5 = sext i32 %13 to i64
  %arrayidx6 = getelementptr inbounds float, ptr %12, i64 %idxprom5
  store float %11, ptr %arrayidx6, align 4
  %14 = load ptr, ptr %aa2.addr, align 8
  %15 = load i32, ptr %i, align 4
  %idxprom7 = sext i32 %15 to i64
  %arrayidx8 = getelementptr inbounds float, ptr %14, i64 %idxprom7
  %16 = load float, ptr %arrayidx8, align 4
  %add = fadd float %16, 1.000000e+00
  %17 = load ptr, ptr %dd2.addr, align 8
  %18 = load i32, ptr %i, align 4
  %idxprom9 = sext i32 %18 to i64
  %arrayidx10 = getelementptr inbounds float, ptr %17, i64 %idxprom9
  store float %add, ptr %arrayidx10, align 4
  %19 = load ptr, ptr %dd2.addr, align 8
  %20 = load i32, ptr %i, align 4
  %idxprom11 = sext i32 %20 to i64
  %arrayidx12 = getelementptr inbounds float, ptr %19, i64 %idxprom11
  %21 = load float, ptr %arrayidx12, align 4
  %22 = load ptr, ptr %bb2.addr, align 8
  %23 = load i32, ptr %i, align 4
  %idxprom13 = sext i32 %23 to i64
  %arrayidx14 = getelementptr inbounds float, ptr %22, i64 %idxprom13
  %24 = load float, ptr %arrayidx14, align 4
  %25 = load ptr, ptr %cc2.addr, align 8
  %26 = load i32, ptr %i, align 4
  %idxprom15 = sext i32 %26 to i64
  %arrayidx16 = getelementptr inbounds float, ptr %25, i64 %idxprom15
  %27 = load float, ptr %arrayidx16, align 4
  %28 = call float @llvm.fmuladd.f32(float %21, float %24, float %27)
  %29 = load ptr, ptr %aa2.addr, align 8
  %30 = load i32, ptr %i, align 4
  %add17 = add nsw i32 %30, 1
  %idxprom18 = sext i32 %add17 to i64
  %arrayidx19 = getelementptr inbounds float, ptr %29, i64 %idxprom18
  store float %28, ptr %arrayidx19, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %31 = load i32, ptr %i, align 4
  %inc = add nsw i32 %31, 1
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
