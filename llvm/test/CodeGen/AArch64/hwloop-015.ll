; RUN: llc < %s -mcpu=a64fx -O1 -fswp -hwloop-max-size=10 --pass-remarks-filter=hardware-loops --pass-remarks-output=-  -debug-aarch64tti -o /dev/null 2>&1 | FileCheck %s
; CHECK: (reason=Instructions over the limit)
; ModuleID = '2901-2-15.c'
source_filename = "2901-2-15.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux"

@sum = dso_local local_unnamed_addr global [100 x i32] zeroinitializer, align 4
@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: nofree nounwind uwtable vscale_range(1,16)
define dso_local i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #0 {
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %6

4:                                                ; preds = %2
  %5 = zext i32 %0 to i64
  br label %47

6:                                                ; preds = %47, %2
  %7 = load i32, ptr @sum, align 4, !tbaa !6
  %8 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %9 = add nsw i32 %8, %7
  %10 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %11 = add nsw i32 %9, %10
  %12 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %13 = add nsw i32 %11, %12
  %14 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %15 = add nsw i32 %13, %14
  %16 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %17 = add nsw i32 %15, %16
  %18 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %19 = add nsw i32 %17, %18
  %20 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %21 = add nsw i32 %19, %20
  %22 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %23 = add nsw i32 %21, %22
  %24 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %25 = add nsw i32 %23, %24
  %26 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %27 = add nsw i32 %25, %26
  %28 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %29 = add nsw i32 %27, %28
  %30 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %31 = add nsw i32 %29, %30
  %32 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %33 = add nsw i32 %31, %32
  %34 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %35 = add nsw i32 %33, %34
  %36 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %37 = add nsw i32 %35, %36
  %38 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %39 = add nsw i32 %37, %38
  %40 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %41 = add nsw i32 %39, %40
  %42 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %43 = add nsw i32 %41, %42
  %44 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %45 = add nsw i32 %43, %44
  %46 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %45)
  ret i32 0

47:                                               ; preds = %4, %47
  %48 = phi i64 [ 0, %4 ], [ %514, %47 ]
  %49 = load ptr, ptr %1, align 8, !tbaa !10
  %50 = getelementptr inbounds i8, ptr %49, i64 %48
  %51 = load i8, ptr %50, align 1, !tbaa !12
  %52 = zext i8 %51 to i32
  %53 = trunc i64 %48 to i32
  %54 = mul nsw i32 %53, %52
  %55 = load i32, ptr @sum, align 4, !tbaa !6
  %56 = add nsw i32 %54, %55
  store i32 %56, ptr @sum, align 4, !tbaa !6
  %57 = load i8, ptr %50, align 1, !tbaa !12
  %58 = zext i8 %57 to i32
  %59 = add nuw nsw i32 %58, 1
  %60 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %61 = add nsw i32 %59, %60
  store i32 %61, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %62 = load i8, ptr %50, align 1, !tbaa !12
  %63 = zext i8 %62 to i32
  %64 = add nuw nsw i32 %63, 2
  %65 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %66 = add nsw i32 %64, %65
  store i32 %66, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %67 = load i8, ptr %50, align 1, !tbaa !12
  %68 = zext i8 %67 to i32
  %69 = add nuw nsw i32 %68, 3
  %70 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %71 = add nsw i32 %69, %70
  store i32 %71, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %72 = load i8, ptr %50, align 1, !tbaa !12
  %73 = zext i8 %72 to i32
  %74 = add nuw nsw i32 %73, 4
  %75 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %76 = add nsw i32 %74, %75
  store i32 %76, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %77 = load i8, ptr %50, align 1, !tbaa !12
  %78 = zext i8 %77 to i32
  %79 = add nuw nsw i32 %78, 5
  %80 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %81 = add nsw i32 %79, %80
  store i32 %81, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %82 = load i8, ptr %50, align 1, !tbaa !12
  %83 = zext i8 %82 to i32
  %84 = add nuw nsw i32 %83, 6
  %85 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %86 = add nsw i32 %84, %85
  store i32 %86, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %87 = load i8, ptr %50, align 1, !tbaa !12
  %88 = zext i8 %87 to i32
  %89 = add nuw nsw i32 %88, 7
  %90 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %91 = add nsw i32 %89, %90
  store i32 %91, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %92 = load i8, ptr %50, align 1, !tbaa !12
  %93 = zext i8 %92 to i32
  %94 = add nuw nsw i32 %93, 8
  %95 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %96 = add nsw i32 %94, %95
  store i32 %96, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %97 = load i8, ptr %50, align 1, !tbaa !12
  %98 = zext i8 %97 to i32
  %99 = add nuw nsw i32 %98, 9
  %100 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %101 = add nsw i32 %99, %100
  store i32 %101, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %102 = load i8, ptr %50, align 1, !tbaa !12
  %103 = zext i8 %102 to i32
  %104 = add nuw nsw i32 %103, 10
  %105 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %106 = add nsw i32 %104, %105
  store i32 %106, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %107 = load i8, ptr %50, align 1, !tbaa !12
  %108 = zext i8 %107 to i32
  %109 = add nuw nsw i32 %108, 2
  %110 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %111 = add nsw i32 %109, %110
  store i32 %111, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %112 = load i8, ptr %50, align 1, !tbaa !12
  %113 = zext i8 %112 to i32
  %114 = add nuw nsw i32 %113, 2
  %115 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %116 = add nsw i32 %114, %115
  store i32 %116, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %117 = load i8, ptr %50, align 1, !tbaa !12
  %118 = zext i8 %117 to i32
  %119 = add nuw nsw i32 %118, 3
  %120 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %121 = add nsw i32 %119, %120
  store i32 %121, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %122 = load i8, ptr %50, align 1, !tbaa !12
  %123 = zext i8 %122 to i32
  %124 = add nuw nsw i32 %123, 4
  %125 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %126 = add nsw i32 %124, %125
  store i32 %126, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %127 = load i8, ptr %50, align 1, !tbaa !12
  %128 = zext i8 %127 to i32
  %129 = add nuw nsw i32 %128, 5
  %130 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %131 = add nsw i32 %129, %130
  store i32 %131, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %132 = load i8, ptr %50, align 1, !tbaa !12
  %133 = zext i8 %132 to i32
  %134 = add nuw nsw i32 %133, 6
  %135 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %136 = add nsw i32 %134, %135
  store i32 %136, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %137 = load i8, ptr %50, align 1, !tbaa !12
  %138 = zext i8 %137 to i32
  %139 = add nuw nsw i32 %138, 7
  %140 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %141 = add nsw i32 %139, %140
  store i32 %141, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %142 = load i8, ptr %50, align 1, !tbaa !12
  %143 = zext i8 %142 to i32
  %144 = add nuw nsw i32 %143, 8
  %145 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %146 = add nsw i32 %144, %145
  store i32 %146, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %147 = load i8, ptr %50, align 1, !tbaa !12
  %148 = zext i8 %147 to i32
  %149 = add nuw nsw i32 %148, 9
  %150 = load i32, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %151 = add nsw i32 %149, %150
  store i32 %151, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %152 = load i8, ptr %50, align 1, !tbaa !12
  %153 = zext i8 %152 to i32
  %154 = trunc i64 %48 to i32
  %155 = mul nsw i32 %154, %153
  %156 = add nsw i32 %155, %56
  store i32 %156, ptr @sum, align 4, !tbaa !6
  %157 = load i8, ptr %50, align 1, !tbaa !12
  %158 = zext i8 %157 to i32
  %159 = add i32 %61, 1
  %160 = sub i32 %159, %158
  %161 = add i32 %160, %156
  store i32 %161, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %162 = load i8, ptr %50, align 1, !tbaa !12
  %163 = zext i8 %162 to i32
  %164 = add i32 %66, 2
  %165 = sub i32 %164, %163
  %166 = add i32 %165, %161
  store i32 %166, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %167 = load i8, ptr %50, align 1, !tbaa !12
  %168 = zext i8 %167 to i32
  %169 = add i32 %71, 3
  %170 = sub i32 %169, %168
  %171 = add i32 %170, %166
  store i32 %171, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %172 = load i8, ptr %50, align 1, !tbaa !12
  %173 = zext i8 %172 to i32
  %174 = add i32 %76, 4
  %175 = sub i32 %174, %173
  %176 = add i32 %175, %171
  store i32 %176, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %177 = load i8, ptr %50, align 1, !tbaa !12
  %178 = zext i8 %177 to i32
  %179 = add i32 %81, 5
  %180 = sub i32 %179, %178
  %181 = add i32 %180, %176
  store i32 %181, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %182 = load i8, ptr %50, align 1, !tbaa !12
  %183 = zext i8 %182 to i32
  %184 = add i32 %86, 6
  %185 = sub i32 %184, %183
  %186 = add i32 %185, %181
  store i32 %186, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %187 = load i8, ptr %50, align 1, !tbaa !12
  %188 = zext i8 %187 to i32
  %189 = add i32 %91, 7
  %190 = sub i32 %189, %188
  %191 = add i32 %190, %186
  store i32 %191, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %192 = load i8, ptr %50, align 1, !tbaa !12
  %193 = zext i8 %192 to i32
  %194 = add i32 %96, 8
  %195 = sub i32 %194, %193
  %196 = add i32 %195, %191
  store i32 %196, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %197 = load i8, ptr %50, align 1, !tbaa !12
  %198 = zext i8 %197 to i32
  %199 = add i32 %101, 9
  %200 = sub i32 %199, %198
  %201 = add i32 %200, %196
  store i32 %201, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %202 = load i8, ptr %50, align 1, !tbaa !12
  %203 = zext i8 %202 to i32
  %204 = add i32 %106, 10
  %205 = sub i32 %204, %203
  %206 = add i32 %205, %201
  store i32 %206, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %207 = load i8, ptr %50, align 1, !tbaa !12
  %208 = zext i8 %207 to i32
  %209 = add i32 %111, 2
  %210 = sub i32 %209, %208
  %211 = add i32 %210, %206
  store i32 %211, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %212 = load i8, ptr %50, align 1, !tbaa !12
  %213 = zext i8 %212 to i32
  %214 = add i32 %116, 2
  %215 = sub i32 %214, %213
  %216 = add i32 %215, %211
  store i32 %216, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %217 = load i8, ptr %50, align 1, !tbaa !12
  %218 = zext i8 %217 to i32
  %219 = add i32 %121, 3
  %220 = sub i32 %219, %218
  %221 = add i32 %220, %216
  store i32 %221, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %222 = load i8, ptr %50, align 1, !tbaa !12
  %223 = zext i8 %222 to i32
  %224 = add i32 %126, 4
  %225 = sub i32 %224, %223
  %226 = add i32 %225, %221
  store i32 %226, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %227 = load i8, ptr %50, align 1, !tbaa !12
  %228 = zext i8 %227 to i32
  %229 = add i32 %131, 5
  %230 = sub i32 %229, %228
  %231 = add i32 %230, %226
  store i32 %231, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %232 = load i8, ptr %50, align 1, !tbaa !12
  %233 = zext i8 %232 to i32
  %234 = add i32 %136, 6
  %235 = sub i32 %234, %233
  %236 = add i32 %235, %231
  store i32 %236, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %237 = load i8, ptr %50, align 1, !tbaa !12
  %238 = zext i8 %237 to i32
  %239 = add i32 %141, 7
  %240 = sub i32 %239, %238
  %241 = add i32 %240, %236
  store i32 %241, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %242 = load i8, ptr %50, align 1, !tbaa !12
  %243 = zext i8 %242 to i32
  %244 = add i32 %146, 8
  %245 = sub i32 %244, %243
  %246 = add i32 %245, %241
  store i32 %246, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %247 = load i8, ptr %50, align 1, !tbaa !12
  %248 = zext i8 %247 to i32
  %249 = add i32 %151, 9
  %250 = sub i32 %249, %248
  %251 = add i32 %250, %246
  store i32 %251, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %252 = load i8, ptr %50, align 1, !tbaa !12
  %253 = zext i8 %252 to i32
  %254 = trunc i64 %48 to i32
  %255 = mul nsw i32 %254, %253
  %256 = add nsw i32 %255, %156
  store i32 %256, ptr @sum, align 4, !tbaa !6
  %257 = load i8, ptr %50, align 1, !tbaa !12
  %258 = zext i8 %257 to i32
  %259 = add i32 %161, 1
  %260 = add i32 %259, %258
  store i32 %260, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %261 = load i8, ptr %50, align 1, !tbaa !12
  %262 = zext i8 %261 to i32
  %263 = add i32 %166, 2
  %264 = add i32 %263, %262
  store i32 %264, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %265 = load i8, ptr %50, align 1, !tbaa !12
  %266 = zext i8 %265 to i32
  %267 = add i32 %171, 3
  %268 = add i32 %267, %266
  store i32 %268, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %269 = load i8, ptr %50, align 1, !tbaa !12
  %270 = zext i8 %269 to i32
  %271 = add i32 %176, 4
  %272 = add i32 %271, %270
  store i32 %272, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %273 = load i8, ptr %50, align 1, !tbaa !12
  %274 = zext i8 %273 to i32
  %275 = add i32 %181, 5
  %276 = add i32 %275, %274
  store i32 %276, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %277 = load i8, ptr %50, align 1, !tbaa !12
  %278 = zext i8 %277 to i32
  %279 = add i32 %186, 6
  %280 = add i32 %279, %278
  store i32 %280, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %281 = load i8, ptr %50, align 1, !tbaa !12
  %282 = zext i8 %281 to i32
  %283 = add i32 %191, 7
  %284 = add i32 %283, %282
  store i32 %284, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %285 = load i8, ptr %50, align 1, !tbaa !12
  %286 = zext i8 %285 to i32
  %287 = add i32 %196, 8
  %288 = add i32 %287, %286
  store i32 %288, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %289 = load i8, ptr %50, align 1, !tbaa !12
  %290 = zext i8 %289 to i32
  %291 = add i32 %201, 9
  %292 = add i32 %291, %290
  store i32 %292, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %293 = load i8, ptr %50, align 1, !tbaa !12
  %294 = zext i8 %293 to i32
  %295 = add i32 %206, 10
  %296 = add i32 %295, %294
  store i32 %296, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %297 = load i8, ptr %50, align 1, !tbaa !12
  %298 = zext i8 %297 to i32
  %299 = add i32 %211, 2
  %300 = add i32 %299, %298
  store i32 %300, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %301 = load i8, ptr %50, align 1, !tbaa !12
  %302 = zext i8 %301 to i32
  %303 = add i32 %216, 2
  %304 = add i32 %303, %302
  store i32 %304, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %305 = load i8, ptr %50, align 1, !tbaa !12
  %306 = zext i8 %305 to i32
  %307 = add i32 %221, 3
  %308 = add i32 %307, %306
  store i32 %308, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %309 = load i8, ptr %50, align 1, !tbaa !12
  %310 = zext i8 %309 to i32
  %311 = add i32 %226, 4
  %312 = add i32 %311, %310
  store i32 %312, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %313 = load i8, ptr %50, align 1, !tbaa !12
  %314 = zext i8 %313 to i32
  %315 = add i32 %231, 5
  %316 = add i32 %315, %314
  store i32 %316, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %317 = load i8, ptr %50, align 1, !tbaa !12
  %318 = zext i8 %317 to i32
  %319 = add i32 %236, 6
  %320 = add i32 %319, %318
  store i32 %320, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %321 = load i8, ptr %50, align 1, !tbaa !12
  %322 = zext i8 %321 to i32
  %323 = add i32 %241, 7
  %324 = add i32 %323, %322
  store i32 %324, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %325 = load i8, ptr %50, align 1, !tbaa !12
  %326 = zext i8 %325 to i32
  %327 = add i32 %246, 8
  %328 = add i32 %327, %326
  store i32 %328, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %329 = load i8, ptr %50, align 1, !tbaa !12
  %330 = zext i8 %329 to i32
  %331 = add i32 %251, 9
  %332 = add i32 %331, %330
  store i32 %332, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %333 = load i8, ptr %50, align 1, !tbaa !12
  %334 = zext i8 %333 to i32
  %335 = trunc i64 %48 to i32
  %336 = mul nsw i32 %335, %334
  %337 = add nsw i32 %336, %256
  store i32 %337, ptr @sum, align 4, !tbaa !6
  %338 = load i8, ptr %50, align 1, !tbaa !12
  %339 = zext i8 %338 to i32
  %340 = add i32 %260, 1
  %341 = sub i32 %340, %339
  %342 = add i32 %341, %337
  store i32 %342, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %343 = load i8, ptr %50, align 1, !tbaa !12
  %344 = zext i8 %343 to i32
  %345 = add i32 %264, 2
  %346 = sub i32 %345, %344
  %347 = add i32 %346, %342
  store i32 %347, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %348 = load i8, ptr %50, align 1, !tbaa !12
  %349 = zext i8 %348 to i32
  %350 = add i32 %268, 3
  %351 = sub i32 %350, %349
  %352 = add i32 %351, %347
  store i32 %352, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %353 = load i8, ptr %50, align 1, !tbaa !12
  %354 = zext i8 %353 to i32
  %355 = add i32 %272, 4
  %356 = sub i32 %355, %354
  %357 = add i32 %356, %352
  store i32 %357, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %358 = load i8, ptr %50, align 1, !tbaa !12
  %359 = zext i8 %358 to i32
  %360 = add i32 %276, 5
  %361 = sub i32 %360, %359
  %362 = add i32 %361, %357
  store i32 %362, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %363 = load i8, ptr %50, align 1, !tbaa !12
  %364 = zext i8 %363 to i32
  %365 = add i32 %280, 6
  %366 = sub i32 %365, %364
  %367 = add i32 %366, %362
  store i32 %367, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %368 = load i8, ptr %50, align 1, !tbaa !12
  %369 = zext i8 %368 to i32
  %370 = add i32 %284, 7
  %371 = sub i32 %370, %369
  %372 = add i32 %371, %367
  store i32 %372, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %373 = load i8, ptr %50, align 1, !tbaa !12
  %374 = zext i8 %373 to i32
  %375 = add i32 %288, 8
  %376 = sub i32 %375, %374
  %377 = add i32 %376, %372
  store i32 %377, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %378 = load i8, ptr %50, align 1, !tbaa !12
  %379 = zext i8 %378 to i32
  %380 = add i32 %292, 9
  %381 = sub i32 %380, %379
  %382 = add i32 %381, %377
  store i32 %382, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %383 = load i8, ptr %50, align 1, !tbaa !12
  %384 = zext i8 %383 to i32
  %385 = add i32 %296, 10
  %386 = sub i32 %385, %384
  %387 = add i32 %386, %382
  store i32 %387, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %388 = load i8, ptr %50, align 1, !tbaa !12
  %389 = zext i8 %388 to i32
  %390 = add i32 %300, 2
  %391 = sub i32 %390, %389
  %392 = add i32 %391, %387
  store i32 %392, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %393 = load i8, ptr %50, align 1, !tbaa !12
  %394 = zext i8 %393 to i32
  %395 = add i32 %304, 2
  %396 = sub i32 %395, %394
  %397 = add i32 %396, %392
  store i32 %397, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %398 = load i8, ptr %50, align 1, !tbaa !12
  %399 = zext i8 %398 to i32
  %400 = add i32 %308, 3
  %401 = sub i32 %400, %399
  %402 = add i32 %401, %397
  store i32 %402, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %403 = load i8, ptr %50, align 1, !tbaa !12
  %404 = zext i8 %403 to i32
  %405 = add i32 %312, 4
  %406 = sub i32 %405, %404
  %407 = add i32 %406, %402
  store i32 %407, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %408 = load i8, ptr %50, align 1, !tbaa !12
  %409 = zext i8 %408 to i32
  %410 = add i32 %316, 5
  %411 = sub i32 %410, %409
  %412 = add i32 %411, %407
  store i32 %412, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %413 = load i8, ptr %50, align 1, !tbaa !12
  %414 = zext i8 %413 to i32
  %415 = add i32 %320, 6
  %416 = sub i32 %415, %414
  %417 = add i32 %416, %412
  store i32 %417, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %418 = load i8, ptr %50, align 1, !tbaa !12
  %419 = zext i8 %418 to i32
  %420 = add i32 %324, 7
  %421 = sub i32 %420, %419
  %422 = add i32 %421, %417
  store i32 %422, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %423 = load i8, ptr %50, align 1, !tbaa !12
  %424 = zext i8 %423 to i32
  %425 = add i32 %328, 8
  %426 = sub i32 %425, %424
  %427 = add i32 %426, %422
  store i32 %427, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %428 = load i8, ptr %50, align 1, !tbaa !12
  %429 = zext i8 %428 to i32
  %430 = add i32 %332, 9
  %431 = sub i32 %430, %429
  %432 = add i32 %431, %427
  store i32 %432, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %433 = load i8, ptr %50, align 1, !tbaa !12
  %434 = zext i8 %433 to i32
  %435 = trunc i64 %48 to i32
  %436 = mul nsw i32 %435, %434
  %437 = add nsw i32 %436, %337
  store i32 %437, ptr @sum, align 4, !tbaa !6
  %438 = load i8, ptr %50, align 1, !tbaa !12
  %439 = zext i8 %438 to i32
  %440 = add i32 %342, 1
  %441 = add i32 %440, %439
  store i32 %441, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 1), align 4, !tbaa !6
  %442 = load i8, ptr %50, align 1, !tbaa !12
  %443 = zext i8 %442 to i32
  %444 = add i32 %347, 2
  %445 = add i32 %444, %443
  store i32 %445, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 2), align 4, !tbaa !6
  %446 = load i8, ptr %50, align 1, !tbaa !12
  %447 = zext i8 %446 to i32
  %448 = add i32 %352, 3
  %449 = add i32 %448, %447
  store i32 %449, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 3), align 4, !tbaa !6
  %450 = load i8, ptr %50, align 1, !tbaa !12
  %451 = zext i8 %450 to i32
  %452 = add i32 %357, 4
  %453 = add i32 %452, %451
  store i32 %453, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 4), align 4, !tbaa !6
  %454 = load i8, ptr %50, align 1, !tbaa !12
  %455 = zext i8 %454 to i32
  %456 = add i32 %362, 5
  %457 = add i32 %456, %455
  store i32 %457, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 5), align 4, !tbaa !6
  %458 = load i8, ptr %50, align 1, !tbaa !12
  %459 = zext i8 %458 to i32
  %460 = add i32 %367, 6
  %461 = add i32 %460, %459
  store i32 %461, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 6), align 4, !tbaa !6
  %462 = load i8, ptr %50, align 1, !tbaa !12
  %463 = zext i8 %462 to i32
  %464 = add i32 %372, 7
  %465 = add i32 %464, %463
  store i32 %465, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 7), align 4, !tbaa !6
  %466 = load i8, ptr %50, align 1, !tbaa !12
  %467 = zext i8 %466 to i32
  %468 = add i32 %377, 8
  %469 = add i32 %468, %467
  store i32 %469, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 8), align 4, !tbaa !6
  %470 = load i8, ptr %50, align 1, !tbaa !12
  %471 = zext i8 %470 to i32
  %472 = add i32 %382, 9
  %473 = add i32 %472, %471
  store i32 %473, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 9), align 4, !tbaa !6
  %474 = load i8, ptr %50, align 1, !tbaa !12
  %475 = zext i8 %474 to i32
  %476 = add i32 %387, 10
  %477 = add i32 %476, %475
  store i32 %477, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 10), align 4, !tbaa !6
  %478 = load i8, ptr %50, align 1, !tbaa !12
  %479 = zext i8 %478 to i32
  %480 = add i32 %392, 2
  %481 = add i32 %480, %479
  store i32 %481, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 11), align 4, !tbaa !6
  %482 = load i8, ptr %50, align 1, !tbaa !12
  %483 = zext i8 %482 to i32
  %484 = add i32 %397, 2
  %485 = add i32 %484, %483
  store i32 %485, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 12), align 4, !tbaa !6
  %486 = load i8, ptr %50, align 1, !tbaa !12
  %487 = zext i8 %486 to i32
  %488 = add i32 %402, 3
  %489 = add i32 %488, %487
  store i32 %489, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 13), align 4, !tbaa !6
  %490 = load i8, ptr %50, align 1, !tbaa !12
  %491 = zext i8 %490 to i32
  %492 = add i32 %407, 4
  %493 = add i32 %492, %491
  store i32 %493, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 14), align 4, !tbaa !6
  %494 = load i8, ptr %50, align 1, !tbaa !12
  %495 = zext i8 %494 to i32
  %496 = add i32 %412, 5
  %497 = add i32 %496, %495
  store i32 %497, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 15), align 4, !tbaa !6
  %498 = load i8, ptr %50, align 1, !tbaa !12
  %499 = zext i8 %498 to i32
  %500 = add i32 %417, 6
  %501 = add i32 %500, %499
  store i32 %501, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 16), align 4, !tbaa !6
  %502 = load i8, ptr %50, align 1, !tbaa !12
  %503 = zext i8 %502 to i32
  %504 = add i32 %422, 7
  %505 = add i32 %504, %503
  store i32 %505, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 17), align 4, !tbaa !6
  %506 = load i8, ptr %50, align 1, !tbaa !12
  %507 = zext i8 %506 to i32
  %508 = add i32 %427, 8
  %509 = add i32 %508, %507
  store i32 %509, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 18), align 4, !tbaa !6
  %510 = load i8, ptr %50, align 1, !tbaa !12
  %511 = zext i8 %510 to i32
  %512 = add i32 %432, 9
  %513 = add i32 %512, %511
  store i32 %513, ptr getelementptr inbounds ([100 x i32], ptr @sum, i64 0, i64 19), align 4, !tbaa !6
  %514 = add nuw nsw i64 %48, 1
  %515 = icmp eq i64 %514, %5
  br i1 %515, label %6, label %47, !llvm.loop !13
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
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!11, !11, i64 0}
!11 = !{!"any pointer", !8, i64 0}
!12 = !{!8, !8, i64 0}
!13 = distinct !{!13, !14, !15}
!14 = !{!"llvm.loop.mustprogress"}
!15 = !{!"llvm.loop.unroll.disable"}
