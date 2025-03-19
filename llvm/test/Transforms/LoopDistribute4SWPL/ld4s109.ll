; RUN: opt -O1 -S -enable-loop-distribute4swpl < %s |& FileCheck %s

; CHECK: loop not distributed for swpl: the optimizer was unable to perform the requested transformation; the transformation might be disabled or specified as part of an unsupported transformation ordering

; CHECK:   br i1 %exitcond.not.24, label %for.inc17, label %for.body3, !llvm.loop ![[LOOP1:.*]]
; CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[D4SWPL1:![0-9]+]], [[D4SWPLF:![0-9]+]]}
; CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
; CHECK-NEXT: [[D4SWPL1]] = !{!"llvm.loop.distribute4swpl.enable", i1 true}
; CHECK-NEXT: [[D4SWPLF]] = !{!"llvm.loop.distribute4swpl.freg", i32 16}

; ModuleID = './target.c'
source_filename = "./target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

; Function Attrs: noinline nounwind uwtable vscale_range(1,16)
define dso_local void @foo(ptr noundef %A, ptr noundef %B, ptr noundef %C, ptr noundef %D) #0 {
entry:
  %A.addr = alloca ptr, align 8
  %B.addr = alloca ptr, align 8
  %C.addr = alloca ptr, align 8
  %D.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  %sum = alloca i32, align 4
  store ptr %A, ptr %A.addr, align 8
  store ptr %B, ptr %B.addr, align 8
  store ptr %C, ptr %C.addr, align 8
  store ptr %D, ptr %D.addr, align 8
  store i32 0, ptr %sum, align 4
  store i32 0, ptr %j, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc17, %entry
  %0 = load i32, ptr %j, align 4
  %cmp = icmp slt i32 %0, 10000
  br i1 %cmp, label %for.body, label %for.end19

for.body:                                         ; preds = %for.cond
  store i32 0, ptr %i, align 4
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %1 = load i32, ptr %i, align 4
  %cmp2 = icmp slt i32 %1, 10000
  br i1 %cmp2, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %2 = load ptr, ptr %A.addr, align 8
  %3 = load i32, ptr %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds i32, ptr %2, i64 %idxprom
  %4 = load i32, ptr %arrayidx, align 4
  %5 = load ptr, ptr %B.addr, align 8
  %6 = load i32, ptr %i, align 4
  %idxprom4 = sext i32 %6 to i64
  %arrayidx5 = getelementptr inbounds i32, ptr %5, i64 %idxprom4
  %7 = load i32, ptr %arrayidx5, align 4
  %mul = mul nsw i32 %4, %7
  %8 = load ptr, ptr %A.addr, align 8
  %9 = load i32, ptr %i, align 4
  %add = add nsw i32 %9, 1
  %idxprom6 = sext i32 %add to i64
  %arrayidx7 = getelementptr inbounds i32, ptr %8, i64 %idxprom6
  store i32 %mul, ptr %arrayidx7, align 4
  %10 = load ptr, ptr %C.addr, align 8
  %11 = load i32, ptr %i, align 4
  %idxprom8 = sext i32 %11 to i64
  %arrayidx9 = getelementptr inbounds i32, ptr %10, i64 %idxprom8
  %12 = load i32, ptr %arrayidx9, align 4
  %13 = load ptr, ptr %D.addr, align 8
  %14 = load i32, ptr %i, align 4
  %idxprom10 = sext i32 %14 to i64
  %arrayidx11 = getelementptr inbounds i32, ptr %13, i64 %idxprom10
  %15 = load i32, ptr %arrayidx11, align 4
  %mul12 = mul nsw i32 %12, %15
  %16 = load i32, ptr %j, align 4
  %mul13 = mul nsw i32 %mul12, %16
  %17 = load ptr, ptr %C.addr, align 8
  %18 = load i32, ptr %i, align 4
  %add14 = add nsw i32 %18, 1
  %idxprom15 = sext i32 %add14 to i64
  %arrayidx16 = getelementptr inbounds i32, ptr %17, i64 %idxprom15
  store i32 %mul13, ptr %arrayidx16, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %19 = load i32, ptr %i, align 4
  %inc = add nsw i32 %19, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond1, !llvm.loop !4

for.end:                                          ; preds = %for.cond1
  br label %for.inc17

for.inc17:                                        ; preds = %for.end
  %20 = load i32, ptr %j, align 4
  %inc18 = add nsw i32 %20, 1
  store i32 %inc18, ptr %j, align 4
  br label %for.cond, !llvm.loop !6

for.end19:                                        ; preds = %for.cond
  ret void
}

attributes #0 = { noinline nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 2}
!2 = !{i32 7, !"frame-pointer", i32 1}
!3 = !{!"clang version 18.1.8"}
!4 = distinct !{!4, !5, !7, !8}
!5 = !{!"llvm.loop.mustprogress"}
!6 = distinct !{!6, !5}
!7 = !{!"llvm.loop.distribute4swpl.enable", i1 true}
!8 = !{!"llvm.loop.distribute4swpl.freg", i32 16}
