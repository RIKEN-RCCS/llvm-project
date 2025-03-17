; RUN: llc -O1 -mcpu=a64fx -pass-remarks=hardware -fswp < %s -o /dev/null |& FileCheck %s
; CHECK: remark: target.c:9:3: distributed loop: 3 of 3 hardware-loop created
; CHECK: remark: target.c:9:3: distributed loop: 1 of 3 hardware-loop created
; CHECK: remark: target.c:9:3: distributed loop: 2 of 3 hardware-loop created
; ModuleID = 'target.ll'
source_filename = "target.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

@aa1 = external local_unnamed_addr global [1000 x float], align 4
@cc1 = external local_unnamed_addr global [1000 x float], align 4
@bb1 = external local_unnamed_addr global [1000 x float], align 4
@dd1_org = external local_unnamed_addr global [1000 x float], align 4
@aa2 = external local_unnamed_addr global [1000 x float], align 4
@cc2 = external local_unnamed_addr global [1000 x float], align 4
@bb2 = external local_unnamed_addr global [1000 x float], align 4
@dd2_org = external local_unnamed_addr global [1000 x float], align 4
@aa3 = external local_unnamed_addr global [1000 x float], align 4
@cc3 = external local_unnamed_addr global [1000 x float], align 4
@bb3 = external local_unnamed_addr global [1000 x float], align 4
@dd3_org = external local_unnamed_addr global [1000 x float], align 4

; Function Attrs: nofree noinline norecurse nosync nounwind memory(readwrite, argmem: none, inaccessiblemem: none) uwtable vscale_range(4,4)
define dso_local void @target(i32 noundef %0) local_unnamed_addr #0 !dbg !10 {
  %2 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %2, label %.lr.ph.preheader, label %._crit_edge, !dbg !14

.lr.ph.preheader:                                 ; preds = %1
  %wide.trip.count = zext nneg i32 %0 to i64, !dbg !13
  br label %.lr.ph.ldist1, !dbg !14

.lr.ph.ldist1:                                    ; preds = %.lr.ph.ldist1, %.lr.ph.preheader
  %indvars.iv.ldist1 = phi i64 [ 0, %.lr.ph.preheader ], [ %indvars.iv.next.ldist1, %.lr.ph.ldist1 ]
  %3 = getelementptr inbounds [1000 x float], ptr @aa1, i64 0, i64 %indvars.iv.ldist1, !dbg !15
  %4 = load float, ptr %3, align 4, !dbg !15
  %5 = getelementptr inbounds [1000 x float], ptr @cc1, i64 0, i64 %indvars.iv.ldist1, !dbg !16
  %6 = load float, ptr %5, align 4, !dbg !16
  %7 = getelementptr inbounds [1000 x float], ptr @bb1, i64 0, i64 %indvars.iv.ldist1, !dbg !17
  %8 = load float, ptr %7, align 4, !dbg !17
  %9 = tail call float @llvm.fmuladd.f32(float %6, float %8, float %4), !dbg !18
  %10 = getelementptr inbounds [1000 x float], ptr @dd1_org, i64 0, i64 %indvars.iv.ldist1, !dbg !19
  store float %9, ptr %10, align 4, !dbg !20
  %indvars.iv.next.ldist1 = add nuw nsw i64 %indvars.iv.ldist1, 1, !dbg !21
  %exitcond.not.ldist1 = icmp eq i64 %indvars.iv.next.ldist1, %wide.trip.count, !dbg !13
  br i1 %exitcond.not.ldist1, label %.lr.ph.ldist2, label %.lr.ph.ldist1, !dbg !14, !llvm.loop !22

.lr.ph.ldist2:                                    ; preds = %.lr.ph.ldist1, %.lr.ph.ldist2
  %indvars.iv.ldist2 = phi i64 [ %indvars.iv.next.ldist2, %.lr.ph.ldist2 ], [ 0, %.lr.ph.ldist1 ]
  %11 = getelementptr inbounds [1000 x float], ptr @aa2, i64 0, i64 %indvars.iv.ldist2, !dbg !26
  %12 = load float, ptr %11, align 4, !dbg !26
  %13 = getelementptr inbounds [1000 x float], ptr @cc2, i64 0, i64 %indvars.iv.ldist2, !dbg !27
  %14 = load float, ptr %13, align 4, !dbg !27
  %15 = getelementptr inbounds [1000 x float], ptr @bb2, i64 0, i64 %indvars.iv.ldist2, !dbg !28
  %16 = load float, ptr %15, align 4, !dbg !28
  %17 = tail call float @llvm.fmuladd.f32(float %14, float %16, float %12), !dbg !29
  %18 = getelementptr inbounds [1000 x float], ptr @dd2_org, i64 0, i64 %indvars.iv.ldist2, !dbg !30
  store float %17, ptr %18, align 4, !dbg !31
  %indvars.iv.next.ldist2 = add nuw nsw i64 %indvars.iv.ldist2, 1, !dbg !21
  %exitcond.not.ldist2 = icmp eq i64 %indvars.iv.next.ldist2, %wide.trip.count, !dbg !13
  br i1 %exitcond.not.ldist2, label %.lr.ph, label %.lr.ph.ldist2, !dbg !14, !llvm.loop !32

.lr.ph:                                           ; preds = %.lr.ph.ldist2, %.lr.ph
  %indvars.iv = phi i64 [ %indvars.iv.next, %.lr.ph ], [ 0, %.lr.ph.ldist2 ]
  %19 = getelementptr inbounds [1000 x float], ptr @aa3, i64 0, i64 %indvars.iv, !dbg !34
  %20 = load float, ptr %19, align 4, !dbg !34
  %21 = getelementptr inbounds [1000 x float], ptr @cc3, i64 0, i64 %indvars.iv, !dbg !35
  %22 = load float, ptr %21, align 4, !dbg !35
  %23 = fadd float %20, %22, !dbg !36
  %24 = getelementptr inbounds [1000 x float], ptr @bb3, i64 0, i64 %indvars.iv, !dbg !37
  %25 = load float, ptr %24, align 4, !dbg !37
  %26 = fadd float %23, %25, !dbg !38
  %27 = fmul float %20, %22, !dbg !39
  %28 = tail call float @llvm.fmuladd.f32(float %27, float %25, float %26), !dbg !40
  %29 = getelementptr inbounds [1000 x float], ptr @dd3_org, i64 0, i64 %indvars.iv, !dbg !41
  store float %28, ptr %29, align 4, !dbg !42
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1, !dbg !21
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count, !dbg !13
  br i1 %exitcond.not, label %._crit_edge, label %.lr.ph, !dbg !14, !llvm.loop !43

._crit_edge:                                      ; preds = %.lr.ph, %1
  ret void, !dbg !45
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

attributes #0 = { nofree noinline norecurse nosync nounwind memory(readwrite, argmem: none, inaccessiblemem: none) uwtable vscale_range(4,4) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind speculatable willreturn memory(none) "target-cpu"="a64fx" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}
!llvm.ident = !{!9}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 18.1.8", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "target.c", directory: "/home/a", checksumkind: CSK_MD5, checksum: "015d1f2ce545127a1fca82ad5719d0ec")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 8, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!9 = !{!"clang version 18.1.8"}
!10 = distinct !DISubprogram(name: "target", scope: !1, file: !1, line: 7, type: !11, scopeLine: 8, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 9, column: 21, scope: !10)
!14 = !DILocation(line: 9, column: 3, scope: !10)
!15 = !DILocation(line: 11, column: 18, scope: !10)
!16 = !DILocation(line: 11, column: 27, scope: !10)
!17 = !DILocation(line: 11, column: 36, scope: !10)
!18 = !DILocation(line: 11, column: 25, scope: !10)
!19 = !DILocation(line: 11, column: 5, scope: !10)
!20 = !DILocation(line: 11, column: 16, scope: !10)
!21 = !DILocation(line: 9, column: 27, scope: !10)
!22 = distinct !{!22, !14, !23, !24, !25}
!23 = !DILocation(line: 15, column: 3, scope: !10)
!24 = !{!"llvm.loop.mustprogress"}
!25 = !{!"llvm.loop.distributed4swpl", i32 3, i32 2}
!26 = !DILocation(line: 12, column: 18, scope: !10)
!27 = !DILocation(line: 12, column: 27, scope: !10)
!28 = !DILocation(line: 12, column: 36, scope: !10)
!29 = !DILocation(line: 12, column: 25, scope: !10)
!30 = !DILocation(line: 12, column: 5, scope: !10)
!31 = !DILocation(line: 12, column: 16, scope: !10)
!32 = distinct !{!32, !14, !23, !24, !33}
!33 = !{!"llvm.loop.distributed4swpl", i32 3, i32 1}
!34 = !DILocation(line: 13, column: 15, scope: !10)
!35 = !DILocation(line: 13, column: 24, scope: !10)
!36 = !DILocation(line: 13, column: 22, scope: !10)
!37 = !DILocation(line: 13, column: 33, scope: !10)
!38 = !DILocation(line: 13, column: 31, scope: !10)
!39 = !DILocation(line: 14, column: 25, scope: !10)
!40 = !DILocation(line: 14, column: 43, scope: !10)
!41 = !DILocation(line: 14, column: 5, scope: !10)
!42 = !DILocation(line: 14, column: 16, scope: !10)
!43 = distinct !{!43, !14, !23, !24, !44}
!44 = !{!"llvm.loop.distributed4swpl", i32 3, i32 3}
!45 = !DILocation(line: 16, column: 1, scope: !10)
