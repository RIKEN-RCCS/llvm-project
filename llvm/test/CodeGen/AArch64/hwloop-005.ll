; RUN: llc < %s -mcpu=a64fx -O1  -ffj-swp  -debug-hardwareloops --pass-remarks-filter=hardware-loops -debug-aarch64tti -o - 2>&1 | FileCheck %s
; CHECK: HardwareLoopInsertion succeeded
; ModuleID = '2901-2-05.c'
source_filename = "2901-2-05.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %7, !dbg !14

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !15
  %6 = zext i32 %0 to i64, !dbg !13
  br label %10, !dbg !14

7:                                                ; preds = %10, %2
  %8 = phi i32 [ 0, %2 ], [ %18, %10 ], !dbg !19
  %9 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %8), !dbg !20
  ret i32 0, !dbg !21

10:                                               ; preds = %4, %10
  %11 = phi i64 [ 0, %4 ], [ %19, %10 ]
  %12 = phi i32 [ 0, %4 ], [ %18, %10 ]
  %13 = getelementptr inbounds i8, ptr %5, i64 %11, !dbg !22
  %14 = load i8, ptr %13, align 1, !dbg !22, !tbaa !23
  %15 = zext i8 %14 to i32, !dbg !22
  %16 = trunc i64 %11 to i32, !dbg !24
  %17 = mul nsw i32 %16, %15, !dbg !24
  %18 = add nuw nsw i32 %17, %12, !dbg !25
  %19 = add nuw nsw i64 %11, 1, !dbg !26
  %20 = icmp eq i64 %19, %6, !dbg !13
  br i1 %20, label %7, label %10, !dbg !14, !llvm.loop !27
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-05.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "c603fcc30df558bb2e21932e32ee2636")
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
!15 = !{!16, !16, i64 0}
!16 = !{!"any pointer", !17, i64 0}
!17 = !{!"omnipotent char", !18, i64 0}
!18 = !{!"Simple C/C++ TBAA"}
!19 = !DILocation(line: 0, scope: !10)
!20 = !DILocation(line: 9, column: 2, scope: !10)
!21 = !DILocation(line: 10, column: 2, scope: !10)
!22 = !DILocation(line: 7, column: 8, scope: !10)
!23 = !{!17, !17, i64 0}
!24 = !DILocation(line: 7, column: 18, scope: !10)
!25 = !DILocation(line: 7, column: 6, scope: !10)
!26 = !DILocation(line: 6, column: 25, scope: !10)
!27 = distinct !{!27, !14, !28, !29, !30}
!28 = !DILocation(line: 8, column: 2, scope: !10)
!29 = !{!"llvm.loop.mustprogress"}
!30 = !{!"llvm.loop.unroll.disable"}
