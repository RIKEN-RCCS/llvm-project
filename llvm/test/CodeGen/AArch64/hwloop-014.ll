; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-hardwareloops -disable-hwloop --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck --allow-empty %s
; CHECK-NOT: DBG
; ModuleID = '2901-2-14.c'
source_filename = "2901-2-14.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %21, !dbg !14

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !15
  %6 = load i8, ptr %5, align 1, !tbaa !19
  %7 = zext i8 %6 to i32
  br label %11, !dbg !20

8:                                                ; preds = %11
  %9 = mul nsw i32 %14, %7, !dbg !21
  %10 = icmp sgt i32 %9, 1024, !dbg !22
  br i1 %10, label %17, label %11, !dbg !20, !llvm.loop !23

11:                                               ; preds = %4, %8
  %12 = phi i32 [ 0, %4 ], [ %9, %8 ]
  %13 = phi i32 [ 0, %4 ], [ %15, %8 ]
  %14 = add nsw i32 %12, %13, !dbg !27
  %15 = add nuw nsw i32 %13, 1, !dbg !28
  %16 = icmp eq i32 %15, %0, !dbg !13
  br i1 %16, label %17, label %8, !dbg !14, !llvm.loop !23

17:                                               ; preds = %8, %11
  %18 = phi i32 [ 0, %11 ], [ 1, %8 ]
  %19 = phi i32 [ %14, %11 ], [ %9, %8 ]
  %20 = icmp slt i32 %15, %0, !dbg !13
  br label %21

21:                                               ; preds = %17, %2
  %22 = phi i1 [ %3, %2 ], [ %20, %17 ], !dbg !13
  %23 = phi i32 [ 0, %2 ], [ %18, %17 ]
  %24 = phi i32 [ 0, %2 ], [ %19, %17 ], !dbg !29
  br i1 %22, label %27, label %25

25:                                               ; preds = %21
  %26 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %24), !dbg !30
  br label %27, !dbg !31

27:                                               ; preds = %21, %25
  %28 = phi i32 [ 0, %25 ], [ %23, %21 ], !dbg !29
  ret i32 %28, !dbg !32
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-14.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "0a67adf190f501d30c56e880270bb01e")
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
!20 = !DILocation(line: 8, column: 7, scope: !10)
!21 = !DILocation(line: 7, column: 6, scope: !10)
!22 = !DILocation(line: 8, column: 11, scope: !10)
!23 = distinct !{!23, !14, !24, !25, !26}
!24 = !DILocation(line: 10, column: 2, scope: !10)
!25 = !{!"llvm.loop.mustprogress"}
!26 = !{!"llvm.loop.unroll.disable"}
!27 = !DILocation(line: 9, column: 6, scope: !10)
!28 = !DILocation(line: 6, column: 25, scope: !10)
!29 = !DILocation(line: 0, scope: !10)
!30 = !DILocation(line: 11, column: 2, scope: !10)
!31 = !DILocation(line: 12, column: 2, scope: !10)
!32 = !DILocation(line: 13, column: 1, scope: !10)
