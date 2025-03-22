# GBEmu CMake Build File
cmake_minimum_required(VERSION 3.5)
project(GBEmu VERSION 0.1 LANGUAGES C)

# global flags for compilation
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

# flags used in debug mode (few optimizations and GDB info)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O1 -ggdb3")

# flags used in release mode (full optimization and stripping)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -s")

# add NDEBUG flag if compiling in release mode
add_compile_definitions($<$<CONFIG:RELEASE>:NDEBUG>)

# add our include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# find SDL2 library
find_library(SDL_LIB
    NAMES SDL2
)

# public headers
set(INCLUDE_FILES
    include/gbemu.h
    include/gbemu/errors.h
)

# source files
set(SRC_FILES
    src/ext/cart.c
    src/ext/ext_bus.c
    src/ext/mem.h
    src/ext/vid_bus.c
    $<$<NOT:$<CONFIG:RELEASE>>:src/log.c>
    src/log.h
    src/types.h
    src/soc/cpu.c
    src/soc/cpu_common.h
    src/soc/dma.c
    src/soc/instr/aritm.c
    src/soc/instr/bits.c
    src/soc/instr/flow.c
    src/soc/instr/instrs.c
    src/soc/instr/instrs.h
    src/soc/instr/isr.c
    src/soc/instr/ld.c
    src/soc/instr/misc.c
    src/soc/joypad.c
    src/soc/ppu.c
    src/soc/soc.c
    src/soc/soc.h
    src/soc/timer.c
)

# final sources
set(PROJECT_SOURCES
    ${INCLUDE_FILES}
    ${SRC_FILES}
)

# final test executable
add_executable(gbemu-test ${PROJECT_SOURCES} main.c)
add_executable(gbemu ${PROJECT_SOURCES} sdl.c)
target_link_libraries(gbemu ${SDL_LIB})
