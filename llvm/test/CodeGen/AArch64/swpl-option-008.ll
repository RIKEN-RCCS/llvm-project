; RUN: llc < %s -mcpu=a64fx -O1 -fswp -debug-aarch64tti -o - 2>&1 | FileCheck %s
; CHECK: enableSWP() is false
; ModuleID = '2901-1.ll'
source_filename = "2901-1.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind
define dso_local i32 @main(i32 %argc, i8** nocapture readonly %argv) local_unnamed_addr #0 !dbg !7 {
entry:
  %cmp8 = icmp sgt i32 %argc, 0, !dbg !9
  br i1 %cmp8, label %for.body.lr.ph, label %for.cond.cleanup, !dbg !10

for.body.lr.ph:                                   ; preds = %entry
  %0 = load i8*, i8** %argv, align 8, !dbg !11, !tbaa !12
  %1 = load i8, i8* %0, align 1, !dbg !11, !tbaa !16
  %conv = zext i8 %1 to i32, !dbg !11
  br label %for.body, !dbg !10

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %sum.0.lcssa = phi i32 [ 0, %entry ], [ %add, %for.body ], !dbg !11
  %call = call i32 (i8*, ...) @printf(i8* nonnull dereferenceable(1) getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %sum.0.lcssa), !dbg !17
  ret i32 0, !dbg !18

for.body:                                         ; preds = %for.body, %for.body.lr.ph
  %i.010 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %sum.09 = phi i32 [ 0, %for.body.lr.ph ], [ %add, %for.body ]
  %mul = mul nsw i32 %sum.09, %conv, !dbg !19
  %add = add nsw i32 %mul, %i.010, !dbg !20
  %inc = add nuw nsw i32 %i.010, 1, !dbg !21
  %exitcond.not = icmp eq i32 %inc, %argc, !dbg !9
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !dbg !10, !llvm.loop !22
}

; Function Attrs: nofree nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="non-leaf" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="non-leaf" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 11.0.0 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, enums: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-1.c", directory: "2901-1")
!2 = !{}
!3 = !{i32 7, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!7 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !8, scopeLine: 3, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!8 = !DISubroutineType(types: !2)
!9 = !DILocation(line: 5, column: 17, scope: !7)
!10 = !DILocation(line: 5, column: 2, scope: !7)
!11 = !DILocation(line: 0, scope: !7)
!12 = !{!13, !13, i64 0}
!13 = !{!"any pointer", !14, i64 0}
!14 = !{!"omnipotent char", !15, i64 0}
!15 = !{!"Simple C/C++ TBAA"}
!16 = !{!14, !14, i64 0}
!17 = !DILocation(line: 9, column: 2, scope: !7)
!18 = !DILocation(line: 10, column: 2, scope: !7)
!19 = !DILocation(line: 6, column: 6, scope: !7)
!20 = !DILocation(line: 7, column: 6, scope: !7)
!21 = !DILocation(line: 5, column: 25, scope: !7)
!22 = distinct !{!22, !10, !23, !24, !25}
!23 = !DILocation(line: 8, column: 2, scope: !7)
!24 = !{!"llvm.loop.unroll.disable"}
!25 = !{!"llvm.loop.pipeline.disable"}
