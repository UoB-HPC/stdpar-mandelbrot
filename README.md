# Heterogeneous Mandelbrot in ISO C++17

<img  alt="Mandelbrot zoom" src="mandelbrot.gif" width="256" style="float: right; padding: 12px"/>

This sample renders the Mandelbrot fractal using only ISO C++17 features.
When compiled with the appropriate compiler, the program can run on both CPUs and GPUs without any
code change.

A small [shim layer](shim.h) (~80 lines with comments) is needed to bridge gaps across Intel
oneDPL+DPCPP,
GCC/Clang, and NVHPC.
For image output, a [single-header GIF image library](https://github.com/charlietangora/gif-h) is
used.

Excluding the shim and GIF library, this sample renders an animated zoom of the Mandelbrot set and
saves it as a GIF image in [less than 100 SLOC](main.cpp).

# Building

This sample uses CMake, to compile:

```shell
> cmake -Bbuild -H. <CMAKE_FLAGS>
> cmake --build build -j
> ./build/stdpar_mendelbrot
> file mandelbrot.gif                                                                                                                                                                                     ✔  3s  tom@soraws-uk  03:44:29 
mandelbrot.gif: GIF image data, version 89a, 256 x 256
```

*There's a [slightly longer and less terse version](main_long.cpp) of [main.cpp](main.cpp), you can
replace references to `main.cpp` in [CMakeLists.txt](CMakeLists.txt) to compile that instead.*

Supported CMake flags:

* `NVHPC_OFFLOAD_GPU` - default=`OFF`; Enable PSTL GPU offloading support (via the
  non-standard `-stdpar`) for the NVHPC SDK.
  The values are Nvidia architectures in ccXY format will be passed in via `-gpu=` (e.g `cc70`)

  Possible values are:
    * `OFF` - (default)
    * `cc35` - Compile for compute capability 3.5
    * `cc50` - Compile for compute capability 5.0
    * `cc60` - Compile for compute capability 6.0
    * `cc62` - Compile for compute capability 6.2
    * `cc70` - Compile for compute capability 7.0
    * `cc72` - Compile for compute capability 7.2
    * `cc75` - Compile for compute capability 7.5
    * `cc80` - Compile for compute capability 8.0
    * `ccall` - Compile for all supported compute capabilities
* `NVHPC_OFFLOAD_CPU` - default=`OFF`; Enable PSTL multicore support (via the
  non-standard `-stdpar`) for the NVHPC SDK.
* `USE_TBB` - default=`OFF`; Link against an in-tree oneTBB via `FetchContent_Declare`
* `USE_ONEDPL` - default=`OFF`; Link oneDPL which implements C++17 executor policies (via
  execution_policy_tag) for different backends.

  Possible values are:

    * `OPENMP` - Implements policies using OpenMP.
      CMake will handle any flags needed to enable OpenMP if the compiler supports it.
    * `TBB` - Implements policies using TBB.
      TBB must be linked via USE_TBB or be available in LD_LIBRARY_PATH.
    * `DPCPP` - Implements policies through SYCL2020.

  This requires the DPC++ compiler (other SYCL compilers are untested), required SYCL flags are
  added automatically.

Worked flag examples:

* `-DCMAKE_CXX_COMPILER=nvc++ -DNVHPC_OFFLOAD_GPU=cc80` - Target NVIDIA Ampere GPUs.
* `-DCMAKE_CXX_COMPILER=nvc++ -DNVHPC_OFFLOAD_CPU` - Target CPUs.
* `-DCMAKE_CXX_COMPILER=dpcpp -DUSE_ONEDPL=DPCPP` - Target Intel GPUs with DPCPP using SYCL2020.
* `-DCMAKE_CXX_COMPILER=c++ -DUSE_ONEDPL=OPENMP` - Target CPUs using oneDPL's OpenMP backend (
  g++/clang++).
* `-DCMAKE_CXX_COMPILER=c++ -DUSE_TBB=ON` - Target CPUs using oneDPL's TBB backend (g++/clang++).

# Licence

```
Copyright 2024 Wei-Chen Lin

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```