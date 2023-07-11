; RUN: llc < %s -mtriple=aarch64-unknown-linux-gnu -mcpu=a64fx -O0 --stop-after=localstackalloc -o - 2>&1 | FileCheck %s
; CHECK: SUBSXri
; CHECK-NOT: SUBSXri
; test-id : 2913-002
;

; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: norecurse nounwind readonly
define dso_local i64 @mt_isel_02(i64 %a, i64* nocapture readonly %b) local_unnamed_addr #0 {
entry:
  %0 = call i64 @llvm.loop.decrement.reg.i64(i64 %a, i64 3)
  ret i64 %0
}

; Function Attrs: nounwind readnone willreturn
declare i64 @llvm.loop.decrement.reg.i64(i64, i64) #2

attributes #0 = { norecurse nounwind readonly "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="non-leaf" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+neon" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind readnone willreturn }
