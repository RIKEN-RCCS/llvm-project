; RUN: llc < %s -mcpu=a64fx -O1 -ffj-swp -debug-hardwareloops --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: HardwareLoopInsertion succeeded
; ModuleID = '2901-2-06.c'
source_filename = "2901-2-06.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@sum = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 !dbg !10 {
  %3 = icmp sgt i32 %0, 0, !dbg !13
  br i1 %3, label %4, label %7, !dbg !14

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !15
  %6 = zext i32 %0 to i64, !dbg !13
  br label %48, !dbg !14

7:                                                ; preds = %48, %2
  %8 = load i32, ptr @sum, align 4, !dbg !19, !tbaa !20
  %9 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !dbg !22, !tbaa !20
  %10 = add nsw i32 %9, %8, !dbg !23
  %11 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !dbg !24, !tbaa !20
  %12 = add nsw i32 %10, %11, !dbg !25
  %13 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !dbg !26, !tbaa !20
  %14 = add nsw i32 %12, %13, !dbg !27
  %15 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !dbg !28, !tbaa !20
  %16 = add nsw i32 %14, %15, !dbg !29
  %17 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !dbg !30, !tbaa !20
  %18 = add nsw i32 %16, %17, !dbg !31
  %19 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !dbg !32, !tbaa !20
  %20 = add nsw i32 %18, %19, !dbg !33
  %21 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !dbg !34, !tbaa !20
  %22 = add nsw i32 %20, %21, !dbg !35
  %23 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !dbg !36, !tbaa !20
  %24 = add nsw i32 %22, %23, !dbg !37
  %25 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !dbg !38, !tbaa !20
  %26 = add nsw i32 %24, %25, !dbg !39
  %27 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !dbg !40, !tbaa !20
  %28 = add nsw i32 %26, %27, !dbg !41
  %29 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !dbg !42, !tbaa !20
  %30 = add nsw i32 %28, %29, !dbg !43
  %31 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !dbg !44, !tbaa !20
  %32 = add nsw i32 %30, %31, !dbg !45
  %33 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !dbg !46, !tbaa !20
  %34 = add nsw i32 %32, %33, !dbg !47
  %35 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !dbg !48, !tbaa !20
  %36 = add nsw i32 %34, %35, !dbg !49
  %37 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !dbg !50, !tbaa !20
  %38 = add nsw i32 %36, %37, !dbg !51
  %39 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !dbg !52, !tbaa !20
  %40 = add nsw i32 %38, %39, !dbg !53
  %41 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !dbg !54, !tbaa !20
  %42 = add nsw i32 %40, %41, !dbg !55
  %43 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !dbg !56, !tbaa !20
  %44 = add nsw i32 %42, %43, !dbg !57
  %45 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !dbg !58, !tbaa !20
  %46 = add nsw i32 %44, %45, !dbg !59
  %47 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %46), !dbg !60
  ret i32 0, !dbg !61

48:                                               ; preds = %4, %48
  %49 = phi i64 [ 0, %4 ], [ %209, %48 ]
  %50 = getelementptr inbounds i8, ptr %5, i64 %49, !dbg !62
  %51 = load i8, ptr %50, align 1, !dbg !62, !tbaa !63
  %52 = zext i8 %51 to i32, !dbg !62
  %53 = load i32, ptr @sum, align 4, !dbg !64, !tbaa !20
  %54 = add nsw i32 %53, %52, !dbg !64
  store i32 %54, ptr @sum, align 4, !dbg !64, !tbaa !20
  %55 = load i8, ptr %50, align 1, !dbg !65, !tbaa !63
  %56 = zext i8 %55 to i32, !dbg !65
  %57 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !dbg !66, !tbaa !20
  %58 = add nsw i32 %57, %56, !dbg !66
  store i32 %58, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !dbg !66, !tbaa !20
  %59 = load i8, ptr %50, align 1, !dbg !67, !tbaa !63
  %60 = zext i8 %59 to i32, !dbg !67
  %61 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !dbg !68, !tbaa !20
  %62 = add nsw i32 %61, %60, !dbg !68
  store i32 %62, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !dbg !68, !tbaa !20
  %63 = load i8, ptr %50, align 1, !dbg !69, !tbaa !63
  %64 = zext i8 %63 to i32, !dbg !69
  %65 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !dbg !70, !tbaa !20
  %66 = add nsw i32 %65, %64, !dbg !70
  store i32 %66, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !dbg !70, !tbaa !20
  %67 = load i8, ptr %50, align 1, !dbg !71, !tbaa !63
  %68 = zext i8 %67 to i32, !dbg !71
  %69 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !dbg !72, !tbaa !20
  %70 = add nsw i32 %69, %68, !dbg !72
  store i32 %70, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !dbg !72, !tbaa !20
  %71 = load i8, ptr %50, align 1, !dbg !73, !tbaa !63
  %72 = zext i8 %71 to i32, !dbg !73
  %73 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !dbg !74, !tbaa !20
  %74 = add nsw i32 %73, %72, !dbg !74
  store i32 %74, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !dbg !74, !tbaa !20
  %75 = load i8, ptr %50, align 1, !dbg !75, !tbaa !63
  %76 = zext i8 %75 to i32, !dbg !75
  %77 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !dbg !76, !tbaa !20
  %78 = add nsw i32 %77, %76, !dbg !76
  store i32 %78, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !dbg !76, !tbaa !20
  %79 = load i8, ptr %50, align 1, !dbg !77, !tbaa !63
  %80 = zext i8 %79 to i32, !dbg !77
  %81 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !dbg !78, !tbaa !20
  %82 = add nsw i32 %81, %80, !dbg !78
  store i32 %82, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !dbg !78, !tbaa !20
  %83 = load i8, ptr %50, align 1, !dbg !79, !tbaa !63
  %84 = zext i8 %83 to i32, !dbg !79
  %85 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !dbg !80, !tbaa !20
  %86 = add nsw i32 %85, %84, !dbg !80
  store i32 %86, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !dbg !80, !tbaa !20
  %87 = load i8, ptr %50, align 1, !dbg !81, !tbaa !63
  %88 = zext i8 %87 to i32, !dbg !81
  %89 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !dbg !82, !tbaa !20
  %90 = add nsw i32 %89, %88, !dbg !82
  store i32 %90, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !dbg !82, !tbaa !20
  %91 = load i8, ptr %50, align 1, !dbg !83, !tbaa !63
  %92 = zext i8 %91 to i32, !dbg !83
  %93 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !dbg !84, !tbaa !20
  %94 = add nsw i32 %93, %92, !dbg !84
  store i32 %94, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !dbg !84, !tbaa !20
  %95 = load i8, ptr %50, align 1, !dbg !85, !tbaa !63
  %96 = zext i8 %95 to i32, !dbg !85
  %97 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !dbg !86, !tbaa !20
  %98 = add nsw i32 %97, %96, !dbg !86
  store i32 %98, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !dbg !86, !tbaa !20
  %99 = load i8, ptr %50, align 1, !dbg !87, !tbaa !63
  %100 = zext i8 %99 to i32, !dbg !87
  %101 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !dbg !88, !tbaa !20
  %102 = add nsw i32 %101, %100, !dbg !88
  store i32 %102, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !dbg !88, !tbaa !20
  %103 = load i8, ptr %50, align 1, !dbg !89, !tbaa !63
  %104 = zext i8 %103 to i32, !dbg !89
  %105 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !dbg !90, !tbaa !20
  %106 = add nsw i32 %105, %104, !dbg !90
  store i32 %106, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !dbg !90, !tbaa !20
  %107 = load i8, ptr %50, align 1, !dbg !91, !tbaa !63
  %108 = zext i8 %107 to i32, !dbg !91
  %109 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !dbg !92, !tbaa !20
  %110 = add nsw i32 %109, %108, !dbg !92
  store i32 %110, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !dbg !92, !tbaa !20
  %111 = load i8, ptr %50, align 1, !dbg !93, !tbaa !63
  %112 = zext i8 %111 to i32, !dbg !93
  %113 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !dbg !94, !tbaa !20
  %114 = add nsw i32 %113, %112, !dbg !94
  store i32 %114, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !dbg !94, !tbaa !20
  %115 = load i8, ptr %50, align 1, !dbg !95, !tbaa !63
  %116 = zext i8 %115 to i32, !dbg !95
  %117 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !dbg !96, !tbaa !20
  %118 = add nsw i32 %117, %116, !dbg !96
  store i32 %118, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !dbg !96, !tbaa !20
  %119 = load i8, ptr %50, align 1, !dbg !97, !tbaa !63
  %120 = zext i8 %119 to i32, !dbg !97
  %121 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !dbg !98, !tbaa !20
  %122 = add nsw i32 %121, %120, !dbg !98
  store i32 %122, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !dbg !98, !tbaa !20
  %123 = load i8, ptr %50, align 1, !dbg !99, !tbaa !63
  %124 = zext i8 %123 to i32, !dbg !99
  %125 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !dbg !100, !tbaa !20
  %126 = add nsw i32 %125, %124, !dbg !100
  store i32 %126, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !dbg !100, !tbaa !20
  %127 = load i8, ptr %50, align 1, !dbg !101, !tbaa !63
  %128 = zext i8 %127 to i32, !dbg !101
  %129 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !dbg !102, !tbaa !20
  %130 = add nsw i32 %129, %128, !dbg !102
  store i32 %130, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !dbg !102, !tbaa !20
  %131 = load i8, ptr %50, align 1, !dbg !103, !tbaa !63
  %132 = zext i8 %131 to i32, !dbg !103
  %133 = add nsw i32 %54, %132, !dbg !104
  store i32 %133, ptr @sum, align 4, !dbg !104, !tbaa !20
  %134 = add nsw i32 %133, -1, !dbg !105
  %135 = add nsw i32 %134, %58, !dbg !106
  store i32 %135, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !dbg !106, !tbaa !20
  %136 = add i32 %62, -2, !dbg !107
  %137 = add i32 %136, %135, !dbg !108
  store i32 %137, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !dbg !108, !tbaa !20
  %138 = add i32 %66, -3, !dbg !109
  %139 = add i32 %138, %137, !dbg !110
  store i32 %139, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !dbg !110, !tbaa !20
  %140 = add i32 %70, -4, !dbg !111
  %141 = add i32 %140, %139, !dbg !112
  store i32 %141, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !dbg !112, !tbaa !20
  %142 = add i32 %74, -5, !dbg !113
  %143 = add i32 %142, %141, !dbg !114
  store i32 %143, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !dbg !114, !tbaa !20
  %144 = add i32 %78, -6, !dbg !115
  %145 = add i32 %144, %143, !dbg !116
  store i32 %145, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !dbg !116, !tbaa !20
  %146 = add i32 %82, -7, !dbg !117
  %147 = add i32 %146, %145, !dbg !118
  store i32 %147, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !dbg !118, !tbaa !20
  %148 = add i32 %86, -8, !dbg !119
  %149 = add i32 %148, %147, !dbg !120
  store i32 %149, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !dbg !120, !tbaa !20
  %150 = add i32 %90, -9, !dbg !121
  %151 = add i32 %150, %149, !dbg !122
  store i32 %151, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !dbg !122, !tbaa !20
  %152 = add i32 %94, -10, !dbg !123
  %153 = add i32 %152, %151, !dbg !124
  store i32 %153, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !dbg !124, !tbaa !20
  %154 = add i32 %98, -2, !dbg !125
  %155 = add i32 %154, %153, !dbg !126
  store i32 %155, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !dbg !126, !tbaa !20
  %156 = add nsw i32 %155, -2, !dbg !127
  %157 = add nsw i32 %156, %102, !dbg !128
  store i32 %157, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !dbg !128, !tbaa !20
  %158 = add i32 %106, -3, !dbg !129
  %159 = add i32 %158, %157, !dbg !130
  store i32 %159, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !dbg !130, !tbaa !20
  %160 = add i32 %110, -4, !dbg !131
  %161 = add i32 %160, %159, !dbg !132
  store i32 %161, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !dbg !132, !tbaa !20
  %162 = add i32 %114, -5, !dbg !133
  %163 = add i32 %162, %161, !dbg !134
  store i32 %163, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !dbg !134, !tbaa !20
  %164 = add i32 %118, -6, !dbg !135
  %165 = add i32 %164, %163, !dbg !136
  store i32 %165, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !dbg !136, !tbaa !20
  %166 = add i32 %122, -7, !dbg !137
  %167 = add i32 %166, %165, !dbg !138
  store i32 %167, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !dbg !138, !tbaa !20
  %168 = add i32 %126, -8, !dbg !139
  %169 = add i32 %168, %167, !dbg !140
  store i32 %169, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !dbg !140, !tbaa !20
  %170 = add i32 %130, -9, !dbg !141
  %171 = add i32 %170, %169, !dbg !142
  store i32 %171, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !dbg !142, !tbaa !20
  %172 = add nsw i32 %135, %134, !dbg !143
  store i32 %172, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !dbg !143, !tbaa !20
  %173 = add nsw i32 %172, -2, !dbg !144
  %174 = add nsw i32 %173, %137, !dbg !145
  store i32 %174, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !dbg !145, !tbaa !20
  %175 = add nsw i32 %174, -3, !dbg !146
  %176 = add nsw i32 %175, %139, !dbg !147
  store i32 %176, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !dbg !147, !tbaa !20
  %177 = add nsw i32 %176, -4, !dbg !148
  %178 = add nsw i32 %177, %141, !dbg !149
  store i32 %178, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !dbg !149, !tbaa !20
  %179 = add nsw i32 %178, -5, !dbg !150
  %180 = add nsw i32 %179, %143, !dbg !151
  store i32 %180, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !dbg !151, !tbaa !20
  %181 = add nsw i32 %180, -6, !dbg !152
  %182 = add nsw i32 %181, %145, !dbg !153
  store i32 %182, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !dbg !153, !tbaa !20
  %183 = add nsw i32 %182, -7, !dbg !154
  %184 = add nsw i32 %183, %147, !dbg !155
  store i32 %184, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !dbg !155, !tbaa !20
  %185 = add nsw i32 %184, -8, !dbg !156
  %186 = add nsw i32 %185, %149, !dbg !157
  store i32 %186, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !dbg !157, !tbaa !20
  %187 = add nsw i32 %186, -9, !dbg !158
  %188 = add nsw i32 %187, %151, !dbg !159
  store i32 %188, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !dbg !159, !tbaa !20
  %189 = add nsw i32 %188, -10, !dbg !160
  %190 = add nsw i32 %189, %153, !dbg !161
  store i32 %190, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !dbg !161, !tbaa !20
  %191 = add i32 %155, -2, !dbg !162
  %192 = add i32 %191, %190, !dbg !163
  store i32 %192, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !dbg !163, !tbaa !20
  %193 = add nsw i32 %192, -2, !dbg !164
  %194 = add nsw i32 %193, %157, !dbg !165
  store i32 %194, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !dbg !165, !tbaa !20
  %195 = add nsw i32 %194, -3, !dbg !166
  %196 = add nsw i32 %195, %159, !dbg !167
  store i32 %196, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !dbg !167, !tbaa !20
  %197 = add nsw i32 %196, -4, !dbg !168
  %198 = add nsw i32 %197, %161, !dbg !169
  store i32 %198, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !dbg !169, !tbaa !20
  %199 = add nsw i32 %198, -5, !dbg !170
  %200 = add nsw i32 %199, %163, !dbg !171
  store i32 %200, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !dbg !171, !tbaa !20
  %201 = add nsw i32 %200, -6, !dbg !172
  %202 = add nsw i32 %201, %165, !dbg !173
  store i32 %202, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !dbg !173, !tbaa !20
  %203 = add nsw i32 %202, -7, !dbg !174
  %204 = add nsw i32 %203, %167, !dbg !175
  store i32 %204, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !dbg !175, !tbaa !20
  %205 = add nsw i32 %204, -8, !dbg !176
  %206 = add nsw i32 %205, %169, !dbg !177
  store i32 %206, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !dbg !177, !tbaa !20
  %207 = add nsw i32 %206, -9, !dbg !178
  %208 = add nsw i32 %207, %171, !dbg !179
  store i32 %208, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !dbg !179, !tbaa !20
  %209 = add nuw nsw i64 %49, 1, !dbg !180
  %210 = icmp eq i64 %209, %6, !dbg !13
  br i1 %210, label %7, label %48, !dbg !14, !llvm.loop !181
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.2", isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "2901-2-06.c", directory: "2901-2", checksumkind: CSK_MD5, checksum: "bd782756f03ec1835cff03490f85d06e")
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
!13 = !DILocation(line: 7, column: 17, scope: !10)
!14 = !DILocation(line: 7, column: 2, scope: !10)
!15 = !{!16, !16, i64 0}
!16 = !{!"any pointer", !17, i64 0}
!17 = !{!"omnipotent char", !18, i64 0}
!18 = !{!"Simple C/C++ TBAA"}
!19 = !DILocation(line: 70, column: 9, scope: !10)
!20 = !{!21, !21, i64 0}
!21 = !{!"int", !17, i64 0}
!22 = !DILocation(line: 70, column: 16, scope: !10)
!23 = !DILocation(line: 70, column: 15, scope: !10)
!24 = !DILocation(line: 70, column: 23, scope: !10)
!25 = !DILocation(line: 70, column: 22, scope: !10)
!26 = !DILocation(line: 70, column: 30, scope: !10)
!27 = !DILocation(line: 70, column: 29, scope: !10)
!28 = !DILocation(line: 70, column: 37, scope: !10)
!29 = !DILocation(line: 70, column: 36, scope: !10)
!30 = !DILocation(line: 70, column: 44, scope: !10)
!31 = !DILocation(line: 70, column: 43, scope: !10)
!32 = !DILocation(line: 70, column: 51, scope: !10)
!33 = !DILocation(line: 70, column: 50, scope: !10)
!34 = !DILocation(line: 70, column: 58, scope: !10)
!35 = !DILocation(line: 70, column: 57, scope: !10)
!36 = !DILocation(line: 70, column: 65, scope: !10)
!37 = !DILocation(line: 70, column: 64, scope: !10)
!38 = !DILocation(line: 70, column: 72, scope: !10)
!39 = !DILocation(line: 70, column: 71, scope: !10)
!40 = !DILocation(line: 71, column: 10, scope: !10)
!41 = !DILocation(line: 71, column: 9, scope: !10)
!42 = !DILocation(line: 71, column: 18, scope: !10)
!43 = !DILocation(line: 71, column: 17, scope: !10)
!44 = !DILocation(line: 71, column: 26, scope: !10)
!45 = !DILocation(line: 71, column: 25, scope: !10)
!46 = !DILocation(line: 71, column: 34, scope: !10)
!47 = !DILocation(line: 71, column: 33, scope: !10)
!48 = !DILocation(line: 71, column: 42, scope: !10)
!49 = !DILocation(line: 71, column: 41, scope: !10)
!50 = !DILocation(line: 71, column: 50, scope: !10)
!51 = !DILocation(line: 71, column: 49, scope: !10)
!52 = !DILocation(line: 71, column: 58, scope: !10)
!53 = !DILocation(line: 71, column: 57, scope: !10)
!54 = !DILocation(line: 71, column: 66, scope: !10)
!55 = !DILocation(line: 71, column: 65, scope: !10)
!56 = !DILocation(line: 71, column: 74, scope: !10)
!57 = !DILocation(line: 71, column: 73, scope: !10)
!58 = !DILocation(line: 71, column: 82, scope: !10)
!59 = !DILocation(line: 71, column: 81, scope: !10)
!60 = !DILocation(line: 69, column: 2, scope: !10)
!61 = !DILocation(line: 72, column: 2, scope: !10)
!62 = !DILocation(line: 8, column: 13, scope: !10)
!63 = !{!17, !17, i64 0}
!64 = !DILocation(line: 8, column: 11, scope: !10)
!65 = !DILocation(line: 9, column: 13, scope: !10)
!66 = !DILocation(line: 9, column: 11, scope: !10)
!67 = !DILocation(line: 10, column: 13, scope: !10)
!68 = !DILocation(line: 10, column: 11, scope: !10)
!69 = !DILocation(line: 11, column: 13, scope: !10)
!70 = !DILocation(line: 11, column: 11, scope: !10)
!71 = !DILocation(line: 12, column: 13, scope: !10)
!72 = !DILocation(line: 12, column: 11, scope: !10)
!73 = !DILocation(line: 13, column: 13, scope: !10)
!74 = !DILocation(line: 13, column: 11, scope: !10)
!75 = !DILocation(line: 14, column: 13, scope: !10)
!76 = !DILocation(line: 14, column: 11, scope: !10)
!77 = !DILocation(line: 15, column: 13, scope: !10)
!78 = !DILocation(line: 15, column: 11, scope: !10)
!79 = !DILocation(line: 16, column: 13, scope: !10)
!80 = !DILocation(line: 16, column: 11, scope: !10)
!81 = !DILocation(line: 17, column: 13, scope: !10)
!82 = !DILocation(line: 17, column: 11, scope: !10)
!83 = !DILocation(line: 18, column: 14, scope: !10)
!84 = !DILocation(line: 18, column: 12, scope: !10)
!85 = !DILocation(line: 19, column: 14, scope: !10)
!86 = !DILocation(line: 19, column: 12, scope: !10)
!87 = !DILocation(line: 20, column: 14, scope: !10)
!88 = !DILocation(line: 20, column: 12, scope: !10)
!89 = !DILocation(line: 21, column: 14, scope: !10)
!90 = !DILocation(line: 21, column: 12, scope: !10)
!91 = !DILocation(line: 22, column: 14, scope: !10)
!92 = !DILocation(line: 22, column: 12, scope: !10)
!93 = !DILocation(line: 23, column: 14, scope: !10)
!94 = !DILocation(line: 23, column: 12, scope: !10)
!95 = !DILocation(line: 24, column: 14, scope: !10)
!96 = !DILocation(line: 24, column: 12, scope: !10)
!97 = !DILocation(line: 25, column: 14, scope: !10)
!98 = !DILocation(line: 25, column: 12, scope: !10)
!99 = !DILocation(line: 26, column: 14, scope: !10)
!100 = !DILocation(line: 26, column: 12, scope: !10)
!101 = !DILocation(line: 27, column: 14, scope: !10)
!102 = !DILocation(line: 27, column: 12, scope: !10)
!103 = !DILocation(line: 28, column: 13, scope: !10)
!104 = !DILocation(line: 28, column: 11, scope: !10)
!105 = !DILocation(line: 29, column: 19, scope: !10)
!106 = !DILocation(line: 29, column: 11, scope: !10)
!107 = !DILocation(line: 30, column: 19, scope: !10)
!108 = !DILocation(line: 30, column: 11, scope: !10)
!109 = !DILocation(line: 31, column: 19, scope: !10)
!110 = !DILocation(line: 31, column: 11, scope: !10)
!111 = !DILocation(line: 32, column: 19, scope: !10)
!112 = !DILocation(line: 32, column: 11, scope: !10)
!113 = !DILocation(line: 33, column: 19, scope: !10)
!114 = !DILocation(line: 33, column: 11, scope: !10)
!115 = !DILocation(line: 34, column: 19, scope: !10)
!116 = !DILocation(line: 34, column: 11, scope: !10)
!117 = !DILocation(line: 35, column: 19, scope: !10)
!118 = !DILocation(line: 35, column: 11, scope: !10)
!119 = !DILocation(line: 36, column: 19, scope: !10)
!120 = !DILocation(line: 36, column: 11, scope: !10)
!121 = !DILocation(line: 37, column: 19, scope: !10)
!122 = !DILocation(line: 37, column: 11, scope: !10)
!123 = !DILocation(line: 38, column: 20, scope: !10)
!124 = !DILocation(line: 38, column: 12, scope: !10)
!125 = !DILocation(line: 39, column: 21, scope: !10)
!126 = !DILocation(line: 39, column: 12, scope: !10)
!127 = !DILocation(line: 40, column: 21, scope: !10)
!128 = !DILocation(line: 40, column: 12, scope: !10)
!129 = !DILocation(line: 41, column: 21, scope: !10)
!130 = !DILocation(line: 41, column: 12, scope: !10)
!131 = !DILocation(line: 42, column: 21, scope: !10)
!132 = !DILocation(line: 42, column: 12, scope: !10)
!133 = !DILocation(line: 43, column: 21, scope: !10)
!134 = !DILocation(line: 43, column: 12, scope: !10)
!135 = !DILocation(line: 44, column: 21, scope: !10)
!136 = !DILocation(line: 44, column: 12, scope: !10)
!137 = !DILocation(line: 45, column: 21, scope: !10)
!138 = !DILocation(line: 45, column: 12, scope: !10)
!139 = !DILocation(line: 46, column: 21, scope: !10)
!140 = !DILocation(line: 46, column: 12, scope: !10)
!141 = !DILocation(line: 47, column: 21, scope: !10)
!142 = !DILocation(line: 47, column: 12, scope: !10)
!143 = !DILocation(line: 48, column: 11, scope: !10)
!144 = !DILocation(line: 49, column: 19, scope: !10)
!145 = !DILocation(line: 49, column: 11, scope: !10)
!146 = !DILocation(line: 50, column: 19, scope: !10)
!147 = !DILocation(line: 50, column: 11, scope: !10)
!148 = !DILocation(line: 51, column: 19, scope: !10)
!149 = !DILocation(line: 51, column: 11, scope: !10)
!150 = !DILocation(line: 52, column: 19, scope: !10)
!151 = !DILocation(line: 52, column: 11, scope: !10)
!152 = !DILocation(line: 53, column: 19, scope: !10)
!153 = !DILocation(line: 53, column: 11, scope: !10)
!154 = !DILocation(line: 54, column: 19, scope: !10)
!155 = !DILocation(line: 54, column: 11, scope: !10)
!156 = !DILocation(line: 55, column: 19, scope: !10)
!157 = !DILocation(line: 55, column: 11, scope: !10)
!158 = !DILocation(line: 56, column: 19, scope: !10)
!159 = !DILocation(line: 56, column: 11, scope: !10)
!160 = !DILocation(line: 57, column: 20, scope: !10)
!161 = !DILocation(line: 57, column: 12, scope: !10)
!162 = !DILocation(line: 58, column: 21, scope: !10)
!163 = !DILocation(line: 58, column: 12, scope: !10)
!164 = !DILocation(line: 59, column: 21, scope: !10)
!165 = !DILocation(line: 59, column: 12, scope: !10)
!166 = !DILocation(line: 60, column: 21, scope: !10)
!167 = !DILocation(line: 60, column: 12, scope: !10)
!168 = !DILocation(line: 61, column: 21, scope: !10)
!169 = !DILocation(line: 61, column: 12, scope: !10)
!170 = !DILocation(line: 62, column: 21, scope: !10)
!171 = !DILocation(line: 62, column: 12, scope: !10)
!172 = !DILocation(line: 63, column: 21, scope: !10)
!173 = !DILocation(line: 63, column: 12, scope: !10)
!174 = !DILocation(line: 64, column: 21, scope: !10)
!175 = !DILocation(line: 64, column: 12, scope: !10)
!176 = !DILocation(line: 65, column: 21, scope: !10)
!177 = !DILocation(line: 65, column: 12, scope: !10)
!178 = !DILocation(line: 66, column: 21, scope: !10)
!179 = !DILocation(line: 66, column: 12, scope: !10)
!180 = !DILocation(line: 7, column: 25, scope: !10)
!181 = distinct !{!181, !14, !182, !183, !184}
!182 = !DILocation(line: 67, column: 2, scope: !10)
!183 = !{!"llvm.loop.mustprogress"}
!184 = !{!"llvm.loop.unroll.disable"}
