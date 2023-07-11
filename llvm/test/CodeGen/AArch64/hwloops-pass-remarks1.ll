; RUN: llc -mcpu=a64fx -O1 -fswp -o /dev/null -pass-remarks-filter=hardware-loops -pass-remarks-output=- %s 2>&1 | FileCheck %s
;
; CHECK: --- !Passed
; CHECK-NEXT: Pass:            hardware-loops
; CHECK-NEXT: Name:            HWLoop
; CHECK-NEXT: DebugLoc:        { File: a.c, Line: 8, Column: 9 }
; CHECK-NEXT: Function:        func
; CHECK-NEXT: Args:
; CHECK-NEXT: - String:          hardware-loop created

; ModuleID = 'a.c'
source_filename = "a.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

@a = external global [1000 x double], align 8
@b = external local_unnamed_addr global [1000 x double], align 8

; Function Attrs: nounwind uwtable vscale_range(4,4)
define dso_local void @func() local_unnamed_addr #0 !dbg !9 {
entry:
  br label %for.cond1.preheader, !dbg !12

for.cond1.preheader:                              ; preds = %entry, %for.cond.cleanup3
  %nl.018 = phi i32 [ 0, %entry ], [ %inc, %for.cond.cleanup3 ]
  br label %for.body4, !dbg !13

for.cond.cleanup:                                 ; preds = %for.cond.cleanup3
  ret void, !dbg !14

for.cond.cleanup3:                                ; preds = %for.body4
  tail call void @dummy(ptr noundef nonnull @a) #2, !dbg !15
  %inc = add nuw nsw i32 %nl.018, 1, !dbg !16
  %exitcond.not = icmp eq i32 %inc, 30000, !dbg !17
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.cond1.preheader, !dbg !12, !llvm.loop !18

for.body4:                                        ; preds = %for.cond1.preheader, %for.body4
  %indvars.iv = phi i64 [ 998, %for.cond1.preheader ], [ %indvars.iv.next, %for.body4 ]
  %arrayidx = getelementptr inbounds [1000 x double], ptr @a, i64 0, i64 %indvars.iv, !dbg !22
  %0 = load double, ptr %arrayidx, align 8, !dbg !22, !tbaa !23
  %arrayidx6 = getelementptr inbounds [1000 x double], ptr @b, i64 0, i64 %indvars.iv, !dbg !27
  %1 = load double, ptr %arrayidx6, align 8, !dbg !27, !tbaa !23
  %add = fadd fast double %1, %0, !dbg !28
  %2 = add nuw nsw i64 %indvars.iv, 1, !dbg !29
  %arrayidx9 = getelementptr inbounds [1000 x double], ptr @a, i64 0, i64 %2, !dbg !30
  store double %add, ptr %arrayidx9, align 8, !dbg !31, !tbaa !23
  %indvars.iv.next = add nsw i64 %indvars.iv, -1, !dbg !32
  %cmp2.not = icmp eq i64 %indvars.iv, 0, !dbg !33
  br i1 %cmp2.not, label %for.cond.cleanup3, label %for.body4, !dbg !13, !llvm.loop !34
}

declare void @dummy(ptr noundef) local_unnamed_addr #1

attributes #0 = { nounwind uwtable vscale_range(4,4) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #2 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4 ()", isOptimized: true, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "a.c", directory: "none")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 7, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 2}
!7 = !{i32 7, !"frame-pointer", i32 1}
!8 = !{!"clang version 15.0.4 ()"}
!9 = distinct !DISubprogram(name: "func", scope: !1, file: !1, line: 4, type: !10, scopeLine: 5, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 7, column: 5, scope: !9)
!13 = !DILocation(line: 8, column: 9, scope: !9)
!14 = !DILocation(line: 13, column: 1, scope: !9)
!15 = !DILocation(line: 11, column: 9, scope: !9)
!16 = !DILocation(line: 7, column: 38, scope: !9)
!17 = !DILocation(line: 7, column: 25, scope: !9)
!18 = distinct !{!18, !12, !19, !20, !21}
!19 = !DILocation(line: 12, column: 5, scope: !9)
!20 = !{!"llvm.loop.mustprogress"}
!21 = !{!"llvm.loop.unroll.disable"}
!22 = !DILocation(line: 9, column: 22, scope: !9)
!23 = !{!24, !24, i64 0}
!24 = !{!"double", !25, i64 0}
!25 = !{!"omnipotent char", !26, i64 0}
!26 = !{!"Simple C/C++ TBAA"}
!27 = !DILocation(line: 9, column: 29, scope: !9)
!28 = !DILocation(line: 9, column: 27, scope: !9)
!29 = !DILocation(line: 9, column: 16, scope: !9)
!30 = !DILocation(line: 9, column: 13, scope: !9)
!31 = !DILocation(line: 9, column: 20, scope: !9)
!32 = !DILocation(line: 8, column: 41, scope: !9)
!33 = !DILocation(line: 8, column: 34, scope: !9)
!34 = distinct !{!34, !13, !35, !20, !21}
!35 = !DILocation(line: 10, column: 9, scope: !9)
