cmake_minimum_required(VERSION 3.25)
project(Cataphract LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_TUNING "Build with SPSA tuning hooks enabled" OFF)

# ---------------------------------------------------------------------------
# Detect ARM vs x86 for Android / native ARM flags
# ---------------------------------------------------------------------------
if (ANDROID)
    if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(CATAPHRACT_ARCH "arm64")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(CATAPHRACT_ARCH "arm32")
    else ()
        set(CATAPHRACT_ARCH "x86_64")
    endif ()
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)")
    set(CATAPHRACT_ARCH "arm64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7|ARM)")
    set(CATAPHRACT_ARCH "arm32")
else ()
    set(CATAPHRACT_ARCH "x86_64")
endif ()

message(STATUS "Cataphract target architecture: ${CATAPHRACT_ARCH}")

add_executable(Cataphract main.cpp
        board/bitboard.hpp
        board/slider.hpp
        position/fen.hpp
        position/position.hpp
        board/lines.hpp
        position/move.hpp
        position/zobrist.hpp
        position/movegen.hpp
        position/perft.hpp
        uci.hpp
        search/movepicker.hpp
        search/search.hpp
        search/transposition.hpp
        search/timer.hpp
        search/see.hpp
        position/position.cpp
        search/history.hpp
        eval/nnue.hpp
        eval/accumulators.hpp
        position/cuckoo.hpp
        search/time.hpp
        position/bench.hpp
        engine.hpp
        search/params.hpp
        search/params.cpp
        options.hpp
        eval/utils.hpp
        eval/arch.hpp
        eval/simd/simd.hpp
        search/thread.hpp
        eval/nnue.cpp
        search/search.cpp
        search/thread.cpp
        search/time.cpp
        search/timer.cpp
        search/see.cpp
        search/movepicker.cpp
        search/history.cpp
        position/fen.cpp
        position/movegen.cpp
        eval/accumulators.cpp
        search/transposition.cpp
        board/bitboard.cpp
        position/move.cpp
        engine.cpp
        uci.cpp
        board/slider.cpp
        board/attacks.hpp
        position/cuckoo.cpp
        position/bench.cpp
        position/zobrist.cpp
        board/lines.cpp
        position/perft.cpp
        eval/utils.cpp
        eval/simd/avx512.cpp
        eval/simd/avx2.cpp
        eval/simd/neon.cpp
        eval/simd/scalar.cpp
)

if(ENABLE_TUNING)
    target_compile_definitions(Cataphract PRIVATE SPSA_TUNE)
endif()

# ---------------------------------------------------------------------------
# Android / NDK path — return early, x86/`-static`/`-lstdc++exp` flags below
# don't apply here.
# ---------------------------------------------------------------------------
if (ANDROID)
    target_compile_options(Cataphract PRIVATE
        -O3 -DNDEBUG
        -Wall -Wextra -Wno-class-memaccess
        -fno-exceptions -fno-rtti
        -ffunction-sections -fdata-sections
    )
    target_compile_definitions(Cataphract PRIVATE NDEBUG)

    if (CATAPHRACT_ARCH STREQUAL "arm64")
        # ARMv8.2-A + dot-product + fp16: broadly supported from ~2018 onward.
        # If you need to support older arm64 chips, drop the +dotprod+fp16.
        target_compile_options(Cataphract PRIVATE -march=armv8.2-a+dotprod+fp16)
    elseif (CATAPHRACT_ARCH STREQUAL "arm32")
        target_compile_options(Cataphract PRIVATE
            -march=armv7-a -mfpu=neon -mfloat-abi=softfp
        )
    endif ()

    target_link_options(Cataphract PRIVATE -Wl,--gc-sections)
    return()
endif ()

# ---------------------------------------------------------------------------
# Native ARM Linux — same NEON flags but with host toolchain
# ---------------------------------------------------------------------------
if (CATAPHRACT_ARCH STREQUAL "arm64")
    target_compile_options(Cataphract PRIVATE -march=armv8.2-a+dotprod+fp16)
elseif (CATAPHRACT_ARCH STREQUAL "arm32")
    target_compile_options(Cataphract PRIVATE -mfpu=neon)
endif ()

# --- keep your existing GCC / Clang / MSVC block here, unchanged ---
