; RUN: llc < %s -mcpu=a64fx -O1 -fswp --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: hardware-loop created

; ModuleID = '2901-2-06.c'
source_filename = "2901-2-06.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@sum = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %7

4:                                                ; preds = %2
  %5 = load ptr, ptr %1, align 8, !tbaa !6
  %6 = zext i32 %0 to i64
  br label %48

7:                                                ; preds = %48, %2
  %8 = load i32, ptr @sum, align 4, !tbaa !10
  %9 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !10
  %10 = add nsw i32 %9, %8
  %11 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !10
  %12 = add nsw i32 %10, %11
  %13 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !10
  %14 = add nsw i32 %12, %13
  %15 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !10
  %16 = add nsw i32 %14, %15
  %17 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !10
  %18 = add nsw i32 %16, %17
  %19 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !10
  %20 = add nsw i32 %18, %19
  %21 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !10
  %22 = add nsw i32 %20, %21
  %23 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !10
  %24 = add nsw i32 %22, %23
  %25 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !10
  %26 = add nsw i32 %24, %25
  %27 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !10
  %28 = add nsw i32 %26, %27
  %29 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !10
  %30 = add nsw i32 %28, %29
  %31 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !10
  %32 = add nsw i32 %30, %31
  %33 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !10
  %34 = add nsw i32 %32, %33
  %35 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !10
  %36 = add nsw i32 %34, %35
  %37 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !10
  %38 = add nsw i32 %36, %37
  %39 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !10
  %40 = add nsw i32 %38, %39
  %41 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !10
  %42 = add nsw i32 %40, %41
  %43 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !10
  %44 = add nsw i32 %42, %43
  %45 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !10
  %46 = add nsw i32 %44, %45
  %47 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %46)
  ret i32 0

48:                                               ; preds = %4, %48
  %49 = phi i64 [ 0, %4 ], [ %209, %48 ]
  %50 = getelementptr inbounds i8, ptr %5, i64 %49
  %51 = load i8, ptr %50, align 1, !tbaa !12
  %52 = zext i8 %51 to i32
  %53 = load i32, ptr @sum, align 4, !tbaa !10
  %54 = add nsw i32 %53, %52
  store i32 %54, ptr @sum, align 4, !tbaa !10
  %55 = load i8, ptr %50, align 1, !tbaa !12
  %56 = zext i8 %55 to i32
  %57 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !10
  %58 = add nsw i32 %57, %56
  store i32 %58, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !10
  %59 = load i8, ptr %50, align 1, !tbaa !12
  %60 = zext i8 %59 to i32
  %61 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !10
  %62 = add nsw i32 %61, %60
  store i32 %62, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !10
  %63 = load i8, ptr %50, align 1, !tbaa !12
  %64 = zext i8 %63 to i32
  %65 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !10
  %66 = add nsw i32 %65, %64
  store i32 %66, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !10
  %67 = load i8, ptr %50, align 1, !tbaa !12
  %68 = zext i8 %67 to i32
  %69 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !10
  %70 = add nsw i32 %69, %68
  store i32 %70, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !10
  %71 = load i8, ptr %50, align 1, !tbaa !12
  %72 = zext i8 %71 to i32
  %73 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !10
  %74 = add nsw i32 %73, %72
  store i32 %74, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !10
  %75 = load i8, ptr %50, align 1, !tbaa !12
  %76 = zext i8 %75 to i32
  %77 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !10
  %78 = add nsw i32 %77, %76
  store i32 %78, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !10
  %79 = load i8, ptr %50, align 1, !tbaa !12
  %80 = zext i8 %79 to i32
  %81 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !10
  %82 = add nsw i32 %81, %80
  store i32 %82, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !10
  %83 = load i8, ptr %50, align 1, !tbaa !12
  %84 = zext i8 %83 to i32
  %85 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !10
  %86 = add nsw i32 %85, %84
  store i32 %86, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !10
  %87 = load i8, ptr %50, align 1, !tbaa !12
  %88 = zext i8 %87 to i32
  %89 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !10
  %90 = add nsw i32 %89, %88
  store i32 %90, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !10
  %91 = load i8, ptr %50, align 1, !tbaa !12
  %92 = zext i8 %91 to i32
  %93 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !10
  %94 = add nsw i32 %93, %92
  store i32 %94, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !10
  %95 = load i8, ptr %50, align 1, !tbaa !12
  %96 = zext i8 %95 to i32
  %97 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !10
  %98 = add nsw i32 %97, %96
  store i32 %98, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !10
  %99 = load i8, ptr %50, align 1, !tbaa !12
  %100 = zext i8 %99 to i32
  %101 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !10
  %102 = add nsw i32 %101, %100
  store i32 %102, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !10
  %103 = load i8, ptr %50, align 1, !tbaa !12
  %104 = zext i8 %103 to i32
  %105 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !10
  %106 = add nsw i32 %105, %104
  store i32 %106, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !10
  %107 = load i8, ptr %50, align 1, !tbaa !12
  %108 = zext i8 %107 to i32
  %109 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !10
  %110 = add nsw i32 %109, %108
  store i32 %110, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !10
  %111 = load i8, ptr %50, align 1, !tbaa !12
  %112 = zext i8 %111 to i32
  %113 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !10
  %114 = add nsw i32 %113, %112
  store i32 %114, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !10
  %115 = load i8, ptr %50, align 1, !tbaa !12
  %116 = zext i8 %115 to i32
  %117 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !10
  %118 = add nsw i32 %117, %116
  store i32 %118, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !10
  %119 = load i8, ptr %50, align 1, !tbaa !12
  %120 = zext i8 %119 to i32
  %121 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !10
  %122 = add nsw i32 %121, %120
  store i32 %122, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !10
  %123 = load i8, ptr %50, align 1, !tbaa !12
  %124 = zext i8 %123 to i32
  %125 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !10
  %126 = add nsw i32 %125, %124
  store i32 %126, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !10
  %127 = load i8, ptr %50, align 1, !tbaa !12
  %128 = zext i8 %127 to i32
  %129 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !10
  %130 = add nsw i32 %129, %128
  store i32 %130, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !10
  %131 = load i8, ptr %50, align 1, !tbaa !12
  %132 = zext i8 %131 to i32
  %133 = add nsw i32 %54, %132
  store i32 %133, ptr @sum, align 4, !tbaa !10
  %134 = add nsw i32 %133, -1
  %135 = add nsw i32 %134, %58
  store i32 %135, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !10
  %136 = add i32 %62, -2
  %137 = add i32 %136, %135
  store i32 %137, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !10
  %138 = add i32 %66, -3
  %139 = add i32 %138, %137
  store i32 %139, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !10
  %140 = add i32 %70, -4
  %141 = add i32 %140, %139
  store i32 %141, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !10
  %142 = add i32 %74, -5
  %143 = add i32 %142, %141
  store i32 %143, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !10
  %144 = add i32 %78, -6
  %145 = add i32 %144, %143
  store i32 %145, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !10
  %146 = add i32 %82, -7
  %147 = add i32 %146, %145
  store i32 %147, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !10
  %148 = add i32 %86, -8
  %149 = add i32 %148, %147
  store i32 %149, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !10
  %150 = add i32 %90, -9
  %151 = add i32 %150, %149
  store i32 %151, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !10
  %152 = add i32 %94, -10
  %153 = add i32 %152, %151
  store i32 %153, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !10
  %154 = add i32 %98, -2
  %155 = add i32 %154, %153
  store i32 %155, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !10
  %156 = add nsw i32 %155, -2
  %157 = add nsw i32 %156, %102
  store i32 %157, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !10
  %158 = add i32 %106, -3
  %159 = add i32 %158, %157
  store i32 %159, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !10
  %160 = add i32 %110, -4
  %161 = add i32 %160, %159
  store i32 %161, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !10
  %162 = add i32 %114, -5
  %163 = add i32 %162, %161
  store i32 %163, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !10
  %164 = add i32 %118, -6
  %165 = add i32 %164, %163
  store i32 %165, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !10
  %166 = add i32 %122, -7
  %167 = add i32 %166, %165
  store i32 %167, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !10
  %168 = add i32 %126, -8
  %169 = add i32 %168, %167
  store i32 %169, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !10
  %170 = add i32 %130, -9
  %171 = add i32 %170, %169
  store i32 %171, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !10
  %172 = add nsw i32 %135, %134
  store i32 %172, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !10
  %173 = add nsw i32 %172, -2
  %174 = add nsw i32 %173, %137
  store i32 %174, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !10
  %175 = add nsw i32 %174, -3
  %176 = add nsw i32 %175, %139
  store i32 %176, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !10
  %177 = add nsw i32 %176, -4
  %178 = add nsw i32 %177, %141
  store i32 %178, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !10
  %179 = add nsw i32 %178, -5
  %180 = add nsw i32 %179, %143
  store i32 %180, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !10
  %181 = add nsw i32 %180, -6
  %182 = add nsw i32 %181, %145
  store i32 %182, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !10
  %183 = add nsw i32 %182, -7
  %184 = add nsw i32 %183, %147
  store i32 %184, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !10
  %185 = add nsw i32 %184, -8
  %186 = add nsw i32 %185, %149
  store i32 %186, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !10
  %187 = add nsw i32 %186, -9
  %188 = add nsw i32 %187, %151
  store i32 %188, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !10
  %189 = add nsw i32 %188, -10
  %190 = add nsw i32 %189, %153
  store i32 %190, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !10
  %191 = add i32 %155, -2
  %192 = add i32 %191, %190
  store i32 %192, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !10
  %193 = add nsw i32 %192, -2
  %194 = add nsw i32 %193, %157
  store i32 %194, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !10
  %195 = add nsw i32 %194, -3
  %196 = add nsw i32 %195, %159
  store i32 %196, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !10
  %197 = add nsw i32 %196, -4
  %198 = add nsw i32 %197, %161
  store i32 %198, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !10
  %199 = add nsw i32 %198, -5
  %200 = add nsw i32 %199, %163
  store i32 %200, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !10
  %201 = add nsw i32 %200, -6
  %202 = add nsw i32 %201, %165
  store i32 %202, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !10
  %203 = add nsw i32 %202, -7
  %204 = add nsw i32 %203, %167
  store i32 %204, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !10
  %205 = add nsw i32 %204, -8
  %206 = add nsw i32 %205, %169
  store i32 %206, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !10
  %207 = add nsw i32 %206, -9
  %208 = add nsw i32 %207, %171
  store i32 %208, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !10
  %209 = add nuw nsw i64 %49, 1
  %210 = icmp eq i64 %209, %6
  br i1 %210, label %7, label %48, !llvm.loop !13
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { nofree nounwind uwtable vscale_range(1,16) "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }
attributes #1 = { nofree nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+crypto,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.2a" }

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
!10 = !{!11, !11, i64 0}
!11 = !{!"int", !8, i64 0}
!12 = !{!8, !8, i64 0}
!13 = distinct !{!13, !14, !15}
!14 = !{!"llvm.loop.mustprogress"}
!15 = !{!"llvm.loop.unroll.disable"}
