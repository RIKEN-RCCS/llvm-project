## ソースコード
git@github.com:RIKEN-RCCS/llvm-project.git  
branch:swpl-2023-1-llvm17  
tag:swpl-2023-1-llvm17_tag_20231222

## ビルド方法

1. リポジトリからソースコードをcloneする

      ```
      $ git clone -b swpl-2023-1-llvm17 https://github.com/RIKEN-RCCS/llvm-project.git
      ```

2. llvm-projectディレクトリ直下でBUILD用ディレクトリを作成する

      ```
      $ cd llvm-project  
      $ mkdir build
      ```

3. cmakeコマンドでMakefileを生成する

      cmakeで指定可能なパラメータはBuilding LLVM with CMake(https://llvm.org/docs/CMake.html )を参照してください。

      ```
      $ cd build  
      $ cmake ../llvm \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=../install \
         -DLLVM_TARGETS_TO_BUILD="AArch64" \
         -DLLVM_ENABLE_PROJECTS="clang;lld" -DLLVM_INCLUDE_BENCHMARKS=OFF  
      ```

      CMAKE_BUILD_TYPE:      ビルドタイプ（Release|Debug）  
      CMAKE_INSTALL_PREFIX:  インストール先  
      LLVM_TARGETS_TO_BUILD: ビルドするアーキテクチャ  
      LLVM_ENABLE_PROJECTS:  ビルドするプロジェクト  

4. makeする

      ```
      $ make -j8 && make install
      ```

5. ビルド結果の確認

      アプリケーションの翻訳を行います。

      ```
      $ cd ../install  
      $ ./bin/clang++ -mcpu=a64fx --target=aarch64-linux-gnu -Ofast -msve-vector-bits=512 -fswp -S ~/axhelm-4.cpp -o ~/axhelm-4.s -foptimization-record-file=./axhelm-4.yaml
      ```

      生成されたaxhelm-4.yamlを参照し、以下のメッセージがあればSWPLが  
      動作しています（メッセージ内の数字は異なる場合があります）。

       「software pipelining (IPC: 2.03, ITR: 4, MVE: 2, II: 65, Stage: 4, 」

## MaxIIおよびbudgetの調整について
以下の翻訳オプションにてMaxII、budgetを調整することで、SWPLの翻訳時間を削減できる場合があります。<br>
budgetはSWPLにて命令の配置を試みる回数の上限であり、大きくなるほど翻訳時間が長くなる傾向となります。

### 翻訳オプション
| オプション名 | 機能 | 指定例 | 備考 |
| --- | --- | --- | --- |
| -swpl-maxii | 命令配置を試みる最大のIIを指定する。 | -mllvm -swpl-maxii=100 | 指定無し、または０が指定された場合は 1000。 |
| -swpl-budget-ratio-less | SWPL対象命令数が100より小さい場合の、budget算出の係数を指定する。<br>budget数は、SWPL対象命令数×係数となる。 | -mllvm -swpl-budget-ratio-less=10.0 | 指定無し、または０が指定された場合は 50.0。 |
| -swpl-budget-ratio-more | SWPL対象命令数が100以上である場合の、budget算出の係数を指定する。<br>budget数は、SWPL対象命令数×係数となる。 | -mllvm -swpl-budget-ratio-more=5.0 | 指定無し、または０が指定された場合は 25.0。 |

### 注意点
SWPLが適用されていたループが、MaxII、budgetの調整によりSWPLが適用されなくなる場合があります。

