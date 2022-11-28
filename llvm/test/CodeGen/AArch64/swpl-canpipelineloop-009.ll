; RUN: llc < %s -O1 -mcpu=a64fx  -ffj-swp  -swpl-max-mem-num=10  -swpl-debug  --pass-remarks-filter=aarch64-swpipeliner  -pass-remarks-missed=aarch64-swpipeliner  --pass-remarks-output=-  -o /dev/null 2>&1 | FileCheck %s
;CHECK:over mem limit num
;CHECK:canPipelineLoop:NG
;CHECK:This loop is not software pipelined because the loop contains too many instructions accessing memory.
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
define dso_local void @test(i32 noundef %0) local_unnamed_addr #0 !dbg !10 {
  %2 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %2, label %3, label %5, !dbg !14

3:                                                ; preds = %1
  %4 = zext i32 %0 to i64, !dbg !13
  br label %6, !dbg !14

5:                                                ; preds = %6, %1
  ret void, !dbg !15

6:                                                ; preds = %3, %6
  %7 = phi i64 [ 0, %3 ], [ %152, %6 ]
  %8 = getelementptr inbounds [100 x double], ptr @b1, i64 0, i64 %7, !dbg !16
  %9 = load double, ptr %8, align 8, !dbg !16, !tbaa !17
  %10 = getelementptr inbounds [100 x double], ptr @c1, i64 0, i64 %7, !dbg !21
  %11 = load double, ptr %10, align 8, !dbg !21, !tbaa !17
  %12 = fadd double %9, %11, !dbg !22
  %13 = getelementptr inbounds [100 x double], ptr @a1, i64 0, i64 %7, !dbg !23
  store double %12, ptr %13, align 8, !dbg !24, !tbaa !17
  %14 = getelementptr inbounds [100 x double], ptr @b2, i64 0, i64 %7, !dbg !25
  %15 = load double, ptr %14, align 8, !dbg !25, !tbaa !17
  %16 = getelementptr inbounds [100 x double], ptr @c2, i64 0, i64 %7, !dbg !26
  %17 = load double, ptr %16, align 8, !dbg !26, !tbaa !17
  %18 = fadd double %15, %17, !dbg !27
  %19 = getelementptr inbounds [100 x double], ptr @a2, i64 0, i64 %7, !dbg !28
  store double %18, ptr %19, align 8, !dbg !29, !tbaa !17
  %20 = getelementptr inbounds [100 x double], ptr @b3, i64 0, i64 %7, !dbg !30
  %21 = load double, ptr %20, align 8, !dbg !30, !tbaa !17
  %22 = getelementptr inbounds [100 x double], ptr @c3, i64 0, i64 %7, !dbg !31
  %23 = load double, ptr %22, align 8, !dbg !31, !tbaa !17
  %24 = fadd double %21, %23, !dbg !32
  %25 = getelementptr inbounds [100 x double], ptr @a3, i64 0, i64 %7, !dbg !33
  store double %24, ptr %25, align 8, !dbg !34, !tbaa !17
  %26 = getelementptr inbounds [100 x double], ptr @b4, i64 0, i64 %7, !dbg !35
  %27 = load double, ptr %26, align 8, !dbg !35, !tbaa !17
  %28 = getelementptr inbounds [100 x double], ptr @c4, i64 0, i64 %7, !dbg !36
  %29 = load double, ptr %28, align 8, !dbg !36, !tbaa !17
  %30 = fadd double %27, %29, !dbg !37
  %31 = getelementptr inbounds [100 x double], ptr @a4, i64 0, i64 %7, !dbg !38
  store double %30, ptr %31, align 8, !dbg !39, !tbaa !17
  %32 = getelementptr inbounds [100 x double], ptr @b5, i64 0, i64 %7, !dbg !40
  %33 = load double, ptr %32, align 8, !dbg !40, !tbaa !17
  %34 = getelementptr inbounds [100 x double], ptr @c5, i64 0, i64 %7, !dbg !41
  %35 = load double, ptr %34, align 8, !dbg !41, !tbaa !17
  %36 = fadd double %33, %35, !dbg !42
  %37 = getelementptr inbounds [100 x double], ptr @a5, i64 0, i64 %7, !dbg !43
  store double %36, ptr %37, align 8, !dbg !44, !tbaa !17
  %38 = getelementptr inbounds [100 x double], ptr @b6, i64 0, i64 %7, !dbg !45
  %39 = load double, ptr %38, align 8, !dbg !45, !tbaa !17
  %40 = getelementptr inbounds [100 x double], ptr @c6, i64 0, i64 %7, !dbg !46
  %41 = load double, ptr %40, align 8, !dbg !46, !tbaa !17
  %42 = fadd double %39, %41, !dbg !47
  %43 = getelementptr inbounds [100 x double], ptr @a6, i64 0, i64 %7, !dbg !48
  store double %42, ptr %43, align 8, !dbg !49, !tbaa !17
  %44 = getelementptr inbounds [100 x double], ptr @b7, i64 0, i64 %7, !dbg !50
  %45 = load double, ptr %44, align 8, !dbg !50, !tbaa !17
  %46 = getelementptr inbounds [100 x double], ptr @c7, i64 0, i64 %7, !dbg !51
  %47 = load double, ptr %46, align 8, !dbg !51, !tbaa !17
  %48 = fadd double %45, %47, !dbg !52
  %49 = getelementptr inbounds [100 x double], ptr @a7, i64 0, i64 %7, !dbg !53
  store double %48, ptr %49, align 8, !dbg !54, !tbaa !17
  %50 = getelementptr inbounds [100 x double], ptr @b8, i64 0, i64 %7, !dbg !55
  %51 = load double, ptr %50, align 8, !dbg !55, !tbaa !17
  %52 = getelementptr inbounds [100 x double], ptr @c8, i64 0, i64 %7, !dbg !56
  %53 = load double, ptr %52, align 8, !dbg !56, !tbaa !17
  %54 = fadd double %51, %53, !dbg !57
  %55 = getelementptr inbounds [100 x double], ptr @a8, i64 0, i64 %7, !dbg !58
  store double %54, ptr %55, align 8, !dbg !59, !tbaa !17
  %56 = getelementptr inbounds [100 x double], ptr @b9, i64 0, i64 %7, !dbg !60
  %57 = load double, ptr %56, align 8, !dbg !60, !tbaa !17
  %58 = getelementptr inbounds [100 x double], ptr @c9, i64 0, i64 %7, !dbg !61
  %59 = load double, ptr %58, align 8, !dbg !61, !tbaa !17
  %60 = fadd double %57, %59, !dbg !62
  %61 = getelementptr inbounds [100 x double], ptr @a9, i64 0, i64 %7, !dbg !63
  store double %60, ptr %61, align 8, !dbg !64, !tbaa !17
  %62 = getelementptr inbounds [100 x double], ptr @ba, i64 0, i64 %7, !dbg !65
  %63 = load double, ptr %62, align 8, !dbg !65, !tbaa !17
  %64 = getelementptr inbounds [100 x double], ptr @ca, i64 0, i64 %7, !dbg !66
  %65 = load double, ptr %64, align 8, !dbg !66, !tbaa !17
  %66 = fadd double %63, %65, !dbg !67
  %67 = getelementptr inbounds [100 x double], ptr @aa, i64 0, i64 %7, !dbg !68
  store double %66, ptr %67, align 8, !dbg !69, !tbaa !17
  %68 = getelementptr inbounds [100 x double], ptr @bb, i64 0, i64 %7, !dbg !70
  %69 = load double, ptr %68, align 8, !dbg !70, !tbaa !17
  %70 = getelementptr inbounds [100 x double], ptr @cb, i64 0, i64 %7, !dbg !71
  %71 = load double, ptr %70, align 8, !dbg !71, !tbaa !17
  %72 = fadd double %69, %71, !dbg !72
  %73 = getelementptr inbounds [100 x double], ptr @ab, i64 0, i64 %7, !dbg !73
  store double %72, ptr %73, align 8, !dbg !74, !tbaa !17
  %74 = getelementptr inbounds [100 x double], ptr @bc, i64 0, i64 %7, !dbg !75
  %75 = load double, ptr %74, align 8, !dbg !75, !tbaa !17
  %76 = getelementptr inbounds [100 x double], ptr @cc, i64 0, i64 %7, !dbg !76
  %77 = load double, ptr %76, align 8, !dbg !76, !tbaa !17
  %78 = fadd double %75, %77, !dbg !77
  %79 = getelementptr inbounds [100 x double], ptr @ac, i64 0, i64 %7, !dbg !78
  store double %78, ptr %79, align 8, !dbg !79, !tbaa !17
  %80 = getelementptr inbounds [100 x double], ptr @bd, i64 0, i64 %7, !dbg !80
  %81 = load double, ptr %80, align 8, !dbg !80, !tbaa !17
  %82 = getelementptr inbounds [100 x double], ptr @cd, i64 0, i64 %7, !dbg !81
  %83 = load double, ptr %82, align 8, !dbg !81, !tbaa !17
  %84 = fadd double %81, %83, !dbg !82
  %85 = getelementptr inbounds [100 x double], ptr @ad, i64 0, i64 %7, !dbg !83
  store double %84, ptr %85, align 8, !dbg !84, !tbaa !17
  %86 = getelementptr inbounds [100 x double], ptr @be, i64 0, i64 %7, !dbg !85
  %87 = load double, ptr %86, align 8, !dbg !85, !tbaa !17
  %88 = getelementptr inbounds [100 x double], ptr @ce, i64 0, i64 %7, !dbg !86
  %89 = load double, ptr %88, align 8, !dbg !86, !tbaa !17
  %90 = fadd double %87, %89, !dbg !87
  %91 = getelementptr inbounds [100 x double], ptr @ae, i64 0, i64 %7, !dbg !88
  store double %90, ptr %91, align 8, !dbg !89, !tbaa !17
  %92 = getelementptr inbounds [100 x double], ptr @y1, i64 0, i64 %7, !dbg !90
  %93 = load double, ptr %92, align 8, !dbg !90, !tbaa !17
  %94 = getelementptr inbounds [100 x double], ptr @z1, i64 0, i64 %7, !dbg !91
  %95 = load double, ptr %94, align 8, !dbg !91, !tbaa !17
  %96 = fadd double %93, %95, !dbg !92
  %97 = getelementptr inbounds [100 x double], ptr @x1, i64 0, i64 %7, !dbg !93
  store double %96, ptr %97, align 8, !dbg !94, !tbaa !17
  %98 = getelementptr inbounds [100 x double], ptr @y2, i64 0, i64 %7, !dbg !95
  %99 = load double, ptr %98, align 8, !dbg !95, !tbaa !17
  %100 = getelementptr inbounds [100 x double], ptr @z2, i64 0, i64 %7, !dbg !96
  %101 = load double, ptr %100, align 8, !dbg !96, !tbaa !17
  %102 = fadd double %99, %101, !dbg !97
  %103 = getelementptr inbounds [100 x double], ptr @x2, i64 0, i64 %7, !dbg !98
  store double %102, ptr %103, align 8, !dbg !99, !tbaa !17
  %104 = getelementptr inbounds [100 x double], ptr @y3, i64 0, i64 %7, !dbg !100
  %105 = load double, ptr %104, align 8, !dbg !100, !tbaa !17
  %106 = getelementptr inbounds [100 x double], ptr @z3, i64 0, i64 %7, !dbg !101
  %107 = load double, ptr %106, align 8, !dbg !101, !tbaa !17
  %108 = fadd double %105, %107, !dbg !102
  %109 = getelementptr inbounds [100 x double], ptr @x3, i64 0, i64 %7, !dbg !103
  store double %108, ptr %109, align 8, !dbg !104, !tbaa !17
  %110 = getelementptr inbounds [100 x double], ptr @y4, i64 0, i64 %7, !dbg !105
  %111 = load double, ptr %110, align 8, !dbg !105, !tbaa !17
  %112 = getelementptr inbounds [100 x double], ptr @z4, i64 0, i64 %7, !dbg !106
  %113 = load double, ptr %112, align 8, !dbg !106, !tbaa !17
  %114 = fadd double %111, %113, !dbg !107
  %115 = getelementptr inbounds [100 x double], ptr @x4, i64 0, i64 %7, !dbg !108
  store double %114, ptr %115, align 8, !dbg !109, !tbaa !17
  %116 = getelementptr inbounds [100 x double], ptr @y5, i64 0, i64 %7, !dbg !110
  %117 = load double, ptr %116, align 8, !dbg !110, !tbaa !17
  %118 = getelementptr inbounds [100 x double], ptr @z5, i64 0, i64 %7, !dbg !111
  %119 = load double, ptr %118, align 8, !dbg !111, !tbaa !17
  %120 = fadd double %117, %119, !dbg !112
  %121 = getelementptr inbounds [100 x double], ptr @x5, i64 0, i64 %7, !dbg !113
  store double %120, ptr %121, align 8, !dbg !114, !tbaa !17
  %122 = getelementptr inbounds [100 x double], ptr @y6, i64 0, i64 %7, !dbg !115
  %123 = load double, ptr %122, align 8, !dbg !115, !tbaa !17
  %124 = getelementptr inbounds [100 x double], ptr @z6, i64 0, i64 %7, !dbg !116
  %125 = load double, ptr %124, align 8, !dbg !116, !tbaa !17
  %126 = fadd double %123, %125, !dbg !117
  %127 = getelementptr inbounds [100 x double], ptr @x6, i64 0, i64 %7, !dbg !118
  store double %126, ptr %127, align 8, !dbg !119, !tbaa !17
  %128 = getelementptr inbounds [100 x double], ptr @y7, i64 0, i64 %7, !dbg !120
  %129 = load double, ptr %128, align 8, !dbg !120, !tbaa !17
  %130 = getelementptr inbounds [100 x double], ptr @z7, i64 0, i64 %7, !dbg !121
  %131 = load double, ptr %130, align 8, !dbg !121, !tbaa !17
  %132 = fadd double %129, %131, !dbg !122
  %133 = getelementptr inbounds [100 x double], ptr @x7, i64 0, i64 %7, !dbg !123
  store double %132, ptr %133, align 8, !dbg !124, !tbaa !17
  %134 = getelementptr inbounds [100 x double], ptr @y8, i64 0, i64 %7, !dbg !125
  %135 = load double, ptr %134, align 8, !dbg !125, !tbaa !17
  %136 = getelementptr inbounds [100 x double], ptr @z8, i64 0, i64 %7, !dbg !126
  %137 = load double, ptr %136, align 8, !dbg !126, !tbaa !17
  %138 = fadd double %135, %137, !dbg !127
  %139 = getelementptr inbounds [100 x double], ptr @x8, i64 0, i64 %7, !dbg !128
  store double %138, ptr %139, align 8, !dbg !129, !tbaa !17
  %140 = getelementptr inbounds [100 x double], ptr @y9, i64 0, i64 %7, !dbg !130
  %141 = load double, ptr %140, align 8, !dbg !130, !tbaa !17
  %142 = getelementptr inbounds [100 x double], ptr @z9, i64 0, i64 %7, !dbg !131
  %143 = load double, ptr %142, align 8, !dbg !131, !tbaa !17
  %144 = fadd double %141, %143, !dbg !132
  %145 = getelementptr inbounds [100 x double], ptr @x9, i64 0, i64 %7, !dbg !133
  store double %144, ptr %145, align 8, !dbg !134, !tbaa !17
  %146 = getelementptr inbounds [100 x double], ptr @ya, i64 0, i64 %7, !dbg !135
  %147 = load double, ptr %146, align 8, !dbg !135, !tbaa !17
  %148 = getelementptr inbounds [100 x double], ptr @za, i64 0, i64 %7, !dbg !136
  %149 = load double, ptr %148, align 8, !dbg !136, !tbaa !17
  %150 = fadd double %147, %149, !dbg !137
  %151 = getelementptr inbounds [100 x double], ptr @xa, i64 0, i64 %7, !dbg !138
  store double %150, ptr %151, align 8, !dbg !139, !tbaa !17
  %152 = add nuw nsw i64 %7, 1, !dbg !140
  %153 = icmp eq i64 %152, %4, !dbg !13
  br i1 %153, label %5, label %6, !dbg !14, !llvm.loop !141
}

attributes #0 = { nofree norecurse nosync nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2912_inst_l.c", directory: "2912", checksumkind: CSK_MD5, checksum: "f6a11f67325db934c656866d16636834")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 7, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 1}
!10 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 28, type: !11, scopeLine: 28, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !12)
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 29, column: 17, scope: !10)
!14 = !DILocation(line: 29, column: 3, scope: !10)
!15 = !DILocation(line: 55, column: 1, scope: !10)
!16 = !DILocation(line: 30, column: 13, scope: !10)
!17 = !{!18, !18, i64 0}
!18 = !{!"double", !19, i64 0}
!19 = !{!"omnipotent char", !20, i64 0}
!20 = !{!"Simple C/C++ TBAA"}
!21 = !DILocation(line: 30, column: 21, scope: !10)
!22 = !DILocation(line: 30, column: 19, scope: !10)
!23 = !DILocation(line: 30, column: 5, scope: !10)
!24 = !DILocation(line: 30, column: 11, scope: !10)
!25 = !DILocation(line: 31, column: 13, scope: !10)
!26 = !DILocation(line: 31, column: 21, scope: !10)
!27 = !DILocation(line: 31, column: 19, scope: !10)
!28 = !DILocation(line: 31, column: 5, scope: !10)
!29 = !DILocation(line: 31, column: 11, scope: !10)
!30 = !DILocation(line: 32, column: 13, scope: !10)
!31 = !DILocation(line: 32, column: 21, scope: !10)
!32 = !DILocation(line: 32, column: 19, scope: !10)
!33 = !DILocation(line: 32, column: 5, scope: !10)
!34 = !DILocation(line: 32, column: 11, scope: !10)
!35 = !DILocation(line: 33, column: 13, scope: !10)
!36 = !DILocation(line: 33, column: 21, scope: !10)
!37 = !DILocation(line: 33, column: 19, scope: !10)
!38 = !DILocation(line: 33, column: 5, scope: !10)
!39 = !DILocation(line: 33, column: 11, scope: !10)
!40 = !DILocation(line: 34, column: 13, scope: !10)
!41 = !DILocation(line: 34, column: 21, scope: !10)
!42 = !DILocation(line: 34, column: 19, scope: !10)
!43 = !DILocation(line: 34, column: 5, scope: !10)
!44 = !DILocation(line: 34, column: 11, scope: !10)
!45 = !DILocation(line: 35, column: 13, scope: !10)
!46 = !DILocation(line: 35, column: 21, scope: !10)
!47 = !DILocation(line: 35, column: 19, scope: !10)
!48 = !DILocation(line: 35, column: 5, scope: !10)
!49 = !DILocation(line: 35, column: 11, scope: !10)
!50 = !DILocation(line: 36, column: 13, scope: !10)
!51 = !DILocation(line: 36, column: 21, scope: !10)
!52 = !DILocation(line: 36, column: 19, scope: !10)
!53 = !DILocation(line: 36, column: 5, scope: !10)
!54 = !DILocation(line: 36, column: 11, scope: !10)
!55 = !DILocation(line: 37, column: 13, scope: !10)
!56 = !DILocation(line: 37, column: 21, scope: !10)
!57 = !DILocation(line: 37, column: 19, scope: !10)
!58 = !DILocation(line: 37, column: 5, scope: !10)
!59 = !DILocation(line: 37, column: 11, scope: !10)
!60 = !DILocation(line: 38, column: 13, scope: !10)
!61 = !DILocation(line: 38, column: 21, scope: !10)
!62 = !DILocation(line: 38, column: 19, scope: !10)
!63 = !DILocation(line: 38, column: 5, scope: !10)
!64 = !DILocation(line: 38, column: 11, scope: !10)
!65 = !DILocation(line: 39, column: 13, scope: !10)
!66 = !DILocation(line: 39, column: 21, scope: !10)
!67 = !DILocation(line: 39, column: 19, scope: !10)
!68 = !DILocation(line: 39, column: 5, scope: !10)
!69 = !DILocation(line: 39, column: 11, scope: !10)
!70 = !DILocation(line: 40, column: 13, scope: !10)
!71 = !DILocation(line: 40, column: 21, scope: !10)
!72 = !DILocation(line: 40, column: 19, scope: !10)
!73 = !DILocation(line: 40, column: 5, scope: !10)
!74 = !DILocation(line: 40, column: 11, scope: !10)
!75 = !DILocation(line: 41, column: 13, scope: !10)
!76 = !DILocation(line: 41, column: 21, scope: !10)
!77 = !DILocation(line: 41, column: 19, scope: !10)
!78 = !DILocation(line: 41, column: 5, scope: !10)
!79 = !DILocation(line: 41, column: 11, scope: !10)
!80 = !DILocation(line: 42, column: 13, scope: !10)
!81 = !DILocation(line: 42, column: 21, scope: !10)
!82 = !DILocation(line: 42, column: 19, scope: !10)
!83 = !DILocation(line: 42, column: 5, scope: !10)
!84 = !DILocation(line: 42, column: 11, scope: !10)
!85 = !DILocation(line: 43, column: 13, scope: !10)
!86 = !DILocation(line: 43, column: 21, scope: !10)
!87 = !DILocation(line: 43, column: 19, scope: !10)
!88 = !DILocation(line: 43, column: 5, scope: !10)
!89 = !DILocation(line: 43, column: 11, scope: !10)
!90 = !DILocation(line: 44, column: 13, scope: !10)
!91 = !DILocation(line: 44, column: 21, scope: !10)
!92 = !DILocation(line: 44, column: 19, scope: !10)
!93 = !DILocation(line: 44, column: 5, scope: !10)
!94 = !DILocation(line: 44, column: 11, scope: !10)
!95 = !DILocation(line: 45, column: 13, scope: !10)
!96 = !DILocation(line: 45, column: 21, scope: !10)
!97 = !DILocation(line: 45, column: 19, scope: !10)
!98 = !DILocation(line: 45, column: 5, scope: !10)
!99 = !DILocation(line: 45, column: 11, scope: !10)
!100 = !DILocation(line: 46, column: 13, scope: !10)
!101 = !DILocation(line: 46, column: 21, scope: !10)
!102 = !DILocation(line: 46, column: 19, scope: !10)
!103 = !DILocation(line: 46, column: 5, scope: !10)
!104 = !DILocation(line: 46, column: 11, scope: !10)
!105 = !DILocation(line: 47, column: 13, scope: !10)
!106 = !DILocation(line: 47, column: 21, scope: !10)
!107 = !DILocation(line: 47, column: 19, scope: !10)
!108 = !DILocation(line: 47, column: 5, scope: !10)
!109 = !DILocation(line: 47, column: 11, scope: !10)
!110 = !DILocation(line: 48, column: 13, scope: !10)
!111 = !DILocation(line: 48, column: 21, scope: !10)
!112 = !DILocation(line: 48, column: 19, scope: !10)
!113 = !DILocation(line: 48, column: 5, scope: !10)
!114 = !DILocation(line: 48, column: 11, scope: !10)
!115 = !DILocation(line: 49, column: 13, scope: !10)
!116 = !DILocation(line: 49, column: 21, scope: !10)
!117 = !DILocation(line: 49, column: 19, scope: !10)
!118 = !DILocation(line: 49, column: 5, scope: !10)
!119 = !DILocation(line: 49, column: 11, scope: !10)
!120 = !DILocation(line: 50, column: 13, scope: !10)
!121 = !DILocation(line: 50, column: 21, scope: !10)
!122 = !DILocation(line: 50, column: 19, scope: !10)
!123 = !DILocation(line: 50, column: 5, scope: !10)
!124 = !DILocation(line: 50, column: 11, scope: !10)
!125 = !DILocation(line: 51, column: 13, scope: !10)
!126 = !DILocation(line: 51, column: 21, scope: !10)
!127 = !DILocation(line: 51, column: 19, scope: !10)
!128 = !DILocation(line: 51, column: 5, scope: !10)
!129 = !DILocation(line: 51, column: 11, scope: !10)
!130 = !DILocation(line: 52, column: 13, scope: !10)
!131 = !DILocation(line: 52, column: 21, scope: !10)
!132 = !DILocation(line: 52, column: 19, scope: !10)
!133 = !DILocation(line: 52, column: 5, scope: !10)
!134 = !DILocation(line: 52, column: 11, scope: !10)
!135 = !DILocation(line: 53, column: 13, scope: !10)
!136 = !DILocation(line: 53, column: 21, scope: !10)
!137 = !DILocation(line: 53, column: 19, scope: !10)
!138 = !DILocation(line: 53, column: 5, scope: !10)
!139 = !DILocation(line: 53, column: 11, scope: !10)
!140 = !DILocation(line: 29, column: 22, scope: !10)
!141 = distinct !{!141, !14, !142, !143, !144}
!142 = !DILocation(line: 54, column: 3, scope: !10)
!143 = !{!"llvm.loop.mustprogress"}
!144 = !{!"llvm.loop.unroll.disable"}
