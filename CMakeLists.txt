cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

project(stdpar-mandelbrot)

include(FetchContent)
FetchContent_Declare(
        gifh  # Licence is public domain
        GIT_REPOSITORY https://github.com/charlietangora/gif-h.git
        GIT_TAG 3d2657b9ad40aac9fd6f75ad079335856e94d664)
FetchContent_Declare(
        oneDPL
        GIT_REPOSITORY https://github.com/oneapi-src/oneDPL.git
        GIT_TAG oneDPL-2021.7.0-release)

set(NVHPC_OFFLOAD_GPU "OFF"
        CACHE STRING
        "Enable PSTL GPU offloading support (via the non-standard `-stdpar`) for the NVHPC SDK.
         The values are Nvidia architectures in ccXY format will be passed in via `-gpu=` (e.g `cc70`)
         Possible values are:
           OFF   - (default)
           cc35  - Compile for compute capability 3.5
           cc50  - Compile for compute capability 5.0
           cc60  - Compile for compute capability 6.0
           cc62  - Compile for compute capability 6.2
           cc70  - Compile for compute capability 7.0
           cc72  - Compile for compute capability 7.2
           cc75  - Compile for compute capability 7.5
           cc80  - Compile for compute capability 8.0
           ccall - Compile for all supported compute capabilities")
set(NVHPC_OFFLOAD_CPU OFF
        CACHE BOOL
        "Enable PSTL multicore support (via the non-standard `-stdpar`) for the NVHPC SDK.")
set(USE_TBB OFF
        CACHE BOOL
        "Link against an in-tree oneTBB via FetchContent_Declare")
set(USE_ONEDPL ""
        CACHE STRING
        "Link oneDPL which implements C++17 executor policies (via execution_policy_tag) for different backends.
        Possible values are:
          OPENMP - Implements policies using OpenMP.
                   CMake will handle any flags needed to enable OpenMP if the compiler supports it.
          TBB    - Implements policies using TBB.
                   TBB must be linked via USE_TBB or be available in LD_LIBRARY_PATH.
          DPCPP  - Implements policies through SYCL2020.
                   This requires the DPC++ compiler (other SYCL compilers are untested), required SYCL flags are added automatically.")


FetchContent_GetProperties(gifh)
if (NOT gifh_POPULATED)
    FetchContent_Populate(gifh)
endif ()


if (USE_ONEDPL)
    string(TOLOWER ${USE_ONEDPL} ONEDPL_BACKEND)
    # XXX oneDPL looks for omp instead of openmp, which mismatches(!) with ONEDPL_PAR_BACKEND if using find_package
    if (ONEDPL_BACKEND STREQUAL "openmp")
        set(ONEDPL_BACKEND omp)
    endif ()
    # Not using FetchContent_MakeAvailable (CMake>= 3.14) because we need EXCLUDE_FROM_ALL
    FetchContent_GetProperties(oneDPL)
    if (NOT oneDPL_POPULATED)
        FetchContent_Populate(oneDPL)
        if (USE_TBB)
            macro(find_package NAME)
                if ("${NAME}" STREQUAL "TBB")
                    message(STATUS "Discarding oneDPL's call to find_package(${NAME} ${ARGN})")
                else ()
                    _find_package(${NAME} ${ARGN})
                endif ()
            endmacro()
        endif ()
        add_subdirectory(${onedpl_SOURCE_DIR} ${onedpl_BINARY_DIR} EXCLUDE_FROM_ALL)

        # Fixup oneDPL's omission on setting DPCPP definitions.
        # We do this after the creation of the oneDPL target.
        if (ONEDPL_BACKEND MATCHES "^(dpcpp|dpcpp_only)$")
            target_compile_definitions(oneDPL INTERFACE ONEDPL_USE_DPCPP_BACKEND=1)
        endif ()

    endif ()
    list(APPEND DEFS USE_ONEDPL)
    list(APPEND LIBS oneDPL)
endif ()

if (USE_TBB)
    list(APPEND LIBS tbb)
endif ()

add_executable(stdpar-mandelbrot main.cpp)
target_include_directories(stdpar-mandelbrot PRIVATE ${gifh_SOURCE_DIR})

if (NVHPC_OFFLOAD_GPU AND NVHPC_OFFLOAD_CPU)
    message(FATAL_ERROR "NVHPC_OFFLOAD_GPU and NVHPC_OFFLOAD_CPU is mutually exclusive.")
else ()
    if (NVHPC_OFFLOAD_GPU)
        set(FLAGS -stdpar -gpu=${NVHPC_OFFLOAD_GPU},fastmath --restrict -Minfo=stdpar)
    elseif (NVHPC_OFFLOAD_CPU)
        set(FLAGS -stdpar -target=multicore,fastmath --restrict -Minfo=stdpar)
    else ()
        set(FLAGS -Ofast -march=native)
    endif ()
    # propagate flags to linker so that it links with the gpu stuff as well
    target_compile_options(stdpar-mandelbrot PRIVATE ${FLAGS})
    target_link_options(stdpar-mandelbrot PRIVATE ${FLAGS})
endif ()

target_compile_definitions(stdpar-mandelbrot PRIVATE ${DEFS})
target_link_libraries(stdpar-mandelbrot PRIVATE ${LIBS})


