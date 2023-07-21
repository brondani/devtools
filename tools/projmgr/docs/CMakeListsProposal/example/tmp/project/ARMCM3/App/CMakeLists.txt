cmake_minimum_required(VERSION 3.22)

# Target options
set(CONTEXT project)
set(OUT_DIR "${SOLUTION_ROOT}/out/project/ARMCM3/App")
set(INT_DIR "${SOLUTION_ROOT}/tmp/3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup project
project(${CONTEXT} LANGUAGES C)

# Clean CMake global flags
set(CMAKE_C_FLAGS "")
set(CMAKE_C_LINK_FLAGS "")

# Compilation database
add_custom_target(database COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INT_DIR}/compile_commands.json" "${OUT_DIR}")

# Setup context
add_executable(${CONTEXT})
set_target_properties(${CONTEXT} PROPERTIES PREFIX "" SUFFIX ".axf" OUTPUT_NAME "project")
set_target_properties(${CONTEXT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})

# Add groups and components
include("groups.cmake")
include("components.cmake")
target_link_libraries(${CONTEXT}
  Application
  ARM_CMSIS_CORE_5_6_0
  ARM_Device_Startup_C_Startup_2_0_3
)

# Compile Options
set(GLOBAL_OPTIONS_CC -mcpu=Cortex-M3 -mfpu=none --target=arm-arm-none-eabi -c -std=gnu11 -ffunction-sections)
target_compile_options(Application PUBLIC ${GLOBAL_OPTIONS_CC})
target_compile_options(ARM_Device_Startup_C_Startup_2_0_3 PUBLIC ${GLOBAL_OPTIONS_CC})

# Linker options
set(LD_SCRIPT "${SOLUTION_ROOT}/RTE/Device/ARMCM3/ARMCM3_ac6.sct")
set(GLOBAL_OPTIONS_LD --cpu=Cortex-M3 --entry=Reset_Handler --scatter=${LD_SCRIPT})
target_link_options(${CONTEXT} PUBLIC ${GLOBAL_OPTIONS_LD})
set_target_properties(${CONTEXT} PROPERTIES LINK_DEPENDS ${LD_SCRIPT})
