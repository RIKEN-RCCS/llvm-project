## Source Code
`git@github.com:RIKEN-RCCS/llvm-project.git`  
branch: `ldist-2025-llvm20` (This is a temporary name)

## Build Instructions

1. Clone the Source Code

      ```
      $ git clone -b ldist-2025-llvm20 https://github.com/RIKEN-RCCS/llvm-project.git
      ```

2. Create a Build Directory

      ```
      $ cd llvm-project
      $ mkdir build
      ```

3. Generate Makefiles with CMake

      For parameters that can be specified with cmake, see [Building LLVM with CMake](https://llvm.org/docs/CMake.html):

      ```
      $ cd build
      $ cmake ../llvm \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=../install \
         -DLLVM_TARGETS_TO_BUILD="AArch64" \
         -DLLVM_ENABLE_PROJECTS="clang;lld" \
         -DLLVM_INCLUDE_BENCHMARKS=OFF
      ```

      CMAKE_BUILD_TYPE:      Build type (`Release` or `Debug`)<br>
      CMAKE_INSTALL_PREFIX:  Installation path<br>
      LLVM_TARGETS_TO_BUILD: Target architecture to build<br>
      LLVM_ENABLE_PROJECTS:  LLVM projects to build<br>

4. Build and install

      ```
      $ make -j8 && make install
      ```

5. Verify the Build

      Translate the application:

      ```
      $ cd ../install
      $ ./bin/clang++ -mcpu=a64fx --target=aarch64-linux-gnu -Ofast -msve-vector-bits=512 -fswp -fls -S ~/axhelm-4.cpp -o ~/axhelm-4.s -foptimization-record-file=./axhelm-4.yaml
      ```

      Refer to the generated axhelm-4.yaml and if you see the following message, SWPL is working (the numbers in the message may be different):

      ```
      software pipelining (IPC: 2.03, ITR: 4, MVE: 2, II: 65, Stage: 4,
      ```

6. Linking (Optional)

      To generate an executable with linking on the login node, add the following options:

      ```
      -fuse-ld=lld --sysroot=/opt/FJSVxos/devkit/aarch64/rfs
      ```

## Adjusting MaxII and Budget
By adjusting `MaxII` and `budget` using the following translation options, you may be able to reduce the translation time of SWPL.<br>
The `budget` parameter specifies the maximum number of times SWPL attempts to place instructions, and a larger value tends to result in longer translation time.


### Compilation Options
| Option | Description | Example | Default |
| --- | --- | --- | --- |
| `-swpl-maxii` | Specifies the maximum II (Initiation Interval) for instruction placement attempts.  | `-mllvm -swpl-maxii=100`  | If unspecified or set to 0, defaults to 1000. |
| `-swpl-budget-ratio-less`| Specifies the coefficient for budget calculation when the number of SWPL target instructions is less than 100. <br>The budget is calculated as: instruction count × coefficient. | `-mllvm -swpl-budget-ratio-less=10.0` | If unspecified or set to 0, defaults to 50.0. |
| `-swpl-budget-ratio-more`| Specifies the coefficient for budget calculation when the number of SWPL target instructions is 100 or more. <br>The budget is calculated as: instruction count × coefficient. | `-mllvm -swpl-budget-ratio-more=5.0` | If unspecified or set to 0, defaults to 25.0. |
### Notes

The loop where SWPL was applied may no longer have SWPL applied due to adjustments in MaxII and budget.

