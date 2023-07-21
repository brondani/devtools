cmake_minimum_required(VERSION 3.22)

# Target options
set(CONTEXT project)
set(OUT_DIR "${SOLUTION_ROOT}/out/project/ARMCM3/GCC")
set(INT_DIR "${SOLUTION_ROOT}/tmp/2")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup project
project(${CONTEXT} LANGUAGES C)

# Clean CMake global flags
set(CMAKE_C_FLAGS "")

# Compilation database
add_custom_target(database COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INT_DIR}/compile_commands.json" "${OUT_DIR}")

# Setup context
add_library(${CONTEXT})
set_target_properties(${CONTEXT} PROPERTIES PREFIX "" SUFFIX ".a" OUTPUT_NAME "libproject")
set_target_properties(${CONTEXT} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

# Add groups and components
include("groups.cmake")
include("components.cmake")
target_link_libraries(${CONTEXT}
  Libraries
  ARM_CMSIS_CORE_5_6_0
)

# Compile Options
set(GLOBAL_OPTIONS_CC -mcpu=cortex-m3 -c -std=gnu11 -fno-short-enums)
target_compile_options(Libraries PUBLIC ${GLOBAL_OPTIONS_CC})
