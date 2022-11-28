; RUN: llc < %s -O1 -mcpu=a64fx  -ffj-swp  -swpl-debug  --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed=aarch64-swpipeliner  -o /dev/null 2>&1 | FileCheck %s
;CHECK:canPipelineLoop:OK
; ModuleID = '2912_2.c'
source_filename = "2912_2.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8
@c = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8
@a = dso_local local_unnamed_addr global [100 x [100 x double]] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @foo(i32 noundef %0) local_unnamed_addr #0 !dbg !10 {
  br label %2, !dbg !13

2:                                                ; preds = %1, %5
  %3 = phi i64 [ 0, %1 ], [ %6, %5 ]
  br label %8, !dbg !14

4:                                                ; preds = %5
  ret void, !dbg !15

5:                                                ; preds = %8
  %6 = add nuw nsw i64 %3, 1, !dbg !16
  %7 = icmp eq i64 %6, 100, !dbg !17
  br i1 %7, label %4, label %2, !dbg !13, !llvm.loop !18

8:                                                ; preds = %2, %8
  %9 = phi i64 [ 0, %2 ], [ %16, %8 ]
  %10 = getelementptr inbounds [100 x [100 x double]], ptr @b, i64 0, i64 %3, i64 %9, !dbg !22
  %11 = load double, ptr %10, align 8, !dbg !22, !tbaa !23
  %12 = getelementptr inbounds [100 x [100 x double]], ptr @c, i64 0, i64 %3, i64 %9, !dbg !27
  %13 = load double, ptr %12, align 8, !dbg !27, !tbaa !23
  %14 = fadd double %11, %13, !dbg !28
  %15 = getelementptr inbounds [100 x [100 x double]], ptr @a, i64 0, i64 %3, i64 %9, !dbg !29
  store double %14, ptr %15, align 8, !dbg !30, !tbaa !23
  %16 = add nuw nsw i64 %9, 1, !dbg !31
  %17 = icmp eq i64 %16, 100, !dbg !32
  br i1 %17, label %5, label %8, !dbg !14, !llvm.loop !33
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2912_2.c", directory: "2912", checksumkind: CSK_MD5, checksum: "bd39c9b757fed5de0e9599b3e153b3b8")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 4, type: !11, scopeLine: 4, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 5, column: 3, scope: !10)
!14 = !DILocation(line: 6, column: 5, scope: !10)
!15 = !DILocation(line: 10, column: 1, scope: !10)
!16 = !DILocation(line: 5, column: 23, scope: !10)
!17 = !DILocation(line: 5, column: 18, scope: !10)
!18 = distinct !{!18, !13, !19, !20, !21}
!19 = !DILocation(line: 9, column: 3, scope: !10)
!20 = !{!"llvm.loop.mustprogress"}
!21 = !{!"llvm.loop.unroll.disable"}
!22 = !DILocation(line: 7, column: 17, scope: !10)
!23 = !{!24, !24, i64 0}
!24 = !{!"double", !25, i64 0}
!25 = !{!"omnipotent char", !26, i64 0}
!26 = !{!"Simple C/C++ TBAA"}
!27 = !DILocation(line: 7, column: 27, scope: !10)
!28 = !DILocation(line: 7, column: 25, scope: !10)
!29 = !DILocation(line: 7, column: 7, scope: !10)
!30 = !DILocation(line: 7, column: 15, scope: !10)
!31 = !DILocation(line: 6, column: 25, scope: !10)
!32 = !DILocation(line: 6, column: 20, scope: !10)
!33 = distinct !{!33, !14, !34, !20, !21}
!34 = !DILocation(line: 8, column: 5, scope: !10)
