
cmake_minimum_required(VERSION 3.22)
include(ExternalProject)

project("solution" NONE)

if ($ENV{CMSIS_PACK_ROOT} STREQUAL "")
  message(ERROR "Environment variable CMSIS_PACK_ROOT must be set")
endif()
cmake_path(SET CMSIS_PACK_ROOT NORMALIZE $ENV{CMSIS_PACK_ROOT})
set(CMAKE_TOOLCHAIN_FILE_AC6  "./toolchains/AC6.cmake")
cmake_path(ABSOLUTE_PATH CMAKE_TOOLCHAIN_FILE_AC6 NORMALIZE OUTPUT_VARIABLE CMAKE_TOOLCHAIN_FILE_AC6)
set(CMAKE_TOOLCHAIN_FILE_GCC  "./toolchains/GCC.cmake")
cmake_path(ABSOLUTE_PATH CMAKE_TOOLCHAIN_FILE_GCC NORMALIZE OUTPUT_VARIABLE CMAKE_TOOLCHAIN_FILE_GCC)
set(SOLUTION_ROOT  "..")
cmake_path(ABSOLUTE_PATH SOLUTION_ROOT NORMALIZE OUTPUT_VARIABLE SOLUTION_ROOT)

# Context specific lists
set(CONTEXTS
  "project.AC6+ARMCM3"
  "project.GCC+ARMCM3"
  "project.App+ARMCM3"
)
list(LENGTH CONTEXTS CONTEXTS_LENGTH)
math(EXPR CONTEXTS_LENGTH "${CONTEXTS_LENGTH}-1")

set(DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/project/ARMCM3/AC6"
  "${CMAKE_CURRENT_SOURCE_DIR}/project/ARMCM3/GCC"
  "${CMAKE_CURRENT_SOURCE_DIR}/project/ARMCM3/App"
)

set(CMAKE_TOOLCHAIN_FILES
  "${CMAKE_TOOLCHAIN_FILE_AC6}"
  "${CMAKE_TOOLCHAIN_FILE_GCC}"
  "${CMAKE_TOOLCHAIN_FILE_AC6}"
)

set(OUTPUTS
  "${SOLUTION_ROOT}/out/project/ARMCM3/AC6/project.lib"
  "${SOLUTION_ROOT}/out/project/ARMCM3/GCC/libproject.a"
  "${SOLUTION_ROOT}/out/project/ARMCM3/App/project.axf"
)

# Common args
set(ARGS
  "-DSOLUTION_ROOT=${SOLUTION_ROOT}"
  "-DCMSIS_PACK_ROOT=${CMSIS_PACK_ROOT}"
)

# Iterate over contexts
foreach(INDEX RANGE ${CONTEXTS_LENGTH})
  math(EXPR N "${INDEX}+1")
  list(GET CONTEXTS ${INDEX} CONTEXT)
  list(GET DIRS ${INDEX} DIR)
  list(GET OUTPUTS ${INDEX} OUTPUT)

  # Context CMake Args
  set(ARGS_CONTEXT ${ARGS})
  list(GET CMAKE_TOOLCHAIN_FILES ${INDEX} CMAKE_TOOLCHAIN_FILE)
  list(APPEND ARGS_CONTEXT "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

  # Create external project, set configure and build steps
  ExternalProject_Add(${CONTEXT}
    PREFIX            ${DIR}
    SOURCE_DIR        ${DIR}
    BINARY_DIR        ${N}
    INSTALL_COMMAND   ""
    TEST_COMMAND      ""
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -G Ninja -S <SOURCE_DIR> -B <BINARY_DIR> ${ARGS_CONTEXT}
    BUILD_COMMAND     ${CMAKE_COMMAND} --build <BINARY_DIR>
    BUILD_ALWAYS      TRUE
    BUILD_BYPRODUCTS  ${OUTPUT}
  )
  ExternalProject_Add_StepTargets(${CONTEXT} build configure)

  # Debug
  message(VERBOSE "Configure Context: ${CMAKE_COMMAND} -G Ninja -S ${DIR} -B ${N} ${ARGS_CONTEXT}")

  # Database generation step
  ExternalProject_Add_Step(${CONTEXT} database
    COMMAND           ${CMAKE_COMMAND} --build <BINARY_DIR> --target database
    EXCLUDE_FROM_MAIN TRUE
    ALWAYS            TRUE
    DEPENDEES         configure
  )
  ExternalProject_Add_StepTargets(${CONTEXT} database)

  # Context pre/post-build steps
  ExternalProject_Add_Step(${CONTEXT} prebuild
    COMMAND           echo "RUN ${CONTEXT} prebuild step"
    DEPENDERS         build
    ALWAYS            TRUE
  )
  ExternalProject_Add_Step(${CONTEXT} postbuild
    COMMAND           echo "RUN ${CONTEXT} postbuild step"
    DEPENDEES         build
    DEPENDS           "${OUTPUT}"
  )
  ExternalProject_Add_StepTargets(${CONTEXT} prebuild postbuild)
endforeach()

# Build dependencies
ExternalProject_Add_StepDependencies(
  project.App+ARMCM3 build
  project.AC6+ARMCM3-build
  project.GCC+ARMCM3-build
)

# Solution pre-build step
add_custom_target(solution-prebuild ALL)
add_custom_command(TARGET solution-prebuild PRE_BUILD
  COMMAND         echo "RUN solution prebuild step"
)
add_dependencies(project.AC6+ARMCM3-prebuild solution-prebuild)
add_dependencies(project.GCC+ARMCM3-prebuild solution-prebuild)
add_dependencies(project.App+ARMCM3-prebuild solution-prebuild)

# Solution post-build step
add_custom_target(solution-postbuild ALL DEPENDS postbuild.stamp)
add_custom_command(
  OUTPUT          postbuild.stamp
  COMMAND         echo "RUN solution postbuild step"
  COMMAND         ${CMAKE_COMMAND} -E touch postbuild.stamp
  DEPENDS         "${OUTPUTS}"
)
add_dependencies(solution-postbuild
  project.AC6+ARMCM3-build
  project.GCC+ARMCM3-build
  project.App+ARMCM3-build
)

