; RUN: llc < %s -O2 -mcpu=a64fx -fswp --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK-NOT: remark: ./swpl-remainder-loop-pipeline-005.cpp:13:5: software pipelining (IPC: 2.00, ITR: 5, MVE: 3, II: 3, Stage: 3, (VReg Fp: 1/32, Int: 17/29, Pred: 1/8)), SRA(PReg Fp: 0/32, Int: 14/29, Pred: 0/8)
; CHECK: remark: ./swpl-remainder-loop-pipeline-005.cpp:13:5: software pipelining (IPC: 2.00, ITR: 11, MVE: 6, II: 3, Stage: 6, (VReg Fp: 15/32, Int: 17/29, Pred: 2/8)), SRA(PReg Fp: 12/32, Int: 11/29, Pred: 1/8)

; ModuleID = './swpl-remainder-loop-pipeline-005.cpp'
source_filename = "./swpl-remainder-loop-pipeline-005.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: mustprogress uwtable vscale_range(4,4)
define dso_local void @_Z4testv() local_unnamed_addr #0 !dbg !8 {
entry:
  %x = alloca [999 x i32], align 4
  %y = alloca [999 x i32], align 4
  %z = alloca [999 x i32], align 4
  call void @llvm.lifetime.start.p0(i64 3996, ptr nonnull %x) #3, !dbg !12
  call void @llvm.lifetime.start.p0(i64 3996, ptr nonnull %y) #3, !dbg !12
  call void @llvm.lifetime.start.p0(i64 3996, ptr nonnull %z) #3, !dbg !12
  br label %vector.body, !dbg !13

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ], !dbg !14
  %0 = getelementptr inbounds [999 x i32], ptr %x, i64 0, i64 %index, !dbg !15
  %wide.load = load <vscale x 4 x i32>, ptr %0, align 4, !dbg !15, !tbaa !16
  %1 = getelementptr inbounds [999 x i32], ptr %y, i64 0, i64 %index, !dbg !20
  %wide.load21 = load <vscale x 4 x i32>, ptr %1, align 4, !dbg !20, !tbaa !16
  %2 = add nsw <vscale x 4 x i32> %wide.load21, %wide.load, !dbg !21
  %3 = getelementptr inbounds [999 x i32], ptr %z, i64 0, i64 %index, !dbg !22
  store <vscale x 4 x i32> %2, ptr %3, align 4, !dbg !23, !tbaa !16
  %index.next = add nuw i64 %index, 16, !dbg !14
  %4 = icmp eq i64 %index.next, 992, !dbg !14
  br i1 %4, label %for.body, label %vector.body, !dbg !14, !llvm.loop !24

for.body:                                         ; preds = %vector.body, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 992, %vector.body ]
  %arrayidx = getelementptr inbounds [999 x i32], ptr %x, i64 0, i64 %indvars.iv, !dbg !15
  %5 = load i32, ptr %arrayidx, align 4, !dbg !15, !tbaa !16
  %arrayidx2 = getelementptr inbounds [999 x i32], ptr %y, i64 0, i64 %indvars.iv, !dbg !20
  %6 = load i32, ptr %arrayidx2, align 4, !dbg !20, !tbaa !16
  %add = add nsw i32 %6, %5, !dbg !21
  %arrayidx4 = getelementptr inbounds [999 x i32], ptr %z, i64 0, i64 %indvars.iv, !dbg !22
  store i32 %add, ptr %arrayidx4, align 4, !dbg !23, !tbaa !16
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !14
  %exitcond.not = icmp eq i64 %indvars.iv.next, 999, !dbg !30
  br i1 %exitcond.not, label %for.end, label %for.body, !dbg !13, !llvm.loop !31

for.end:                                          ; preds = %for.body
  %7 = load i32, ptr %z, align 4, !dbg !33, !tbaa !16
  %call = tail call noundef i32 (ptr, ...) @_Z6printfPKcz(ptr noundef nonnull @.str, i32 noundef %7), !dbg !34
  call void @llvm.lifetime.end.p0(i64 3996, ptr nonnull %z) #3, !dbg !35
  call void @llvm.lifetime.end.p0(i64 3996, ptr nonnull %y) #3, !dbg !35
  call void @llvm.lifetime.end.p0(i64 3996, ptr nonnull %x) #3, !dbg !35
  ret void, !dbg !35
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

declare !dbg !36 dso_local noundef i32 @_Z6printfPKcz(ptr noundef, ...) local_unnamed_addr #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

attributes #0 = { mustprogress uwtable vscale_range(4,4) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 16.0.6 (git@172.16.1.70:a64fx-swpl/llvm-project.git 480b07140f3f303f700e0beac86f912bc51b6629)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "swpl-remainder-loop-pipeline-005.cpp", directory: "/home/sano", checksumkind: CSK_MD5, checksum: "4571d9a2fae63ca9656ee3a8e1ef8fad")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 16.0.6 (git@172.16.1.70:a64fx-swpl/llvm-project.git 480b07140f3f303f700e0beac86f912bc51b6629)"}
!8 = distinct !DISubprogram(name: "test", scope: !9, file: !9, line: 8, type: !10, scopeLine: 8, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!9 = !DIFile(filename: "./swpl-remainder-loop-pipeline-005.cpp", directory: "/home/sano", checksumkind: CSK_MD5, checksum: "4571d9a2fae63ca9656ee3a8e1ef8fad")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 11, column: 5, scope: !8)
!13 = !DILocation(line: 13, column: 5, scope: !8)
!14 = !DILocation(line: 13, column: 25, scope: !8)
!15 = !DILocation(line: 14, column: 16, scope: !8)
!16 = !{!17, !17, i64 0}
!17 = !{!"int", !18, i64 0}
!18 = !{!"omnipotent char", !19, i64 0}
!19 = !{!"Simple C++ TBAA"}
!20 = !DILocation(line: 14, column: 23, scope: !8)
!21 = !DILocation(line: 14, column: 21, scope: !8)
!22 = !DILocation(line: 14, column: 9, scope: !8)
!23 = !DILocation(line: 14, column: 14, scope: !8)
!24 = distinct !{!24, !13, !25, !26, !27, !28, !29}
!25 = !DILocation(line: 16, column: 5, scope: !8)
!26 = !{!"llvm.loop.mustprogress"}
!27 = !{!"llvm.loop.unroll.disable"}
!28 = !{!"llvm.loop.isvectorized", i32 1}
!29 = !{!"llvm.loop.unroll.runtime.disable"}
!30 = !DILocation(line: 13, column: 19, scope: !8)
!31 = distinct !{!31, !32}
!32 = distinct !{!32, !37, !28}
!33 = !DILocation(line: 17, column: 11, scope: !8)
!34 = !DILocation(line: 18, column: 2, scope: !8)
!35 = !DILocation(line: 19, column: 1, scope: !8)
!36 = !DISubprogram(name: "printf", scope: !9, file: !9, line: 6, type: !10, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized, retainedNodes: !11)
!37 = !{!"llvm.remainder.pipeline.disable"}
