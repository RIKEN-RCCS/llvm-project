; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp  -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: canSaveCmo

; ModuleID = '2901-3-05.c'
source_filename = "2901-3-05.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = tail call i32 @sub() #3, !dbg !13
  %4 = icmp sgt i32 %3, 0, !dbg !14
  br i1 %4, label %5, label %16, !dbg !15

5:                                                ; preds = %2
  %6 = icmp sgt i32 %0, 0
  %7 = zext i32 %0 to i64
  %8 = zext i32 %0 to i64
  br label %9, !dbg !15

9:                                                ; preds = %5, %19
  %10 = phi i32 [ 0, %5 ], [ %21, %19 ]
  %11 = phi i32 [ 0, %5 ], [ %20, %19 ]
  br i1 %6, label %12, label %19, !dbg !16

12:                                               ; preds = %9
  %13 = load ptr, ptr %1, align 8, !tbaa !17
  %14 = load i8, ptr %13, align 1, !tbaa !21
  %15 = zext i8 %14 to i64, !dbg !16
  br label %24, !dbg !16

16:                                               ; preds = %19, %2
  %17 = phi i32 [ 0, %2 ], [ %20, %19 ], !dbg !22
  %18 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %17), !dbg !23
  ret i32 0, !dbg !24

19:                                               ; preds = %34, %9
  %20 = phi i32 [ %11, %9 ], [ %36, %34 ], !dbg !25
  %21 = add nuw nsw i32 %10, 1, !dbg !26
  %22 = tail call i32 @sub() #3, !dbg !13
  %23 = icmp slt i32 %21, %22, !dbg !14
  br i1 %23, label %9, label %16, !dbg !15, !llvm.loop !27

24:                                               ; preds = %12, %34
  %25 = phi i64 [ 0, %12 ], [ %37, %34 ]
  %26 = phi i32 [ %11, %12 ], [ %36, %34 ]
  %27 = add nuw nsw i64 %25, %15, !dbg !31
  %28 = trunc i64 %27 to i32, !dbg !32
  %29 = add nsw i32 %26, %28, !dbg !32
  %30 = getelementptr inbounds i8, ptr %13, i64 %25, !dbg !33
  %31 = load i8, ptr %30, align 1, !dbg !33, !tbaa !21
  %32 = zext i8 %31 to i32, !dbg !33
  %33 = mul nsw i32 %29, %32, !dbg !34
  br label %39, !dbg !35

34:                                               ; preds = %39
  %35 = trunc i64 %27 to i32, !dbg !36
  %36 = add nsw i32 %47, %35, !dbg !36
  %37 = add nuw nsw i64 %25, 1, !dbg !37
  %38 = icmp eq i64 %37, %7, !dbg !38
  br i1 %38, label %19, label %24, !dbg !16, !llvm.loop !39

39:                                               ; preds = %24, %39
  %40 = phi i64 [ %25, %24 ], [ %48, %39 ]
  %41 = phi i32 [ %33, %24 ], [ %47, %39 ]
  %42 = getelementptr inbounds i8, ptr %13, i64 %40, !dbg !41
  %43 = load i8, ptr %42, align 1, !dbg !41, !tbaa !21
  %44 = zext i8 %43 to i32, !dbg !41
  %45 = mul nsw i32 %41, %44, !dbg !42
  %46 = trunc i64 %40 to i32, !dbg !43
  %47 = sub nsw i32 %45, %46, !dbg !43
  %48 = add nuw nsw i64 %40, 1, !dbg !44
  %49 = icmp eq i64 %48, %8, !dbg !45
  br i1 %49, label %34, label %39, !dbg !35, !llvm.loop !46
}

declare i32 @sub(...) local_unnamed_addr #1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-3-05.c", directory: "/home/kawashima/swpl-test/MT/2901/2901-3", checksumkind: CSK_MD5, checksum: "e725c0c66ccc08be99eca17f76a40458")
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
!14 = !DILocation(line: 6, column: 16, scope: !10)
!15 = !DILocation(line: 6, column: 1, scope: !10)
!16 = !DILocation(line: 8, column: 3, scope: !10)
!17 = !{!18, !18, i64 0}
!18 = !{!"any pointer", !19, i64 0}
!19 = !{!"omnipotent char", !20, i64 0}
!20 = !{!"Simple C/C++ TBAA"}
!21 = !{!19, !19, i64 0}
!22 = !DILocation(line: 5, column: 7, scope: !10)
!23 = !DILocation(line: 18, column: 3, scope: !10)
!24 = !DILocation(line: 20, column: 1, scope: !10)
!25 = !DILocation(line: 0, scope: !10)
!26 = !DILocation(line: 6, column: 25, scope: !10)
!27 = distinct !{!27, !15, !28, !29, !30}
!28 = !DILocation(line: 17, column: 1, scope: !10)
!29 = !{!"llvm.loop.mustprogress"}
!30 = !{!"llvm.loop.unroll.disable"}
!31 = !DILocation(line: 9, column: 20, scope: !10)
!32 = !DILocation(line: 9, column: 8, scope: !10)
!33 = !DILocation(line: 10, column: 10, scope: !10)
!34 = !DILocation(line: 10, column: 8, scope: !10)
!35 = !DILocation(line: 11, column: 5, scope: !10)
!36 = !DILocation(line: 15, column: 8, scope: !10)
!37 = !DILocation(line: 8, column: 26, scope: !10)
!38 = !DILocation(line: 8, column: 18, scope: !10)
!39 = distinct !{!39, !16, !40, !29, !30}
!40 = !DILocation(line: 16, column: 3, scope: !10)
!41 = !DILocation(line: 12, column: 12, scope: !10)
!42 = !DILocation(line: 12, column: 10, scope: !10)
!43 = !DILocation(line: 13, column: 10, scope: !10)
!44 = !DILocation(line: 11, column: 28, scope: !10)
!45 = !DILocation(line: 11, column: 20, scope: !10)
!46 = distinct !{!46, !35, !47, !29, !30}
!47 = !DILocation(line: 14, column: 5, scope: !10)
