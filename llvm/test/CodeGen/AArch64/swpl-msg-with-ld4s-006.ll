; RUN: llc -O3 -mcpu=a64fx -pass-remarks=aarch64-swpipeliner -pass-remarks-missed=aarch64-swpipeliner -pass-remarks-analysis=aarch64-swpipeliner -o /dev/null %s 2>&1 |& FileCheck %s
; CHECK: remark: sample.c:10:3: SVE instruction latency is calculated at 512bit.
; CHECK: remark: sample.c:10:3: distributed loop: 3 of 3 Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: sample.c:10:3: distributed loop: 3 of 3 The number of COPY instructions in the kernel loop for software pipelining is 12.
; CHECK: remark: sample.c:10:3: distributed loop: 3 of 3 software pipelining
; CHECK: remark: sample.c:10:3: distributed loop: 1 of 3 Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: sample.c:10:3: distributed loop: 1 of 3 The number of COPY instructions in the kernel loop for software pipelining is 0.
; CHECK: remark: sample.c:10:3: distributed loop: 1 of 3 software pipelining
; CHECK: remark: sample.c:10:3: distributed loop: 2 of 3 Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: sample.c:10:3: distributed loop: 2 of 3 The number of COPY instructions in the kernel loop for software pipelining is 2.
; CHECK: remark: sample.c:10:3: distributed loop: 2 of 3 software pipelining

; ModuleID = './sample.c'
source_filename = "./sample.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4)
define dso_local float @foo(i32 noundef %n, ptr nocapture noundef %A, ptr nocapture noundef readonly %B, ptr nocapture noundef writeonly %C, ptr nocapture noundef readonly %D, ptr nocapture noundef readonly %E, ptr nocapture noundef readonly %C1, ptr nocapture noundef readonly %D1, ptr nocapture noundef readnone %E1) local_unnamed_addr #0 !dbg !9 {
entry:
  %cmp30 = icmp sgt i32 %n, 0, !dbg !13
  br i1 %cmp30, label %for.body.preheader, label %for.end, !dbg !14

for.body.preheader:                               ; preds = %entry
  %0 = zext nneg i32 %n to i64, !dbg !14
  %load_initial = load float, ptr %A, align 4
  br label %for.body.ldist1, !dbg !14

for.body.ldist1:                                  ; preds = %for.body.ldist1, %for.body.preheader
  %store_forwarded = phi float [ %load_initial, %for.body.preheader ], [ %3, %for.body.ldist1 ]
  %indvars.iv.ldist1 = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next.ldist1, %for.body.ldist1 ]
  %arrayidx2.ldist1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.ldist1, !dbg !15
  %1 = load float, ptr %arrayidx2.ldist1, align 4, !dbg !15, !tbaa !16
  %arrayidx4.ldist1 = getelementptr inbounds float, ptr %C1, i64 %indvars.iv.ldist1, !dbg !20
  %2 = load float, ptr %arrayidx4.ldist1, align 4, !dbg !20, !tbaa !16
  %3 = tail call float @llvm.fmuladd.f32(float %store_forwarded, float %1, float %2), !dbg !21
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1, !dbg !22
  %arrayidx6.ldist1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1, !dbg !23
  store float %3, ptr %arrayidx6.ldist1, align 4, !dbg !24, !tbaa !16
  %exitcond.not.ldist1 = icmp eq i64 %indvars.iv.next.ldist1, %0, !dbg !13
  br i1 %exitcond.not.ldist1, label %for.body.ldist2, label %for.body.ldist1, !dbg !14, !llvm.loop !25

for.body.ldist2:                                  ; preds = %for.body.ldist1, %for.body.ldist2
  %indvars.iv.ldist2 = phi i64 [ %indvars.iv.next.ldist2, %for.body.ldist2 ], [ 0, %for.body.ldist1 ]
  %indvars.iv.next.ldist2 = add nuw nsw i64 %indvars.iv.ldist2, 1, !dbg !22
  %arrayidx10.ldist2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.ldist2, !dbg !32
  %4 = load float, ptr %arrayidx10.ldist2, align 4, !dbg !32, !tbaa !16
  %arrayidx12.ldist2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.ldist2, !dbg !33
  %5 = load float, ptr %arrayidx12.ldist2, align 4, !dbg !33, !tbaa !16
  %arrayidx14.ldist2 = getelementptr inbounds float, ptr %D1, i64 %indvars.iv.ldist2, !dbg !34
  %6 = load float, ptr %arrayidx14.ldist2, align 4, !dbg !34, !tbaa !16
  %7 = tail call float @llvm.fmuladd.f32(float %4, float %5, float %6), !dbg !35
  %arrayidx16.ldist2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.ldist2, !dbg !36
  store float %7, ptr %arrayidx16.ldist2, align 4, !dbg !37, !tbaa !16
  %exitcond.not.ldist2 = icmp eq i64 %indvars.iv.next.ldist2, %0, !dbg !13
  br i1 %exitcond.not.ldist2, label %for.body, label %for.body.ldist2, !dbg !14, !llvm.loop !38

for.body:                                         ; preds = %for.body.ldist2, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.ldist2 ]
  %z.032 = phi float [ %add8, %for.body ], [ 0.000000e+00, %for.body.ldist2 ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !22
  %8 = trunc i64 %indvars.iv to i32, !dbg !40
  %9 = add i32 %8, %n, !dbg !40
  %conv = sitofp i32 %9 to float, !dbg !40
  %add8 = fadd float %z.032, %conv, !dbg !41
  %exitcond.not = icmp eq i64 %indvars.iv.next, %0, !dbg !13
  br i1 %exitcond.not, label %for.end, label %for.body, !dbg !14, !llvm.loop !42

for.end:                                          ; preds = %for.body, %entry
  %z.0.lcssa = phi float [ 0.000000e+00, %entry ], [ %add8, %for.body ], !dbg !44
  ret float %z.0.lcssa, !dbg !45
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(4,4) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 18.1.8", isOptimized: true, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "./sample.c", directory: ".")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 8, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 2}
!7 = !{i32 7, !"frame-pointer", i32 1}
!8 = !{!"clang version 18.1.8"}
!9 = distinct !DISubprogram(name: "foo", scope: !10, file: !10, line: 2, type: !11, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!10 = !DIFile(filename: "sample.c", directory: ".")
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 10, column: 17, scope: !9)
!14 = !DILocation(line: 10, column: 3, scope: !9)
!15 = !DILocation(line: 11, column: 23, scope: !9)
!16 = !{!17, !17, i64 0}
!17 = !{!"float", !18, i64 0}
!18 = !{!"omnipotent char", !19, i64 0}
!19 = !{!"Simple C/C++ TBAA"}
!20 = !DILocation(line: 11, column: 30, scope: !9)
!21 = !DILocation(line: 11, column: 28, scope: !9)
!22 = !DILocation(line: 11, column: 9, scope: !9)
!23 = !DILocation(line: 11, column: 5, scope: !9)
!24 = !DILocation(line: 11, column: 14, scope: !9)
!25 = distinct !{!25, !14, !26, !27, !28, !29, !30, !31}
!26 = !DILocation(line: 15, column: 3, scope: !9)
!27 = !{!"llvm.loop.mustprogress"}
!28 = !{!"llvm.loop.unroll.disable"}
!29 = !{!"llvm.loop.pipeline.enable"}
!30 = !{!"llvm.loop.pipeline.nodep"}
!31 = !{!"llvm.loop.distributed4swpl", i32 3, i32 2}
!32 = !DILocation(line: 13, column: 12, scope: !9)
!33 = !DILocation(line: 13, column: 19, scope: !9)
!34 = !DILocation(line: 13, column: 26, scope: !9)
!35 = !DILocation(line: 13, column: 24, scope: !9)
!36 = !DILocation(line: 13, column: 5, scope: !9)
!37 = !DILocation(line: 13, column: 10, scope: !9)
!38 = distinct !{!38, !14, !26, !27, !28, !29, !30, !39}
!39 = !{!"llvm.loop.distributed4swpl", i32 3, i32 1}
!40 = !DILocation(line: 12, column: 10, scope: !9)
!41 = !DILocation(line: 12, column: 7, scope: !9)
!42 = distinct !{!42, !14, !26, !27, !28, !29, !30, !43}
!43 = !{!"llvm.loop.distributed4swpl", i32 3, i32 3}
!44 = !DILocation(line: 0, scope: !9)
!45 = !DILocation(line: 16, column: 3, scope: !9)
