
include("global.cmake")

add_library(Libraries OBJECT 
  "${SOLUTION_ROOT}/Lib2/lib.c"
)
target_include_directories(Libraries PUBLIC 
  ${GLOBAL_INCLUDE_PATHS}
  "${SOLUTION_ROOT}/Lib2"
)
target_compile_definitions(Libraries PUBLIC
  ${GLOBAL_DEFINES}
)
