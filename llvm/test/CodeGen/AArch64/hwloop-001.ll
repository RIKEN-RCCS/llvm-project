; RUN: llc < %s -mcpu=a64fx -O1  -ffj-swp  -debug-hardwareloops --pass-remarks=hardware-loops -debug-aarch64tti -o - 2>&1 | FileCheck %s
; CHECK: HardwareLoopInsertion succeeded
; ModuleID = '2901-2.c'
source_filename = "2901-2.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %8, !dbg !14

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !15
  %6 = load i8, ptr %5, align 1, !tbaa !19
  %7 = zext i8 %6 to i32
  br label %11, !dbg !14

8:                                                ; preds = %11, %2
  %9 = phi i32 [ 0, %2 ], [ %15, %11 ], !dbg !20
  %10 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %9), !dbg !21
  ret i32 0, !dbg !22

11:                                               ; preds = %4, %11
  %12 = phi i32 [ 0, %4 ], [ %16, %11 ]
  %13 = phi i32 [ 0, %4 ], [ %15, %11 ]
  %14 = mul nsw i32 %13, %7, !dbg !23
  %15 = add nsw i32 %14, %12, !dbg !24
  %16 = add nuw nsw i32 %12, 1, !dbg !25
  %17 = icmp eq i32 %16, %0, !dbg !13
  br i1 %17, label %8, label %11, !dbg !14, !llvm.loop !26
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "d75aab33407b44335b9de39a213ff2a2")
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
!19 = !{!17, !17, i64 0}
!20 = !DILocation(line: 0, scope: !10)
!21 = !DILocation(line: 10, column: 2, scope: !10)
!22 = !DILocation(line: 11, column: 2, scope: !10)
!23 = !DILocation(line: 7, column: 6, scope: !10)
!24 = !DILocation(line: 8, column: 6, scope: !10)
!25 = !DILocation(line: 6, column: 25, scope: !10)
!26 = distinct !{!26, !14, !27, !28, !29}
!27 = !DILocation(line: 9, column: 2, scope: !10)
!28 = !{!"llvm.loop.mustprogress"}
!29 = !{!"llvm.loop.unroll.disable"}