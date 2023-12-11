; RUN: llc < %s -fswp -O2 -mcpu=a64fx --pass-remarks=aarch64-swpipeliner --pass-remarks-analysis=aarch64-swpipeliner -o /dev/null 2>&1 | FileCheck %s
; CHECK: remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: software pipelining 
; CHECK-NEXT:remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK:remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: software pipelining 
; CHECK-NEXT: remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: Since the pragma pipeline_nodep was specified, it was assumed that there is no dependency between memory access instructions in the loop.
; CHECK: remark: ./pragma-loop-pipeline-nodep004.cpp:26:5: software pipelining 

; ModuleID = './pragma-loop-pipeline-nodep004.cpp'
source_filename = "./pragma-loop-pipeline-nodep004.cpp"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-hurd-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(1,16)
define dso_local void @_Z3fooPfS_S_S_S_(ptr nocapture noundef %A, ptr nocapture noundef readonly %B, ptr nocapture noundef writeonly %C, ptr nocapture noundef readonly %D, ptr nocapture noundef readonly %E) local_unnamed_addr #0 !dbg !8 {
entry:
  %E40 = ptrtoint ptr %E to i64, !dbg !12
  %D39 = ptrtoint ptr %D to i64, !dbg !12
  %C38 = ptrtoint ptr %C to i64, !dbg !12
  %scevgep = getelementptr i8, ptr %A, i64 40004, !dbg !12
  %scevgep22 = getelementptr i8, ptr %C, i64 40000, !dbg !12
  %scevgep23 = getelementptr i8, ptr %D, i64 40000, !dbg !12
  %scevgep24 = getelementptr i8, ptr %E, i64 40000, !dbg !12
  %scevgep25 = getelementptr i8, ptr %B, i64 40000, !dbg !12
  %bound0 = icmp ugt ptr %scevgep22, %A, !dbg !12
  %bound1 = icmp ugt ptr %scevgep, %C, !dbg !12
  %found.conflict = and i1 %bound0, %bound1, !dbg !12
  %bound026 = icmp ugt ptr %scevgep23, %A, !dbg !12
  %bound127 = icmp ugt ptr %scevgep, %D, !dbg !12
  %found.conflict28 = and i1 %bound026, %bound127, !dbg !12
  %conflict.rdx = or i1 %found.conflict, %found.conflict28, !dbg !12
  %bound029 = icmp ugt ptr %scevgep24, %A, !dbg !12
  %bound130 = icmp ugt ptr %scevgep, %E, !dbg !12
  %found.conflict31 = and i1 %bound029, %bound130, !dbg !12
  %conflict.rdx32 = or i1 %conflict.rdx, %found.conflict31, !dbg !12
  %bound033 = icmp ugt ptr %scevgep25, %C, !dbg !12
  %bound134 = icmp ugt ptr %scevgep22, %B, !dbg !12
  %found.conflict35 = and i1 %bound033, %bound134, !dbg !12
  %conflict.rdx36 = or i1 %conflict.rdx32, %found.conflict35, !dbg !12
  br i1 %conflict.rdx36, label %for.body.lver.orig.lver.check, label %for.body.ph.ldist1, !dbg !12

for.body.lver.orig.lver.check:                    ; preds = %entry
  %scevgep58 = getelementptr i8, ptr %A, i64 40004, !dbg !12
  %scevgep59 = getelementptr i8, ptr %C, i64 40000, !dbg !12
  %bound060 = icmp ugt ptr %scevgep59, %A, !dbg !12
  %bound161 = icmp ugt ptr %scevgep58, %C, !dbg !12
  %found.conflict62 = and i1 %bound060, %bound161, !dbg !12
  br i1 %found.conflict62, label %for.body.lver.orig.lver.orig, label %for.body.lver.orig.ph, !dbg !12

for.body.lver.orig.lver.orig:                     ; preds = %for.body.lver.orig.lver.check, %for.body.lver.orig.lver.orig
  %indvars.iv.lver.orig.lver.orig = phi i64 [ %indvars.iv.next.lver.orig.lver.orig.9, %for.body.lver.orig.lver.orig ], [ 0, %for.body.lver.orig.lver.check ]
  %arrayidx.lver.orig.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.lver.orig.lver.orig, !dbg !13
  %0 = load float, ptr %arrayidx.lver.orig.lver.orig, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig = getelementptr inbounds float, ptr %B, i64 %indvars.iv.lver.orig.lver.orig, !dbg !18
  %1 = load float, ptr %arrayidx2.lver.orig.lver.orig, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig = fmul fast float %1, %0, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig = or i64 %indvars.iv.lver.orig.lver.orig, 1, !dbg !20
  %arrayidx4.lver.orig.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !21
  store float %mul.lver.orig.lver.orig, ptr %arrayidx4.lver.orig.lver.orig, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig = getelementptr inbounds float, ptr %D, i64 %indvars.iv.lver.orig.lver.orig, !dbg !23
  %2 = load float, ptr %arrayidx6.lver.orig.lver.orig, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig = getelementptr inbounds float, ptr %E, i64 %indvars.iv.lver.orig.lver.orig, !dbg !24
  %3 = load float, ptr %arrayidx8.lver.orig.lver.orig, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig = fmul fast float %3, %2, !dbg !25
  %arrayidx11.lver.orig.lver.orig = getelementptr inbounds float, ptr %C, i64 %indvars.iv.lver.orig.lver.orig, !dbg !26
  store float %mul9.lver.orig.lver.orig, ptr %arrayidx11.lver.orig.lver.orig, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !13
  %4 = load float, ptr %arrayidx.lver.orig.lver.orig.1, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !18
  %5 = load float, ptr %arrayidx2.lver.orig.lver.orig.1, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.1 = fmul fast float %5, %4, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.1 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 2, !dbg !20
  %arrayidx4.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !21
  store float %mul.lver.orig.lver.orig.1, ptr %arrayidx4.lver.orig.lver.orig.1, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !23
  %6 = load float, ptr %arrayidx6.lver.orig.lver.orig.1, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !24
  %7 = load float, ptr %arrayidx8.lver.orig.lver.orig.1, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.1 = fmul fast float %7, %6, !dbg !25
  %arrayidx11.lver.orig.lver.orig.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig, !dbg !26
  store float %mul9.lver.orig.lver.orig.1, ptr %arrayidx11.lver.orig.lver.orig.1, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !13
  %8 = load float, ptr %arrayidx.lver.orig.lver.orig.2, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !18
  %9 = load float, ptr %arrayidx2.lver.orig.lver.orig.2, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.2 = fmul fast float %9, %8, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.2 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 3, !dbg !20
  %arrayidx4.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !21
  store float %mul.lver.orig.lver.orig.2, ptr %arrayidx4.lver.orig.lver.orig.2, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !23
  %10 = load float, ptr %arrayidx6.lver.orig.lver.orig.2, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !24
  %11 = load float, ptr %arrayidx8.lver.orig.lver.orig.2, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.2 = fmul fast float %11, %10, !dbg !25
  %arrayidx11.lver.orig.lver.orig.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.1, !dbg !26
  store float %mul9.lver.orig.lver.orig.2, ptr %arrayidx11.lver.orig.lver.orig.2, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !13
  %12 = load float, ptr %arrayidx.lver.orig.lver.orig.3, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !18
  %13 = load float, ptr %arrayidx2.lver.orig.lver.orig.3, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.3 = fmul fast float %13, %12, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.3 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 4, !dbg !20
  %arrayidx4.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !21
  store float %mul.lver.orig.lver.orig.3, ptr %arrayidx4.lver.orig.lver.orig.3, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !23
  %14 = load float, ptr %arrayidx6.lver.orig.lver.orig.3, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !24
  %15 = load float, ptr %arrayidx8.lver.orig.lver.orig.3, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.3 = fmul fast float %15, %14, !dbg !25
  %arrayidx11.lver.orig.lver.orig.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.2, !dbg !26
  store float %mul9.lver.orig.lver.orig.3, ptr %arrayidx11.lver.orig.lver.orig.3, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !13
  %16 = load float, ptr %arrayidx.lver.orig.lver.orig.4, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !18
  %17 = load float, ptr %arrayidx2.lver.orig.lver.orig.4, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.4 = fmul fast float %17, %16, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.4 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 5, !dbg !20
  %arrayidx4.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !21
  store float %mul.lver.orig.lver.orig.4, ptr %arrayidx4.lver.orig.lver.orig.4, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !23
  %18 = load float, ptr %arrayidx6.lver.orig.lver.orig.4, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !24
  %19 = load float, ptr %arrayidx8.lver.orig.lver.orig.4, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.4 = fmul fast float %19, %18, !dbg !25
  %arrayidx11.lver.orig.lver.orig.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.3, !dbg !26
  store float %mul9.lver.orig.lver.orig.4, ptr %arrayidx11.lver.orig.lver.orig.4, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !13
  %20 = load float, ptr %arrayidx.lver.orig.lver.orig.5, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !18
  %21 = load float, ptr %arrayidx2.lver.orig.lver.orig.5, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.5 = fmul fast float %21, %20, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.5 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 6, !dbg !20
  %arrayidx4.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !21
  store float %mul.lver.orig.lver.orig.5, ptr %arrayidx4.lver.orig.lver.orig.5, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !23
  %22 = load float, ptr %arrayidx6.lver.orig.lver.orig.5, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !24
  %23 = load float, ptr %arrayidx8.lver.orig.lver.orig.5, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.5 = fmul fast float %23, %22, !dbg !25
  %arrayidx11.lver.orig.lver.orig.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.4, !dbg !26
  store float %mul9.lver.orig.lver.orig.5, ptr %arrayidx11.lver.orig.lver.orig.5, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !13
  %24 = load float, ptr %arrayidx.lver.orig.lver.orig.6, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !18
  %25 = load float, ptr %arrayidx2.lver.orig.lver.orig.6, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.6 = fmul fast float %25, %24, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.6 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 7, !dbg !20
  %arrayidx4.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !21
  store float %mul.lver.orig.lver.orig.6, ptr %arrayidx4.lver.orig.lver.orig.6, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !23
  %26 = load float, ptr %arrayidx6.lver.orig.lver.orig.6, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !24
  %27 = load float, ptr %arrayidx8.lver.orig.lver.orig.6, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.6 = fmul fast float %27, %26, !dbg !25
  %arrayidx11.lver.orig.lver.orig.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.5, !dbg !26
  store float %mul9.lver.orig.lver.orig.6, ptr %arrayidx11.lver.orig.lver.orig.6, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !13
  %28 = load float, ptr %arrayidx.lver.orig.lver.orig.7, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !18
  %29 = load float, ptr %arrayidx2.lver.orig.lver.orig.7, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.7 = fmul fast float %29, %28, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.7 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 8, !dbg !20
  %arrayidx4.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !21
  store float %mul.lver.orig.lver.orig.7, ptr %arrayidx4.lver.orig.lver.orig.7, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !23
  %30 = load float, ptr %arrayidx6.lver.orig.lver.orig.7, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !24
  %31 = load float, ptr %arrayidx8.lver.orig.lver.orig.7, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.7 = fmul fast float %31, %30, !dbg !25
  %arrayidx11.lver.orig.lver.orig.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.6, !dbg !26
  store float %mul9.lver.orig.lver.orig.7, ptr %arrayidx11.lver.orig.lver.orig.7, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !13
  %32 = load float, ptr %arrayidx.lver.orig.lver.orig.8, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !18
  %33 = load float, ptr %arrayidx2.lver.orig.lver.orig.8, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.8 = fmul fast float %33, %32, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.8 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 9, !dbg !20
  %arrayidx4.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !21
  store float %mul.lver.orig.lver.orig.8, ptr %arrayidx4.lver.orig.lver.orig.8, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !23
  %34 = load float, ptr %arrayidx6.lver.orig.lver.orig.8, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !24
  %35 = load float, ptr %arrayidx8.lver.orig.lver.orig.8, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.8 = fmul fast float %35, %34, !dbg !25
  %arrayidx11.lver.orig.lver.orig.8 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.7, !dbg !26
  store float %mul9.lver.orig.lver.orig.8, ptr %arrayidx11.lver.orig.lver.orig.8, align 4, !dbg !27, !tbaa !14
  %arrayidx.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !13
  %36 = load float, ptr %arrayidx.lver.orig.lver.orig.9, align 4, !dbg !13, !tbaa !14
  %arrayidx2.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !18
  %37 = load float, ptr %arrayidx2.lver.orig.lver.orig.9, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.lver.orig.9 = fmul fast float %37, %36, !dbg !19
  %indvars.iv.next.lver.orig.lver.orig.9 = add nuw nsw i64 %indvars.iv.lver.orig.lver.orig, 10, !dbg !20
  %arrayidx4.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.lver.orig.9, !dbg !21
  store float %mul.lver.orig.lver.orig.9, ptr %arrayidx4.lver.orig.lver.orig.9, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !23
  %38 = load float, ptr %arrayidx6.lver.orig.lver.orig.9, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !24
  %39 = load float, ptr %arrayidx8.lver.orig.lver.orig.9, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.lver.orig.9 = fmul fast float %39, %38, !dbg !25
  %arrayidx11.lver.orig.lver.orig.9 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.lver.orig.8, !dbg !26
  store float %mul9.lver.orig.lver.orig.9, ptr %arrayidx11.lver.orig.lver.orig.9, align 4, !dbg !27, !tbaa !14
  %exitcond.not.lver.orig.lver.orig.9 = icmp eq i64 %indvars.iv.next.lver.orig.lver.orig.9, 10000, !dbg !28
  br i1 %exitcond.not.lver.orig.lver.orig.9, label %for.end, label %for.body.lver.orig.lver.orig, !dbg !12, !llvm.loop !29

for.body.lver.orig.ph:                            ; preds = %for.body.lver.orig.lver.check
  %load_initial = load float, ptr %A, align 4
  br label %for.body.lver.orig, !dbg !12

for.body.lver.orig:                               ; preds = %for.body.lver.orig, %for.body.lver.orig.ph
  %store_forwarded = phi float [ %load_initial, %for.body.lver.orig.ph ], [ %mul.lver.orig.9, %for.body.lver.orig ]
  %indvars.iv.lver.orig = phi i64 [ 0, %for.body.lver.orig.ph ], [ %indvars.iv.next.lver.orig.9, %for.body.lver.orig ]
  %arrayidx2.lver.orig = getelementptr inbounds float, ptr %B, i64 %indvars.iv.lver.orig, !dbg !18
  %40 = load float, ptr %arrayidx2.lver.orig, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig = fmul fast float %40, %store_forwarded, !dbg !19
  %indvars.iv.next.lver.orig = or i64 %indvars.iv.lver.orig, 1, !dbg !20
  %arrayidx4.lver.orig = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig, !dbg !21
  store float %mul.lver.orig, ptr %arrayidx4.lver.orig, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig = getelementptr inbounds float, ptr %D, i64 %indvars.iv.lver.orig, !dbg !23
  %41 = load float, ptr %arrayidx6.lver.orig, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig = getelementptr inbounds float, ptr %E, i64 %indvars.iv.lver.orig, !dbg !24
  %42 = load float, ptr %arrayidx8.lver.orig, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig = fmul fast float %42, %41, !dbg !25
  %arrayidx11.lver.orig = getelementptr inbounds float, ptr %C, i64 %indvars.iv.lver.orig, !dbg !26
  store float %mul9.lver.orig, ptr %arrayidx11.lver.orig, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig, !dbg !18
  %43 = load float, ptr %arrayidx2.lver.orig.1, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.1 = fmul fast float %43, %mul.lver.orig, !dbg !19
  %indvars.iv.next.lver.orig.1 = add nuw nsw i64 %indvars.iv.lver.orig, 2, !dbg !20
  %arrayidx4.lver.orig.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.1, !dbg !21
  store float %mul.lver.orig.1, ptr %arrayidx4.lver.orig.1, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig, !dbg !23
  %44 = load float, ptr %arrayidx6.lver.orig.1, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig, !dbg !24
  %45 = load float, ptr %arrayidx8.lver.orig.1, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.1 = fmul fast float %45, %44, !dbg !25
  %arrayidx11.lver.orig.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig, !dbg !26
  store float %mul9.lver.orig.1, ptr %arrayidx11.lver.orig.1, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.1, !dbg !18
  %46 = load float, ptr %arrayidx2.lver.orig.2, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.2 = fmul fast float %46, %mul.lver.orig.1, !dbg !19
  %indvars.iv.next.lver.orig.2 = add nuw nsw i64 %indvars.iv.lver.orig, 3, !dbg !20
  %arrayidx4.lver.orig.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.2, !dbg !21
  store float %mul.lver.orig.2, ptr %arrayidx4.lver.orig.2, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.1, !dbg !23
  %47 = load float, ptr %arrayidx6.lver.orig.2, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.1, !dbg !24
  %48 = load float, ptr %arrayidx8.lver.orig.2, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.2 = fmul fast float %48, %47, !dbg !25
  %arrayidx11.lver.orig.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.1, !dbg !26
  store float %mul9.lver.orig.2, ptr %arrayidx11.lver.orig.2, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.2, !dbg !18
  %49 = load float, ptr %arrayidx2.lver.orig.3, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.3 = fmul fast float %49, %mul.lver.orig.2, !dbg !19
  %indvars.iv.next.lver.orig.3 = add nuw nsw i64 %indvars.iv.lver.orig, 4, !dbg !20
  %arrayidx4.lver.orig.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.3, !dbg !21
  store float %mul.lver.orig.3, ptr %arrayidx4.lver.orig.3, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.2, !dbg !23
  %50 = load float, ptr %arrayidx6.lver.orig.3, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.2, !dbg !24
  %51 = load float, ptr %arrayidx8.lver.orig.3, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.3 = fmul fast float %51, %50, !dbg !25
  %arrayidx11.lver.orig.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.2, !dbg !26
  store float %mul9.lver.orig.3, ptr %arrayidx11.lver.orig.3, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.3, !dbg !18
  %52 = load float, ptr %arrayidx2.lver.orig.4, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.4 = fmul fast float %52, %mul.lver.orig.3, !dbg !19
  %indvars.iv.next.lver.orig.4 = add nuw nsw i64 %indvars.iv.lver.orig, 5, !dbg !20
  %arrayidx4.lver.orig.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.4, !dbg !21
  store float %mul.lver.orig.4, ptr %arrayidx4.lver.orig.4, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.3, !dbg !23
  %53 = load float, ptr %arrayidx6.lver.orig.4, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.3, !dbg !24
  %54 = load float, ptr %arrayidx8.lver.orig.4, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.4 = fmul fast float %54, %53, !dbg !25
  %arrayidx11.lver.orig.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.3, !dbg !26
  store float %mul9.lver.orig.4, ptr %arrayidx11.lver.orig.4, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.4, !dbg !18
  %55 = load float, ptr %arrayidx2.lver.orig.5, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.5 = fmul fast float %55, %mul.lver.orig.4, !dbg !19
  %indvars.iv.next.lver.orig.5 = add nuw nsw i64 %indvars.iv.lver.orig, 6, !dbg !20
  %arrayidx4.lver.orig.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.5, !dbg !21
  store float %mul.lver.orig.5, ptr %arrayidx4.lver.orig.5, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.4, !dbg !23
  %56 = load float, ptr %arrayidx6.lver.orig.5, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.4, !dbg !24
  %57 = load float, ptr %arrayidx8.lver.orig.5, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.5 = fmul fast float %57, %56, !dbg !25
  %arrayidx11.lver.orig.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.4, !dbg !26
  store float %mul9.lver.orig.5, ptr %arrayidx11.lver.orig.5, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.5, !dbg !18
  %58 = load float, ptr %arrayidx2.lver.orig.6, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.6 = fmul fast float %58, %mul.lver.orig.5, !dbg !19
  %indvars.iv.next.lver.orig.6 = add nuw nsw i64 %indvars.iv.lver.orig, 7, !dbg !20
  %arrayidx4.lver.orig.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.6, !dbg !21
  store float %mul.lver.orig.6, ptr %arrayidx4.lver.orig.6, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.5, !dbg !23
  %59 = load float, ptr %arrayidx6.lver.orig.6, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.5, !dbg !24
  %60 = load float, ptr %arrayidx8.lver.orig.6, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.6 = fmul fast float %60, %59, !dbg !25
  %arrayidx11.lver.orig.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.5, !dbg !26
  store float %mul9.lver.orig.6, ptr %arrayidx11.lver.orig.6, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.6, !dbg !18
  %61 = load float, ptr %arrayidx2.lver.orig.7, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.7 = fmul fast float %61, %mul.lver.orig.6, !dbg !19
  %indvars.iv.next.lver.orig.7 = add nuw nsw i64 %indvars.iv.lver.orig, 8, !dbg !20
  %arrayidx4.lver.orig.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.7, !dbg !21
  store float %mul.lver.orig.7, ptr %arrayidx4.lver.orig.7, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.6, !dbg !23
  %62 = load float, ptr %arrayidx6.lver.orig.7, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.6, !dbg !24
  %63 = load float, ptr %arrayidx8.lver.orig.7, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.7 = fmul fast float %63, %62, !dbg !25
  %arrayidx11.lver.orig.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.6, !dbg !26
  store float %mul9.lver.orig.7, ptr %arrayidx11.lver.orig.7, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.7, !dbg !18
  %64 = load float, ptr %arrayidx2.lver.orig.8, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.8 = fmul fast float %64, %mul.lver.orig.7, !dbg !19
  %indvars.iv.next.lver.orig.8 = add nuw nsw i64 %indvars.iv.lver.orig, 9, !dbg !20
  %arrayidx4.lver.orig.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.8, !dbg !21
  store float %mul.lver.orig.8, ptr %arrayidx4.lver.orig.8, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.8 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.7, !dbg !23
  %65 = load float, ptr %arrayidx6.lver.orig.8, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.7, !dbg !24
  %66 = load float, ptr %arrayidx8.lver.orig.8, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.8 = fmul fast float %66, %65, !dbg !25
  %arrayidx11.lver.orig.8 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.7, !dbg !26
  store float %mul9.lver.orig.8, ptr %arrayidx11.lver.orig.8, align 4, !dbg !27, !tbaa !14
  %arrayidx2.lver.orig.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.lver.orig.8, !dbg !18
  %67 = load float, ptr %arrayidx2.lver.orig.9, align 4, !dbg !18, !tbaa !14
  %mul.lver.orig.9 = fmul fast float %67, %mul.lver.orig.8, !dbg !19
  %indvars.iv.next.lver.orig.9 = add nuw nsw i64 %indvars.iv.lver.orig, 10, !dbg !20
  %arrayidx4.lver.orig.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.lver.orig.9, !dbg !21
  store float %mul.lver.orig.9, ptr %arrayidx4.lver.orig.9, align 4, !dbg !22, !tbaa !14
  %arrayidx6.lver.orig.9 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.lver.orig.8, !dbg !23
  %68 = load float, ptr %arrayidx6.lver.orig.9, align 4, !dbg !23, !tbaa !14
  %arrayidx8.lver.orig.9 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.lver.orig.8, !dbg !24
  %69 = load float, ptr %arrayidx8.lver.orig.9, align 4, !dbg !24, !tbaa !14
  %mul9.lver.orig.9 = fmul fast float %69, %68, !dbg !25
  %arrayidx11.lver.orig.9 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.lver.orig.8, !dbg !26
  store float %mul9.lver.orig.9, ptr %arrayidx11.lver.orig.9, align 4, !dbg !27, !tbaa !14
  %exitcond.not.lver.orig.9 = icmp eq i64 %indvars.iv.next.lver.orig.9, 10000, !dbg !28
  br i1 %exitcond.not.lver.orig.9, label %for.end, label %for.body.lver.orig, !dbg !12, !llvm.loop !29

for.body.ph.ldist1:                               ; preds = %entry
  %load_initial64 = load float, ptr %A, align 4
  br label %for.body.ldist1, !dbg !12

for.body.ldist1:                                  ; preds = %for.body.ldist1, %for.body.ph.ldist1
  %store_forwarded65 = phi float [ %load_initial64, %for.body.ph.ldist1 ], [ %mul.ldist1.24, %for.body.ldist1 ]
  %indvars.iv.ldist1 = phi i64 [ 0, %for.body.ph.ldist1 ], [ %indvars.iv.next.ldist1.24, %for.body.ldist1 ]
  %arrayidx2.ldist1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.ldist1, !dbg !18
  %70 = load float, ptr %arrayidx2.ldist1, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1 = fmul fast float %70, %store_forwarded65, !dbg !19
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1, !dbg !20
  %arrayidx4.ldist1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1, !dbg !21
  store float %mul.ldist1, ptr %arrayidx4.ldist1, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.1 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1, !dbg !18
  %71 = load float, ptr %arrayidx2.ldist1.1, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.1 = fmul fast float %71, %mul.ldist1, !dbg !19
  %indvars.iv.next.ldist1.1 = add nuw nsw i64 %indvars.iv.ldist1, 2, !dbg !20
  %arrayidx4.ldist1.1 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.1, !dbg !21
  store float %mul.ldist1.1, ptr %arrayidx4.ldist1.1, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.2 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.1, !dbg !18
  %72 = load float, ptr %arrayidx2.ldist1.2, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.2 = fmul fast float %72, %mul.ldist1.1, !dbg !19
  %indvars.iv.next.ldist1.2 = add nuw nsw i64 %indvars.iv.ldist1, 3, !dbg !20
  %arrayidx4.ldist1.2 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.2, !dbg !21
  store float %mul.ldist1.2, ptr %arrayidx4.ldist1.2, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.3 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.2, !dbg !18
  %73 = load float, ptr %arrayidx2.ldist1.3, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.3 = fmul fast float %73, %mul.ldist1.2, !dbg !19
  %indvars.iv.next.ldist1.3 = add nuw nsw i64 %indvars.iv.ldist1, 4, !dbg !20
  %arrayidx4.ldist1.3 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.3, !dbg !21
  store float %mul.ldist1.3, ptr %arrayidx4.ldist1.3, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.4 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.3, !dbg !18
  %74 = load float, ptr %arrayidx2.ldist1.4, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.4 = fmul fast float %74, %mul.ldist1.3, !dbg !19
  %indvars.iv.next.ldist1.4 = add nuw nsw i64 %indvars.iv.ldist1, 5, !dbg !20
  %arrayidx4.ldist1.4 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.4, !dbg !21
  store float %mul.ldist1.4, ptr %arrayidx4.ldist1.4, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.5 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.4, !dbg !18
  %75 = load float, ptr %arrayidx2.ldist1.5, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.5 = fmul fast float %75, %mul.ldist1.4, !dbg !19
  %indvars.iv.next.ldist1.5 = add nuw nsw i64 %indvars.iv.ldist1, 6, !dbg !20
  %arrayidx4.ldist1.5 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.5, !dbg !21
  store float %mul.ldist1.5, ptr %arrayidx4.ldist1.5, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.6 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.5, !dbg !18
  %76 = load float, ptr %arrayidx2.ldist1.6, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.6 = fmul fast float %76, %mul.ldist1.5, !dbg !19
  %indvars.iv.next.ldist1.6 = add nuw nsw i64 %indvars.iv.ldist1, 7, !dbg !20
  %arrayidx4.ldist1.6 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.6, !dbg !21
  store float %mul.ldist1.6, ptr %arrayidx4.ldist1.6, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.7 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.6, !dbg !18
  %77 = load float, ptr %arrayidx2.ldist1.7, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.7 = fmul fast float %77, %mul.ldist1.6, !dbg !19
  %indvars.iv.next.ldist1.7 = add nuw nsw i64 %indvars.iv.ldist1, 8, !dbg !20
  %arrayidx4.ldist1.7 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.7, !dbg !21
  store float %mul.ldist1.7, ptr %arrayidx4.ldist1.7, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.8 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.7, !dbg !18
  %78 = load float, ptr %arrayidx2.ldist1.8, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.8 = fmul fast float %78, %mul.ldist1.7, !dbg !19
  %indvars.iv.next.ldist1.8 = add nuw nsw i64 %indvars.iv.ldist1, 9, !dbg !20
  %arrayidx4.ldist1.8 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.8, !dbg !21
  store float %mul.ldist1.8, ptr %arrayidx4.ldist1.8, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.9 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.8, !dbg !18
  %79 = load float, ptr %arrayidx2.ldist1.9, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.9 = fmul fast float %79, %mul.ldist1.8, !dbg !19
  %indvars.iv.next.ldist1.9 = add nuw nsw i64 %indvars.iv.ldist1, 10, !dbg !20
  %arrayidx4.ldist1.9 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.9, !dbg !21
  store float %mul.ldist1.9, ptr %arrayidx4.ldist1.9, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.10 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.9, !dbg !18
  %80 = load float, ptr %arrayidx2.ldist1.10, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.10 = fmul fast float %80, %mul.ldist1.9, !dbg !19
  %indvars.iv.next.ldist1.10 = add nuw nsw i64 %indvars.iv.ldist1, 11, !dbg !20
  %arrayidx4.ldist1.10 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.10, !dbg !21
  store float %mul.ldist1.10, ptr %arrayidx4.ldist1.10, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.11 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.10, !dbg !18
  %81 = load float, ptr %arrayidx2.ldist1.11, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.11 = fmul fast float %81, %mul.ldist1.10, !dbg !19
  %indvars.iv.next.ldist1.11 = add nuw nsw i64 %indvars.iv.ldist1, 12, !dbg !20
  %arrayidx4.ldist1.11 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.11, !dbg !21
  store float %mul.ldist1.11, ptr %arrayidx4.ldist1.11, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.12 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.11, !dbg !18
  %82 = load float, ptr %arrayidx2.ldist1.12, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.12 = fmul fast float %82, %mul.ldist1.11, !dbg !19
  %indvars.iv.next.ldist1.12 = add nuw nsw i64 %indvars.iv.ldist1, 13, !dbg !20
  %arrayidx4.ldist1.12 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.12, !dbg !21
  store float %mul.ldist1.12, ptr %arrayidx4.ldist1.12, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.13 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.12, !dbg !18
  %83 = load float, ptr %arrayidx2.ldist1.13, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.13 = fmul fast float %83, %mul.ldist1.12, !dbg !19
  %indvars.iv.next.ldist1.13 = add nuw nsw i64 %indvars.iv.ldist1, 14, !dbg !20
  %arrayidx4.ldist1.13 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.13, !dbg !21
  store float %mul.ldist1.13, ptr %arrayidx4.ldist1.13, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.14 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.13, !dbg !18
  %84 = load float, ptr %arrayidx2.ldist1.14, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.14 = fmul fast float %84, %mul.ldist1.13, !dbg !19
  %indvars.iv.next.ldist1.14 = add nuw nsw i64 %indvars.iv.ldist1, 15, !dbg !20
  %arrayidx4.ldist1.14 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.14, !dbg !21
  store float %mul.ldist1.14, ptr %arrayidx4.ldist1.14, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.15 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.14, !dbg !18
  %85 = load float, ptr %arrayidx2.ldist1.15, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.15 = fmul fast float %85, %mul.ldist1.14, !dbg !19
  %indvars.iv.next.ldist1.15 = add nuw nsw i64 %indvars.iv.ldist1, 16, !dbg !20
  %arrayidx4.ldist1.15 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.15, !dbg !21
  store float %mul.ldist1.15, ptr %arrayidx4.ldist1.15, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.16 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.15, !dbg !18
  %86 = load float, ptr %arrayidx2.ldist1.16, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.16 = fmul fast float %86, %mul.ldist1.15, !dbg !19
  %indvars.iv.next.ldist1.16 = add nuw nsw i64 %indvars.iv.ldist1, 17, !dbg !20
  %arrayidx4.ldist1.16 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.16, !dbg !21
  store float %mul.ldist1.16, ptr %arrayidx4.ldist1.16, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.17 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.16, !dbg !18
  %87 = load float, ptr %arrayidx2.ldist1.17, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.17 = fmul fast float %87, %mul.ldist1.16, !dbg !19
  %indvars.iv.next.ldist1.17 = add nuw nsw i64 %indvars.iv.ldist1, 18, !dbg !20
  %arrayidx4.ldist1.17 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.17, !dbg !21
  store float %mul.ldist1.17, ptr %arrayidx4.ldist1.17, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.18 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.17, !dbg !18
  %88 = load float, ptr %arrayidx2.ldist1.18, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.18 = fmul fast float %88, %mul.ldist1.17, !dbg !19
  %indvars.iv.next.ldist1.18 = add nuw nsw i64 %indvars.iv.ldist1, 19, !dbg !20
  %arrayidx4.ldist1.18 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.18, !dbg !21
  store float %mul.ldist1.18, ptr %arrayidx4.ldist1.18, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.19 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.18, !dbg !18
  %89 = load float, ptr %arrayidx2.ldist1.19, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.19 = fmul fast float %89, %mul.ldist1.18, !dbg !19
  %indvars.iv.next.ldist1.19 = add nuw nsw i64 %indvars.iv.ldist1, 20, !dbg !20
  %arrayidx4.ldist1.19 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.19, !dbg !21
  store float %mul.ldist1.19, ptr %arrayidx4.ldist1.19, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.20 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.19, !dbg !18
  %90 = load float, ptr %arrayidx2.ldist1.20, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.20 = fmul fast float %90, %mul.ldist1.19, !dbg !19
  %indvars.iv.next.ldist1.20 = add nuw nsw i64 %indvars.iv.ldist1, 21, !dbg !20
  %arrayidx4.ldist1.20 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.20, !dbg !21
  store float %mul.ldist1.20, ptr %arrayidx4.ldist1.20, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.21 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.20, !dbg !18
  %91 = load float, ptr %arrayidx2.ldist1.21, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.21 = fmul fast float %91, %mul.ldist1.20, !dbg !19
  %indvars.iv.next.ldist1.21 = add nuw nsw i64 %indvars.iv.ldist1, 22, !dbg !20
  %arrayidx4.ldist1.21 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.21, !dbg !21
  store float %mul.ldist1.21, ptr %arrayidx4.ldist1.21, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.22 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.21, !dbg !18
  %92 = load float, ptr %arrayidx2.ldist1.22, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.22 = fmul fast float %92, %mul.ldist1.21, !dbg !19
  %indvars.iv.next.ldist1.22 = add nuw nsw i64 %indvars.iv.ldist1, 23, !dbg !20
  %arrayidx4.ldist1.22 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.22, !dbg !21
  store float %mul.ldist1.22, ptr %arrayidx4.ldist1.22, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.23 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.22, !dbg !18
  %93 = load float, ptr %arrayidx2.ldist1.23, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.23 = fmul fast float %93, %mul.ldist1.22, !dbg !19
  %indvars.iv.next.ldist1.23 = add nuw nsw i64 %indvars.iv.ldist1, 24, !dbg !20
  %arrayidx4.ldist1.23 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.23, !dbg !21
  store float %mul.ldist1.23, ptr %arrayidx4.ldist1.23, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %arrayidx2.ldist1.24 = getelementptr inbounds float, ptr %B, i64 %indvars.iv.next.ldist1.23, !dbg !18
  %94 = load float, ptr %arrayidx2.ldist1.24, align 4, !dbg !18, !tbaa !14, !alias.scope !38
  %mul.ldist1.24 = fmul fast float %94, %mul.ldist1.23, !dbg !19
  %indvars.iv.next.ldist1.24 = add nuw nsw i64 %indvars.iv.ldist1, 25, !dbg !20
  %arrayidx4.ldist1.24 = getelementptr inbounds float, ptr %A, i64 %indvars.iv.next.ldist1.24, !dbg !21
  store float %mul.ldist1.24, ptr %arrayidx4.ldist1.24, align 4, !dbg !22, !tbaa !14, !alias.scope !41, !noalias !43
  %exitcond.not.ldist1.24 = icmp eq i64 %indvars.iv.next.ldist1.24, 10000, !dbg !28
  br i1 %exitcond.not.ldist1.24, label %vector.memcheck, label %for.body.ldist1, !dbg !12, !llvm.loop !47

vector.memcheck:                                  ; preds = %for.body.ldist1
  %95 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %96 = shl nuw nsw i64 %95, 6, !dbg !12
  %97 = sub i64 %C38, %D39, !dbg !12
  %diff.check = icmp ult i64 %97, %96, !dbg !12
  %98 = shl nuw nsw i64 %95, 6, !dbg !12
  %99 = sub i64 %C38, %E40, !dbg !12
  %diff.check41 = icmp ult i64 %99, %98, !dbg !12
  %conflict.rdx42 = or i1 %diff.check, %diff.check41, !dbg !12
  br i1 %conflict.rdx42, label %for.body.preheader, label %vector.ph, !dbg !12

vector.ph:                                        ; preds = %vector.memcheck
  %100 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %.neg = mul nuw nsw i64 %100, 16368, !dbg !12
  %n.vec = and i64 %.neg, 10000, !dbg !12
  %101 = tail call i64 @llvm.vscale.i64()
  %102 = shl nuw nsw i64 %101, 2
  %103 = tail call i64 @llvm.vscale.i64()
  %104 = shl nuw nsw i64 %103, 3
  %105 = tail call i64 @llvm.vscale.i64()
  %106 = mul nuw nsw i64 %105, 12
  %107 = tail call i64 @llvm.vscale.i64()
  %108 = shl nuw nsw i64 %107, 2
  %109 = tail call i64 @llvm.vscale.i64()
  %110 = shl nuw nsw i64 %109, 3
  %111 = tail call i64 @llvm.vscale.i64()
  %112 = mul nuw nsw i64 %111, 12
  %113 = tail call i64 @llvm.vscale.i64()
  %114 = shl nuw nsw i64 %113, 2
  %115 = tail call i64 @llvm.vscale.i64()
  %116 = shl nuw nsw i64 %115, 3
  %117 = tail call i64 @llvm.vscale.i64()
  %118 = mul nuw nsw i64 %117, 12
  %119 = tail call i64 @llvm.vscale.i64()
  %120 = shl nuw nsw i64 %119, 4
  br label %vector.body, !dbg !12

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ], !dbg !20
  %121 = getelementptr inbounds float, ptr %D, i64 %index, !dbg !23
  %wide.load = load <vscale x 4 x float>, ptr %121, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %122 = getelementptr inbounds float, ptr %121, i64 %102, !dbg !23
  %wide.load44 = load <vscale x 4 x float>, ptr %122, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %123 = getelementptr inbounds float, ptr %121, i64 %104, !dbg !23
  %wide.load45 = load <vscale x 4 x float>, ptr %123, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %124 = getelementptr inbounds float, ptr %121, i64 %106, !dbg !23
  %wide.load46 = load <vscale x 4 x float>, ptr %124, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %125 = getelementptr inbounds float, ptr %E, i64 %index, !dbg !24
  %wide.load47 = load <vscale x 4 x float>, ptr %125, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %126 = getelementptr inbounds float, ptr %125, i64 %108, !dbg !24
  %wide.load48 = load <vscale x 4 x float>, ptr %126, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %127 = getelementptr inbounds float, ptr %125, i64 %110, !dbg !24
  %wide.load49 = load <vscale x 4 x float>, ptr %127, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %128 = getelementptr inbounds float, ptr %125, i64 %112, !dbg !24
  %wide.load50 = load <vscale x 4 x float>, ptr %128, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %129 = fmul fast <vscale x 4 x float> %wide.load47, %wide.load, !dbg !25
  %130 = fmul fast <vscale x 4 x float> %wide.load48, %wide.load44, !dbg !25
  %131 = fmul fast <vscale x 4 x float> %wide.load49, %wide.load45, !dbg !25
  %132 = fmul fast <vscale x 4 x float> %wide.load50, %wide.load46, !dbg !25
  %133 = getelementptr inbounds float, ptr %C, i64 %index, !dbg !26
  store <vscale x 4 x float> %129, ptr %133, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %134 = getelementptr inbounds float, ptr %133, i64 %114, !dbg !27
  store <vscale x 4 x float> %130, ptr %134, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %135 = getelementptr inbounds float, ptr %133, i64 %116, !dbg !27
  store <vscale x 4 x float> %131, ptr %135, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %136 = getelementptr inbounds float, ptr %133, i64 %118, !dbg !27
  store <vscale x 4 x float> %132, ptr %136, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %index.next = add nuw i64 %index, %120, !dbg !20
  %137 = icmp eq i64 %index.next, %n.vec, !dbg !20
  br i1 %137, label %middle.block, label %vector.body, !dbg !20, !llvm.loop !51

middle.block:                                     ; preds = %vector.body
  %cmp.n = icmp eq i64 %n.vec, 10000, !dbg !12
  br i1 %cmp.n, label %for.end, label %vec.epilog.iter.check, !dbg !12

vec.epilog.iter.check:                            ; preds = %middle.block
  %n.vec.remaining = sub nuw nsw i64 10000, %n.vec, !dbg !12
  %138 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %139 = shl nuw nsw i64 %138, 1, !dbg !12
  %min.epilog.iters.check = icmp ult i64 %n.vec.remaining, %139, !dbg !12
  br i1 %min.epilog.iters.check, label %for.body.preheader, label %vec.epilog.ph, !dbg !12

vec.epilog.ph:                                    ; preds = %vec.epilog.iter.check
  %140 = tail call i64 @llvm.vscale.i64(), !dbg !12
  %.neg66 = mul nuw nsw i64 %140, 16382, !dbg !12
  %n.vec52 = and i64 %.neg66, 10000, !dbg !12
  %141 = tail call i64 @llvm.vscale.i64()
  %142 = shl nuw nsw i64 %141, 1
  br label %vec.epilog.vector.body, !dbg !12

vec.epilog.vector.body:                           ; preds = %vec.epilog.vector.body, %vec.epilog.ph
  %index54 = phi i64 [ %n.vec, %vec.epilog.ph ], [ %index.next57, %vec.epilog.vector.body ], !dbg !20
  %143 = getelementptr inbounds float, ptr %D, i64 %index54, !dbg !23
  %wide.load55 = load <vscale x 2 x float>, ptr %143, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %144 = getelementptr inbounds float, ptr %E, i64 %index54, !dbg !24
  %wide.load56 = load <vscale x 2 x float>, ptr %144, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %145 = fmul fast <vscale x 2 x float> %wide.load56, %wide.load55, !dbg !25
  %146 = getelementptr inbounds float, ptr %C, i64 %index54, !dbg !26
  store <vscale x 2 x float> %145, ptr %146, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %index.next57 = add nuw i64 %index54, %142, !dbg !20
  %147 = icmp eq i64 %index.next57, %n.vec52, !dbg !20
  br i1 %147, label %vec.epilog.middle.block, label %vec.epilog.vector.body, !dbg !20, !llvm.loop !54

vec.epilog.middle.block:                          ; preds = %vec.epilog.vector.body
  %cmp.n53 = icmp eq i64 %n.vec52, 10000, !dbg !12
  br i1 %cmp.n53, label %for.end, label %for.body.preheader, !dbg !12

for.body.preheader:                               ; preds = %vector.memcheck, %vec.epilog.iter.check, %vec.epilog.middle.block
  %indvars.iv.ph = phi i64 [ 0, %vector.memcheck ], [ %n.vec, %vec.epilog.iter.check ], [ %n.vec52, %vec.epilog.middle.block ]
  br label %for.body, !dbg !12

for.body:                                         ; preds = %for.body, %for.body.preheader
  %indvars.iv = phi i64 [ %indvars.iv.ph, %for.body.preheader ], [ %indvars.iv.next.7, %for.body ]
  %indvars.iv.next = or i64 %indvars.iv, 1, !dbg !20
  %arrayidx6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv, !dbg !23
  %148 = load float, ptr %arrayidx6, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8 = getelementptr inbounds float, ptr %E, i64 %indvars.iv, !dbg !24
  %149 = load float, ptr %arrayidx8, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9 = fmul fast float %149, %148, !dbg !25
  %arrayidx11 = getelementptr inbounds float, ptr %C, i64 %indvars.iv, !dbg !26
  store float %mul9, ptr %arrayidx11, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.1 = or i64 %indvars.iv, 2, !dbg !20
  %arrayidx6.1 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next, !dbg !23
  %150 = load float, ptr %arrayidx6.1, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.1 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next, !dbg !24
  %151 = load float, ptr %arrayidx8.1, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.1 = fmul fast float %151, %150, !dbg !25
  %arrayidx11.1 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next, !dbg !26
  store float %mul9.1, ptr %arrayidx11.1, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.2 = or i64 %indvars.iv, 3, !dbg !20
  %arrayidx6.2 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.1, !dbg !23
  %152 = load float, ptr %arrayidx6.2, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.2 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.1, !dbg !24
  %153 = load float, ptr %arrayidx8.2, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.2 = fmul fast float %153, %152, !dbg !25
  %arrayidx11.2 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.1, !dbg !26
  store float %mul9.2, ptr %arrayidx11.2, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.3 = or i64 %indvars.iv, 4, !dbg !20
  %arrayidx6.3 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.2, !dbg !23
  %154 = load float, ptr %arrayidx6.3, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.3 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.2, !dbg !24
  %155 = load float, ptr %arrayidx8.3, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.3 = fmul fast float %155, %154, !dbg !25
  %arrayidx11.3 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.2, !dbg !26
  store float %mul9.3, ptr %arrayidx11.3, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.4 = or i64 %indvars.iv, 5, !dbg !20
  %arrayidx6.4 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.3, !dbg !23
  %156 = load float, ptr %arrayidx6.4, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.4 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.3, !dbg !24
  %157 = load float, ptr %arrayidx8.4, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.4 = fmul fast float %157, %156, !dbg !25
  %arrayidx11.4 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.3, !dbg !26
  store float %mul9.4, ptr %arrayidx11.4, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.5 = or i64 %indvars.iv, 6, !dbg !20
  %arrayidx6.5 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.4, !dbg !23
  %158 = load float, ptr %arrayidx6.5, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.5 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.4, !dbg !24
  %159 = load float, ptr %arrayidx8.5, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.5 = fmul fast float %159, %158, !dbg !25
  %arrayidx11.5 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.4, !dbg !26
  store float %mul9.5, ptr %arrayidx11.5, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.6 = or i64 %indvars.iv, 7, !dbg !20
  %arrayidx6.6 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.5, !dbg !23
  %160 = load float, ptr %arrayidx6.6, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.6 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.5, !dbg !24
  %161 = load float, ptr %arrayidx8.6, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.6 = fmul fast float %161, %160, !dbg !25
  %arrayidx11.6 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.5, !dbg !26
  store float %mul9.6, ptr %arrayidx11.6, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv, 8, !dbg !20
  %arrayidx6.7 = getelementptr inbounds float, ptr %D, i64 %indvars.iv.next.6, !dbg !23
  %162 = load float, ptr %arrayidx6.7, align 4, !dbg !23, !tbaa !14, !alias.scope !48
  %arrayidx8.7 = getelementptr inbounds float, ptr %E, i64 %indvars.iv.next.6, !dbg !24
  %163 = load float, ptr %arrayidx8.7, align 4, !dbg !24, !tbaa !14, !alias.scope !49
  %mul9.7 = fmul fast float %163, %162, !dbg !25
  %arrayidx11.7 = getelementptr inbounds float, ptr %C, i64 %indvars.iv.next.6, !dbg !26
  store float %mul9.7, ptr %arrayidx11.7, align 4, !dbg !27, !tbaa !14, !alias.scope !50, !noalias !38
  %exitcond.not.7 = icmp eq i64 %indvars.iv.next.7, 10000, !dbg !28
  br i1 %exitcond.not.7, label %for.end, label %for.body, !dbg !12, !llvm.loop !56

for.end:                                          ; preds = %for.body, %for.body.lver.orig, %for.body.lver.orig.lver.orig, %middle.block, %vec.epilog.middle.block
  ret void, !dbg !57
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i64 @llvm.vscale.i64() #1

attributes #0 = { mustprogress nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" "unsafe-fp-math"="true" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(none) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6}
!llvm.ident = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 17.0.3 ", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "pragma-loop-pipeline-nodep004.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "d656a4052cb1dd8fe6edc46e4206f7dc")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"uwtable", i32 2}
!6 = !{i32 7, !"frame-pointer", i32 1}
!7 = !{!"clang version 17.0.3 "}
!8 = distinct !DISubprogram(name: "foo", scope: !9, file: !9, line: 19, type: !10, scopeLine: 19, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!9 = !DIFile(filename: "./pragma-loop-pipeline-nodep004.cpp", directory: "/home/xxxx", checksumkind: CSK_MD5, checksum: "d656a4052cb1dd8fe6edc46e4206f7dc")
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 26, column: 5, scope: !8)
!13 = !DILocation(line: 31, column: 20, scope: !8)
!14 = !{!15, !15, i64 0}
!15 = !{!"float", !16, i64 0}
!16 = !{!"omnipotent char", !17, i64 0}
!17 = !{!"Simple C++ TBAA"}
!18 = !DILocation(line: 31, column: 27, scope: !8)
!19 = !DILocation(line: 31, column: 25, scope: !8)
!20 = !DILocation(line: 31, column: 13, scope: !8)
!21 = !DILocation(line: 31, column: 9, scope: !8)
!22 = !DILocation(line: 31, column: 18, scope: !8)
!23 = !DILocation(line: 32, column: 16, scope: !8)
!24 = !DILocation(line: 32, column: 23, scope: !8)
!25 = !DILocation(line: 32, column: 21, scope: !8)
!26 = !DILocation(line: 32, column: 9, scope: !8)
!27 = !DILocation(line: 32, column: 14, scope: !8)
!28 = !DILocation(line: 26, column: 19, scope: !8)
!29 = distinct !{!29, !12, !30, !31, !32}
!30 = !DILocation(line: 33, column: 5, scope: !8)
!31 = !{!"llvm.loop.mustprogress"}
!32 = distinct !{!32, !12, !30, !31, !33, !34, !36, !37}
!33 = !{!"llvm.loop.vectorize.width", i32 1}
!34 = !{!"llvm.loop.vectorize.followup_all", !{!"llvm.loop.pipeline.nodep"}}
!35 = distinct !{!35, !12, !30, !31, !36, !37}
!36 = !{!"llvm.loop.isvectorized"}
!37 = !{!"llvm.loop.pipeline.enable"}
!38 = !{!39}
!39 = distinct !{!39, !40}
!40 = distinct !{!40, !"LVerDomain"}
!41 = !{!42}
!42 = distinct !{!42, !40}
!43 = !{!44, !45, !46}
!44 = distinct !{!44, !40}
!45 = distinct !{!45, !40}
!46 = distinct !{!46, !40}
!47 = distinct !{!47, !32}
!48 = !{!45}
!49 = !{!46}
!50 = !{!44}
!51 = distinct !{!51, !32, !52, !53}
!52 = !{!"llvm.loop.isvectorized", i32 1}
!53 = !{!"llvm.loop.unroll.runtime.disable"}
!54 = distinct !{!54, !32, !52, !53, !55}
!55 = !{!"llvm.remainder.pipeline.disable"}
!56 = distinct !{!56, !32, !55, !52}
!57 = !DILocation(line: 34, column: 1, scope: !8)
