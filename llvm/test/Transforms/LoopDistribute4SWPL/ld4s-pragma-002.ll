; RUN: opt -O1 -S -mcpu=a64fx -pass-remarks=loop-distribute4swpl -pass-remarks-missed=loop-distribute4swpl -pass-remarks-analysis=loop-distribute4swpl -distribute4swpl-limit-freg=1 -distribute4swpl-limit-ireg=1 -o /dev/null < %s |& FileCheck %s

; CHECK: distributed loop. num of distributied is 2.

; ModuleID = './ls4s-pragma-002.c'
source_filename = "./ls4s-pragma-002.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable vscale_range(4,4)
define dso_local float @foo(i32 noundef %n, ptr noundef %A, ptr noundef %B, ptr noundef %C, ptr noundef %D, ptr noundef %E, ptr noundef %C1, ptr noundef %D1, ptr noundef %E1) #0 !dbg !9 {
entry:
  %n.addr = alloca i32, align 4
  %A.addr = alloca ptr, align 8
  %B.addr = alloca ptr, align 8
  %C.addr = alloca ptr, align 8
  %D.addr = alloca ptr, align 8
  %E.addr = alloca ptr, align 8
  %C1.addr = alloca ptr, align 8
  %D1.addr = alloca ptr, align 8
  %E1.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %z = alloca float, align 4
  store i32 %n, ptr %n.addr, align 4
  store ptr %A, ptr %A.addr, align 8
  store ptr %B, ptr %B.addr, align 8
  store ptr %C, ptr %C.addr, align 8
  store ptr %D, ptr %D.addr, align 8
  store ptr %E, ptr %E.addr, align 8
  store ptr %C1, ptr %C1.addr, align 8
  store ptr %D1, ptr %D1.addr, align 8
  store ptr %E1, ptr %E1.addr, align 8
  store float 0.000000e+00, ptr %z, align 4, !dbg !13
  store i32 0, ptr %i, align 4, !dbg !14
  br label %for.cond, !dbg !15

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4, !dbg !16
  %1 = load i32, ptr %n.addr, align 4, !dbg !17
  %cmp = icmp slt i32 %0, %1, !dbg !18
  br i1 %cmp, label %for.body, label %for.end, !dbg !19

for.body:                                         ; preds = %for.cond
  %2 = load ptr, ptr %A.addr, align 8, !dbg !20
  %3 = load i32, ptr %i, align 4, !dbg !21
  %idxprom = sext i32 %3 to i64, !dbg !20
  %arrayidx = getelementptr inbounds float, ptr %2, i64 %idxprom, !dbg !20
  %4 = load float, ptr %arrayidx, align 4, !dbg !20
  %5 = load ptr, ptr %B.addr, align 8, !dbg !22
  %6 = load i32, ptr %i, align 4, !dbg !23
  %idxprom1 = sext i32 %6 to i64, !dbg !22
  %arrayidx2 = getelementptr inbounds float, ptr %5, i64 %idxprom1, !dbg !22
  %7 = load float, ptr %arrayidx2, align 4, !dbg !22
  %8 = load ptr, ptr %C1.addr, align 8, !dbg !24
  %9 = load i32, ptr %i, align 4, !dbg !25
  %idxprom3 = sext i32 %9 to i64, !dbg !24
  %arrayidx4 = getelementptr inbounds float, ptr %8, i64 %idxprom3, !dbg !24
  %10 = load float, ptr %arrayidx4, align 4, !dbg !24
  %11 = call float @llvm.fmuladd.f32(float %4, float %7, float %10), !dbg !26
  %12 = load ptr, ptr %A.addr, align 8, !dbg !27
  %13 = load i32, ptr %i, align 4, !dbg !28
  %add = add nsw i32 %13, 1, !dbg !29
  %idxprom5 = sext i32 %add to i64, !dbg !27
  %arrayidx6 = getelementptr inbounds float, ptr %12, i64 %idxprom5, !dbg !27
  store float %11, ptr %arrayidx6, align 4, !dbg !30
  %14 = load i32, ptr %n.addr, align 4, !dbg !31
  %15 = load i32, ptr %i, align 4, !dbg !32
  %add7 = add nsw i32 %14, %15, !dbg !33
  %conv = sitofp i32 %add7 to float, !dbg !31
  %16 = load float, ptr %z, align 4, !dbg !34
  %add8 = fadd float %16, %conv, !dbg !34
  store float %add8, ptr %z, align 4, !dbg !34
  %17 = load ptr, ptr %D.addr, align 8, !dbg !35
  %18 = load i32, ptr %i, align 4, !dbg !36
  %idxprom9 = sext i32 %18 to i64, !dbg !35
  %arrayidx10 = getelementptr inbounds float, ptr %17, i64 %idxprom9, !dbg !35
  %19 = load float, ptr %arrayidx10, align 4, !dbg !35
  %20 = load ptr, ptr %E.addr, align 8, !dbg !37
  %21 = load i32, ptr %i, align 4, !dbg !38
  %idxprom11 = sext i32 %21 to i64, !dbg !37
  %arrayidx12 = getelementptr inbounds float, ptr %20, i64 %idxprom11, !dbg !37
  %22 = load float, ptr %arrayidx12, align 4, !dbg !37
  %23 = load ptr, ptr %D1.addr, align 8, !dbg !39
  %24 = load i32, ptr %i, align 4, !dbg !40
  %idxprom13 = sext i32 %24 to i64, !dbg !39
  %arrayidx14 = getelementptr inbounds float, ptr %23, i64 %idxprom13, !dbg !39
  %25 = load float, ptr %arrayidx14, align 4, !dbg !39
  %26 = call float @llvm.fmuladd.f32(float %19, float %22, float %25), !dbg !41
  %27 = load ptr, ptr %C.addr, align 8, !dbg !42
  %28 = load i32, ptr %i, align 4, !dbg !43
  %idxprom15 = sext i32 %28 to i64, !dbg !42
  %arrayidx16 = getelementptr inbounds float, ptr %27, i64 %idxprom15, !dbg !42
  store float %26, ptr %arrayidx16, align 4, !dbg !44
  br label %for.inc, !dbg !45

for.inc:                                          ; preds = %for.body
  %29 = load i32, ptr %i, align 4, !dbg !46
  %inc = add nsw i32 %29, 1, !dbg !46
  store i32 %inc, ptr %i, align 4, !dbg !46
  br label %for.cond, !dbg !19, !llvm.loop !47

for.end:                                          ; preds = %for.cond
  %30 = load float, ptr %z, align 4, !dbg !51
  ret float %30, !dbg !52
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

attributes #0 = { noinline nounwind uwtable vscale_range(4,4) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="a64fx" "target-features"="+aes,+crc,+fp-armv8,+fullfp16,+lse,+neon,+outline-atomics,+ras,+rdm,+sha2,+sve,+v8.1a,+v8.2a,+v8a,-fmv" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 18.1.8 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git c336981949d00c4730a27886ee106adcc1921dbe)", isOptimized: false, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "ls4s-pragma-002.c", directory: "/SHARE/mas/sotu")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 8, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 2}
!7 = !{i32 7, !"frame-pointer", i32 1}
!8 = !{!"clang version 18.1.8 (http://172.16.1.70:10081/a64fx-swpl/llvm-project.git c336981949d00c4730a27886ee106adcc1921dbe)"}
!9 = distinct !DISubprogram(name: "foo", scope: !10, file: !10, line: 5, type: !11, scopeLine: 8, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!10 = !DIFile(filename: "./ls4s-pragma-002.c", directory: "/SHARE/mas/sotu")
!11 = !DISubroutineType(types: !12)
!12 = !{}
!13 = !DILocation(line: 10, column: 9, scope: !9)
!14 = !DILocation(line: 12, column: 10, scope: !9)
!15 = !DILocation(line: 12, column: 8, scope: !9)
!16 = !DILocation(line: 12, column: 15, scope: !9)
!17 = !DILocation(line: 12, column: 19, scope: !9)
!18 = !DILocation(line: 12, column: 17, scope: !9)
!19 = !DILocation(line: 12, column: 3, scope: !9)
!20 = !DILocation(line: 13, column: 16, scope: !9)
!21 = !DILocation(line: 13, column: 18, scope: !9)
!22 = !DILocation(line: 13, column: 23, scope: !9)
!23 = !DILocation(line: 13, column: 25, scope: !9)
!24 = !DILocation(line: 13, column: 30, scope: !9)
!25 = !DILocation(line: 13, column: 33, scope: !9)
!26 = !DILocation(line: 13, column: 28, scope: !9)
!27 = !DILocation(line: 13, column: 5, scope: !9)
!28 = !DILocation(line: 13, column: 7, scope: !9)
!29 = !DILocation(line: 13, column: 9, scope: !9)
!30 = !DILocation(line: 13, column: 14, scope: !9)
!31 = !DILocation(line: 14, column: 10, scope: !9)
!32 = !DILocation(line: 14, column: 12, scope: !9)
!33 = !DILocation(line: 14, column: 11, scope: !9)
!34 = !DILocation(line: 14, column: 7, scope: !9)
!35 = !DILocation(line: 15, column: 12, scope: !9)
!36 = !DILocation(line: 15, column: 14, scope: !9)
!37 = !DILocation(line: 15, column: 19, scope: !9)
!38 = !DILocation(line: 15, column: 21, scope: !9)
!39 = !DILocation(line: 15, column: 26, scope: !9)
!40 = !DILocation(line: 15, column: 29, scope: !9)
!41 = !DILocation(line: 15, column: 24, scope: !9)
!42 = !DILocation(line: 15, column: 5, scope: !9)
!43 = !DILocation(line: 15, column: 7, scope: !9)
!44 = !DILocation(line: 15, column: 10, scope: !9)
!45 = !DILocation(line: 17, column: 3, scope: !9)
!46 = !DILocation(line: 12, column: 23, scope: !9)
!47 = distinct !{!47, !19, !45, !48, !49, !50}
!48 = !{!"llvm.loop.mustprogress"}
!49 = !{!"llvm.loop.distribute4swpl.enable", i1 true}
!50 = !{!"llvm.loop.distribute4swpl.ireg", i32 30}
!51 = !DILocation(line: 18, column: 10, scope: !9)
!52 = !DILocation(line: 18, column: 3, scope: !9)
