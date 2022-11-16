; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-aarch64tti -debug-hardwareloops --pass-remarks-filter=hardware-loops -o - 2>&1 | FileCheck %s
; CHECK: HardwareLoopInsertion succeeded
; ModuleID = '2901-2-09.c'
source_filename = "2901-2-09.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %32, !dbg !14

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !15
  %6 = zext i32 %0 to i64, !dbg !13
  %7 = add nsw i64 %6, -1, !dbg !14
  %8 = and i64 %6, 7, !dbg !14
  %9 = icmp ult i64 %7, 7, !dbg !14
  br i1 %9, label %12, label %10, !dbg !14

10:                                               ; preds = %4
  %11 = and i64 %6, 4294967288, !dbg !14
  br label %35, !dbg !14

12:                                               ; preds = %35, %4
  %13 = phi double [ undef, %4 ], [ %109, %35 ]
  %14 = phi i64 [ 0, %4 ], [ %110, %35 ]
  %15 = phi double [ 0.000000e+00, %4 ], [ %109, %35 ]
  %16 = icmp eq i64 %8, 0, !dbg !14
  br i1 %16, label %32, label %17, !dbg !14

17:                                               ; preds = %12, %17
  %18 = phi i64 [ %29, %17 ], [ %14, %12 ]
  %19 = phi double [ %28, %17 ], [ %15, %12 ]
  %20 = phi i64 [ %30, %17 ], [ 0, %12 ]
  %21 = getelementptr inbounds i8, ptr %5, i64 %18, !dbg !19
  %22 = load i8, ptr %21, align 1, !dbg !19, !tbaa !20
  %23 = zext i8 %22 to i32, !dbg !19
  %24 = trunc i64 %18 to i32, !dbg !21
  %25 = mul nsw i32 %24, %23, !dbg !21
  %26 = sitofp i32 %25 to double, !dbg !19
  %27 = tail call fast double @llvm.sqrt.f64(double %26), !dbg !22
  %28 = fadd fast double %27, %19, !dbg !23
  %29 = add nuw nsw i64 %18, 1, !dbg !24
  %30 = add i64 %20, 1, !dbg !14
  %31 = icmp eq i64 %30, %8, !dbg !14
  br i1 %31, label %32, label %17, !dbg !14, !llvm.loop !25

32:                                               ; preds = %12, %17, %2
  %33 = phi double [ 0.000000e+00, %2 ], [ %13, %12 ], [ %28, %17 ], !dbg !27
  %34 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, double noundef %33), !dbg !28
  ret i32 0, !dbg !29

35:                                               ; preds = %35, %10
  %36 = phi i64 [ 0, %10 ], [ %110, %35 ]
  %37 = phi double [ 0.000000e+00, %10 ], [ %109, %35 ]
  %38 = phi i64 [ 0, %10 ], [ %111, %35 ]
  %39 = getelementptr inbounds i8, ptr %5, i64 %36, !dbg !19
  %40 = load i8, ptr %39, align 1, !dbg !19, !tbaa !20
  %41 = zext i8 %40 to i32, !dbg !19
  %42 = trunc i64 %36 to i32, !dbg !21
  %43 = mul nsw i32 %42, %41, !dbg !21
  %44 = sitofp i32 %43 to double, !dbg !19
  %45 = tail call fast double @llvm.sqrt.f64(double %44), !dbg !22
  %46 = fadd fast double %45, %37, !dbg !23
  %47 = or i64 %36, 1, !dbg !24
  %48 = getelementptr inbounds i8, ptr %5, i64 %47, !dbg !19
  %49 = load i8, ptr %48, align 1, !dbg !19, !tbaa !20
  %50 = zext i8 %49 to i32, !dbg !19
  %51 = trunc i64 %47 to i32, !dbg !21
  %52 = mul nsw i32 %51, %50, !dbg !21
  %53 = sitofp i32 %52 to double, !dbg !19
  %54 = tail call fast double @llvm.sqrt.f64(double %53), !dbg !22
  %55 = fadd fast double %54, %46, !dbg !23
  %56 = or i64 %36, 2, !dbg !24
  %57 = getelementptr inbounds i8, ptr %5, i64 %56, !dbg !19
  %58 = load i8, ptr %57, align 1, !dbg !19, !tbaa !20
  %59 = zext i8 %58 to i32, !dbg !19
  %60 = trunc i64 %56 to i32, !dbg !21
  %61 = mul nsw i32 %60, %59, !dbg !21
  %62 = sitofp i32 %61 to double, !dbg !19
  %63 = tail call fast double @llvm.sqrt.f64(double %62), !dbg !22
  %64 = fadd fast double %63, %55, !dbg !23
  %65 = or i64 %36, 3, !dbg !24
  %66 = getelementptr inbounds i8, ptr %5, i64 %65, !dbg !19
  %67 = load i8, ptr %66, align 1, !dbg !19, !tbaa !20
  %68 = zext i8 %67 to i32, !dbg !19
  %69 = trunc i64 %65 to i32, !dbg !21
  %70 = mul nsw i32 %69, %68, !dbg !21
  %71 = sitofp i32 %70 to double, !dbg !19
  %72 = tail call fast double @llvm.sqrt.f64(double %71), !dbg !22
  %73 = fadd fast double %72, %64, !dbg !23
  %74 = or i64 %36, 4, !dbg !24
  %75 = getelementptr inbounds i8, ptr %5, i64 %74, !dbg !19
  %76 = load i8, ptr %75, align 1, !dbg !19, !tbaa !20
  %77 = zext i8 %76 to i32, !dbg !19
  %78 = trunc i64 %74 to i32, !dbg !21
  %79 = mul nsw i32 %78, %77, !dbg !21
  %80 = sitofp i32 %79 to double, !dbg !19
  %81 = tail call fast double @llvm.sqrt.f64(double %80), !dbg !22
  %82 = fadd fast double %81, %73, !dbg !23
  %83 = or i64 %36, 5, !dbg !24
  %84 = getelementptr inbounds i8, ptr %5, i64 %83, !dbg !19
  %85 = load i8, ptr %84, align 1, !dbg !19, !tbaa !20
  %86 = zext i8 %85 to i32, !dbg !19
  %87 = trunc i64 %83 to i32, !dbg !21
  %88 = mul nsw i32 %87, %86, !dbg !21
  %89 = sitofp i32 %88 to double, !dbg !19
  %90 = tail call fast double @llvm.sqrt.f64(double %89), !dbg !22
  %91 = fadd fast double %90, %82, !dbg !23
  %92 = or i64 %36, 6, !dbg !24
  %93 = getelementptr inbounds i8, ptr %5, i64 %92, !dbg !19
  %94 = load i8, ptr %93, align 1, !dbg !19, !tbaa !20
  %95 = zext i8 %94 to i32, !dbg !19
  %96 = trunc i64 %92 to i32, !dbg !21
  %97 = mul nsw i32 %96, %95, !dbg !21
  %98 = sitofp i32 %97 to double, !dbg !19
  %99 = tail call fast double @llvm.sqrt.f64(double %98), !dbg !22
  %100 = fadd fast double %99, %91, !dbg !23
  %101 = or i64 %36, 7, !dbg !24
  %102 = getelementptr inbounds i8, ptr %5, i64 %101, !dbg !19
  %103 = load i8, ptr %102, align 1, !dbg !19, !tbaa !20
  %104 = zext i8 %103 to i32, !dbg !19
  %105 = trunc i64 %101 to i32, !dbg !21
  %106 = mul nsw i32 %105, %104, !dbg !21
  %107 = sitofp i32 %106 to double, !dbg !19
  %108 = tail call fast double @llvm.sqrt.f64(double %107), !dbg !22
  %109 = fadd fast double %108, %100, !dbg !23
  %110 = add nuw nsw i64 %36, 8, !dbg !24
  %111 = add i64 %38, 8, !dbg !14
  %112 = icmp eq i64 %111, %11, !dbg !14
  br i1 %112, label %12, label %35, !dbg !14, !llvm.loop !30
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind readnone speculatable willreturn
declare double @llvm.sqrt.f64(double) #1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nofree nounwind "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-09.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "6e26e22c62ba43fdc5ee914384edaee1")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 5, type: !11, scopeLine: 5, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 8, column: 17, scope: !10)
!14 = !DILocation(line: 8, column: 2, scope: !10)
!15 = !{!16, !16, i64 0}
!16 = !{!"any pointer", !17, i64 0}
!17 = !{!"omnipotent char", !18, i64 0}
!18 = !{!"Simple C/C++ TBAA"}
!19 = !DILocation(line: 9, column: 13, scope: !10)
!20 = !{!17, !17, i64 0}
!21 = !DILocation(line: 9, column: 23, scope: !10)
!22 = !DILocation(line: 9, column: 8, scope: !10)
!23 = !DILocation(line: 9, column: 6, scope: !10)
!24 = !DILocation(line: 8, column: 25, scope: !10)
!25 = distinct !{!25, !26}
!26 = !{!"llvm.loop.unroll.disable"}
!27 = !DILocation(line: 0, scope: !10)
!28 = !DILocation(line: 11, column: 2, scope: !10)
!29 = !DILocation(line: 12, column: 2, scope: !10)
!30 = distinct !{!30, !14, !31, !32}
!31 = !DILocation(line: 10, column: 2, scope: !10)
!32 = !{!"llvm.loop.mustprogress"}
