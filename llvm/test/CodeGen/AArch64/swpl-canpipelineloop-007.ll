; RUN: llc < %s -O1 -fswp  -swpl-max-inst-num=100  -mcpu=a64fx  -swpl-debug --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed= --pass-remarks-output=- -o /dev/null 2>&1 | FileCheck %s
;CHECK:over inst limit num
;CHECK:canPipelineLoop:NG
;CHECK:This loop is not software pipelined because the loop contains too many instructions
; ModuleID = '2912_inst_l.c'
source_filename = "2912_inst_l.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@b1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@b9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@c9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@a9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ba = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ca = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@aa = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@bb = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@cb = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ab = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@bc = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@cc = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ac = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@bd = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@cd = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ad = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@be = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ce = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ae = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x1 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x2 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x3 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x4 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x5 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x6 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x7 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x8 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@y9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@z9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@x9 = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@ya = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@za = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8
@xa = dso_local local_unnamed_addr global [100 x double] zeroinitializer, align 8

; Function Attrs: nofree norecurse nosync nounwind uwtable vscale_range(1,16)
define dso_local void @test(i32 noundef %0) local_unnamed_addr #0 {
  %2 = icmp sgt i32 %0, 0
  br i1 %2, label %3, label %5

3:                                                ; preds = %1
  %4 = zext i32 %0 to i64
  br label %6

5:                                                ; preds = %6, %1
  ret void

6:                                                ; preds = %3, %6
  %7 = phi i64 [ 0, %3 ], [ %152, %6 ]
  %8 = getelementptr inbounds [100 x double], ptr @b1, i64 0, i64 %7
  %9 = load double, ptr %8, align 8, !tbaa !6
  %10 = getelementptr inbounds [100 x double], ptr @c1, i64 0, i64 %7
  %11 = load double, ptr %10, align 8, !tbaa !6
  %12 = fadd double %9, %11
  %13 = getelementptr inbounds [100 x double], ptr @a1, i64 0, i64 %7
  store double %12, ptr %13, align 8, !tbaa !6
  %14 = getelementptr inbounds [100 x double], ptr @b2, i64 0, i64 %7
  %15 = load double, ptr %14, align 8, !tbaa !6
  %16 = getelementptr inbounds [100 x double], ptr @c2, i64 0, i64 %7
  %17 = load double, ptr %16, align 8, !tbaa !6
  %18 = fadd double %15, %17
  %19 = getelementptr inbounds [100 x double], ptr @a2, i64 0, i64 %7
  store double %18, ptr %19, align 8, !tbaa !6
  %20 = getelementptr inbounds [100 x double], ptr @b3, i64 0, i64 %7
  %21 = load double, ptr %20, align 8, !tbaa !6
  %22 = getelementptr inbounds [100 x double], ptr @c3, i64 0, i64 %7
  %23 = load double, ptr %22, align 8, !tbaa !6
  %24 = fadd double %21, %23
  %25 = getelementptr inbounds [100 x double], ptr @a3, i64 0, i64 %7
  store double %24, ptr %25, align 8, !tbaa !6
  %26 = getelementptr inbounds [100 x double], ptr @b4, i64 0, i64 %7
  %27 = load double, ptr %26, align 8, !tbaa !6
  %28 = getelementptr inbounds [100 x double], ptr @c4, i64 0, i64 %7
  %29 = load double, ptr %28, align 8, !tbaa !6
  %30 = fadd double %27, %29
  %31 = getelementptr inbounds [100 x double], ptr @a4, i64 0, i64 %7
  store double %30, ptr %31, align 8, !tbaa !6
  %32 = getelementptr inbounds [100 x double], ptr @b5, i64 0, i64 %7
  %33 = load double, ptr %32, align 8, !tbaa !6
  %34 = getelementptr inbounds [100 x double], ptr @c5, i64 0, i64 %7
  %35 = load double, ptr %34, align 8, !tbaa !6
  %36 = fadd double %33, %35
  %37 = getelementptr inbounds [100 x double], ptr @a5, i64 0, i64 %7
  store double %36, ptr %37, align 8, !tbaa !6
  %38 = getelementptr inbounds [100 x double], ptr @b6, i64 0, i64 %7
  %39 = load double, ptr %38, align 8, !tbaa !6
  %40 = getelementptr inbounds [100 x double], ptr @c6, i64 0, i64 %7
  %41 = load double, ptr %40, align 8, !tbaa !6
  %42 = fadd double %39, %41
  %43 = getelementptr inbounds [100 x double], ptr @a6, i64 0, i64 %7
  store double %42, ptr %43, align 8, !tbaa !6
  %44 = getelementptr inbounds [100 x double], ptr @b7, i64 0, i64 %7
  %45 = load double, ptr %44, align 8, !tbaa !6
  %46 = getelementptr inbounds [100 x double], ptr @c7, i64 0, i64 %7
  %47 = load double, ptr %46, align 8, !tbaa !6
  %48 = fadd double %45, %47
  %49 = getelementptr inbounds [100 x double], ptr @a7, i64 0, i64 %7
  store double %48, ptr %49, align 8, !tbaa !6
  %50 = getelementptr inbounds [100 x double], ptr @b8, i64 0, i64 %7
  %51 = load double, ptr %50, align 8, !tbaa !6
  %52 = getelementptr inbounds [100 x double], ptr @c8, i64 0, i64 %7
  %53 = load double, ptr %52, align 8, !tbaa !6
  %54 = fadd double %51, %53
  %55 = getelementptr inbounds [100 x double], ptr @a8, i64 0, i64 %7
  store double %54, ptr %55, align 8, !tbaa !6
  %56 = getelementptr inbounds [100 x double], ptr @b9, i64 0, i64 %7
  %57 = load double, ptr %56, align 8, !tbaa !6
  %58 = getelementptr inbounds [100 x double], ptr @c9, i64 0, i64 %7
  %59 = load double, ptr %58, align 8, !tbaa !6
  %60 = fadd double %57, %59
  %61 = getelementptr inbounds [100 x double], ptr @a9, i64 0, i64 %7
  store double %60, ptr %61, align 8, !tbaa !6
  %62 = getelementptr inbounds [100 x double], ptr @ba, i64 0, i64 %7
  %63 = load double, ptr %62, align 8, !tbaa !6
  %64 = getelementptr inbounds [100 x double], ptr @ca, i64 0, i64 %7
  %65 = load double, ptr %64, align 8, !tbaa !6
  %66 = fadd double %63, %65
  %67 = getelementptr inbounds [100 x double], ptr @aa, i64 0, i64 %7
  store double %66, ptr %67, align 8, !tbaa !6
  %68 = getelementptr inbounds [100 x double], ptr @bb, i64 0, i64 %7
  %69 = load double, ptr %68, align 8, !tbaa !6
  %70 = getelementptr inbounds [100 x double], ptr @cb, i64 0, i64 %7
  %71 = load double, ptr %70, align 8, !tbaa !6
  %72 = fadd double %69, %71
  %73 = getelementptr inbounds [100 x double], ptr @ab, i64 0, i64 %7
  store double %72, ptr %73, align 8, !tbaa !6
  %74 = getelementptr inbounds [100 x double], ptr @bc, i64 0, i64 %7
  %75 = load double, ptr %74, align 8, !tbaa !6
  %76 = getelementptr inbounds [100 x double], ptr @cc, i64 0, i64 %7
  %77 = load double, ptr %76, align 8, !tbaa !6
  %78 = fadd double %75, %77
  %79 = getelementptr inbounds [100 x double], ptr @ac, i64 0, i64 %7
  store double %78, ptr %79, align 8, !tbaa !6
  %80 = getelementptr inbounds [100 x double], ptr @bd, i64 0, i64 %7
  %81 = load double, ptr %80, align 8, !tbaa !6
  %82 = getelementptr inbounds [100 x double], ptr @cd, i64 0, i64 %7
  %83 = load double, ptr %82, align 8, !tbaa !6
  %84 = fadd double %81, %83
  %85 = getelementptr inbounds [100 x double], ptr @ad, i64 0, i64 %7
  store double %84, ptr %85, align 8, !tbaa !6
  %86 = getelementptr inbounds [100 x double], ptr @be, i64 0, i64 %7
  %87 = load double, ptr %86, align 8, !tbaa !6
  %88 = getelementptr inbounds [100 x double], ptr @ce, i64 0, i64 %7
  %89 = load double, ptr %88, align 8, !tbaa !6
  %90 = fadd double %87, %89
  %91 = getelementptr inbounds [100 x double], ptr @ae, i64 0, i64 %7
  store double %90, ptr %91, align 8, !tbaa !6
  %92 = getelementptr inbounds [100 x double], ptr @y1, i64 0, i64 %7
  %93 = load double, ptr %92, align 8, !tbaa !6
  %94 = getelementptr inbounds [100 x double], ptr @z1, i64 0, i64 %7
  %95 = load double, ptr %94, align 8, !tbaa !6
  %96 = fadd double %93, %95
  %97 = getelementptr inbounds [100 x double], ptr @x1, i64 0, i64 %7
  store double %96, ptr %97, align 8, !tbaa !6
  %98 = getelementptr inbounds [100 x double], ptr @y2, i64 0, i64 %7
  %99 = load double, ptr %98, align 8, !tbaa !6
  %100 = getelementptr inbounds [100 x double], ptr @z2, i64 0, i64 %7
  %101 = load double, ptr %100, align 8, !tbaa !6
  %102 = fadd double %99, %101
  %103 = getelementptr inbounds [100 x double], ptr @x2, i64 0, i64 %7
  store double %102, ptr %103, align 8, !tbaa !6
  %104 = getelementptr inbounds [100 x double], ptr @y3, i64 0, i64 %7
  %105 = load double, ptr %104, align 8, !tbaa !6
  %106 = getelementptr inbounds [100 x double], ptr @z3, i64 0, i64 %7
  %107 = load double, ptr %106, align 8, !tbaa !6
  %108 = fadd double %105, %107
  %109 = getelementptr inbounds [100 x double], ptr @x3, i64 0, i64 %7
  store double %108, ptr %109, align 8, !tbaa !6
  %110 = getelementptr inbounds [100 x double], ptr @y4, i64 0, i64 %7
  %111 = load double, ptr %110, align 8, !tbaa !6
  %112 = getelementptr inbounds [100 x double], ptr @z4, i64 0, i64 %7
  %113 = load double, ptr %112, align 8, !tbaa !6
  %114 = fadd double %111, %113
  %115 = getelementptr inbounds [100 x double], ptr @x4, i64 0, i64 %7
  store double %114, ptr %115, align 8, !tbaa !6
  %116 = getelementptr inbounds [100 x double], ptr @y5, i64 0, i64 %7
  %117 = load double, ptr %116, align 8, !tbaa !6
  %118 = getelementptr inbounds [100 x double], ptr @z5, i64 0, i64 %7
  %119 = load double, ptr %118, align 8, !tbaa !6
  %120 = fadd double %117, %119
  %121 = getelementptr inbounds [100 x double], ptr @x5, i64 0, i64 %7
  store double %120, ptr %121, align 8, !tbaa !6
  %122 = getelementptr inbounds [100 x double], ptr @y6, i64 0, i64 %7
  %123 = load double, ptr %122, align 8, !tbaa !6
  %124 = getelementptr inbounds [100 x double], ptr @z6, i64 0, i64 %7
  %125 = load double, ptr %124, align 8, !tbaa !6
  %126 = fadd double %123, %125
  %127 = getelementptr inbounds [100 x double], ptr @x6, i64 0, i64 %7
  store double %126, ptr %127, align 8, !tbaa !6
  %128 = getelementptr inbounds [100 x double], ptr @y7, i64 0, i64 %7
  %129 = load double, ptr %128, align 8, !tbaa !6
  %130 = getelementptr inbounds [100 x double], ptr @z7, i64 0, i64 %7
  %131 = load double, ptr %130, align 8, !tbaa !6
  %132 = fadd double %129, %131
  %133 = getelementptr inbounds [100 x double], ptr @x7, i64 0, i64 %7
  store double %132, ptr %133, align 8, !tbaa !6
  %134 = getelementptr inbounds [100 x double], ptr @y8, i64 0, i64 %7
  %135 = load double, ptr %134, align 8, !tbaa !6
  %136 = getelementptr inbounds [100 x double], ptr @z8, i64 0, i64 %7
  %137 = load double, ptr %136, align 8, !tbaa !6
  %138 = fadd double %135, %137
  %139 = getelementptr inbounds [100 x double], ptr @x8, i64 0, i64 %7
  store double %138, ptr %139, align 8, !tbaa !6
  %140 = getelementptr inbounds [100 x double], ptr @y9, i64 0, i64 %7
  %141 = load double, ptr %140, align 8, !tbaa !6
  %142 = getelementptr inbounds [100 x double], ptr @z9, i64 0, i64 %7
  %143 = load double, ptr %142, align 8, !tbaa !6
  %144 = fadd double %141, %143
  %145 = getelementptr inbounds [100 x double], ptr @x9, i64 0, i64 %7
  store double %144, ptr %145, align 8, !tbaa !6
  %146 = getelementptr inbounds [100 x double], ptr @ya, i64 0, i64 %7
  %147 = load double, ptr %146, align 8, !tbaa !6
  %148 = getelementptr inbounds [100 x double], ptr @za, i64 0, i64 %7
  %149 = load double, ptr %148, align 8, !tbaa !6
  %150 = fadd double %147, %149
  %151 = getelementptr inbounds [100 x double], ptr @xa, i64 0, i64 %7
  store double %150, ptr %151, align 8, !tbaa !6
  %152 = add nuw nsw i64 %7, 1
  %153 = icmp eq i64 %152, %4
  br i1 %153, label %5, label %6, !llvm.loop !10
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!6 = !{!7, !7, i64 0}
!7 = !{!"double", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = distinct !{!10, !11, !12}
!11 = !{!"llvm.loop.mustprogress"}
!12 = !{!"llvm.loop.unroll.disable"}
