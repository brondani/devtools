
include("global.cmake")

add_library(Libraries OBJECT 
  "${SOLUTION_ROOT}/Lib1/lib.c"
)
target_include_directories(Libraries PUBLIC 
  ${GLOBAL_INCLUDE_PATHS}
  "${SOLUTION_ROOT}/Lib1"
)
target_compile_definitions(Libraries PUBLIC
  ${GLOBAL_DEFINES}
)
