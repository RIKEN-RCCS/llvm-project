; RUN: llc < %s -fswp -O2 -mcpu=a64fx --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: remark: ./pragma-loop-pipeline-nodep017.cpp:27:5: Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: ./pragma-loop-pipeline-nodep017.cpp:27:5: software pipelining 

; ModuleID = './pragma-loop-pipeline-nodep017.cpp'
source_filename = "./pragma-loop-pipeline-nodep017.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@x = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@y = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@z = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: mustprogress uwtable vscale_range(1,16)
define dso_local void @_Z6test_1v() local_unnamed_addr #0 !dbg !8 {
iter.check:
  %0 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %.neg = mul nuw nsw i64 %0, 1008, !dbg !12
  %n.vec = and i64 %.neg, 992, !dbg !12
  %1 = tail call i64 @llvm.vscale.i64()
  %2 = shl nuw nsw i64 %1, 2
  %3 = tail call i64 @llvm.vscale.i64()
  %4 = shl nuw nsw i64 %3, 3
  %5 = tail call i64 @llvm.vscale.i64()
  %6 = mul nuw nsw i64 %5, 12
  %7 = tail call i64 @llvm.vscale.i64()
  %8 = shl nuw nsw i64 %7, 2
  %9 = tail call i64 @llvm.vscale.i64()
  %10 = shl nuw nsw i64 %9, 3
  %11 = tail call i64 @llvm.vscale.i64()
  %12 = mul nuw nsw i64 %11, 12
  %13 = tail call i64 @llvm.vscale.i64()
  %14 = shl nuw nsw i64 %13, 2
  %15 = tail call i64 @llvm.vscale.i64()
  %16 = shl nuw nsw i64 %15, 3
  %17 = tail call i64 @llvm.vscale.i64()
  %18 = mul nuw nsw i64 %17, 12
  %19 = tail call i64 @llvm.vscale.i64()
  %20 = shl nuw nsw i64 %19, 4
  br label %vector.body, !dbg !12

vector.body:                                      ; preds = %vector.body, %iter.check
  %index = phi i64 [ 0, %iter.check ], [ %index.next, %vector.body ], !dbg !13
  %21 = getelementptr inbounds [1000 x i32], ptr @x, i64 0, i64 %index, !dbg !14
  %wide.load = load <vscale x 4 x i32>, ptr %21, align 4, !dbg !14, !tbaa !15
  %22 = getelementptr inbounds i32, ptr %21, i64 %2, !dbg !14
  %wide.load12 = load <vscale x 4 x i32>, ptr %22, align 4, !dbg !14, !tbaa !15
  %23 = getelementptr inbounds i32, ptr %21, i64 %4, !dbg !14
  %wide.load13 = load <vscale x 4 x i32>, ptr %23, align 4, !dbg !14, !tbaa !15
  %24 = getelementptr inbounds i32, ptr %21, i64 %6, !dbg !14
  %wide.load14 = load <vscale x 4 x i32>, ptr %24, align 4, !dbg !14, !tbaa !15
  %25 = getelementptr inbounds [1000 x i32], ptr @y, i64 0, i64 %index, !dbg !19
  %wide.load15 = load <vscale x 4 x i32>, ptr %25, align 4, !dbg !19, !tbaa !15
  %26 = getelementptr inbounds i32, ptr %25, i64 %8, !dbg !19
  %wide.load16 = load <vscale x 4 x i32>, ptr %26, align 4, !dbg !19, !tbaa !15
  %27 = getelementptr inbounds i32, ptr %25, i64 %10, !dbg !19
  %wide.load17 = load <vscale x 4 x i32>, ptr %27, align 4, !dbg !19, !tbaa !15
  %28 = getelementptr inbounds i32, ptr %25, i64 %12, !dbg !19
  %wide.load18 = load <vscale x 4 x i32>, ptr %28, align 4, !dbg !19, !tbaa !15
  %29 = add nsw <vscale x 4 x i32> %wide.load15, %wide.load, !dbg !20
  %30 = add nsw <vscale x 4 x i32> %wide.load16, %wide.load12, !dbg !20
  %31 = add nsw <vscale x 4 x i32> %wide.load17, %wide.load13, !dbg !20
  %32 = add nsw <vscale x 4 x i32> %wide.load18, %wide.load14, !dbg !20
  %33 = getelementptr inbounds [1000 x i32], ptr @z, i64 0, i64 %index, !dbg !21
  store <vscale x 4 x i32> %29, ptr %33, align 4, !dbg !22, !tbaa !15
  %34 = getelementptr inbounds i32, ptr %33, i64 %14, !dbg !22
  store <vscale x 4 x i32> %30, ptr %34, align 4, !dbg !22, !tbaa !15
  %35 = getelementptr inbounds i32, ptr %33, i64 %16, !dbg !22
  store <vscale x 4 x i32> %31, ptr %35, align 4, !dbg !22, !tbaa !15
  %36 = getelementptr inbounds i32, ptr %33, i64 %18, !dbg !22
  store <vscale x 4 x i32> %32, ptr %36, align 4, !dbg !22, !tbaa !15
  %index.next = add nuw i64 %index, %20, !dbg !13
  %37 = icmp eq i64 %index.next, %n.vec, !dbg !13
  br i1 %37, label %vec.epilog.iter.check, label %vector.body, !dbg !13, !llvm.loop !23

vec.epilog.iter.check:                            ; preds = %vector.body
  %n.vec.remaining = sub nuw nsw i64 1000, %n.vec, !dbg !12
  %38 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %39 = shl nuw nsw i64 %38, 1, !dbg !12
  %min.epilog.iters.check = icmp ult i64 %n.vec.remaining, %39, !dbg !12
  br i1 %min.epilog.iters.check, label %for.body.preheader, label %vec.epilog.ph, !dbg !12

vec.epilog.ph:                                    ; preds = %vec.epilog.iter.check
  %40 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %.neg26 = mul nuw nsw i64 %40, 1022, !dbg !12
  %n.vec20 = and i64 %.neg26, 1000, !dbg !12
  %41 = tail call i64 @llvm.vscale.i64()
  %42 = shl nuw nsw i64 %41, 1
  br label %vec.epilog.vector.body, !dbg !12

vec.epilog.vector.body:                           ; preds = %vec.epilog.vector.body, %vec.epilog.ph
  %index22 = phi i64 [ %n.vec, %vec.epilog.ph ], [ %index.next25, %vec.epilog.vector.body ], !dbg !13
  %43 = getelementptr inbounds [1000 x i32], ptr @x, i64 0, i64 %index22, !dbg !14
  %wide.load23 = load <vscale x 2 x i32>, ptr %43, align 4, !dbg !14, !tbaa !15
  %44 = getelementptr inbounds [1000 x i32], ptr @y, i64 0, i64 %index22, !dbg !19
  %wide.load24 = load <vscale x 2 x i32>, ptr %44, align 4, !dbg !19, !tbaa !15
  %45 = add nsw <vscale x 2 x i32> %wide.load24, %wide.load23, !dbg !20
  %46 = getelementptr inbounds [1000 x i32], ptr @z, i64 0, i64 %index22, !dbg !21
  store <vscale x 2 x i32> %45, ptr %46, align 4, !dbg !22, !tbaa !15
  %index.next25 = add nuw i64 %index22, %42, !dbg !13
  %47 = icmp eq i64 %index.next25, %n.vec20, !dbg !13
  br i1 %47, label %vec.epilog.middle.block, label %vec.epilog.vector.body, !dbg !13, !llvm.loop !29

vec.epilog.middle.block:                          ; preds = %vec.epilog.vector.body
  %cmp.n21 = icmp eq i64 %n.vec20, 1000, !dbg !12
  br i1 %cmp.n21, label %for.end, label %for.body.preheader, !dbg !12

for.body.preheader:                               ; preds = %vec.epilog.iter.check, %vec.epilog.middle.block
  %indvars.iv.ph = phi i64 [ %n.vec, %vec.epilog.iter.check ], [ %n.vec20, %vec.epilog.middle.block ]
  br label %for.body, !dbg !12

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ %indvars.iv.ph, %for.body.preheader ]
  %arrayidx = getelementptr inbounds [1000 x i32], ptr @x, i64 0, i64 %indvars.iv, !dbg !14
  %48 = load i32, ptr %arrayidx, align 4, !dbg !14, !tbaa !15
  %arrayidx2 = getelementptr inbounds [1000 x i32], ptr @y, i64 0, i64 %indvars.iv, !dbg !19
  %49 = load i32, ptr %arrayidx2, align 4, !dbg !19, !tbaa !15
  %add = add nsw i32 %49, %48, !dbg !20
  %arrayidx4 = getelementptr inbounds [1000 x i32], ptr @z, i64 0, i64 %indvars.iv, !dbg !21
  store i32 %add, ptr %arrayidx4, align 4, !dbg !22, !tbaa !15
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !13
  %exitcond.not = icmp eq i64 %indvars.iv.next, 1000, !dbg !31
  br i1 %exitcond.not, label %for.end, label %for.body, !dbg !12, !llvm.loop !32

for.end:                                          ; preds = %for.body, %vec.epilog.middle.block
  %50 = load i32, ptr @z, align 4, !dbg !33, !tbaa !15
  %call = tail call noundef i32 (ptr, ...) @_Z6printfPKcz(ptr noundef nonnull @.str, i32 noundef %50), !dbg !34
  ret void, !dbg !35
}

declare !dbg !36 dso_local noundef i32 @_Z6printfPKcz(ptr noundef, ...) local_unnamed_addr #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i64 @llvm.vscale.i64() #2

attributes #0 = { mustprogress uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #1 = { "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 17.0.3 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-nodep017.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "71173b8a6d5017411a589a84823a92c7")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 17.0.3 "}
!8 = distinct !DISubprogram(name: "test_1", scope: !9, file: !9, line: 22, type: !10, scopeLine: 22, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!9 = !DIFile(filename: "./pragma-loop-pipeline-nodep017.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "71173b8a6d5017411a589a84823a92c7")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 27, column: 5, scope: !8)
!13 = !DILocation(line: 27, column: 25, scope: !8)
!14 = !DILocation(line: 29, column: 16, scope: !8)
!15 = !{!16, !16, i64 0}
!16 = !{!"int", !17, i64 0}
!17 = !{!"omnipotent char", !18, i64 0}
!18 = !{!"Simple C++ TBAA"}
!19 = !DILocation(line: 29, column: 23, scope: !8)
!20 = !DILocation(line: 29, column: 21, scope: !8)
!21 = !DILocation(line: 29, column: 9, scope: !8)
!22 = !DILocation(line: 29, column: 14, scope: !8)
!23 = distinct !{!23, !12, !24, !25, !26, !27, !28}
!24 = !DILocation(line: 30, column: 5, scope: !8)
!25 = !{!"llvm.loop.mustprogress"}
!26 = !{!"llvm.loop.pipeline.enable"}
!27 = !{!"llvm.loop.isvectorized", i32 1}
!28 = !{!"llvm.loop.unroll.runtime.disable"}
!29 = distinct !{!29, !12, !24, !25, !26, !27, !28, !30}
!30 = !{!"llvm.remainder.pipeline.disable"}
!31 = !DILocation(line: 27, column: 19, scope: !8)
!32 = distinct !{!32, !12, !24, !25, !26, !28, !30, !27}
!33 = !DILocation(line: 31, column: 11, scope: !8)
!34 = !DILocation(line: 32, column: 2, scope: !8)
!35 = !DILocation(line: 33, column: 1, scope: !8)
!36 = !DISubprogram(name: "printf", scope: !9, file: !9, line: 19, type: !10, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
