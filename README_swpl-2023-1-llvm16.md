## ソースコード
git@github.com:RIKEN-RCCS/llvm-project.git  
branch:swpl-2023-1-llvm16  
tag:swpl-2023-1-llvm16_tag_202308

## ビルド方法

1. リポジトリからソースコードをcloneする

      ```
      $ git clone -b swpl-2023-1-llvm16 https://github.com/RIKEN-RCCS/llvm-project.git
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
      $ make -j8 & make install
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
