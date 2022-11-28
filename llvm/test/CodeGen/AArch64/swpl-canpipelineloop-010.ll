; RUN: llc < %s -mcpu=a64fx -swpl-debug -O1  -ffj-swp --pass-remarks-missed=aarch64-swpipeliner --pass-remarks-filter=aarch64-swpipeliner --pass-remarks-output=- -o /dev/null 2>&1 | FileCheck %s
;CHECK: canPipelineLoop:OK

; ModuleID = '2912.c'
source_filename = "2912.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0) local_unnamed_addr #0 !dbg !10 {
  %2 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %2, label %3, label %5, !dbg !14

3:                                                ; preds = %1
  %4 = zext i32 %0 to i64, !dbg !13
  br label %6, !dbg !14

5:                                                ; preds = %6, %1
  ret void, !dbg !15

6:                                                ; preds = %3, %6
  %7 = phi i64 [ 0, %3 ], [ %14, %6 ]
  %8 = getelementptr inbounds [100 x double], ptr @b, i64 0, i64 %7, !dbg !16
  %9 = load double, ptr %8, align 8, !dbg !16, !tbaa !17
  %10 = getelementptr inbounds [100 x double], ptr @c, i64 0, i64 %7, !dbg !21
  %11 = load double, ptr %10, align 8, !dbg !21, !tbaa !17
  %12 = fadd double %9, %11, !dbg !22
  %13 = getelementptr inbounds [100 x double], ptr @a, i64 0, i64 %7, !dbg !23
  store double %12, ptr %13, align 8, !dbg !24, !tbaa !17
  %14 = add nuw nsw i64 %7, 1, !dbg !25
  %15 = icmp eq i64 %14, %4, !dbg !13
  br i1 %15, label %5, label %6, !dbg !14, !llvm.loop !26
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2912.c", directory: "2912", checksumkind: CSK_MD5, checksum: "49b7449ffe0a6f6f4a597a8972606d26")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 4, type: !11, scopeLine: 4, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 5, column: 17, scope: !10)
!14 = !DILocation(line: 5, column: 3, scope: !10)
!15 = !DILocation(line: 8, column: 1, scope: !10)
!16 = !DILocation(line: 6, column: 12, scope: !10)
!17 = !{!18, !18, i64 0}
!18 = !{!"double", !19, i64 0}
!19 = !{!"omnipotent char", !20, i64 0}
!20 = !{!"Simple C/C++ TBAA"}
!21 = !DILocation(line: 6, column: 19, scope: !10)
!22 = !DILocation(line: 6, column: 17, scope: !10)
!23 = !DILocation(line: 6, column: 5, scope: !10)
!24 = !DILocation(line: 6, column: 10, scope: !10)
!25 = !DILocation(line: 5, column: 22, scope: !10)
!26 = distinct !{!26, !14, !27, !28, !29}
!27 = !DILocation(line: 7, column: 3, scope: !10)
!28 = !{!"llvm.loop.mustprogress"}
!29 = !{!"llvm.loop.unroll.disable"}
