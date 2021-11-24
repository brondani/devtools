add_executable(ProjMgrUnitTests src/ProjMgrUnitTests.cpp src/ProjMgrTestEnv.cpp)

set_property(TARGET ProjMgrUnitTests PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(ProjMgrUnitTests PUBLIC projmgrlib gtest_main)
target_include_directories(ProjMgrUnitTests PUBLIC ../include ./src)

add_definitions(-DTEST_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/")

add_test(NAME ProjMgrUnitTests
         COMMAND ProjMgrUnitTests --gtest_output=xml:projmgr_unit_test_report.xml
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
