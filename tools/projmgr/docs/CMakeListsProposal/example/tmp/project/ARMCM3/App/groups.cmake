
include("global.cmake")

add_library(Application OBJECT 
  ${SOLUTION_ROOT}/main.c
)
target_include_directories(Application PUBLIC 
  ${GLOBAL_INCLUDE_PATHS}
)
target_link_libraries(Application
  "${SOLUTION_ROOT}/out/project/ARMCM3/AC6/project.lib"
  "${SOLUTION_ROOT}/out/project/ARMCM3/GCC/libproject.a"
)
target_compile_definitions(Application PUBLIC
  ${GLOBAL_DEFINES}
)
