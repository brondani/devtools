include("global.cmake")

add_library(ARM_CMSIS_CORE_5_6_0 INTERFACE)
target_include_directories(ARM_CMSIS_CORE_5_6_0 INTERFACE 
  "${CMSIS_PACK_ROOT}/ARM/CMSIS/5.9.0/CMSIS/Core/Include"
)

add_library(ARM_Device_Startup_C_Startup_2_0_3 OBJECT 
  "${SOLUTION_ROOT}/RTE/Device/ARMCM3/startup_ARMCM3.c"
  "${SOLUTION_ROOT}/RTE/Device/ARMCM3/system_ARMCM3.c"
)
target_include_directories(ARM_Device_Startup_C_Startup_2_0_3 PUBLIC 
  ${GLOBAL_INCLUDE_PATHS}
)
target_compile_definitions(ARM_Device_Startup_C_Startup_2_0_3 PUBLIC
  ${GLOBAL_DEFINES}
)
