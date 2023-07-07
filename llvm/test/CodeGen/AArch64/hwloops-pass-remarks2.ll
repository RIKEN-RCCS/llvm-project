; RUN: llc -mcpu=a64fx -O1 -fswp -o /dev/null -pass-remarks-filter=hardware-loops -pass-remarks-output=- %s 2>&1 | FileCheck %s
;
; CHECK: --- !Missed
; CHECK-NEXT: Pass:            hardware-loops
; CHECK-NEXT: Name:            HWLoopNoCandidate
; CHECK-NEXT: DebugLoc:        { File: a.c, Line: 13, Column: 9 }
; CHECK-NEXT: Function:        func
; CHECK-NEXT: Args:
; CHECK-NEXT:   - String:          'hardware-loop not created: '
; CHECK-NEXT:   - String:          'loop is not a candidate(reason=Loop count cannot be calculated)'
; CHECK: --- !Missed
; CHECK-NEXT: Pass:            hardware-loops
; CHECK-NEXT: Name:            HWLoopNotProfitable
; CHECK-NEXT: DebugLoc:        { File: a.c, Line: 10, Column: 5 }
; CHECK-NEXT: Function:        func
; CHECK-NEXT: Args:
; CHECK-NEXT:   - String:          'hardware-loop not created: '
; CHECK-NEXT:   - String:          'it''s not profitable to create a hardware-loop(reason=Not the innermost loop)'
;
; ModuleID = 'a.c'
source_filename = "a.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linug-unknown-gnu"

%struct.anon = type { i32, i32 }

@x = dso_local local_unnamed_addr global ptr null, align 8
@b = external dso_local local_unnamed_addr global [1000 x double], align 8
@a = external dso_local local_unnamed_addr global [1000 x double], align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(4,4)
define dso_local void @func() local_unnamed_addr #0 !dbg !7 {
entry:
  %0 = load ptr, ptr @x, align 8, !dbg !10, !tbaa !11
  %1 = load i32, ptr %0, align 4, !dbg !15, !tbaa !16
  %cmp216 = icmp slt i32 %1, 1001
  br i1 %cmp216, label %for.body.us.preheader, label %for.cond.cleanup, !dbg !19

for.body.us.preheader:                            ; preds = %entry
  %sub = add i32 %1, -1
  %b = getelementptr inbounds %struct.anon, ptr %0, i64 0, i32 1, !dbg !20
  %2 = load i32, ptr %b, align 4, !dbg !20, !tbaa !21
  %3 = sext i32 %sub to i64, !dbg !22
  %4 = sext i32 %2 to i64, !dbg !22
  br label %for.body.us, !dbg !22

for.body.us:                                      ; preds = %for.body.us.preheader, %for.cond1.for.cond.cleanup3_crit_edge.us
  %nl.019.us = phi i32 [ %inc.us, %for.cond1.for.cond.cleanup3_crit_edge.us ], [ 0, %for.body.us.preheader ]
  br label %for.body4.us, !dbg !19

for.body4.us:                                     ; preds = %for.body.us, %for.body4.us
  %indvars.iv21 = phi i64 [ 0, %for.body.us ], [ %indvars.iv.next22, %for.body4.us ]
  %indvars.iv = phi i64 [ %3, %for.body.us ], [ %indvars.iv.next, %for.body4.us ]
  %indvars.iv.next22 = add nuw nsw i64 %indvars.iv21, 1, !dbg !23
  %5 = sub nsw i64 999, %indvars.iv21, !dbg !24
  %arrayidx.us = getelementptr inbounds [1000 x double], ptr @b, i64 0, i64 %5, !dbg !25
  %6 = load double, ptr %arrayidx.us, align 8, !dbg !25, !tbaa !26
  %arrayidx7.us = getelementptr inbounds [1000 x double], ptr @a, i64 0, i64 %indvars.iv, !dbg !28
  %7 = load double, ptr %arrayidx7.us, align 8, !dbg !29, !tbaa !26
  %add8.us = fadd fast double %7, %6, !dbg !29
  store double %add8.us, ptr %arrayidx7.us, align 8, !dbg !29, !tbaa !26
  %indvars.iv.next = add i64 %indvars.iv, %4, !dbg !30
  %cmp2.us = icmp slt i64 %indvars.iv.next, 1000, !dbg !31
  br i1 %cmp2.us, label %for.body4.us, label %for.cond1.for.cond.cleanup3_crit_edge.us, !dbg !19, !llvm.loop !32

for.cond1.for.cond.cleanup3_crit_edge.us:         ; preds = %for.body4.us
  %inc.us = add nuw nsw i32 %nl.019.us, 1, !dbg !36
  %exitcond.not = icmp eq i32 %inc.us, 10000, !dbg !37
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body.us, !dbg !22, !llvm.loop !38

for.cond.cleanup:                                 ; preds = %for.cond1.for.cond.cleanup3_crit_edge.us, %entry
  ret void, !dbg !40
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(4,4) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4 ()", isOptimized: true, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "a.c", directory: "/src")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 7, !"uwtable", i32 2}
!5 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!"clang version 15.0.4 ()"}
!7 = distinct !DISubprogram(name: "func", scope: !1, file: !1, line: 4, type: !8, scopeLine: 5, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !9)
!8 = !DISubroutineType(types: !9)
!9 = !{}
!10 = !DILocation(line: 6, column: 14, scope: !7)
!11 = !{!12, !12, i64 0}
!12 = !{!"any pointer", !13, i64 0}
!13 = !{!"omnipotent char", !14, i64 0}
!14 = !{!"Simple C/C++ TBAA"}
!15 = !DILocation(line: 6, column: 17, scope: !7)
!16 = !{!17, !18, i64 0}
!17 = !{!"", !18, i64 0, !18, i64 4}
!18 = !{!"int", !13, i64 0}
!19 = !DILocation(line: 13, column: 9, scope: !7)
!20 = !DILocation(line: 7, column: 17, scope: !7)
!21 = !{!17, !18, i64 4}
!22 = !DILocation(line: 10, column: 5, scope: !7)
!23 = !DILocation(line: 14, column: 15, scope: !7)
!24 = !DILocation(line: 15, column: 28, scope: !7)
!25 = !DILocation(line: 15, column: 21, scope: !7)
!26 = !{!27, !27, i64 0}
!27 = !{!"double", !13, i64 0}
!28 = !DILocation(line: 15, column: 13, scope: !7)
!29 = !DILocation(line: 15, column: 18, scope: !7)
!30 = !DILocation(line: 13, column: 40, scope: !7)
!31 = !DILocation(line: 13, column: 30, scope: !7)
!32 = distinct !{!32, !19, !33, !34, !35}
!33 = !DILocation(line: 16, column: 9, scope: !7)
!34 = !{!"llvm.loop.mustprogress"}
!35 = !{!"llvm.loop.unroll.disable"}
!36 = !DILocation(line: 10, column: 36, scope: !7)
!37 = !DILocation(line: 10, column: 25, scope: !7)
!38 = distinct !{!38, !22, !39, !34, !35}
!39 = !DILocation(line: 17, column: 5, scope: !7)
!40 = !DILocation(line: 18, column: 1, scope: !7)
