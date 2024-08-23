; RUN: llc < %s -mcpu=a64fx -O1 -fswp --pass-remarks-output=- --pass-remarks-filter=hardware-loops -o /dev/null | FileCheck %s
; CHECK: hardware-loop created
; ModuleID = '2901-2-09.c'
source_filename = "2901-2-09.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %32

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !6
  %6 = zext i32 %0 to i64
  %7 = add nsw i64 %6, -1
  %8 = and i64 %6, 7
  %9 = icmp ult i64 %7, 7
  br i1 %9, label %12, label %10

10:                                               ; preds = %4
  %11 = and i64 %6, 4294967288
  br label %35

12:                                               ; preds = %35, %4
  %13 = phi double [ undef, %4 ], [ %109, %35 ]
  %14 = phi i64 [ 0, %4 ], [ %110, %35 ]
  %15 = phi double [ 0.000000e+00, %4 ], [ %109, %35 ]
  %16 = icmp eq i64 %8, 0
  br i1 %16, label %32, label %17

17:                                               ; preds = %12, %17
  %18 = phi i64 [ %29, %17 ], [ %14, %12 ]
  %19 = phi double [ %28, %17 ], [ %15, %12 ]
  %20 = phi i64 [ %30, %17 ], [ 0, %12 ]
  %21 = getelementptr inbounds i8, ptr %5, i64 %18
  %22 = load i8, ptr %21, align 1, !tbaa !10
  %23 = zext i8 %22 to i32
  %24 = trunc i64 %18 to i32
  %25 = mul nsw i32 %24, %23
  %26 = sitofp i32 %25 to double
  %27 = tail call fast double @llvm.sqrt.f64(double %26)
  %28 = fadd fast double %27, %19
  %29 = add nuw nsw i64 %18, 1
  %30 = add i64 %20, 1
  %31 = icmp eq i64 %30, %8
  br i1 %31, label %32, label %17, !llvm.loop !11

32:                                               ; preds = %12, %17, %2
  %33 = phi double [ 0.000000e+00, %2 ], [ %13, %12 ], [ %28, %17 ]
  %34 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, double noundef %33)
  ret i32 0

35:                                               ; preds = %35, %10
  %36 = phi i64 [ 0, %10 ], [ %110, %35 ]
  %37 = phi double [ 0.000000e+00, %10 ], [ %109, %35 ]
  %38 = phi i64 [ 0, %10 ], [ %111, %35 ]
  %39 = getelementptr inbounds i8, ptr %5, i64 %36
  %40 = load i8, ptr %39, align 1, !tbaa !10
  %41 = zext i8 %40 to i32
  %42 = trunc i64 %36 to i32
  %43 = mul nsw i32 %42, %41
  %44 = sitofp i32 %43 to double
  %45 = tail call fast double @llvm.sqrt.f64(double %44)
  %46 = fadd fast double %45, %37
  %47 = or i64 %36, 1
  %48 = getelementptr inbounds i8, ptr %5, i64 %47
  %49 = load i8, ptr %48, align 1, !tbaa !10
  %50 = zext i8 %49 to i32
  %51 = trunc i64 %47 to i32
  %52 = mul nsw i32 %51, %50
  %53 = sitofp i32 %52 to double
  %54 = tail call fast double @llvm.sqrt.f64(double %53)
  %55 = fadd fast double %54, %46
  %56 = or i64 %36, 2
  %57 = getelementptr inbounds i8, ptr %5, i64 %56
  %58 = load i8, ptr %57, align 1, !tbaa !10
  %59 = zext i8 %58 to i32
  %60 = trunc i64 %56 to i32
  %61 = mul nsw i32 %60, %59
  %62 = sitofp i32 %61 to double
  %63 = tail call fast double @llvm.sqrt.f64(double %62)
  %64 = fadd fast double %63, %55
  %65 = or i64 %36, 3
  %66 = getelementptr inbounds i8, ptr %5, i64 %65
  %67 = load i8, ptr %66, align 1, !tbaa !10
  %68 = zext i8 %67 to i32
  %69 = trunc i64 %65 to i32
  %70 = mul nsw i32 %69, %68
  %71 = sitofp i32 %70 to double
  %72 = tail call fast double @llvm.sqrt.f64(double %71)
  %73 = fadd fast double %72, %64
  %74 = or i64 %36, 4
  %75 = getelementptr inbounds i8, ptr %5, i64 %74
  %76 = load i8, ptr %75, align 1, !tbaa !10
  %77 = zext i8 %76 to i32
  %78 = trunc i64 %74 to i32
  %79 = mul nsw i32 %78, %77
  %80 = sitofp i32 %79 to double
  %81 = tail call fast double @llvm.sqrt.f64(double %80)
  %82 = fadd fast double %81, %73
  %83 = or i64 %36, 5
  %84 = getelementptr inbounds i8, ptr %5, i64 %83
  %85 = load i8, ptr %84, align 1, !tbaa !10
  %86 = zext i8 %85 to i32
  %87 = trunc i64 %83 to i32
  %88 = mul nsw i32 %87, %86
  %89 = sitofp i32 %88 to double
  %90 = tail call fast double @llvm.sqrt.f64(double %89)
  %91 = fadd fast double %90, %82
  %92 = or i64 %36, 6
  %93 = getelementptr inbounds i8, ptr %5, i64 %92
  %94 = load i8, ptr %93, align 1, !tbaa !10
  %95 = zext i8 %94 to i32
  %96 = trunc i64 %92 to i32
  %97 = mul nsw i32 %96, %95
  %98 = sitofp i32 %97 to double
  %99 = tail call fast double @llvm.sqrt.f64(double %98)
  %100 = fadd fast double %99, %91
  %101 = or i64 %36, 7
  %102 = getelementptr inbounds i8, ptr %5, i64 %101
  %103 = load i8, ptr %102, align 1, !tbaa !10
  %104 = zext i8 %103 to i32
  %105 = trunc i64 %101 to i32
  %106 = mul nsw i32 %105, %104
  %107 = sitofp i32 %106 to double
  %108 = tail call fast double @llvm.sqrt.f64(double %107)
  %109 = fadd fast double %108, %100
  %110 = add nuw nsw i64 %36, 8
  %111 = add i64 %38, 8
  %112 = icmp eq i64 %111, %11
  br i1 %112, label %12, label %35, !llvm.loop !13
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind readnone speculatable willreturn
declare double @llvm.sqrt.f64(double) #1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #2

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { nofree nounwind "approx-func-fp-math"="true" "frame-pointer"="non-leaf" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" "unsafe-fp-math"="true" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!7, !7, i64 0}
!7 = !{!"any pointer", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!8, !8, i64 0}
!11 = distinct !{!11, !12}
!12 = !{!"llvm.loop.unroll.disable"}
!13 = distinct !{!13, !14}
!14 = !{!"llvm.loop.mustprogress"}
