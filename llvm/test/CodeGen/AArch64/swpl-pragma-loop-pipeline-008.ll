; RUN: llc < %s -O2 -mcpu=a64fx -fswp -swpl-debug --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: Specified Swpl disable by option/pragma.

; ModuleID = './pragma-loop-pipeline-008.cpp'
source_filename = "./pragma-loop-pipeline-008.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

; Function Attrs: argmemonly mustprogress nofree norecurse nosync nounwind writeonly uwtable vscale_range(1,16)
define dso_local void @_Z8for_testPii(ptr nocapture noundef writeonly %List, i32 noundef %Length) local_unnamed_addr #0 !dbg !8 {
entry:
  %cmp4 = icmp sgt i32 %Length, 0, !dbg !12
  br i1 %cmp4, label %for.body.preheader, label %for.cond.cleanup, !dbg !13

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %Length to i64, !dbg !12
  %0 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %1 = shl nuw nsw i64 %0, 4, !dbg !13
  %min.iters.check = icmp ugt i64 %1, %wide.trip.count, !dbg !13
  br i1 %min.iters.check, label %for.body.preheader11, label %vector.ph, !dbg !13

vector.ph:                                        ; preds = %for.body.preheader
  %2 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %3 = shl nuw nsw i64 %2, 4, !dbg !13
  %n.mod.vf = urem i64 %wide.trip.count, %3, !dbg !13
  %n.vec = sub nuw nsw i64 %wide.trip.count, %n.mod.vf, !dbg !13
  %4 = tail call <vscale x 4 x i32> @llvm.experimental.stepvector.nxv4i32(), !dbg !13
  %5 = tail call i32 @llvm.vscale.i32(), !dbg !13
  %6 = shl nuw nsw i32 %5, 2, !dbg !13
  %.splatinsert = insertelement <vscale x 4 x i32> poison, i32 %6, i64 0, !dbg !13
  %.splat = shufflevector <vscale x 4 x i32> %.splatinsert, <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer, !dbg !13
  %7 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %8 = shl nuw nsw i32 %7, 2
  %9 = zext i32 %8 to i64
  %10 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %11 = shl nuw nsw i32 %10, 3
  %12 = zext i32 %11 to i64
  %13 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %14 = mul nuw nsw i32 %13, 12
  %15 = zext i32 %14 to i64
  %16 = tail call i64 @llvm.vscale.i64(), !dbg !14
  %17 = shl nuw nsw i64 %16, 4
  br label %vector.body, !dbg !13

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ], !dbg !15
  %vec.ind = phi <vscale x 4 x i32> [ %4, %vector.ph ], [ %vec.ind.next, %vector.body ], !dbg !16
  %step.add = add <vscale x 4 x i32> %vec.ind, %.splat, !dbg !16
  %step.add8 = add <vscale x 4 x i32> %step.add, %.splat, !dbg !16
  %step.add9 = add <vscale x 4 x i32> %step.add8, %.splat, !dbg !16
  %18 = getelementptr inbounds i32, ptr %List, i64 %index, !dbg !17
  %19 = shl <vscale x 4 x i32> %vec.ind, shufflevector (<vscale x 4 x i32> insertelement (<vscale x 4 x i32> poison, i32 1, i32 0), <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer), !dbg !16
  %20 = shl <vscale x 4 x i32> %step.add, shufflevector (<vscale x 4 x i32> insertelement (<vscale x 4 x i32> poison, i32 1, i32 0), <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer), !dbg !16
  %21 = shl <vscale x 4 x i32> %step.add8, shufflevector (<vscale x 4 x i32> insertelement (<vscale x 4 x i32> poison, i32 1, i32 0), <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer), !dbg !16
  %22 = shl <vscale x 4 x i32> %step.add9, shufflevector (<vscale x 4 x i32> insertelement (<vscale x 4 x i32> poison, i32 1, i32 0), <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer), !dbg !16
  store <vscale x 4 x i32> %19, ptr %18, align 4, !dbg !16, !tbaa !18
  %23 = getelementptr inbounds i32, ptr %18, i64 %9, !dbg !16
  store <vscale x 4 x i32> %20, ptr %23, align 4, !dbg !16, !tbaa !18
  %24 = getelementptr inbounds i32, ptr %18, i64 %12, !dbg !16
  store <vscale x 4 x i32> %21, ptr %24, align 4, !dbg !16, !tbaa !18
  %25 = getelementptr inbounds i32, ptr %18, i64 %15, !dbg !16
  store <vscale x 4 x i32> %22, ptr %25, align 4, !dbg !16, !tbaa !18
  %index.next = add nuw i64 %index, %17, !dbg !15
  %vec.ind.next = add <vscale x 4 x i32> %step.add9, %.splat, !dbg !16
  %26 = icmp eq i64 %index.next, %n.vec, !dbg !15
  br i1 %26, label %middle.block, label %vector.body, !dbg !15, !llvm.loop !22

middle.block:                                     ; preds = %vector.body
  %cmp.n = icmp eq i64 %n.mod.vf, 0, !dbg !13
  br i1 %cmp.n, label %for.cond.cleanup, label %for.body.preheader11, !dbg !13

for.body.preheader11:                             ; preds = %for.body.preheader, %middle.block
  %indvars.iv.ph = phi i64 [ 0, %for.body.preheader ], [ %n.vec, %middle.block ]
  %27 = sub nsw i64 %wide.trip.count, %indvars.iv.ph, !dbg !13
  %28 = xor i64 %indvars.iv.ph, -1, !dbg !13
  %29 = add nsw i64 %28, %wide.trip.count, !dbg !13
  %xtraiter = and i64 %27, 7, !dbg !13
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0, !dbg !13
  br i1 %lcmp.mod.not, label %for.body.prol.loopexit, label %for.body.prol, !dbg !13

for.body.prol:                                    ; preds = %for.body.preheader11, %for.body.prol
  %indvars.iv.prol = phi i64 [ %indvars.iv.next.prol, %for.body.prol ], [ %indvars.iv.ph, %for.body.preheader11 ]
  %prol.iter = phi i64 [ %prol.iter.next, %for.body.prol ], [ 0, %for.body.preheader11 ]
  %arrayidx.prol = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.prol, !dbg !17
  %indvars.iv.tr.prol = trunc i64 %indvars.iv.prol to i32, !dbg !16
  %30 = shl i32 %indvars.iv.tr.prol, 1, !dbg !16
  store i32 %30, ptr %arrayidx.prol, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.prol = add nuw nsw i64 %indvars.iv.prol, 1, !dbg !15
  %prol.iter.next = add i64 %prol.iter, 1, !dbg !13
  %prol.iter.cmp.not = icmp eq i64 %prol.iter.next, %xtraiter, !dbg !13
  br i1 %prol.iter.cmp.not, label %for.body.prol.loopexit, label %for.body.prol, !dbg !13, !llvm.loop !28

for.body.prol.loopexit:                           ; preds = %for.body.prol, %for.body.preheader11
  %indvars.iv.unr = phi i64 [ %indvars.iv.ph, %for.body.preheader11 ], [ %indvars.iv.next.prol, %for.body.prol ]
  %31 = icmp ult i64 %29, 7, !dbg !13
  br i1 %31, label %for.cond.cleanup, label %for.body, !dbg !13

for.cond.cleanup:                                 ; preds = %for.body.prol.loopexit, %for.body, %middle.block, %entry
  ret void, !dbg !30

for.body:                                         ; preds = %for.body.prol.loopexit, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next.7, %for.body ], [ %indvars.iv.unr, %for.body.prol.loopexit ]
  %arrayidx = getelementptr inbounds i32, ptr %List, i64 %indvars.iv, !dbg !17
  %indvars.iv.tr = trunc i64 %indvars.iv to i32, !dbg !16
  %32 = shl i32 %indvars.iv.tr, 1, !dbg !16
  store i32 %32, ptr %arrayidx, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !15
  %arrayidx.1 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next, !dbg !17
  %indvars.iv.tr.1 = trunc i64 %indvars.iv.next to i32, !dbg !16
  %33 = shl i32 %indvars.iv.tr.1, 1, !dbg !16
  store i32 %33, ptr %arrayidx.1, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv, 2, !dbg !15
  %arrayidx.2 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.1, !dbg !17
  %indvars.iv.tr.2 = trunc i64 %indvars.iv.next.1 to i32, !dbg !16
  %34 = shl i32 %indvars.iv.tr.2, 1, !dbg !16
  store i32 %34, ptr %arrayidx.2, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv, 3, !dbg !15
  %arrayidx.3 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.2, !dbg !17
  %indvars.iv.tr.3 = trunc i64 %indvars.iv.next.2 to i32, !dbg !16
  %35 = shl i32 %indvars.iv.tr.3, 1, !dbg !16
  store i32 %35, ptr %arrayidx.3, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.3 = add nuw nsw i64 %indvars.iv, 4, !dbg !15
  %arrayidx.4 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.3, !dbg !17
  %indvars.iv.tr.4 = trunc i64 %indvars.iv.next.3 to i32, !dbg !16
  %36 = shl i32 %indvars.iv.tr.4, 1, !dbg !16
  store i32 %36, ptr %arrayidx.4, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.4 = add nuw nsw i64 %indvars.iv, 5, !dbg !15
  %arrayidx.5 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.4, !dbg !17
  %indvars.iv.tr.5 = trunc i64 %indvars.iv.next.4 to i32, !dbg !16
  %37 = shl i32 %indvars.iv.tr.5, 1, !dbg !16
  store i32 %37, ptr %arrayidx.5, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.5 = add nuw nsw i64 %indvars.iv, 6, !dbg !15
  %arrayidx.6 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.5, !dbg !17
  %indvars.iv.tr.6 = trunc i64 %indvars.iv.next.5 to i32, !dbg !16
  %38 = shl i32 %indvars.iv.tr.6, 1, !dbg !16
  store i32 %38, ptr %arrayidx.6, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.6 = add nuw nsw i64 %indvars.iv, 7, !dbg !15
  %arrayidx.7 = getelementptr inbounds i32, ptr %List, i64 %indvars.iv.next.6, !dbg !17
  %indvars.iv.tr.7 = trunc i64 %indvars.iv.next.6 to i32, !dbg !16
  %39 = shl i32 %indvars.iv.tr.7, 1, !dbg !16
  store i32 %39, ptr %arrayidx.7, align 4, !dbg !16, !tbaa !18
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv, 8, !dbg !15
  %exitcond.not.7 = icmp eq i64 %indvars.iv.next.7, %wide.trip.count, !dbg !12
  br i1 %exitcond.not.7, label %for.cond.cleanup, label %for.body, !dbg !13, !llvm.loop !31
}

; Function Attrs: nocallback nofree nosync nounwind readnone willreturn
declare i64 @llvm.vscale.i64() #1

; Function Attrs: nocallback nofree nosync nounwind readnone willreturn
declare <vscale x 4 x i32> @llvm.experimental.stepvector.nxv4i32() #1

; Function Attrs: nocallback nofree nosync nounwind readnone willreturn
declare i32 @llvm.vscale.i32() #1

attributes #0 = { argmemonly mustprogress nofree norecurse nosync nounwind writeonly uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { nocallback nofree nosync nounwind readnone willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-008.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "70312014c379ed0a9d22d67c73e2df35")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)"}
!8 = distinct !DISubprogram(name: "for_test", scope: !9, file: !9, line: 1, type: !10, scopeLine: 1, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!9 = !DIFile(filename: "./pragma-loop-pipeline-008.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "70312014c379ed0a9d22d67c73e2df35")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 4, column: 21, scope: !8)
!13 = !DILocation(line: 4, column: 3, scope: !8)
!14 = !DILocation(line: 0, scope: !8)
!15 = !DILocation(line: 4, column: 32, scope: !8)
!16 = !DILocation(line: 5, column: 13, scope: !8)
!17 = !DILocation(line: 5, column: 5, scope: !8)
!18 = !{!19, !19, i64 0}
!19 = !{!"int", !20, i64 0}
!20 = !{!"omnipotent char", !21, i64 0}
!21 = !{!"Simple C++ TBAA"}
!22 = distinct !{!22, !23}
!23 = distinct !{!23, !13, !24, !25, !26, !27}
!24 = !DILocation(line: 6, column: 3, scope: !8)
!25 = !{!"llvm.loop.mustprogress"}
!26 = !{!"llvm.loop.isvectorized"}
!27 = !{!"llvm.loop.pipeline.disable", i1 true}
!28 = distinct !{!28, !29, !27}
!29 = !{!"llvm.loop.unroll.disable"}
!30 = !DILocation(line: 7, column: 1, scope: !8)
!31 = distinct !{!31, !23}
