; RUN: llc < %s -fswp -O2 -mcpu=a64fx --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: remark: ./pragma-loop-pipeline-nodep010-1.cpp:26:5: Since the pragma pipline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: ./pragma-loop-pipeline-nodep010-1.cpp:26:5: software pipelining (IPC: 2.00, ITR: 5, MVE: 3, II: 3, Stage: 3, (VReg Fp: 1/32, Int: 17/29, Pred: 1/8)), SRA(PReg Fp: 0/32, Int: 14/29, Pred: 0/8) 

; ModuleID = './pragma-loop-pipeline-nodep010-1.cpp'
source_filename = "./pragma-loop-pipeline-nodep010-1.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

@x = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@y = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@z = dso_local local_unnamed_addr global [1000 x i32] zeroinitializer, align 4
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: mustprogress uwtable vscale_range(1,16)
define dso_local void @_Z6test_1v() local_unnamed_addr #0 !dbg !8 {
entry:
  br label %for.body, !dbg !12

for.body:                                         ; preds = %entry, %for.body
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds [1000 x i32], ptr @x, i64 0, i64 %indvars.iv, !dbg !13
  %0 = load i32, ptr %arrayidx, align 4, !dbg !13, !tbaa !14
  %arrayidx2 = getelementptr inbounds [1000 x i32], ptr @y, i64 0, i64 %indvars.iv, !dbg !18
  %1 = load i32, ptr %arrayidx2, align 4, !dbg !18, !tbaa !14
  %add = add nsw i32 %1, %0, !dbg !19
  %arrayidx4 = getelementptr inbounds [1000 x i32], ptr @z, i64 0, i64 %indvars.iv, !dbg !20
  store i32 %add, ptr %arrayidx4, align 4, !dbg !21, !tbaa !14
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !22
  %exitcond.not = icmp eq i64 %indvars.iv.next, 1000, !dbg !23
  br i1 %exitcond.not, label %for.end, label %for.body, !dbg !12, !llvm.loop !24

for.end:                                          ; preds = %for.body
  %2 = load i32, ptr @z, align 4, !dbg !30, !tbaa !14
  %call = tail call noundef i32 (ptr, ...) @_Z6printfPKcz(ptr noundef nonnull @.str, i32 noundef %2), !dbg !31
  ret void, !dbg !32
}

declare !dbg !33 dso_local noundef i32 @_Z6printfPKcz(ptr noundef, ...) local_unnamed_addr #1

attributes #0 = { mustprogress uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 17.0.3 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git a1d6a811fe4e3988a1c6817ceebeed72e1a425df)", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-nodep010-1.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "886d896bed8c0639a2546056299c47de")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 17.0.3 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git a1d6a811fe4e3988a1c6817ceebeed72e1a425df)"}
!8 = distinct !DISubprogram(name: "test_1", scope: !9, file: !9, line: 21, type: !10, scopeLine: 21, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!9 = !DIFile(filename: "./pragma-loop-pipeline-nodep010-1.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "886d896bed8c0639a2546056299c47de")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 26, column: 5, scope: !8)
!13 = !DILocation(line: 28, column: 16, scope: !8)
!14 = !{!15, !15, i64 0}
!15 = !{!"int", !16, i64 0}
!16 = !{!"omnipotent char", !17, i64 0}
!17 = !{!"Simple C++ TBAA"}
!18 = !DILocation(line: 28, column: 23, scope: !8)
!19 = !DILocation(line: 28, column: 21, scope: !8)
!20 = !DILocation(line: 28, column: 9, scope: !8)
!21 = !DILocation(line: 28, column: 14, scope: !8)
!22 = !DILocation(line: 26, column: 25, scope: !8)
!23 = !DILocation(line: 26, column: 19, scope: !8)
!24 = distinct !{!24, !12, !25, !26, !27, !28
, !29}
!25 = !DILocation(line: 29, column: 5, scope: !8)
!26 = !{!"llvm.loop.mustprogress"}
!27 = !{!"llvm.loop.unroll.disable"}
!28 = !{!"llvm.loop.pipeline.enable"}
!29 = !{!"llvm.loop.pipeline.nodep"}
!30 = !DILocation(line: 30, column: 11, scope: !8)
!31 = !DILocation(line: 31, column: 2, scope: !8)
!32 = !DILocation(line: 32, column: 1, scope: !8)
!33 = !DISubprogram(name: "printf", scope: !9, file: !9, line: 18, type: !10, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
