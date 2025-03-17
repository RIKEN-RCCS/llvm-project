; RUN: opt -O1 -S < %s |& FileCheck %s

; CHECK:   br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop ![[LOOP1:.*]]
; CHECK: ![[LOOP1]] = distinct !{![[LOOP1]], [[MP:![0-9]+]], [[UR:![0-9]+]], [[D4SWPL:![0-9]+]]}
; CHECK-NEXT: [[MP]] = !{!"llvm.loop.mustprogress"}
; CHECK-NEXT: [[UR]] = !{!"llvm.loop.unroll.disable"}
; CHECK-NEXT: [[D4SWPL]] = !{!"llvm.loop.distribute4swpl.enable", i1 false}

; ModuleID = './target.c'
source_filename = "./target.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @target(i32 noundef %n, ptr nocapture noundef writeonly %dd1, ptr nocapture noundef %dd2, ptr nocapture noundef %aa1, ptr nocapture noundef readnone %aa2, ptr nocapture noundef readonly %bb1, ptr nocapture noundef readnone %bb2, ptr nocapture noundef readonly %cc1, ptr nocapture noundef readonly %cc2) local_unnamed_addr #0 {
entry:
  %cmp23 = icmp sgt i32 %n, 2
  br i1 %cmp23, label %for.body.lr.ph, label %for.cond.cleanup

for.body.lr.ph:                                   ; preds = %entry
  %sub = add nsw i32 %n, -2
  %arrayidx10 = getelementptr inbounds float, ptr %dd2, i64 1
  %wide.trip.count = zext i32 %sub to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  ret void

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %arrayidx = getelementptr inbounds float, ptr %aa1, i64 %indvars.iv
  %0 = load float, ptr %arrayidx, align 4, !tbaa !5
  %arrayidx2 = getelementptr inbounds float, ptr %bb1, i64 %indvars.iv
  %1 = load float, ptr %arrayidx2, align 4, !tbaa !5
  %add = fadd float %0, %1
  %arrayidx4 = getelementptr inbounds float, ptr %dd1, i64 %indvars.iv
  store float %add, ptr %arrayidx4, align 4, !tbaa !5
  %arrayidx6 = getelementptr inbounds float, ptr %cc1, i64 %indvars.iv
  %2 = load float, ptr %arrayidx6, align 4, !tbaa !5
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %arrayidx9 = getelementptr inbounds float, ptr %aa1, i64 %indvars.iv.next
  store float %2, ptr %arrayidx9, align 4, !tbaa !5
  %3 = load float, ptr %arrayidx10, align 4, !tbaa !5
  %4 = trunc i64 %indvars.iv to i32
  %rem = urem i32 %4, 10
  %idxprom11 = zext nneg i32 %rem to i64
  %arrayidx12 = getelementptr inbounds float, ptr %cc2, i64 %idxprom11
  %5 = load float, ptr %arrayidx12, align 4, !tbaa !5
  %mul = fmul float %3, %5
  store float %mul, ptr %dd2, align 4, !tbaa !5
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop !9
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 18.1.8"}
!5 = !{!6, !6, i64 0}
!6 = !{!"float", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10, !11, !12}
!10 = !{!"llvm.loop.mustprogress"}
!11 = !{!"llvm.loop.unroll.disable"}
!12 = !{!"llvm.loop.distribute4swpl.enable", i1 false}
