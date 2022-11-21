; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp  -enable-savecmp  -debug-aarch64tti  -o - 2>&1 | FileCheck %s
; CHECK: return false(!isHardwareLoopProfitable())
; ModuleID = '2901-3-01.c'
source_filename = "2901-3-01.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %7, label %4, !dbg !14

4:                                                ; preds = %7, %2
  %5 = phi i32 [ 0, %2 ], [ %15, %7 ], !dbg !15
  %6 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %5), !dbg !16
  ret i32 0, !dbg !17

7:                                                ; preds = %2, %7
  %8 = phi i32 [ %16, %7 ], [ 0, %2 ]
  %9 = phi i32 [ %15, %7 ], [ 0, %2 ]
  %10 = load ptr, ptr %1, align 8, !dbg !18, !tbaa !19
  %11 = load i8, ptr %10, align 1, !dbg !18, !tbaa !23
  %12 = zext i8 %11 to i32, !dbg !18
  %13 = mul nsw i32 %9, %12, !dbg !24
  %14 = tail call i32 @sub(i32 noundef %8) #3, !dbg !25
  %15 = add nsw i32 %13, %14, !dbg !26
  %16 = add nuw nsw i32 %8, 1, !dbg !27
  %17 = icmp eq i32 %16, %0, !dbg !13
  br i1 %17, label %4, label %7, !dbg !14, !llvm.loop !28
}

declare !dbg !32 i32 @sub(i32 noundef) local_unnamed_addr #1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-3-01.c", directory: "2901-3", checksumkind: CSK_MD5, checksum: "6d993962b2d3cf4db2362f108bce1177")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 4, type: !11, scopeLine: 4, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 6, column: 17, scope: !10)
!14 = !DILocation(line: 6, column: 2, scope: !10)
!15 = !DILocation(line: 0, scope: !10)
!16 = !DILocation(line: 10, column: 2, scope: !10)
!17 = !DILocation(line: 11, column: 2, scope: !10)
!18 = !DILocation(line: 7, column: 8, scope: !10)
!19 = !{!20, !20, i64 0}
!20 = !{!"any pointer", !21, i64 0}
!21 = !{!"omnipotent char", !22, i64 0}
!22 = !{!"Simple C/C++ TBAA"}
!23 = !{!21, !21, i64 0}
!24 = !DILocation(line: 7, column: 6, scope: !10)
!25 = !DILocation(line: 8, column: 8, scope: !10)
!26 = !DILocation(line: 8, column: 6, scope: !10)
!27 = !DILocation(line: 6, column: 25, scope: !10)
!28 = distinct !{!28, !14, !29, !30, !31}
!29 = !DILocation(line: 9, column: 2, scope: !10)
!30 = !{!"llvm.loop.mustprogress"}
!31 = !{!"llvm.loop.unroll.disable"}
!32 = !DISubprogram(name: "sub", scope: !1, file: !1, line: 3, type: !11, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized, retainedNodes: !12)
