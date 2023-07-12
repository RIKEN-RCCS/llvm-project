; RUN: llc < %s -O2 -fswp -mcpu=a64fx -swpl-debug --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: Specified Swpl disable by option/pragma.

; ModuleID = './pragma-loop-pipeline-004.c'
source_filename = "./pragma-loop-pipeline-004.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @pipeline_enable() local_unnamed_addr #0 !dbg !8 {
entry:
  %x = alloca [1000 x i32], align 4
  %y = alloca [1000 x i32], align 4
  %z = alloca [1000 x i32], align 4
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %x) #4, !dbg !12
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %y) #4, !dbg !12
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %z) #4, !dbg !12
  %0 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %1 = shl nuw nsw i64 %0, 4, !dbg !13
  %n.mod.vf = urem i64 1000, %1, !dbg !13
  %n.vec = sub nuw nsw i64 1000, %n.mod.vf, !dbg !13
  %2 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %3 = shl nuw nsw i32 %2, 2
  %4 = zext i32 %3 to i64
  %5 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %6 = shl nuw nsw i32 %5, 3
  %7 = zext i32 %6 to i64
  %8 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %9 = mul nuw nsw i32 %8, 12
  %10 = zext i32 %9 to i64
  %11 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %12 = shl nuw nsw i32 %11, 2
  %13 = zext i32 %12 to i64
  %14 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %15 = shl nuw nsw i32 %14, 3
  %16 = zext i32 %15 to i64
  %17 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %18 = mul nuw nsw i32 %17, 12
  %19 = zext i32 %18 to i64
  %20 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %21 = shl nuw nsw i32 %20, 2
  %22 = zext i32 %21 to i64
  %23 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %24 = shl nuw nsw i32 %23, 3
  %25 = zext i32 %24 to i64
  %26 = tail call i32 @llvm.vscale.i32(), !dbg !14
  %27 = mul nuw nsw i32 %26, 12
  %28 = zext i32 %27 to i64
  %29 = tail call i64 @llvm.vscale.i64(), !dbg !14
  %30 = shl nuw nsw i64 %29, 4
  br label %vector.body, !dbg !13

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ], !dbg !15
  %31 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %index, !dbg !16
  %wide.load = load <vscale x 4 x i32>, ptr %31, align 4, !dbg !16, !tbaa !17
  %32 = getelementptr inbounds i32, ptr %31, i64 %4, !dbg !16
  %wide.load21 = load <vscale x 4 x i32>, ptr %32, align 4, !dbg !16, !tbaa !17
  %33 = getelementptr inbounds i32, ptr %31, i64 %7, !dbg !16
  %wide.load22 = load <vscale x 4 x i32>, ptr %33, align 4, !dbg !16, !tbaa !17
  %34 = getelementptr inbounds i32, ptr %31, i64 %10, !dbg !16
  %wide.load23 = load <vscale x 4 x i32>, ptr %34, align 4, !dbg !16, !tbaa !17
  %35 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %index, !dbg !21
  %wide.load24 = load <vscale x 4 x i32>, ptr %35, align 4, !dbg !21, !tbaa !17
  %36 = getelementptr inbounds i32, ptr %35, i64 %13, !dbg !21
  %wide.load25 = load <vscale x 4 x i32>, ptr %36, align 4, !dbg !21, !tbaa !17
  %37 = getelementptr inbounds i32, ptr %35, i64 %16, !dbg !21
  %wide.load26 = load <vscale x 4 x i32>, ptr %37, align 4, !dbg !21, !tbaa !17
  %38 = getelementptr inbounds i32, ptr %35, i64 %19, !dbg !21
  %wide.load27 = load <vscale x 4 x i32>, ptr %38, align 4, !dbg !21, !tbaa !17
  %39 = add nsw <vscale x 4 x i32> %wide.load24, %wide.load, !dbg !22
  %40 = add nsw <vscale x 4 x i32> %wide.load25, %wide.load21, !dbg !22
  %41 = add nsw <vscale x 4 x i32> %wide.load26, %wide.load22, !dbg !22
  %42 = add nsw <vscale x 4 x i32> %wide.load27, %wide.load23, !dbg !22
  %43 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %index, !dbg !23
  store <vscale x 4 x i32> %39, ptr %43, align 4, !dbg !24, !tbaa !17
  %44 = getelementptr inbounds i32, ptr %43, i64 %22, !dbg !24
  store <vscale x 4 x i32> %40, ptr %44, align 4, !dbg !24, !tbaa !17
  %45 = getelementptr inbounds i32, ptr %43, i64 %25, !dbg !24
  store <vscale x 4 x i32> %41, ptr %45, align 4, !dbg !24, !tbaa !17
  %46 = getelementptr inbounds i32, ptr %43, i64 %28, !dbg !24
  store <vscale x 4 x i32> %42, ptr %46, align 4, !dbg !24, !tbaa !17
  %index.next = add nuw i64 %index, %30, !dbg !15
  %47 = icmp eq i64 %index.next, %n.vec, !dbg !15
  br i1 %47, label %middle.block, label %vector.body, !dbg !15, !llvm.loop !25

middle.block:                                     ; preds = %vector.body
  %cmp.n = icmp eq i64 %n.mod.vf, 0, !dbg !13
  br i1 %cmp.n, label %for.end, label %for.body, !dbg !13

for.body:                                         ; preds = %middle.block, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next.7, %for.body ], [ %n.vec, %middle.block ]
  %arrayidx = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv, !dbg !16
  %48 = load i32, ptr %arrayidx, align 4, !dbg !16, !tbaa !17
  %arrayidx2 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv, !dbg !21
  %49 = load i32, ptr %arrayidx2, align 4, !dbg !21, !tbaa !17
  %add = add nsw i32 %49, %48, !dbg !22
  %arrayidx4 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv, !dbg !23
  store i32 %add, ptr %arrayidx4, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !15
  %arrayidx.1 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next, !dbg !16
  %50 = load i32, ptr %arrayidx.1, align 4, !dbg !16, !tbaa !17
  %arrayidx2.1 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next, !dbg !21
  %51 = load i32, ptr %arrayidx2.1, align 4, !dbg !21, !tbaa !17
  %add.1 = add nsw i32 %51, %50, !dbg !22
  %arrayidx4.1 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next, !dbg !23
  store i32 %add.1, ptr %arrayidx4.1, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv, 2, !dbg !15
  %arrayidx.2 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.1, !dbg !16
  %52 = load i32, ptr %arrayidx.2, align 4, !dbg !16, !tbaa !17
  %arrayidx2.2 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.1, !dbg !21
  %53 = load i32, ptr %arrayidx2.2, align 4, !dbg !21, !tbaa !17
  %add.2 = add nsw i32 %53, %52, !dbg !22
  %arrayidx4.2 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.1, !dbg !23
  store i32 %add.2, ptr %arrayidx4.2, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv, 3, !dbg !15
  %arrayidx.3 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.2, !dbg !16
  %54 = load i32, ptr %arrayidx.3, align 4, !dbg !16, !tbaa !17
  %arrayidx2.3 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.2, !dbg !21
  %55 = load i32, ptr %arrayidx2.3, align 4, !dbg !21, !tbaa !17
  %add.3 = add nsw i32 %55, %54, !dbg !22
  %arrayidx4.3 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.2, !dbg !23
  store i32 %add.3, ptr %arrayidx4.3, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.3 = add nuw nsw i64 %indvars.iv, 4, !dbg !15
  %arrayidx.4 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.3, !dbg !16
  %56 = load i32, ptr %arrayidx.4, align 4, !dbg !16, !tbaa !17
  %arrayidx2.4 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.3, !dbg !21
  %57 = load i32, ptr %arrayidx2.4, align 4, !dbg !21, !tbaa !17
  %add.4 = add nsw i32 %57, %56, !dbg !22
  %arrayidx4.4 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.3, !dbg !23
  store i32 %add.4, ptr %arrayidx4.4, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.4 = add nuw nsw i64 %indvars.iv, 5, !dbg !15
  %arrayidx.5 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.4, !dbg !16
  %58 = load i32, ptr %arrayidx.5, align 4, !dbg !16, !tbaa !17
  %arrayidx2.5 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.4, !dbg !21
  %59 = load i32, ptr %arrayidx2.5, align 4, !dbg !21, !tbaa !17
  %add.5 = add nsw i32 %59, %58, !dbg !22
  %arrayidx4.5 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.4, !dbg !23
  store i32 %add.5, ptr %arrayidx4.5, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.5 = add nuw nsw i64 %indvars.iv, 6, !dbg !15
  %arrayidx.6 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.5, !dbg !16
  %60 = load i32, ptr %arrayidx.6, align 4, !dbg !16, !tbaa !17
  %arrayidx2.6 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.5, !dbg !21
  %61 = load i32, ptr %arrayidx2.6, align 4, !dbg !21, !tbaa !17
  %add.6 = add nsw i32 %61, %60, !dbg !22
  %arrayidx4.6 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.5, !dbg !23
  store i32 %add.6, ptr %arrayidx4.6, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.6 = add nuw nsw i64 %indvars.iv, 7, !dbg !15
  %arrayidx.7 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv.next.6, !dbg !16
  %62 = load i32, ptr %arrayidx.7, align 4, !dbg !16, !tbaa !17
  %arrayidx2.7 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv.next.6, !dbg !21
  %63 = load i32, ptr %arrayidx2.7, align 4, !dbg !21, !tbaa !17
  %add.7 = add nsw i32 %63, %62, !dbg !22
  %arrayidx4.7 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv.next.6, !dbg !23
  store i32 %add.7, ptr %arrayidx4.7, align 4, !dbg !24, !tbaa !17
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv, 8, !dbg !15
  %exitcond.not.7 = icmp eq i64 %indvars.iv.next.7, 1000, !dbg !31
  br i1 %exitcond.not.7, label %for.end, label %for.body, !dbg !13, !llvm.loop !32

for.end:                                          ; preds = %for.body, %middle.block
  %64 = load i32, ptr %z, align 4, !dbg !33, !tbaa !17
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %64), !dbg !34
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %z) #4, !dbg !35
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %y) #4, !dbg !35
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %x) #4, !dbg !35
  ret i32 0, !dbg !36
}

; Function Attrs: argmemonly mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nofree nounwind
declare dso_local noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

; Function Attrs: argmemonly mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nocallback nofree nosync nounwind readnone willreturn
declare i64 @llvm.vscale.i64() #3

; Function Attrs: nocallback nofree nosync nounwind readnone willreturn
declare i32 @llvm.vscale.i32() #3

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { argmemonly mustprogress nocallback nofree nosync nounwind willreturn }
attributes #2 = { nofree nounwind "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #3 = { nocallback nofree nosync nounwind readnone willreturn }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-004.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "a03489307ae9e628f44b02cbcb851453")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 15.0.4 (git@172.16.1.70:a64fx-swpl/llvm-project.git 69cde8a4ef2dde77486223557abc5942772c96f6)"}
!8 = distinct !DISubprogram(name: "pipeline_enable", scope: !9, file: !9, line: 5, type: !10, scopeLine: 5, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!9 = !DIFile(filename: "./pragma-loop-pipeline-004.c", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "a03489307ae9e628f44b02cbcb851453")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 8, column: 5, scope: !8)
!13 = !DILocation(line: 12, column: 5, scope: !8)
!14 = !DILocation(line: 0, scope: !8)
!15 = !DILocation(line: 12, column: 25, scope: !8)
!16 = !DILocation(line: 13, column: 16, scope: !8)
!17 = !{!18, !18, i64 0}
!18 = !{!"int", !19, i64 0}
!19 = !{!"omnipotent char", !20, i64 0}
!20 = !{!"Simple C/C++ TBAA"}
!21 = !DILocation(line: 13, column: 23, scope: !8)
!22 = !DILocation(line: 13, column: 21, scope: !8)
!23 = !DILocation(line: 13, column: 9, scope: !8)
!24 = !DILocation(line: 13, column: 14, scope: !8)
!25 = distinct !{!25, !26}
!26 = distinct !{!26, !13, !27, !28, !29, !30}
!27 = !DILocation(line: 15, column: 5, scope: !8)
!28 = !{!"llvm.loop.mustprogress"}
!29 = !{!"llvm.loop.isvectorized"}
!30 = !{!"llvm.loop.pipeline.disable", i1 true}
!31 = !DILocation(line: 12, column: 19, scope: !8)
!32 = distinct !{!32, !26}
!33 = !DILocation(line: 16, column: 11, scope: !8)
!34 = !DILocation(line: 17, column: 2, scope: !8)
!35 = !DILocation(line: 19, column: 1, scope: !8)
!36 = !DILocation(line: 18, column: 5, scope: !8)
