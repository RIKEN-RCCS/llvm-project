; RUN: llc < %s -O2 -fswp -mcpu=a64fx -swpl-debug --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: This loop tries to schedule with the InitiationInterval=55 specified in the pragma.

; ModuleID = './pragma-loop-pipeline-001.c'
source_filename = "./pragma-loop-pipeline-001.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @pipeline_enable(i32 noundef %length, ptr nocapture noundef readonly %value) local_unnamed_addr #0 !dbg !8 {
entry:
  %cmp6 = icmp sgt i32 %length, 0, !dbg !12
  br i1 %cmp6, label %for.body.lr.ph, label %for.cond.cleanup, !dbg !13

for.body.lr.ph:                                   ; preds = %entry
  %0 = load ptr, ptr %value, align 8, !tbaa !14
  %1 = load i8, ptr %0, align 1, !tbaa !18
  %conv = zext i8 %1 to i32
  %2 = add i32 %length, -1, !dbg !13
  %xtraiter = and i32 %length, 7, !dbg !13
  %3 = icmp ult i32 %2, 7, !dbg !13
  br i1 %3, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.lr.ph.new, !dbg !13

for.body.lr.ph.new:                               ; preds = %for.body.lr.ph
  %unroll_iter = and i32 %length, -8, !dbg !13
  br label %for.body, !dbg !13

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %for.body, %for.body.lr.ph
  %add.lcssa.ph = phi i32 [ undef, %for.body.lr.ph ], [ %add.7, %for.body ]
  %i.08.unr = phi i32 [ 0, %for.body.lr.ph ], [ %inc.7, %for.body ]
  %sum.07.unr = phi i32 [ 0, %for.body.lr.ph ], [ %add.7, %for.body ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0, !dbg !13
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil, !dbg !13

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil
  %i.08.epil = phi i32 [ %inc.epil, %for.body.epil ], [ %i.08.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %sum.07.epil = phi i32 [ %add.epil, %for.body.epil ], [ %sum.07.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body.epil ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa ]
  %mul.epil = mul nsw i32 %sum.07.epil, %conv, !dbg !19
  %add.epil = add nsw i32 %mul.epil, %i.08.epil, !dbg !20
  %inc.epil = add nuw nsw i32 %i.08.epil, 1, !dbg !21
  %epil.iter.next = add i32 %epil.iter, 1, !dbg !13
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter, !dbg !13
  br i1 %epil.iter.cmp.not, label %for.cond.cleanup, label %for.body.epil, !dbg !13, !llvm.loop !22

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil, %entry
  %sum.0.lcssa = phi i32 [ 0, %entry ], [ %add.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %add.epil, %for.body.epil ], !dbg !24
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %sum.0.lcssa), !dbg !25
  ret i32 0, !dbg !26

for.body:                                         ; preds = %for.body, %for.body.lr.ph.new
  %i.08 = phi i32 [ 0, %for.body.lr.ph.new ], [ %inc.7, %for.body ]
  %sum.07 = phi i32 [ 0, %for.body.lr.ph.new ], [ %add.7, %for.body ]
  %niter = phi i32 [ 0, %for.body.lr.ph.new ], [ %niter.next.7, %for.body ]
  %mul = mul nsw i32 %sum.07, %conv, !dbg !19
  %add = add nsw i32 %mul, %i.08, !dbg !20
  %inc = or i32 %i.08, 1, !dbg !21
  %mul.1 = mul nsw i32 %add, %conv, !dbg !19
  %add.1 = add nsw i32 %mul.1, %inc, !dbg !20
  %inc.1 = or i32 %i.08, 2, !dbg !21
  %mul.2 = mul nsw i32 %add.1, %conv, !dbg !19
  %add.2 = add nsw i32 %mul.2, %inc.1, !dbg !20
  %inc.2 = or i32 %i.08, 3, !dbg !21
  %mul.3 = mul nsw i32 %add.2, %conv, !dbg !19
  %add.3 = add nsw i32 %mul.3, %inc.2, !dbg !20
  %inc.3 = or i32 %i.08, 4, !dbg !21
  %mul.4 = mul nsw i32 %add.3, %conv, !dbg !19
  %add.4 = add nsw i32 %mul.4, %inc.3, !dbg !20
  %inc.4 = or i32 %i.08, 5, !dbg !21
  %mul.5 = mul nsw i32 %add.4, %conv, !dbg !19
  %add.5 = add nsw i32 %mul.5, %inc.4, !dbg !20
  %inc.5 = or i32 %i.08, 6, !dbg !21
  %mul.6 = mul nsw i32 %add.5, %conv, !dbg !19
  %add.6 = add nsw i32 %mul.6, %inc.5, !dbg !20
  %inc.6 = or i32 %i.08, 7, !dbg !21
  %mul.7 = mul nsw i32 %add.6, %conv, !dbg !19
  %add.7 = add nsw i32 %mul.7, %inc.6, !dbg !20
  %inc.7 = add nuw nsw i32 %i.08, 8, !dbg !21
  %niter.next.7 = add i32 %niter, 8, !dbg !13
  %niter.ncmp.7 = icmp eq i32 %niter.next.7, %unroll_iter, !dbg !13
  br i1 %niter.ncmp.7, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body, !dbg !13, !llvm.loop !27
}

; Function Attrs: nofree nounwind
declare dso_local noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { nofree nounwind "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-001.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "1f8c779f885d351a7b531c988f5da5ed")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)"}
!8 = distinct !DISubprogram(name: "pipeline_enable", scope: !9, file: !9, line: 3, type: !10, scopeLine: 3, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!9 = !DIFile(filename: "./pragma-loop-pipeline-001.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "1f8c779f885d351a7b531c988f5da5ed")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 8, column: 20, scope: !8)
!13 = !DILocation(line: 8, column: 5, scope: !8)
!14 = !{!15, !15, i64 0}
!15 = !{!"any pointer", !16, i64 0}
!16 = !{!"omnipotent char", !17, i64 0}
!17 = !{!"Simple C/C++ TBAA"}
!18 = !{!16, !16, i64 0}
!19 = !DILocation(line: 9, column: 12, scope: !8)
!20 = !DILocation(line: 10, column: 12, scope: !8)
!21 = !DILocation(line: 8, column: 30, scope: !8)
!22 = distinct !{!22, !23}
!23 = !{!"llvm.loop.unroll.disable"}
!24 = !DILocation(line: 0, scope: !8)
!25 = !DILocation(line: 12, column: 2, scope: !8)
!26 = !DILocation(line: 13, column: 2, scope: !8)
!27 = distinct !{!27, !13, !28, !29, !30, !31}
!28 = !DILocation(line: 11, column: 5, scope: !8)
!29 = !{!"llvm.loop.mustprogress"}
!30 = !{!"llvm.loop.vectorize.enable", i1 true}
!31 = !{!"llvm.loop.vectorize.followup_all"}
!32 = distinct !{!32, !13, !28, !29, !33, !34}
!33 = !{!"llvm.loop.isvectorized"}
!34 = !{!"llvm.loop.pipeline.initiationinterval", i32 55}
