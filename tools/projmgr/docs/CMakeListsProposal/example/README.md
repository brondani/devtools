# Example: Multiple contexts and toolchains CMakeLists

This solution example demonstrates the use of a SuperProject CMakeLists orchestrating the build of inter-dependent contexts using multiple toolchains.

The yml files are present in the `original` directory only as reference, they are not processed in this demo.
The solution contains 3 contexts originated by 3 build types:
- `AC6`: library compiled by AC6
- `GCC`: library compiled by GCC
- `App`: application using the previous libraries, compiled and linked by AC6

The proposed SuperProject CMakeLists is in the `tmp` directory and the context-level CMakeLists are in the folders:
- `tmp/project/ARMCM3/AC6`
- `tmp/project/ARMCM3/GCC`
- `tmp/project/ARMCM3/App`

The ouput folders are configured respectively:
- `out/project/ARMCM3/AC6`
- `out/project/ARMCM3/GCC`
- `out/project/ARMCM3/App`

While the CMake objects are placed respectively:
- `tmp/1`
- `tmp/2`
- `tmp/3`

## Prerequisites

Build system:
- `CMake`
- `Ninja`

The following toolchains must be previously installed and registered via their [environment variables](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/blob/main/docs/installation.md#environment-variables):
- `AC6_TOOLCHAIN_6_19_0`
- `GCC_TOOLCHAIN_12_2_1`

The environment variable `CMSIS_PACK_ROOT` must be set, the CMSIS-Pack repository must be initialized and the following pack must be installed:
- `ARM::CMSIS@5.9.0`

## How to use

On the `example` directory run CMake configure step:
```
cmake -G Ninja -S tmp -B tmp
```

Build all artifacts:
```
cmake --build tmp
```

Build artifacts of a given context:
```
cmake --build tmp --target <context>
cmake --build tmp --target project.App+ARMCM3
```

Put the compilation database in the `out` directory of a given context:
```
cmake --build tmp --target <context>-database
cmake --build tmp --target project.App+ARMCM3-database
```