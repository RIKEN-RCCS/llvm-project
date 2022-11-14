; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: (reason=CALL or ASM exists)

; ModuleID = '2901-2-08.c'
source_filename = "2901-2-08.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %6, !dbg !14

4:                                                ; preds = %2
  %5 = zext i32 %0 to i64, !dbg !13
  br label %9, !dbg !14

6:                                                ; preds = %9, %2
  %7 = phi i32 [ 0, %2 ], [ %22, %9 ], !dbg !15
  %8 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %7), !dbg !16
  ret i32 0, !dbg !17

9:                                                ; preds = %4, %9
  %10 = phi i64 [ 0, %4 ], [ %25, %9 ]
  %11 = phi i32 [ 0, %4 ], [ %22, %9 ]
  %12 = load ptr, ptr %1, align 8, !dbg !18, !tbaa !19
  %13 = getelementptr inbounds i8, ptr %12, i64 %10, !dbg !18
  %14 = load i8, ptr %13, align 1, !dbg !18, !tbaa !23
  %15 = zext i8 %14 to i32, !dbg !18
  %16 = trunc i64 %10 to i32, !dbg !24
  %17 = add nsw i32 %16, %0, !dbg !24
  %18 = trunc i64 %10 to i32, !dbg !25
  %19 = shl i32 %18, 1, !dbg !25
  %20 = mul i32 %19, %17, !dbg !25
  %21 = mul i32 %20, %15, !dbg !28
  %22 = add nsw i32 %21, %11, !dbg !29
  %23 = trunc i64 %10 to i32, !dbg !30
  %24 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %23), !dbg !30
  %25 = add nuw nsw i64 %10, 1, !dbg !31
  %26 = icmp eq i64 %25, %5, !dbg !13
  br i1 %26, label %6, label %9, !dbg !14, !llvm.loop !32
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn uwtable vscale_range(1,16)
define dso_local i32 @sub(i32 noundef %0, i32 noundef %1) local_unnamed_addr #1 !dbg !26 {
  %3 = shl nsw i32 %0, 1, !dbg !36
  %4 = mul nsw i32 %3, %1, !dbg !37
  ret i32 %4, !dbg !38
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { mustprogress nofree norecurse nosync nounwind readnone willreturn uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-08.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "f7e71907dd9c36cda1e73d282aa51060")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 6, type: !11, scopeLine: 6, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 8, column: 17, scope: !10)
!14 = !DILocation(line: 8, column: 2, scope: !10)
!15 = !DILocation(line: 0, scope: !10)
!16 = !DILocation(line: 12, column: 2, scope: !10)
!17 = !DILocation(line: 13, column: 2, scope: !10)
!18 = !DILocation(line: 9, column: 12, scope: !10)
!19 = !{!20, !20, i64 0}
!20 = !{!"any pointer", !21, i64 0}
!21 = !{!"omnipotent char", !22, i64 0}
!22 = !{!"Simple C/C++ TBAA"}
!23 = !{!21, !21, i64 0}
!24 = !DILocation(line: 9, column: 30, scope: !10)
!25 = !DILocation(line: 17, column: 11, scope: !26, inlinedAt: !27)
!26 = distinct !DISubprogram(name: "sub", scope: !1, file: !1, line: 16, type: !11, scopeLine: 16, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!27 = distinct !DILocation(line: 9, column: 8, scope: !10)
!28 = !DILocation(line: 17, column: 13, scope: !26, inlinedAt: !27)
!29 = !DILocation(line: 9, column: 6, scope: !10)
!30 = !DILocation(line: 10, column: 3, scope: !10)
!31 = !DILocation(line: 8, column: 25, scope: !10)
!32 = distinct !{!32, !14, !33, !34, !35}
!33 = !DILocation(line: 11, column: 2, scope: !10)
!34 = !{!"llvm.loop.mustprogress"}
!35 = !{!"llvm.loop.unroll.disable"}
!36 = !DILocation(line: 17, column: 11, scope: !26)
!37 = !DILocation(line: 17, column: 13, scope: !26)
!38 = !DILocation(line: 17, column: 3, scope: !26)
