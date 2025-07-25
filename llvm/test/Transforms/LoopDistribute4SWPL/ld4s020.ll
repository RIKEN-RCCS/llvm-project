; RUN: opt -O1 -enable-loop-distribute4swpl -S -o /dev/null -pass-remarks-analysis=loop-distribute4swpl \
; RUN:   < %s |& FileCheck %s

; CHECK: loop not distributed: The division unit became one

; ModuleID = 'a.c'
source_filename = "a.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind  uwtable vscale_range(1,16)
define dso_local void @sub(i32 noundef %n, float noundef %x, ptr noundef %p, ptr noundef %q, ptr noundef %r) #0 {
entry:
  %n.addr = alloca i32, align 4
  %x.addr = alloca float, align 4
  %p.addr = alloca ptr, align 8
  %q.addr = alloca ptr, align 8
  %r.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  store i32 %n, ptr %n.addr, align 4
  store float %x, ptr %x.addr, align 4
  store ptr %p, ptr %p.addr, align 8
  store ptr %q, ptr %q.addr, align 8
  store ptr %r, ptr %r.addr, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load ptr, ptr %q.addr, align 8
  %3 = load i32, ptr %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds float, ptr %2, i64 %idxprom
  %4 = load float, ptr %arrayidx, align 4
  %5 = load ptr, ptr %p.addr, align 8
  %6 = load i32, ptr %i, align 4
  %7 = load float, ptr %x.addr, align 4
  %conv = fptosi float %7 to i32
  %add = add nsw i32 %6, %conv
  %idxprom1 = sext i32 %add to i64
  %arrayidx2 = getelementptr inbounds float, ptr %5, i64 %idxprom1
  %8 = load float, ptr %arrayidx2, align 4
  %add3 = fadd float %8, %4
  store float %add3, ptr %arrayidx2, align 4
  %9 = load ptr, ptr %q.addr, align 8
  %10 = load i32, ptr %i, align 4
  %rem = srem i32 %10, 5
  %idxprom4 = sext i32 %rem to i64
  %arrayidx5 = getelementptr inbounds float, ptr %9, i64 %idxprom4
  %11 = load float, ptr %arrayidx5, align 4
  %12 = load ptr, ptr %r.addr, align 8
  %13 = load i32, ptr %i, align 4
  %idxprom6 = sext i32 %13 to i64
  %arrayidx7 = getelementptr inbounds float, ptr %12, i64 %idxprom6
  store float %11, ptr %arrayidx7, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %14 = load i32, ptr %i, align 4
  %inc = add nsw i32 %14, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
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
