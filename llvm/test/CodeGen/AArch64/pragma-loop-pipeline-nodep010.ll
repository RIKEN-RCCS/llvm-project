; RUN: llc < %s -fswp -O2 -mcpu=a64fx --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: remark: ./pragma-loop-pipeline-nodep004.cpp:26:1: Since the pragma pipline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: /pragma-loop-pipeline-nodep004.cpp:26:5: software pipelining (IPC: 2.00, ITR: 5, MVE: 3, II: 3, Stage: 3, (VReg Fp: 1/32, Int: 17/29, Pred: 1/8)), SRA(PReg Fp: 0/32, Int: 14/29, Pred: 0/8) 

; ModuleID = './pragma-loop-pipeline-nodep004.cpp'
source_filename = "./pragma-loop-pipeline-nodep004.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) vscale_range(1,16)
define dso_local void @_Z3fooPfS_S_S_S_(ptr nocapture noundef %A, ptr nocapture noundef readonly %B, ptr nocapture noundef writeonly %C, ptr nocapture noundef readonly %D, ptr nocapture noundef readonly %E) local_unnamed_addr #0 {
entry:
  %scevgep = getelementptr i8, ptr %A, i64 40004
  %scevgep22 = getelementptr i8, ptr %C, i64 40000
  %scevgep23 = getelementptr i8, ptr %D, i64 40000
  %scevgep24 = getelementptr i8, ptr %E, i64 40000
  %scevgep25 = getelementptr i8, ptr %B, i64 40000
  %bound0 = icmp ugt ptr %scevgep22, %A
  %bound1 = icmp ugt ptr %scevgep, %C
  %found.conflict = and i1 %bound0, %bound1
  %bound026 = icmp ugt ptr %scevgep23, %A
  %bound127 = icmp ugt ptr %scevgep, %D
  %found.conflict28 = and i1 %bound026, %bound127
  %conflict.rdx = or i1 %found.conflict, %found.conflict28
  %bound029 = icmp ugt ptr %scevgep24, %A
  %bound130 = icmp ugt ptr %scevgep, %E
  %found.conflict31 = and i1 %bound029, %bound130
  %conflict.rdx32 = or i1 %conflict.rdx, %found.conflict31
  %bound033 = icmp ugt ptr %scevgep25, %C
  %bound134 = icmp ugt ptr %scevgep22, %B
  %found.conflict35 = and i1 %bound033, %bound134
  %conflict.rdx36 = or i1 %conflict.rdx32, %found.conflict35
  br i1 %conflict.rdx36, label %for.body.lver.orig.lver.check, label %for.body.ph.ldist1

for.body.lver.orig.lver.check:                    ; preds = %entry
  %scevgep38 = getelementptr i8, ptr %A, i64 40004
  %scevgep39 = getelementptr i8, ptr %C, i64 40000
  %bound040 = icmp ugt ptr %scevgep39, %A
  %bound141 = icmp ugt ptr %scevgep38, %C
  %found.conflict42 = and i1 %bound040, %bound141
  br i1 %found.conflict42, label %for.body.lver.orig.lver.orig, label %for.body.lver.orig.ph

for.body.lver.orig.lver.orig:                     ; preds = %for.body.lver.orig.lver.check, %for.body.lver.orig.lver.orig
  %indvars.iv.lver.orig.lver.orig = phi i64 [ %indvars.iv.next.lver.orig.lver.orig.9, %for.body.lver.orig.lver.orig ], [ 0, %for.body.lver.orig.lver.check ]
  %arrayidx.lver.orig.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.lver.orig.lver.orig
  %0 = load float, ptr %arrayidx.lver.orig.lver.orig, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig = getelementptr inbounds float, ptr %B, i64 %indvars.iv.lver.orig.lver.orig
  %1 = load float, ptr %arrayidx2.lver.orig.lver.orig, align 4, !tbaa !2
  %mul.lver.orig.lver.orig = fmul float %0, %1
  %indvars.iv.next.lver.orig.lver.orig = or i64 %indvars.iv.lver.orig.lver.orig, 1
  %arrayidx4.lver.orig.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig
  store float %mul.lver.orig.lver.orig, ptr %arrayidx4.lver.orig.lver.orig, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig = getelementptr inbounds float, ptr %D, i64 %indvars.iv.lver.orig.lver.orig
  %2 = load float, ptr %arrayidx6.lver.orig.lver.orig, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig = getelementptr inbounds float, ptr %E, i64 %indvars.iv.lver.orig.lver.orig
  %3 = load float, ptr %arrayidx8.lver.orig.lver.orig, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig = fmul float %2, %3
  %arrayidx11.lver.orig.lver.orig = getelementptr inbounds float, ptr %C, i64 %indvars.iv.lver.orig.lver.orig
  store float %mul9.lver.orig.lver.orig, ptr %arrayidx11.lver.orig.lver.orig, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig
  %4 = load float, ptr %arrayidx.lver.orig.lver.orig.1, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig
  %5 = load float, ptr %arrayidx2.lver.orig.lver.orig.1, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.1 = fmul float %4, %5
  %indvars.iv.next.lver.orig.lver.orig.1 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 2
  %arrayidx4.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.1
  store float %mul.lver.orig.lver.orig.1, ptr %arrayidx4.lver.orig.lver.orig.1, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig
  %6 = load float, ptr %arrayidx6.lver.orig.lver.orig.1, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig
  %7 = load float, ptr %arrayidx8.lver.orig.lver.orig.1, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.1 = fmul float %6, %7
  %arrayidx11.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig
  store float %mul9.lver.orig.lver.orig.1, ptr %arrayidx11.lver.orig.lver.orig.1, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.1
  %8 = load float, ptr %arrayidx.lver.orig.lver.orig.2, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.1
  %9 = load float, ptr %arrayidx2.lver.orig.lver.orig.2, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.2 = fmul float %8, %9
  %indvars.iv.next.lver.orig.lver.orig.2 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 3
  %arrayidx4.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.2
  store float %mul.lver.orig.lver.orig.2, ptr %arrayidx4.lver.orig.lver.orig.2, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.1
  %10 = load float, ptr %arrayidx6.lver.orig.lver.orig.2, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.1
  %11 = load float, ptr %arrayidx8.lver.orig.lver.orig.2, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.2 = fmul float %10, %11
  %arrayidx11.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.1
  store float %mul9.lver.orig.lver.orig.2, ptr %arrayidx11.lver.orig.lver.orig.2, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.2
  %12 = load float, ptr %arrayidx.lver.orig.lver.orig.3, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.2
  %13 = load float, ptr %arrayidx2.lver.orig.lver.orig.3, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.3 = fmul float %12, %13
  %indvars.iv.next.lver.orig.lver.orig.3 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 4
  %arrayidx4.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.3
  store float %mul.lver.orig.lver.orig.3, ptr %arrayidx4.lver.orig.lver.orig.3, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.2
  %14 = load float, ptr %arrayidx6.lver.orig.lver.orig.3, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.2
  %15 = load float, ptr %arrayidx8.lver.orig.lver.orig.3, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.3 = fmul float %14, %15
  %arrayidx11.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.2
  store float %mul9.lver.orig.lver.orig.3, ptr %arrayidx11.lver.orig.lver.orig.3, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.3
  %16 = load float, ptr %arrayidx.lver.orig.lver.orig.4, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.3
  %17 = load float, ptr %arrayidx2.lver.orig.lver.orig.4, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.4 = fmul float %16, %17
  %indvars.iv.next.lver.orig.lver.orig.4 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 5
  %arrayidx4.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.4
  store float %mul.lver.orig.lver.orig.4, ptr %arrayidx4.lver.orig.lver.orig.4, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.3
  %18 = load float, ptr %arrayidx6.lver.orig.lver.orig.4, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.3
  %19 = load float, ptr %arrayidx8.lver.orig.lver.orig.4, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.4 = fmul float %18, %19
  %arrayidx11.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.3
  store float %mul9.lver.orig.lver.orig.4, ptr %arrayidx11.lver.orig.lver.orig.4, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.4
  %20 = load float, ptr %arrayidx.lver.orig.lver.orig.5, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.4
  %21 = load float, ptr %arrayidx2.lver.orig.lver.orig.5, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.5 = fmul float %20, %21
  %indvars.iv.next.lver.orig.lver.orig.5 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 6
  %arrayidx4.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.5
  store float %mul.lver.orig.lver.orig.5, ptr %arrayidx4.lver.orig.lver.orig.5, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.4
  %22 = load float, ptr %arrayidx6.lver.orig.lver.orig.5, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.4
  %23 = load float, ptr %arrayidx8.lver.orig.lver.orig.5, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.5 = fmul float %22, %23
  %arrayidx11.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.4
  store float %mul9.lver.orig.lver.orig.5, ptr %arrayidx11.lver.orig.lver.orig.5, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.5
  %24 = load float, ptr %arrayidx.lver.orig.lver.orig.6, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.5
  %25 = load float, ptr %arrayidx2.lver.orig.lver.orig.6, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.6 = fmul float %24, %25
  %indvars.iv.next.lver.orig.lver.orig.6 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 7
  %arrayidx4.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.6
  store float %mul.lver.orig.lver.orig.6, ptr %arrayidx4.lver.orig.lver.orig.6, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.5
  %26 = load float, ptr %arrayidx6.lver.orig.lver.orig.6, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.5
  %27 = load float, ptr %arrayidx8.lver.orig.lver.orig.6, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.6 = fmul float %26, %27
  %arrayidx11.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.5
  store float %mul9.lver.orig.lver.orig.6, ptr %arrayidx11.lver.orig.lver.orig.6, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.6
  %28 = load float, ptr %arrayidx.lver.orig.lver.orig.7, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.6
  %29 = load float, ptr %arrayidx2.lver.orig.lver.orig.7, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.7 = fmul float %28, %29
  %indvars.iv.next.lver.orig.lver.orig.7 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 8
  %arrayidx4.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.7
  store float %mul.lver.orig.lver.orig.7, ptr %arrayidx4.lver.orig.lver.orig.7, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.6
  %30 = load float, ptr %arrayidx6.lver.orig.lver.orig.7, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.6
  %31 = load float, ptr %arrayidx8.lver.orig.lver.orig.7, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.7 = fmul float %30, %31
  %arrayidx11.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.6
  store float %mul9.lver.orig.lver.orig.7, ptr %arrayidx11.lver.orig.lver.orig.7, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.7
  %32 = load float, ptr %arrayidx.lver.orig.lver.orig.8, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.7
  %33 = load float, ptr %arrayidx2.lver.orig.lver.orig.8, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.8 = fmul float %32, %33
  %indvars.iv.next.lver.orig.lver.orig.8 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 9
  %arrayidx4.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.8
  store float %mul.lver.orig.lver.orig.8, ptr %arrayidx4.lver.orig.lver.orig.8, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.7
  %34 = load float, ptr %arrayidx6.lver.orig.lver.orig.8, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.7
  %35 = load float, ptr %arrayidx8.lver.orig.lver.orig.8, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.8 = fmul float %34, %35
  %arrayidx11.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.7
  store float %mul9.lver.orig.lver.orig.8, ptr %arrayidx11.lver.orig.lver.orig.8, align 4, !tbaa !2
  %arrayidx.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.8
  %36 = load float, ptr %arrayidx.lver.orig.lver.orig.9, align 4, !tbaa !2
  %arrayidx2.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.8
  %37 = load float, ptr %arrayidx2.lver.orig.lver.orig.9, align 4, !tbaa !2
  %mul.lver.orig.lver.orig.9 = fmul float %36, %37
  %indvars.iv.next.lver.orig.lver.orig.9 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 10
  %arrayidx4.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.9
  store float %mul.lver.orig.lver.orig.9, ptr %arrayidx4.lver.orig.lver.orig.9, align 4, !tbaa !2
  %arrayidx6.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.8
  %38 = load float, ptr %arrayidx6.lver.orig.lver.orig.9, align 4, !tbaa !2
  %arrayidx8.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.8
  %39 = load float, ptr %arrayidx8.lver.orig.lver.orig.9, align 4, !tbaa !2
  %mul9.lver.orig.lver.orig.9 = fmul float %38, %39
  %arrayidx11.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.8
  store float %mul9.lver.orig.lver.orig.9, ptr %arrayidx11.lver.orig.lver.orig.9, align 4, !tbaa !2
  %exitcond.not.lver.orig.lver.orig.9 = icmp eq i64 %indvars.iv.next.lver.orig.lver.orig.9, 10000
  br i1 %exitcond.not.lver.orig.lver.orig.9, label %for.end, label %for.body.lver.orig.lver.orig, !llvm.loop !6

for.body.lver.orig.ph:                            ; preds = %for.body.lver.orig.lver.check
  %load_initial = load float, ptr %A, align 4
  br label %for.body.lver.orig

for.body.lver.orig:                               ; preds = %for.body.lver.orig, %for.body.lver.orig.ph
  %store_forwarded = phi float [ %load_initial, %for.body.lver.orig.ph ], [ %mul.lver.orig.9, %for.body.lver.orig ]
  %indvars.iv.lver.orig = phi i64 [ 0, %for.body.lver.orig.ph ], [ %indvars.iv.next.lver.orig.9, %for.body.lver.orig ]
  %arrayidx2.lver.orig = getelementptr inbounds float, ptr %B, i64 %indvars.iv.lver.orig
  %40 = load float, ptr %arrayidx2.lver.orig, align 4, !tbaa !2
  %mul.lver.orig = fmul float %store_forwarded, %40
  %indvars.iv.next.lver.orig = or i64 %indvars.iv.lver.orig, 1
  %arrayidx4.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig
  store float %mul.lver.orig, ptr %arrayidx4.lver.orig, align 4, !tbaa !2
  %arrayidx6.lver.orig = getelementptr inbounds float, ptr %D, i64 %indvars.iv.lver.orig
  %41 = load float, ptr %arrayidx6.lver.orig, align 4, !tbaa !2
  %arrayidx8.lver.orig = getelementptr inbounds float, ptr %E, i64 %indvars.iv.lver.orig
  %42 = load float, ptr %arrayidx8.lver.orig, align 4, !tbaa !2
  %mul9.lver.orig = fmul float %41, %42
  %arrayidx11.lver.orig = getelementptr inbounds float, ptr %C, i64 %indvars.iv.lver.orig
  store float %mul9.lver.orig, ptr %arrayidx11.lver.orig, align 4, !tbaa !2
  %arrayidx2.lver.orig.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig
  %43 = load float, ptr %arrayidx2.lver.orig.1, align 4, !tbaa !2
  %mul.lver.orig.1 = fmul float %mul.lver.orig, %43
  %indvars.iv.next.lver.orig.1 = add nuw nsw i64 %indvars.iv.lver.orig, 2
  %arrayidx4.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.1
  store float %mul.lver.orig.1, ptr %arrayidx4.lver.orig.1, align 4, !tbaa !2
  %arrayidx6.lver.orig.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig
  %44 = load float, ptr %arrayidx6.lver.orig.1, align 4, !tbaa !2
  %arrayidx8.lver.orig.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig
  %45 = load float, ptr %arrayidx8.lver.orig.1, align 4, !tbaa !2
  %mul9.lver.orig.1 = fmul float %44, %45
  %arrayidx11.lver.orig.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig
  store float %mul9.lver.orig.1, ptr %arrayidx11.lver.orig.1, align 4, !tbaa !2
  %arrayidx2.lver.orig.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.1
  %46 = load float, ptr %arrayidx2.lver.orig.2, align 4, !tbaa !2
  %mul.lver.orig.2 = fmul float %mul.lver.orig.1, %46
  %indvars.iv.next.lver.orig.2 = add nuw nsw i64 %indvars.iv.lver.orig, 3
  %arrayidx4.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.2
  store float %mul.lver.orig.2, ptr %arrayidx4.lver.orig.2, align 4, !tbaa !2
  %arrayidx6.lver.orig.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.1
  %47 = load float, ptr %arrayidx6.lver.orig.2, align 4, !tbaa !2
  %arrayidx8.lver.orig.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.1
  %48 = load float, ptr %arrayidx8.lver.orig.2, align 4, !tbaa !2
  %mul9.lver.orig.2 = fmul float %47, %48
  %arrayidx11.lver.orig.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.1
  store float %mul9.lver.orig.2, ptr %arrayidx11.lver.orig.2, align 4, !tbaa !2
  %arrayidx2.lver.orig.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.2
  %49 = load float, ptr %arrayidx2.lver.orig.3, align 4, !tbaa !2
  %mul.lver.orig.3 = fmul float %mul.lver.orig.2, %49
  %indvars.iv.next.lver.orig.3 = add nuw nsw i64 %indvars.iv.lver.orig, 4
  %arrayidx4.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.3
  store float %mul.lver.orig.3, ptr %arrayidx4.lver.orig.3, align 4, !tbaa !2
  %arrayidx6.lver.orig.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.2
  %50 = load float, ptr %arrayidx6.lver.orig.3, align 4, !tbaa !2
  %arrayidx8.lver.orig.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.2
  %51 = load float, ptr %arrayidx8.lver.orig.3, align 4, !tbaa !2
  %mul9.lver.orig.3 = fmul float %50, %51
  %arrayidx11.lver.orig.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.2
  store float %mul9.lver.orig.3, ptr %arrayidx11.lver.orig.3, align 4, !tbaa !2
  %arrayidx2.lver.orig.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.3
  %52 = load float, ptr %arrayidx2.lver.orig.4, align 4, !tbaa !2
  %mul.lver.orig.4 = fmul float %mul.lver.orig.3, %52
  %indvars.iv.next.lver.orig.4 = add nuw nsw i64 %indvars.iv.lver.orig, 5
  %arrayidx4.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.4
  store float %mul.lver.orig.4, ptr %arrayidx4.lver.orig.4, align 4, !tbaa !2
  %arrayidx6.lver.orig.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.3
  %53 = load float, ptr %arrayidx6.lver.orig.4, align 4, !tbaa !2
  %arrayidx8.lver.orig.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.3
  %54 = load float, ptr %arrayidx8.lver.orig.4, align 4, !tbaa !2
  %mul9.lver.orig.4 = fmul float %53, %54
  %arrayidx11.lver.orig.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.3
  store float %mul9.lver.orig.4, ptr %arrayidx11.lver.orig.4, align 4, !tbaa !2
  %arrayidx2.lver.orig.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.4
  %55 = load float, ptr %arrayidx2.lver.orig.5, align 4, !tbaa !2
  %mul.lver.orig.5 = fmul float %mul.lver.orig.4, %55
  %indvars.iv.next.lver.orig.5 = add nuw nsw i64 %indvars.iv.lver.orig, 6
  %arrayidx4.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.5
  store float %mul.lver.orig.5, ptr %arrayidx4.lver.orig.5, align 4, !tbaa !2
  %arrayidx6.lver.orig.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.4
  %56 = load float, ptr %arrayidx6.lver.orig.5, align 4, !tbaa !2
  %arrayidx8.lver.orig.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.4
  %57 = load float, ptr %arrayidx8.lver.orig.5, align 4, !tbaa !2
  %mul9.lver.orig.5 = fmul float %56, %57
  %arrayidx11.lver.orig.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.4
  store float %mul9.lver.orig.5, ptr %arrayidx11.lver.orig.5, align 4, !tbaa !2
  %arrayidx2.lver.orig.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.5
  %58 = load float, ptr %arrayidx2.lver.orig.6, align 4, !tbaa !2
  %mul.lver.orig.6 = fmul float %mul.lver.orig.5, %58
  %indvars.iv.next.lver.orig.6 = add nuw nsw i64 %indvars.iv.lver.orig, 7
  %arrayidx4.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.6
  store float %mul.lver.orig.6, ptr %arrayidx4.lver.orig.6, align 4, !tbaa !2
  %arrayidx6.lver.orig.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.5
  %59 = load float, ptr %arrayidx6.lver.orig.6, align 4, !tbaa !2
  %arrayidx8.lver.orig.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.5
  %60 = load float, ptr %arrayidx8.lver.orig.6, align 4, !tbaa !2
  %mul9.lver.orig.6 = fmul float %59, %60
  %arrayidx11.lver.orig.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.5
  store float %mul9.lver.orig.6, ptr %arrayidx11.lver.orig.6, align 4, !tbaa !2
  %arrayidx2.lver.orig.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.6
  %61 = load float, ptr %arrayidx2.lver.orig.7, align 4, !tbaa !2
  %mul.lver.orig.7 = fmul float %mul.lver.orig.6, %61
  %indvars.iv.next.lver.orig.7 = add nuw nsw i64 %indvars.iv.lver.orig, 8
  %arrayidx4.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.7
  store float %mul.lver.orig.7, ptr %arrayidx4.lver.orig.7, align 4, !tbaa !2
  %arrayidx6.lver.orig.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.6
  %62 = load float, ptr %arrayidx6.lver.orig.7, align 4, !tbaa !2
  %arrayidx8.lver.orig.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.6
  %63 = load float, ptr %arrayidx8.lver.orig.7, align 4, !tbaa !2
  %mul9.lver.orig.7 = fmul float %62, %63
  %arrayidx11.lver.orig.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.6
  store float %mul9.lver.orig.7, ptr %arrayidx11.lver.orig.7, align 4, !tbaa !2
  %arrayidx2.lver.orig.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.7
  %64 = load float, ptr %arrayidx2.lver.orig.8, align 4, !tbaa !2
  %mul.lver.orig.8 = fmul float %mul.lver.orig.7, %64
  %indvars.iv.next.lver.orig.8 = add nuw nsw i64 %indvars.iv.lver.orig, 9
  %arrayidx4.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.8
  store float %mul.lver.orig.8, ptr %arrayidx4.lver.orig.8, align 4, !tbaa !2
  %arrayidx6.lver.orig.8 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.7
  %65 = load float, ptr %arrayidx6.lver.orig.8, align 4, !tbaa !2
  %arrayidx8.lver.orig.8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.7
  %66 = load float, ptr %arrayidx8.lver.orig.8, align 4, !tbaa !2
  %mul9.lver.orig.8 = fmul float %65, %66
  %arrayidx11.lver.orig.8 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.7
  store float %mul9.lver.orig.8, ptr %arrayidx11.lver.orig.8, align 4, !tbaa !2
  %arrayidx2.lver.orig.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.8
  %67 = load float, ptr %arrayidx2.lver.orig.9, align 4, !tbaa !2
  %mul.lver.orig.9 = fmul float %mul.lver.orig.8, %67
  %indvars.iv.next.lver.orig.9 = add nuw nsw i64 %indvars.iv.lver.orig, 10
  %arrayidx4.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.9
  store float %mul.lver.orig.9, ptr %arrayidx4.lver.orig.9, align 4, !tbaa !2
  %arrayidx6.lver.orig.9 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.8
  %68 = load float, ptr %arrayidx6.lver.orig.9, align 4, !tbaa !2
  %arrayidx8.lver.orig.9 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.8
  %69 = load float, ptr %arrayidx8.lver.orig.9, align 4, !tbaa !2
  %mul9.lver.orig.9 = fmul float %68, %69
  %arrayidx11.lver.orig.9 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.8
  store float %mul9.lver.orig.9, ptr %arrayidx11.lver.orig.9, align 4, !tbaa !2
  %exitcond.not.lver.orig.9 = icmp eq i64 %indvars.iv.next.lver.orig.9, 10000
  br i1 %exitcond.not.lver.orig.9, label %for.end, label %for.body.lver.orig, !llvm.loop !6

for.body.ph.ldist1:                               ; preds = %entry
  %load_initial44 = load float, ptr %A, align 4
  br label %for.body.ldist1

for.body.ldist1:                                  ; preds = %for.body.ldist1, %for.body.ph.ldist1
  %store_forwarded45 = phi float [ %load_initial44, %for.body.ph.ldist1 ], [ %mul.ldist1.24, %for.body.ldist1 ]
  %indvars.iv.ldist1 = phi i64 [ 0, %for.body.ph.ldist1 ], [ %indvars.iv.next.ldist1.24, %for.body.ldist1 ]
  %arrayidx2.ldist1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.ldist1
  %70 = load float, ptr %arrayidx2.ldist1, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1 = fmul float %store_forwarded45, %70
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1
  %arrayidx4.ldist1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1
  store float %mul.ldist1, ptr %arrayidx4.ldist1, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1
  %71 = load float, ptr %arrayidx2.ldist1.1, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.1 = fmul float %mul.ldist1, %71
  %indvars.iv.next.ldist1.1 = add nuw nsw i64 %indvars.iv.ldist1, 2
  %arrayidx4.ldist1.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.1
  store float %mul.ldist1.1, ptr %arrayidx4.ldist1.1, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.1
  %72 = load float, ptr %arrayidx2.ldist1.2, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.2 = fmul float %mul.ldist1.1, %72
  %indvars.iv.next.ldist1.2 = add nuw nsw i64 %indvars.iv.ldist1, 3
  %arrayidx4.ldist1.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.2
  store float %mul.ldist1.2, ptr %arrayidx4.ldist1.2, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.2
  %73 = load float, ptr %arrayidx2.ldist1.3, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.3 = fmul float %mul.ldist1.2, %73
  %indvars.iv.next.ldist1.3 = add nuw nsw i64 %indvars.iv.ldist1, 4
  %arrayidx4.ldist1.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.3
  store float %mul.ldist1.3, ptr %arrayidx4.ldist1.3, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.3
  %74 = load float, ptr %arrayidx2.ldist1.4, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.4 = fmul float %mul.ldist1.3, %74
  %indvars.iv.next.ldist1.4 = add nuw nsw i64 %indvars.iv.ldist1, 5
  %arrayidx4.ldist1.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.4
  store float %mul.ldist1.4, ptr %arrayidx4.ldist1.4, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.4
  %75 = load float, ptr %arrayidx2.ldist1.5, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.5 = fmul float %mul.ldist1.4, %75
  %indvars.iv.next.ldist1.5 = add nuw nsw i64 %indvars.iv.ldist1, 6
  %arrayidx4.ldist1.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.5
  store float %mul.ldist1.5, ptr %arrayidx4.ldist1.5, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.5
  %76 = load float, ptr %arrayidx2.ldist1.6, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.6 = fmul float %mul.ldist1.5, %76
  %indvars.iv.next.ldist1.6 = add nuw nsw i64 %indvars.iv.ldist1, 7
  %arrayidx4.ldist1.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.6
  store float %mul.ldist1.6, ptr %arrayidx4.ldist1.6, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.6
  %77 = load float, ptr %arrayidx2.ldist1.7, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.7 = fmul float %mul.ldist1.6, %77
  %indvars.iv.next.ldist1.7 = add nuw nsw i64 %indvars.iv.ldist1, 8
  %arrayidx4.ldist1.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.7
  store float %mul.ldist1.7, ptr %arrayidx4.ldist1.7, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.7
  %78 = load float, ptr %arrayidx2.ldist1.8, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.8 = fmul float %mul.ldist1.7, %78
  %indvars.iv.next.ldist1.8 = add nuw nsw i64 %indvars.iv.ldist1, 9
  %arrayidx4.ldist1.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.8
  store float %mul.ldist1.8, ptr %arrayidx4.ldist1.8, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.8
  %79 = load float, ptr %arrayidx2.ldist1.9, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.9 = fmul float %mul.ldist1.8, %79
  %indvars.iv.next.ldist1.9 = add nuw nsw i64 %indvars.iv.ldist1, 10
  %arrayidx4.ldist1.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.9
  store float %mul.ldist1.9, ptr %arrayidx4.ldist1.9, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.10 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.9
  %80 = load float, ptr %arrayidx2.ldist1.10, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.10 = fmul float %mul.ldist1.9, %80
  %indvars.iv.next.ldist1.10 = add nuw nsw i64 %indvars.iv.ldist1, 11
  %arrayidx4.ldist1.10 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.10
  store float %mul.ldist1.10, ptr %arrayidx4.ldist1.10, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.11 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.10
  %81 = load float, ptr %arrayidx2.ldist1.11, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.11 = fmul float %mul.ldist1.10, %81
  %indvars.iv.next.ldist1.11 = add nuw nsw i64 %indvars.iv.ldist1, 12
  %arrayidx4.ldist1.11 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.11
  store float %mul.ldist1.11, ptr %arrayidx4.ldist1.11, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.12 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.11
  %82 = load float, ptr %arrayidx2.ldist1.12, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.12 = fmul float %mul.ldist1.11, %82
  %indvars.iv.next.ldist1.12 = add nuw nsw i64 %indvars.iv.ldist1, 13
  %arrayidx4.ldist1.12 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.12
  store float %mul.ldist1.12, ptr %arrayidx4.ldist1.12, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.13 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.12
  %83 = load float, ptr %arrayidx2.ldist1.13, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.13 = fmul float %mul.ldist1.12, %83
  %indvars.iv.next.ldist1.13 = add nuw nsw i64 %indvars.iv.ldist1, 14
  %arrayidx4.ldist1.13 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.13
  store float %mul.ldist1.13, ptr %arrayidx4.ldist1.13, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.14 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.13
  %84 = load float, ptr %arrayidx2.ldist1.14, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.14 = fmul float %mul.ldist1.13, %84
  %indvars.iv.next.ldist1.14 = add nuw nsw i64 %indvars.iv.ldist1, 15
  %arrayidx4.ldist1.14 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.14
  store float %mul.ldist1.14, ptr %arrayidx4.ldist1.14, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.15 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.14
  %85 = load float, ptr %arrayidx2.ldist1.15, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.15 = fmul float %mul.ldist1.14, %85
  %indvars.iv.next.ldist1.15 = add nuw nsw i64 %indvars.iv.ldist1, 16
  %arrayidx4.ldist1.15 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.15
  store float %mul.ldist1.15, ptr %arrayidx4.ldist1.15, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.16 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.15
  %86 = load float, ptr %arrayidx2.ldist1.16, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.16 = fmul float %mul.ldist1.15, %86
  %indvars.iv.next.ldist1.16 = add nuw nsw i64 %indvars.iv.ldist1, 17
  %arrayidx4.ldist1.16 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.16
  store float %mul.ldist1.16, ptr %arrayidx4.ldist1.16, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.17 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.16
  %87 = load float, ptr %arrayidx2.ldist1.17, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.17 = fmul float %mul.ldist1.16, %87
  %indvars.iv.next.ldist1.17 = add nuw nsw i64 %indvars.iv.ldist1, 18
  %arrayidx4.ldist1.17 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.17
  store float %mul.ldist1.17, ptr %arrayidx4.ldist1.17, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.18 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.17
  %88 = load float, ptr %arrayidx2.ldist1.18, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.18 = fmul float %mul.ldist1.17, %88
  %indvars.iv.next.ldist1.18 = add nuw nsw i64 %indvars.iv.ldist1, 19
  %arrayidx4.ldist1.18 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.18
  store float %mul.ldist1.18, ptr %arrayidx4.ldist1.18, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.19 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.18
  %89 = load float, ptr %arrayidx2.ldist1.19, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.19 = fmul float %mul.ldist1.18, %89
  %indvars.iv.next.ldist1.19 = add nuw nsw i64 %indvars.iv.ldist1, 20
  %arrayidx4.ldist1.19 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.19
  store float %mul.ldist1.19, ptr %arrayidx4.ldist1.19, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.20 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.19
  %90 = load float, ptr %arrayidx2.ldist1.20, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.20 = fmul float %mul.ldist1.19, %90
  %indvars.iv.next.ldist1.20 = add nuw nsw i64 %indvars.iv.ldist1, 21
  %arrayidx4.ldist1.20 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.20
  store float %mul.ldist1.20, ptr %arrayidx4.ldist1.20, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.21 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.20
  %91 = load float, ptr %arrayidx2.ldist1.21, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.21 = fmul float %mul.ldist1.20, %91
  %indvars.iv.next.ldist1.21 = add nuw nsw i64 %indvars.iv.ldist1, 22
  %arrayidx4.ldist1.21 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.21
  store float %mul.ldist1.21, ptr %arrayidx4.ldist1.21, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.22 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.21
  %92 = load float, ptr %arrayidx2.ldist1.22, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.22 = fmul float %mul.ldist1.21, %92
  %indvars.iv.next.ldist1.22 = add nuw nsw i64 %indvars.iv.ldist1, 23
  %arrayidx4.ldist1.22 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.22
  store float %mul.ldist1.22, ptr %arrayidx4.ldist1.22, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.23 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.22
  %93 = load float, ptr %arrayidx2.ldist1.23, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.23 = fmul float %mul.ldist1.22, %93
  %indvars.iv.next.ldist1.23 = add nuw nsw i64 %indvars.iv.ldist1, 24
  %arrayidx4.ldist1.23 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.23
  store float %mul.ldist1.23, ptr %arrayidx4.ldist1.23, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %arrayidx2.ldist1.24 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.23
  %94 = load float, ptr %arrayidx2.ldist1.24, align 4, !tbaa !2, !alias.scope !15
  %mul.ldist1.24 = fmul float %mul.ldist1.23, %94
  %indvars.iv.next.ldist1.24 = add nuw nsw i64 %indvars.iv.ldist1, 25
  %arrayidx4.ldist1.24 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.24
  store float %mul.ldist1.24, ptr %arrayidx4.ldist1.24, align 4, !tbaa !2, !alias.scope !18, !noalias !20
  %exitcond.not.ldist1.24 = icmp eq i64 %indvars.iv.next.ldist1.24, 10000
  br i1 %exitcond.not.ldist1.24, label %for.body, label %for.body.ldist1, !llvm.loop !24

for.body:                                         ; preds = %for.body.ldist1, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next.24, %for.body ], [ 0, %for.body.ldist1 ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv
  %95 = load float, ptr %arrayidx6, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv
  %96 = load float, ptr %arrayidx8, align 4, !tbaa !2, !alias.scope !26
  %mul9 = fmul float %95, %96
  %arrayidx11 = getelementptr inbounds float, ptr %C, i64 %indvars.iv
  store float %mul9, ptr %arrayidx11, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv, 2
  %arrayidx6.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next
  %97 = load float, ptr %arrayidx6.1, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next
  %98 = load float, ptr %arrayidx8.1, align 4, !tbaa !2, !alias.scope !26
  %mul9.1 = fmul float %97, %98
  %arrayidx11.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next
  store float %mul9.1, ptr %arrayidx11.1, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv, 3
  %arrayidx6.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.1
  %99 = load float, ptr %arrayidx6.2, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.1
  %100 = load float, ptr %arrayidx8.2, align 4, !tbaa !2, !alias.scope !26
  %mul9.2 = fmul float %99, %100
  %arrayidx11.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.1
  store float %mul9.2, ptr %arrayidx11.2, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.3 = add nuw nsw i64 %indvars.iv, 4
  %arrayidx6.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.2
  %101 = load float, ptr %arrayidx6.3, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.2
  %102 = load float, ptr %arrayidx8.3, align 4, !tbaa !2, !alias.scope !26
  %mul9.3 = fmul float %101, %102
  %arrayidx11.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.2
  store float %mul9.3, ptr %arrayidx11.3, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.4 = add nuw nsw i64 %indvars.iv, 5
  %arrayidx6.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.3
  %103 = load float, ptr %arrayidx6.4, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.3
  %104 = load float, ptr %arrayidx8.4, align 4, !tbaa !2, !alias.scope !26
  %mul9.4 = fmul float %103, %104
  %arrayidx11.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.3
  store float %mul9.4, ptr %arrayidx11.4, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.5 = add nuw nsw i64 %indvars.iv, 6
  %arrayidx6.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.4
  %105 = load float, ptr %arrayidx6.5, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.4
  %106 = load float, ptr %arrayidx8.5, align 4, !tbaa !2, !alias.scope !26
  %mul9.5 = fmul float %105, %106
  %arrayidx11.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.4
  store float %mul9.5, ptr %arrayidx11.5, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.6 = add nuw nsw i64 %indvars.iv, 7
  %arrayidx6.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.5
  %107 = load float, ptr %arrayidx6.6, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.5
  %108 = load float, ptr %arrayidx8.6, align 4, !tbaa !2, !alias.scope !26
  %mul9.6 = fmul float %107, %108
  %arrayidx11.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.5
  store float %mul9.6, ptr %arrayidx11.6, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv, 8
  %arrayidx6.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.6
  %109 = load float, ptr %arrayidx6.7, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.6
  %110 = load float, ptr %arrayidx8.7, align 4, !tbaa !2, !alias.scope !26
  %mul9.7 = fmul float %109, %110
  %arrayidx11.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.6
  store float %mul9.7, ptr %arrayidx11.7, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.8 = add nuw nsw i64 %indvars.iv, 9
  %arrayidx6.8 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.7
  %111 = load float, ptr %arrayidx6.8, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.7
  %112 = load float, ptr %arrayidx8.8, align 4, !tbaa !2, !alias.scope !26
  %mul9.8 = fmul float %111, %112
  %arrayidx11.8 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.7
  store float %mul9.8, ptr %arrayidx11.8, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.9 = add nuw nsw i64 %indvars.iv, 10
  %arrayidx6.9 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.8
  %113 = load float, ptr %arrayidx6.9, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.9 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.8
  %114 = load float, ptr %arrayidx8.9, align 4, !tbaa !2, !alias.scope !26
  %mul9.9 = fmul float %113, %114
  %arrayidx11.9 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.8
  store float %mul9.9, ptr %arrayidx11.9, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.10 = add nuw nsw i64 %indvars.iv, 11
  %arrayidx6.10 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.9
  %115 = load float, ptr %arrayidx6.10, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.10 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.9
  %116 = load float, ptr %arrayidx8.10, align 4, !tbaa !2, !alias.scope !26
  %mul9.10 = fmul float %115, %116
  %arrayidx11.10 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.9
  store float %mul9.10, ptr %arrayidx11.10, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.11 = add nuw nsw i64 %indvars.iv, 12
  %arrayidx6.11 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.10
  %117 = load float, ptr %arrayidx6.11, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.11 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.10
  %118 = load float, ptr %arrayidx8.11, align 4, !tbaa !2, !alias.scope !26
  %mul9.11 = fmul float %117, %118
  %arrayidx11.11 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.10
  store float %mul9.11, ptr %arrayidx11.11, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.12 = add nuw nsw i64 %indvars.iv, 13
  %arrayidx6.12 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.11
  %119 = load float, ptr %arrayidx6.12, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.12 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.11
  %120 = load float, ptr %arrayidx8.12, align 4, !tbaa !2, !alias.scope !26
  %mul9.12 = fmul float %119, %120
  %arrayidx11.12 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.11
  store float %mul9.12, ptr %arrayidx11.12, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.13 = add nuw nsw i64 %indvars.iv, 14
  %arrayidx6.13 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.12
  %121 = load float, ptr %arrayidx6.13, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.13 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.12
  %122 = load float, ptr %arrayidx8.13, align 4, !tbaa !2, !alias.scope !26
  %mul9.13 = fmul float %121, %122
  %arrayidx11.13 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.12
  store float %mul9.13, ptr %arrayidx11.13, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.14 = add nuw nsw i64 %indvars.iv, 15
  %arrayidx6.14 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.13
  %123 = load float, ptr %arrayidx6.14, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.14 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.13
  %124 = load float, ptr %arrayidx8.14, align 4, !tbaa !2, !alias.scope !26
  %mul9.14 = fmul float %123, %124
  %arrayidx11.14 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.13
  store float %mul9.14, ptr %arrayidx11.14, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.15 = add nuw nsw i64 %indvars.iv, 16
  %arrayidx6.15 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.14
  %125 = load float, ptr %arrayidx6.15, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.15 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.14
  %126 = load float, ptr %arrayidx8.15, align 4, !tbaa !2, !alias.scope !26
  %mul9.15 = fmul float %125, %126
  %arrayidx11.15 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.14
  store float %mul9.15, ptr %arrayidx11.15, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.16 = add nuw nsw i64 %indvars.iv, 17
  %arrayidx6.16 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.15
  %127 = load float, ptr %arrayidx6.16, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.16 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.15
  %128 = load float, ptr %arrayidx8.16, align 4, !tbaa !2, !alias.scope !26
  %mul9.16 = fmul float %127, %128
  %arrayidx11.16 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.15
  store float %mul9.16, ptr %arrayidx11.16, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.17 = add nuw nsw i64 %indvars.iv, 18
  %arrayidx6.17 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.16
  %129 = load float, ptr %arrayidx6.17, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.17 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.16
  %130 = load float, ptr %arrayidx8.17, align 4, !tbaa !2, !alias.scope !26
  %mul9.17 = fmul float %129, %130
  %arrayidx11.17 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.16
  store float %mul9.17, ptr %arrayidx11.17, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.18 = add nuw nsw i64 %indvars.iv, 19
  %arrayidx6.18 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.17
  %131 = load float, ptr %arrayidx6.18, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.18 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.17
  %132 = load float, ptr %arrayidx8.18, align 4, !tbaa !2, !alias.scope !26
  %mul9.18 = fmul float %131, %132
  %arrayidx11.18 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.17
  store float %mul9.18, ptr %arrayidx11.18, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.19 = add nuw nsw i64 %indvars.iv, 20
  %arrayidx6.19 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.18
  %133 = load float, ptr %arrayidx6.19, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.19 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.18
  %134 = load float, ptr %arrayidx8.19, align 4, !tbaa !2, !alias.scope !26
  %mul9.19 = fmul float %133, %134
  %arrayidx11.19 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.18
  store float %mul9.19, ptr %arrayidx11.19, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.20 = add nuw nsw i64 %indvars.iv, 21
  %arrayidx6.20 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.19
  %135 = load float, ptr %arrayidx6.20, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.20 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.19
  %136 = load float, ptr %arrayidx8.20, align 4, !tbaa !2, !alias.scope !26
  %mul9.20 = fmul float %135, %136
  %arrayidx11.20 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.19
  store float %mul9.20, ptr %arrayidx11.20, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.21 = add nuw nsw i64 %indvars.iv, 22
  %arrayidx6.21 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.20
  %137 = load float, ptr %arrayidx6.21, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.21 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.20
  %138 = load float, ptr %arrayidx8.21, align 4, !tbaa !2, !alias.scope !26
  %mul9.21 = fmul float %137, %138
  %arrayidx11.21 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.20
  store float %mul9.21, ptr %arrayidx11.21, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.22 = add nuw nsw i64 %indvars.iv, 23
  %arrayidx6.22 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.21
  %139 = load float, ptr %arrayidx6.22, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.22 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.21
  %140 = load float, ptr %arrayidx8.22, align 4, !tbaa !2, !alias.scope !26
  %mul9.22 = fmul float %139, %140
  %arrayidx11.22 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.21
  store float %mul9.22, ptr %arrayidx11.22, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.23 = add nuw nsw i64 %indvars.iv, 24
  %arrayidx6.23 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.22
  %141 = load float, ptr %arrayidx6.23, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.23 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.22
  %142 = load float, ptr %arrayidx8.23, align 4, !tbaa !2, !alias.scope !26
  %mul9.23 = fmul float %141, %142
  %arrayidx11.23 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.22
  store float %mul9.23, ptr %arrayidx11.23, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %indvars.iv.next.24 = add nuw nsw i64 %indvars.iv, 25
  %arrayidx6.24 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.23
  %143 = load float, ptr %arrayidx6.24, align 4, !tbaa !2, !alias.scope !25
  %arrayidx8.24 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.23
  %144 = load float, ptr %arrayidx8.24, align 4, !tbaa !2, !alias.scope !26
  %mul9.24 = fmul float %143, %144
  %arrayidx11.24 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.23
  store float %mul9.24, ptr %arrayidx11.24, align 4, !tbaa !2, !alias.scope !27, !noalias !15
  %exitcond.not.24 = icmp eq i64 %indvars.iv.next.24, 10000
  br i1 %exitcond.not.24, label %for.end, label %for.body, !llvm.loop !28

for.end:                                          ; preds = %for.body, %for.body.lver.orig, %for.body.lver.orig.lver.orig
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) vscale_range(1,16) "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 17.0.3 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git a1d6a811fe4e3988a1c6817ceebeed72e1a425df)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"float", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C++ TBAA"}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7, !9, !10, !12}
!9 = !{!"llvm.loop.vectorize.width", i32 1}
!10 = !{!"llvm.loop.vectorize.followup_all", !11}
!11 = distinct !{!11, !7, !12, !13, !14}
!12 = !{!"llvm.loop.isvectorized"}
!13 = !{!"llvm.loop.pipeline.enable"}
!14 = !{!"llvm.loop.pipeline.nodep"}
!15 = !{!16}
!16 = distinct !{!16, !17}
!17 = distinct !{!17, !"LVerDomain"}
!18 = !{!19}
!19 = distinct !{!19, !17}
!20 = !{!21, !22, !23}
!21 = distinct !{!21, !17}
!22 = distinct !{!22, !17}
!23 = distinct !{!23, !17}
!24 = distinct !{!24, !8}
!25 = !{!22}
!26 = !{!23}
!27 = !{!21}
!28 = distinct !{!28, !8}
