## Source Code
git@github.com:RIKEN-RCCS/llvm-project.git  
branch:swpl-2023-1-llvm17  
tag:swpl-2023-1-llvm17_tag_20231222

## Build Instructions

1. Clone the source code from the repository

      ```
      $ git clone -b swpl-2023-1-llvm17 https://github.com/RIKEN-RCCS/llvm-project.git
      ```

2. Create a build directory in the root of llvm-project directory

      ```
      $ cd llvm-project  
      $ mkdir build
      ```

3. Generate a Makefile using the cmake command

      Refer to Building LLVM with CMake (https://llvm.org/docs/CMake.html) for possible parameters to specify.

      ```
      $ cd build  
      $ cmake ../llvm \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=../install \
         -DLLVM_TARGETS_TO_BUILD="AArch64" \
         -DLLVM_ENABLE_PROJECTS="clang;lld" -DLLVM_INCLUDE_BENCHMARKS=OFF  
      ```

      CMAKE_BUILD_TYPE:      Build type (Release | Debug) 
      CMAKE_INSTALL_PREFIX:  Installation destination  
      LLVM_TARGETS_TO_BUILD: Architectures to build
      LLVM_ENABLE_PROJECTS:  Project to build

4. Execute make

      ```
      $ make -j8 && make install
      ```

5. Verify the build results

      Compile the application.

      ```
      $ cd ../install  
      $ ./bin/clang++ -mcpu=a64fx --target=aarch64-linux-gnu -Ofast -msve-vector-bits=512 -fswp -S ~/axhelm-4.cpp -o ~/axhelm-4.s -foptimization-record-file=./axhelm-4.yaml
      ```

      Refer to the generated axhelm-4.yaml.
      If the following message appears, SWPL is applied (numbers in the message may vary) 

       "software pipelining (IPC: 2.03, ITR: 4, MVE: 2, II: 65, Stage: 4, "

6. To link and generate an executable

      When generating an executable on the login node, add the following options.

      ```
      -fuse-ldlld --sysroot=/opt/FJSVxos/devkit/aarch64/rfs
      ```

## Adjustment for MaxII and budget
Adjusting MaxII and budget with the following compiling option may reduce SWPL compilation time. <br>
The budget is the upper limit on the number of attempts to place instructions in SWPL,
and higher budget tends to increase compiling time.

### Compile options
| Option Name | Function | Example | Note |
| --- | --- | --- | --- |
| -swpl-maxii | Specifies the maximum II for attempting instruction placement. | -mllvm -swpl-maxii=100 | If unspecified or zero, the default is 1000. |
| -swpl-budget-ratio-less | Specifies the coefficient for calculating the budget when the number of SWPL target instructions is less than 100. <br> The budget number is the number of SWPL target instructions \times coefficient. | -mllvm -swpl-budget-ratio-less=10.0 | If unspecified or zero, the default is 50.0. |
| -swpl-budget-ratio-more | Specifies the coefficient for calculating the budget when the number of SWPL target instructions is 100 or more. <br> The budget number is the number of SWPL target instruction \times coefficient. | -mllvm -swpl-budget-ratio-more=5.0 | If unspecified or zero, the default is 25.0. |

### Note
Adjustments to MaxII and budget may result in loops that were previously applicable for SWPL becoming non-applicable.

