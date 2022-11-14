; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: HardwareLoopInsertion succeeded

; ModuleID = '2901-2-04.c'
source_filename = "2901-2-04.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !9 {
  %3 = icmp sgt i32 %0, 0, !dbg !12
  br i1 %3, label %4, label %10, !dbg !13

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !14
  %6 = load i8, ptr %5, align 1, !tbaa !18
  %7 = zext i8 %6 to i32
  %8 = zext i32 %0 to i64, !dbg !12
  %9 = zext i32 %0 to i64
  br label %13, !dbg !13

10:                                               ; preds = %17, %2
  %11 = phi i32 [ 0, %2 ], [ %28, %17 ], !dbg !19
  %12 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %11), !dbg !20
  ret i32 0, !dbg !21

13:                                               ; preds = %4, %17
  %14 = phi i64 [ 0, %4 ], [ %18, %17 ]
  %15 = phi i32 [ 0, %4 ], [ %28, %17 ]
  %16 = mul nsw i32 %15, %7, !dbg !22
  br label %20, !dbg !23

17:                                               ; preds = %20
  %18 = add nuw nsw i64 %14, 1, !dbg !13
  %19 = icmp eq i64 %18, %8, !dbg !12
  br i1 %19, label %10, label %13, !dbg !13, !llvm.loop !24

20:                                               ; preds = %13, %20
  %21 = phi i64 [ %14, %13 ], [ %29, %20 ]
  %22 = phi i32 [ %16, %13 ], [ %28, %20 ]
  %23 = getelementptr inbounds i8, ptr %5, i64 %21, !dbg !28
  %24 = load i8, ptr %23, align 1, !dbg !28, !tbaa !18
  %25 = zext i8 %24 to i32, !dbg !28
  %26 = mul nsw i32 %22, %25, !dbg !29
  %27 = trunc i64 %21 to i32, !dbg !30
  %28 = sub nsw i32 %26, %27, !dbg !30
  %29 = add nuw nsw i64 %21, 1, !dbg !31
  %30 = icmp eq i64 %29, %9, !dbg !32
  br i1 %30, label %17, label %20, !dbg !23, !llvm.loop !33
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git b4284707eafc4de5690f3eb6721462d210cd3c07)", isOptimized: true, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-04.c", directory: "/home/kawashima/swpl-test/MT/2901/2901-2")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 7, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 2}
!7 = !{i32 7, !"frame-pointer", i32 1}
!8 = !{!"clang version 15.0.2 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git b4284707eafc4de5690f3eb6721462d210cd3c07)"}
!9 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 4, type: !10, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 6, column: 17, scope: !9)
!13 = !DILocation(line: 6, column: 2, scope: !9)
!14 = !{!15, !15, i64 0}
!15 = !{!"any pointer", !16, i64 0}
!16 = !{!"omnipotent char", !17, i64 0}
!17 = !{!"Simple C/C++ TBAA"}
!18 = !{!16, !16, i64 0}
!19 = !DILocation(line: 0, scope: !9)
!20 = !DILocation(line: 13, column: 2, scope: !9)
!21 = !DILocation(line: 15, column: 1, scope: !9)
!22 = !DILocation(line: 7, column: 6, scope: !9)
!23 = !DILocation(line: 8, column: 3, scope: !9)
!24 = distinct !{!24, !13, !25, !26, !27}
!25 = !DILocation(line: 12, column: 2, scope: !9)
!26 = !{!"llvm.loop.mustprogress"}
!27 = !{!"llvm.loop.unroll.disable"}
!28 = !DILocation(line: 9, column: 12, scope: !9)
!29 = !DILocation(line: 9, column: 10, scope: !9)
!30 = !DILocation(line: 10, column: 10, scope: !9)
!31 = !DILocation(line: 8, column: 26, scope: !9)
!32 = !DILocation(line: 8, column: 18, scope: !9)
!33 = distinct !{!33, !23, !34, !26, !27}
!34 = !DILocation(line: 11, column: 3, scope: !9)
