; RUN: llc < %s -O1 -mcpu=a64fx -fswp -swpl-debug -pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
;CHECK-NOT:This loop tries to schedule with the InitiationInterval= {{[0-9]*}} specified in the pragma.

; ModuleID = '2901-1.c'
source_filename = "2901-1.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 !dbg !10 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
  store ptr %1, ptr %5, align 8
  store i32 0, ptr %6, align 4, !dbg !13
  store i32 0, ptr %7, align 4, !dbg !14
  br label %8, !dbg !15

8:                                                ; preds = %24, %2
  %9 = load i32, ptr %7, align 4, !dbg !16
  %10 = load i32, ptr %4, align 4, !dbg !17
  %11 = icmp slt i32 %9, %10, !dbg !18
  br i1 %11, label %12, label %27, !dbg !19

12:                                               ; preds = %8
  %13 = load ptr, ptr %5, align 8, !dbg !20
  %14 = getelementptr inbounds ptr, ptr %13, i64 0, !dbg !20
  %15 = load ptr, ptr %14, align 8, !dbg !20
  %16 = getelementptr inbounds i8, ptr %15, i64 0, !dbg !20
  %17 = load i8, ptr %16, align 1, !dbg !20
  %18 = zext i8 %17 to i32, !dbg !20
  %19 = load i32, ptr %6, align 4, !dbg !21
  %20 = mul nsw i32 %19, %18, !dbg !21
  store i32 %20, ptr %6, align 4, !dbg !21
  %21 = load i32, ptr %7, align 4, !dbg !22
  %22 = load i32, ptr %6, align 4, !dbg !23
  %23 = add nsw i32 %22, %21, !dbg !23
  store i32 %23, ptr %6, align 4, !dbg !23
  br label %24, !dbg !24

24:                                               ; preds = %12
  %25 = load i32, ptr %7, align 4, !dbg !25
  %26 = add nsw i32 %25, 1, !dbg !25
  store i32 %26, ptr %7, align 4, !dbg !25
  br label %8, !dbg !19, !llvm.loop !26

27:                                               ; preds = %8
  %28 = load i32, ptr %6, align 4, !dbg !28
  %29 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %28), !dbg !29
  ret i32 0, !dbg !30
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-1.c", directory: "", checksumkind: CSK_MD5, checksum: "6262637c686d49c224b0e7699868da7c")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!9 = !{!"clang version 15.0.4"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !11, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 4, column: 6, scope: !10)
!14 = !DILocation(line: 5, column: 11, scope: !10)
!15 = !DILocation(line: 5, column: 7, scope: !10)
!16 = !DILocation(line: 5, column: 16, scope: !10)
!17 = !DILocation(line: 5, column: 18, scope: !10)
!18 = !DILocation(line: 5, column: 17, scope: !10)
!19 = !DILocation(line: 5, column: 2, scope: !10)
!20 = !DILocation(line: 6, column: 8, scope: !10)
!21 = !DILocation(line: 6, column: 6, scope: !10)
!22 = !DILocation(line: 7, column: 8, scope: !10)
!23 = !DILocation(line: 7, column: 6, scope: !10)
!24 = !DILocation(line: 8, column: 2, scope: !10)
!25 = !DILocation(line: 5, column: 25, scope: !10)
!26 = distinct !{!26, !19, !24, !27}
!27 = !{!"llvm.loop.mustprogress"}
!28 = !DILocation(line: 9, column: 17, scope: !10)
!29 = !DILocation(line: 9, column: 2, scope: !10)
!30 = !DILocation(line: 10, column: 2, scope: !10)
