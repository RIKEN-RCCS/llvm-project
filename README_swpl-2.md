## ソースコード
git@github.com:RIKEN-RCCS/llvm-project.git  
branch:swpl-2  
tag:swpl-2_tag_20230324

## ビルド方法

1. リポジトリからソースコードをcloneする

      git clone -b swpl-2 https://github.com/RIKEN-RCCS/llvm-project.git

2. llvm-projectディレクトリ直下でBUILD用ディレクトリを作成する

      cd llvm-project  
      mkdir build

3. cmakeコマンドでMakefileを生成する

      cmakeで指定可能なパラメータはBuilding LLVM with CMake(https://llvm.org/docs/CMake.html)を参照してください。

      cd build  
      cmake ../llvm \  
         -DCMAKE_BUILD_TYPE=Release \  
         -DCMAKE_INSTALL_PREFIX=/home/rccs-sdt/a01043/swpl2 \  
         -DLLVM_TARGETS_TO_BUILD="AArch64" \  
         -DLLVM_ENABLE_PROJECTS="clang;lld" -DLLVM_INCLUDE_BENCHMARKS=OFF  

      CMAKE_BUILD_TYPE:      ビルドタイプ（Release|Debug）  
      CMAKE_INSTALL_PREFIX:  インストール先  
      LLVM_TARGETS_TO_BUILD: ビルドするアーキテクチャ  
      LLVM_ENABLE_PROJECTS:  ビルドするプロジェクト  

4. makeする

      make & make install

5. ビルド結果の確認

      テストリポジトリ内のスクリプトで動作を確認できます。
      以下にスクリプトの動作方法を記載しています。

      RIKEN-RCCS/tests_for_llvm_a64fx/benchmark/script/swpl-2/axhelm_comp/README.md


      README.mdの「SWPLの翻訳時情報採取」に従い、ビルドしたコンパイラのディレクトリを設定し、  
      スクリプトを実行します。

      ./swpl2_compile.sh

      スクリプト動作が成功すると、評価用情報ファイルの格納先ディレクトリに  
     「指定オプション」/compile_logディレクトリが作成されます。  
      compile_log配下に生成されたaxhelm-4.optrecord.yamlを参照し、以下のメッセージがあればSWPLが  
      動作しています（メッセージ内の数字は異なる場合があります）。

       「software pipelining (IPC: 1.12, ITR: 3, MVE: 2, II: 121, Stage: 1, 」
