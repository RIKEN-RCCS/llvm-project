; RUN: llc < %s -O2 -mcpu=a64fx --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: remark: ./pragma-loop-pipeline-nodep012.cpp:25:5: Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK-NOT: remark: ./pragma-loop-pipeline-nodep012.cpp:25:5: software pipelining 

; ModuleID = './pragma-loop-pipeline-nodep012.cpp'
source_filename = "./pragma-loop-pipeline-nodep012.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: mustprogress uwtable vscale_range(1,16)
define dso_local void @_Z6test_1v() local_unnamed_addr #0 !dbg !8 {
iter.check:
  %x = alloca [1000 x i32], align 4
  %y = alloca [1000 x i32], align 4
  %z = alloca [1000 x i32], align 4
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %x) #4, !dbg !12
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %y) #4, !dbg !12
  call void @llvm.lifetime.start.p0(i64 4000, ptr nonnull %z) #4, !dbg !12
  %0 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %.neg = mul nuw nsw i64 %0, 1008, !dbg !13
  %n.vec = and i64 %.neg, 992, !dbg !13
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
  br label %vector.body, !dbg !13

vector.body:                                      ; preds = %vector.body, %iter.check
  %index = phi i64 [ 0, %iter.check ], [ %index.next, %vector.body ], !dbg !14
  %21 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %index, !dbg !15
  %wide.load = load <vscale x 4 x i32>, ptr %21, align 4, !dbg !15, !tbaa !16
  %22 = getelementptr inbounds i32, ptr %21, i64 %2, !dbg !15
  %wide.load13 = load <vscale x 4 x i32>, ptr %22, align 4, !dbg !15, !tbaa !16
  %23 = getelementptr inbounds i32, ptr %21, i64 %4, !dbg !15
  %wide.load14 = load <vscale x 4 x i32>, ptr %23, align 4, !dbg !15, !tbaa !16
  %24 = getelementptr inbounds i32, ptr %21, i64 %6, !dbg !15
  %wide.load15 = load <vscale x 4 x i32>, ptr %24, align 4, !dbg !15, !tbaa !16
  %25 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %index, !dbg !20
  %wide.load16 = load <vscale x 4 x i32>, ptr %25, align 4, !dbg !20, !tbaa !16
  %26 = getelementptr inbounds i32, ptr %25, i64 %8, !dbg !20
  %wide.load17 = load <vscale x 4 x i32>, ptr %26, align 4, !dbg !20, !tbaa !16
  %27 = getelementptr inbounds i32, ptr %25, i64 %10, !dbg !20
  %wide.load18 = load <vscale x 4 x i32>, ptr %27, align 4, !dbg !20, !tbaa !16
  %28 = getelementptr inbounds i32, ptr %25, i64 %12, !dbg !20
  %wide.load19 = load <vscale x 4 x i32>, ptr %28, align 4, !dbg !20, !tbaa !16
  %29 = add nsw <vscale x 4 x i32> %wide.load16, %wide.load, !dbg !21
  %30 = add nsw <vscale x 4 x i32> %wide.load17, %wide.load13, !dbg !21
  %31 = add nsw <vscale x 4 x i32> %wide.load18, %wide.load14, !dbg !21
  %32 = add nsw <vscale x 4 x i32> %wide.load19, %wide.load15, !dbg !21
  %33 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %index, !dbg !22
  store <vscale x 4 x i32> %29, ptr %33, align 4, !dbg !23, !tbaa !16
  %34 = getelementptr inbounds i32, ptr %33, i64 %14, !dbg !23
  store <vscale x 4 x i32> %30, ptr %34, align 4, !dbg !23, !tbaa !16
  %35 = getelementptr inbounds i32, ptr %33, i64 %16, !dbg !23
  store <vscale x 4 x i32> %31, ptr %35, align 4, !dbg !23, !tbaa !16
  %36 = getelementptr inbounds i32, ptr %33, i64 %18, !dbg !23
  store <vscale x 4 x i32> %32, ptr %36, align 4, !dbg !23, !tbaa !16
  %index.next = add nuw i64 %index, %20, !dbg !14
  %37 = icmp eq i64 %index.next, %n.vec, !dbg !14
  br i1 %37, label %vec.epilog.iter.check, label %vector.body, !dbg !14, !llvm.loop !24

vec.epilog.iter.check:                            ; preds = %vector.body
  %n.vec.remaining = sub nuw nsw i64 1000, %n.vec, !dbg !13
  %38 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %39 = shl nuw nsw i64 %38, 1, !dbg !13
  %min.epilog.iters.check = icmp ult i64 %n.vec.remaining, %39, !dbg !13
  br i1 %min.epilog.iters.check, label %for.body.preheader, label %vec.epilog.ph, !dbg !13

vec.epilog.ph:                                    ; preds = %vec.epilog.iter.check
  %40 = tail call i64 @llvm.vscale.i64(), !dbg !13
  %.neg27 = mul nuw nsw i64 %40, 1022, !dbg !13
  %n.vec21 = and i64 %.neg27, 1000, !dbg !13
  %41 = tail call i64 @llvm.vscale.i64()
  %42 = shl nuw nsw i64 %41, 1
  br label %vec.epilog.vector.body, !dbg !13

vec.epilog.vector.body:                           ; preds = %vec.epilog.vector.body, %vec.epilog.ph
  %index23 = phi i64 [ %n.vec, %vec.epilog.ph ], [ %index.next26, %vec.epilog.vector.body ], !dbg !14
  %43 = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %index23, !dbg !15
  %wide.load24 = load <vscale x 2 x i32>, ptr %43, align 4, !dbg !15, !tbaa !16
  %44 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %index23, !dbg !20
  %wide.load25 = load <vscale x 2 x i32>, ptr %44, align 4, !dbg !20, !tbaa !16
  %45 = add nsw <vscale x 2 x i32> %wide.load25, %wide.load24, !dbg !21
  %46 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %index23, !dbg !22
  store <vscale x 2 x i32> %45, ptr %46, align 4, !dbg !23, !tbaa !16
  %index.next26 = add nuw i64 %index23, %42, !dbg !14
  %47 = icmp eq i64 %index.next26, %n.vec21, !dbg !14
  br i1 %47, label %vec.epilog.middle.block, label %vec.epilog.vector.body, !dbg !14, !llvm.loop !30

vec.epilog.middle.block:                          ; preds = %vec.epilog.vector.body
  %cmp.n22 = icmp eq i64 %n.vec21, 1000, !dbg !13
  br i1 %cmp.n22, label %for.end, label %for.body.preheader, !dbg !13

for.body.preheader:                               ; preds = %vec.epilog.iter.check, %vec.epilog.middle.block
  %indvars.iv.ph = phi i64 [ %n.vec, %vec.epilog.iter.check ], [ %n.vec21, %vec.epilog.middle.block ]
  br label %for.body, !dbg !13

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ %indvars.iv.ph, %for.body.preheader ]
  %arrayidx = getelementptr inbounds [1000 x i32], ptr %x, i64 0, i64 %indvars.iv, !dbg !15
  %48 = load i32, ptr %arrayidx, align 4, !dbg !15, !tbaa !16
  %arrayidx2 = getelementptr inbounds [1000 x i32], ptr %y, i64 0, i64 %indvars.iv, !dbg !20
  %49 = load i32, ptr %arrayidx2, align 4, !dbg !20, !tbaa !16
  %add = add nsw i32 %49, %48, !dbg !21
  %arrayidx4 = getelementptr inbounds [1000 x i32], ptr %z, i64 0, i64 %indvars.iv, !dbg !22
  store i32 %add, ptr %arrayidx4, align 4, !dbg !23, !tbaa !16
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !14
  %exitcond.not = icmp eq i64 %indvars.iv.next, 1000, !dbg !31
  br i1 %exitcond.not, label %for.end, label %for.body, !dbg !13, !llvm.loop !32

for.end:                                          ; preds = %for.body, %vec.epilog.middle.block
  %50 = load i32, ptr %z, align 4, !dbg !33, !tbaa !16
  %call = tail call noundef i32 (ptr, ...) @_Z6printfPKcz(ptr noundef nonnull @.str, i32 noundef %50), !dbg !34
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %z) #4, !dbg !35
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %y) #4, !dbg !35
  call void @llvm.lifetime.end.p0(i64 4000, ptr nonnull %x) #4, !dbg !35
  ret void, !dbg !35
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

declare !dbg !36 dso_local noundef i32 @_Z6printfPKcz(ptr noundef, ...) local_unnamed_addr #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i64 @llvm.vscale.i64() #3

attributes #0 = { mustprogress uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(none) }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 17.0.3 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-nodep012.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "fde0d01989614e8678c127a3edb6297c")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 17.0.3 "}
!8 = distinct !DISubprogram(name: "test_1", scope: !9, file: !9, line: 20, type: !10, scopeLine: 20, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!9 = !DIFile(filename: "./pragma-loop-pipeline-nodep012.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "fde0d01989614e8678c127a3edb6297c")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 22, column: 5, scope: !8)
!13 = !DILocation(line: 25, column: 5, scope: !8)
!14 = !DILocation(line: 25, column: 25, scope: !8)
!15 = !DILocation(line: 27, column: 16, scope: !8)
!16 = !{!17, !17, i64 0}
!17 = !{!"int", !18, i64 0}
!18 = !{!"omnipotent char", !19, i64 0}
!19 = !{!"Simple C++ TBAA"}
!20 = !DILocation(line: 27, column: 23, scope: !8)
!21 = !DILocation(line: 27, column: 21, scope: !8)
!22 = !DILocation(line: 27, column: 9, scope: !8)
!23 = !DILocation(line: 27, column: 14, scope: !8)
!24 = distinct !{!24, !13, !25, !26, !27, !28, !29}
!25 = !DILocation(line: 28, column: 5, scope: !8)
!26 = !{!"llvm.loop.mustprogress"}
!27 = !{!"llvm.loop.pipeline.nodep"}
!28 = !{!"llvm.loop.isvectorized", i32 1}
!29 = !{!"llvm.loop.unroll.runtime.disable"}
!30 = distinct !{!30, !13, !25, !26, !27, !28, !29}
!31 = !DILocation(line: 25, column: 19, scope: !8)
!32 = distinct !{!32, !13, !25, !26, !27, !29, !28}
!33 = !DILocation(line: 29, column: 11, scope: !8)
!34 = !DILocation(line: 30, column: 2, scope: !8)
!35 = !DILocation(line: 31, column: 1, scope: !8)
!36 = !DISubprogram(name: "printf", scope: !9, file: !9, line: 18, type: !10, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
