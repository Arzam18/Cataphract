cmake_minimum_required(VERSION 3.30)
project(Cataphract LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_TUNING "Build with SPSA tuning hooks enabled" OFF)

# ---------------------------------------------------------------------------
# Architecture detection (Android ABIs + native ARM + native x86)
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

# ---------------------------------------------------------------------------
# Generate net_data.hpp from eval/net.bin.
# Replaces Clang's #embed, which is unavailable in the NDK r28 toolchain.
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CATAPHRACT_NET_HEADER "${CMAKE_CURRENT_BINARY_DIR}/net_data.hpp")

add_custom_command(
    OUTPUT  ${CATAPHRACT_NET_HEADER}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/bin2header.py
            ${CMAKE_SOURCE_DIR}/net.bin
            ${CATAPHRACT_NET_HEADER}
    DEPENDS ${CMAKE_SOURCE_DIR}/net.bin
            ${CMAKE_SOURCE_DIR}/tools/bin2header.py
    COMMENT "Generating net_data.hpp from eval/net.bin"
    VERBATIM
)

add_custom_target(net_data DEPENDS ${CATAPHRACT_NET_HEADER})

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
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

add_dependencies(Cataphract net_data)
target_include_directories(Cataphract PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (ENABLE_TUNING)
    target_compile_definitions(Cataphract PRIVATE SPSA_TUNE)
endif ()

# ---------------------------------------------------------------------------
# Android / NDK build path — returns early so Linux-only flags never apply.
# ---------------------------------------------------------------------------
if (ANDROID)
    target_compile_options(Cataphract PRIVATE
        -std=c++26
        -O3 -DNDEBUG
        -Wall -Wextra
        -fno-exceptions -fno-rtti
        -ffunction-sections -fdata-sections
    )
    target_compile_definitions(Cataphract PRIVATE NDEBUG)

    if (CATAPHRACT_ARCH STREQUAL "arm64")
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
# Native ARM (Linux) — same NEON flags, host toolchain
# ---------------------------------------------------------------------------
if (CATAPHRACT_ARCH STREQUAL "arm64")
    target_compile_options(Cataphract PRIVATE -march=armv8.2-a+dotprod+fp16)
elseif (CATAPHRACT_ARCH STREQUAL "arm32")
    target_compile_options(Cataphract PRIVATE -mfpu=neon)
endif ()

# ---------------------------------------------------------------------------
# Native Linux / macOS
# ---------------------------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Cataphract PRIVATE "-lstdc++exp")

    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE
                -O0 -Wall -Wextra -Wno-class-memaccess -g -march=native -fno-omit-frame-pointer
        )

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -Wall -Wextra -Wno-class-memaccess -DNDEBUG -fno-exceptions
                -ffunction-sections -fdata-sections -fno-rtti
                -O3 -flto=auto -fipa-pta
                -march=native -mtune=native
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=auto -static -s
                -Wl,--gc-sections
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(Cataphract PRIVATE -O0 -Wextra -g -march=native -fno-exceptions
                -fno-omit-frame-pointer
                -fsanitize=undefined -fsanitize=implicit-integer-truncation
                -fsanitize=implicit-integer-arithmetic-value-change -fsanitize=implicit-integer-conversion
                -fsanitize=implicit-conversion -fsanitize=signed-integer-overflow
                -fsanitize=nullability -fsanitize=local-bounds
                -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero
                -fsanitize=implicit-unsigned-integer-truncation -fsanitize=implicit-signed-integer-truncation
                -fsanitize=implicit-integer-sign-change
                -fsanitize=thread
                -fno-sanitize=shift
                -Wno-c23-extensions
        )
        target_link_options(Cataphract PRIVATE
                -fsanitize=undefined -fsanitize=implicit-integer-truncation
                -fsanitize=implicit-integer-arithmetic-value-change -fsanitize=implicit-integer-conversion
                -fsanitize=implicit-conversion -fsanitize=signed-integer-overflow
                -fsanitize=nullability -fsanitize=local-bounds
                -fsanitize=float-divide-by-zero -fsanitize=integer-divide-by-zero
                -fsanitize=implicit-unsigned-integer-truncation -fsanitize=implicit-signed-integer-truncation
                -fsanitize=implicit-integer-sign-change
                -fsanitize=thread
                -fno-sanitize=shift
                -fuse-ld=lld
        )

    elseif (CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(Cataphract PRIVATE
                -Wall -Wextra -DNDEBUG
                -O3 -flto=full -fno-exceptions -march=native -mtune=native -fomit-frame-pointer
                -ffunction-sections -fdata-sections -fexperimental-library -fno-rtti
                -Wno-c23-extensions
        )
        target_compile_definitions(Cataphract PRIVATE NDEBUG)
        target_link_options(Cataphract PRIVATE
                -flto=full -static -s
                -fuse-ld=lld
                -Wl,--gc-sections -Wl,--icf=all
        )
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC not supported.")
endif ()
