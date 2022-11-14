; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp  -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: (reason=CALL or ASM exists)
; ModuleID = '2901-2-12.c'
source_filename = "2901-2-12.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@A = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4
@B = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4

; Function Attrs: nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %6, !dbg !14

4:                                                ; preds = %2
  %5 = zext i32 %0 to i64, !dbg !13
  br label %9, !dbg !14

6:                                                ; preds = %9, %2
  %7 = phi double [ 0.000000e+00, %2 ], [ %19, %9 ], !dbg !15
  %8 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, double noundef %7), !dbg !16
  ret i32 0, !dbg !17

9:                                                ; preds = %4, %9
  %10 = phi i64 [ 0, %4 ], [ %20, %9 ]
  %11 = phi double [ 0.000000e+00, %4 ], [ %19, %9 ]
  %12 = load ptr, ptr %1, align 8, !dbg !18, !tbaa !19
  %13 = getelementptr inbounds i8, ptr %12, i64 %10, !dbg !18
  %14 = load i8, ptr %13, align 1, !dbg !18, !tbaa !23
  %15 = zext i8 %14 to i32, !dbg !18
  %16 = trunc i64 %10 to i32, !dbg !24
  %17 = mul nsw i32 %16, %15, !dbg !24
  %18 = sitofp i32 %17 to double, !dbg !18
  %19 = fadd double %11, %18, !dbg !25
  tail call void asm sideeffect " nop", ""() #2, !dbg !26, !srcloc !27
  %20 = add nuw nsw i64 %10, 1, !dbg !28
  %21 = icmp eq i64 %20, %5, !dbg !13
  br i1 %21, label %6, label %9, !dbg !14, !llvm.loop !29
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #2 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-12.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "babac8f38247a15bd301fb87de2f305a")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 8, type: !11, scopeLine: 8, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 11, column: 17, scope: !10)
!14 = !DILocation(line: 11, column: 2, scope: !10)
!15 = !DILocation(line: 0, scope: !10)
!16 = !DILocation(line: 15, column: 2, scope: !10)
!17 = !DILocation(line: 16, column: 2, scope: !10)
!18 = !DILocation(line: 12, column: 10, scope: !10)
!19 = !{!20, !20, i64 0}
!20 = !{!"any pointer", !21, i64 0}
!21 = !{!"omnipotent char", !22, i64 0}
!22 = !{!"Simple C/C++ TBAA"}
!23 = !{!21, !21, i64 0}
!24 = !DILocation(line: 12, column: 20, scope: !10)
!25 = !DILocation(line: 12, column: 8, scope: !10)
!26 = !DILocation(line: 13, column: 5, scope: !10)
!27 = !{i64 294}
!28 = !DILocation(line: 11, column: 25, scope: !10)
!29 = distinct !{!29, !14, !30, !31, !32}
!30 = !DILocation(line: 14, column: 2, scope: !10)
!31 = !{!"llvm.loop.mustprogress"}
!32 = !{!"llvm.loop.unroll.disable"}
