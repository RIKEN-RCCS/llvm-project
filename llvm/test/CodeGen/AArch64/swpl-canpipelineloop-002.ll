; RUN: llc < %s -mcpu=a64fx -O0 -swpl-debug -ffj-swp --pass-remarks-missed=aarch64-swpipeliner --pass-remarks-filter=aarch64-swpipeliner --pass-remarks-output=- -debug-aarch64tti -o /dev/null 2>&1 | FileCheck --allow-empty %s
;CHECK-NOT: canPipelineLoop


; ModuleID = '2912.c'
source_filename = "2912.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local global [100 x double] zeroinitializer, align 8
@c = dso_local global [100 x double] zeroinitializer, align 8
@a = dso_local global [100 x double] zeroinitializer, align 8

; Function Attrs: noinline nounwind optnone uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0) #0 !dbg !10 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4, !dbg !13
  br label %4, !dbg !14

4:                                                ; preds = %21, %1
  %5 = load i32, ptr %3, align 4, !dbg !15
  %6 = load i32, ptr %2, align 4, !dbg !16
  %7 = icmp slt i32 %5, %6, !dbg !17
  br i1 %7, label %8, label %24, !dbg !18

8:                                                ; preds = %4
  %9 = load i32, ptr %3, align 4, !dbg !19
  %10 = sext i32 %9 to i64, !dbg !20
  %11 = getelementptr inbounds [100 x double], ptr @b, i64 0, i64 %10, !dbg !20
  %12 = load double, ptr %11, align 8, !dbg !20
  %13 = load i32, ptr %3, align 4, !dbg !21
  %14 = sext i32 %13 to i64, !dbg !22
  %15 = getelementptr inbounds [100 x double], ptr @c, i64 0, i64 %14, !dbg !22
  %16 = load double, ptr %15, align 8, !dbg !22
  %17 = fadd double %12, %16, !dbg !23
  %18 = load i32, ptr %3, align 4, !dbg !24
  %19 = sext i32 %18 to i64, !dbg !25
  %20 = getelementptr inbounds [100 x double], ptr @a, i64 0, i64 %19, !dbg !25
  store double %17, ptr %20, align 8, !dbg !26
  br label %21, !dbg !27

21:                                               ; preds = %8
  %22 = load i32, ptr %3, align 4, !dbg !28
  %23 = add nsw i32 %22, 1, !dbg !28
  store i32 %23, ptr %3, align 4, !dbg !28
  br label %4, !dbg !18, !llvm.loop !29

24:                                               ; preds = %4
  ret void, !dbg !31
}

attributes #0 = { noinline nounwind optnone uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 ", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2912.c", directory: "2912", checksumkind: CSK_MD5, checksum: "49b7449ffe0a6f6f4a597a8972606d26")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 4, type: !11, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 5, column: 12, scope: !10)
!14 = !DILocation(line: 5, column: 8, scope: !10)
!15 = !DILocation(line: 5, column: 16, scope: !10)
!16 = !DILocation(line: 5, column: 18, scope: !10)
!17 = !DILocation(line: 5, column: 17, scope: !10)
!18 = !DILocation(line: 5, column: 3, scope: !10)
!19 = !DILocation(line: 6, column: 14, scope: !10)
!20 = !DILocation(line: 6, column: 12, scope: !10)
!21 = !DILocation(line: 6, column: 21, scope: !10)
!22 = !DILocation(line: 6, column: 19, scope: !10)
!23 = !DILocation(line: 6, column: 17, scope: !10)
!24 = !DILocation(line: 6, column: 7, scope: !10)
!25 = !DILocation(line: 6, column: 5, scope: !10)
!26 = !DILocation(line: 6, column: 10, scope: !10)
!27 = !DILocation(line: 7, column: 3, scope: !10)
!28 = !DILocation(line: 5, column: 22, scope: !10)
!29 = distinct !{!29, !18, !27, !30}
!30 = !{!"llvm.loop.mustprogress"}
!31 = !DILocation(line: 8, column: 1, scope: !10)
