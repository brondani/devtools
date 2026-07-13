---
name: target-debugging
description: "Use when: discovering CMake targets, debugging Open-CMSIS-Pack devtools executables or tests, using VS Code launch configurations, or selecting a target executable path."
---

# Target Discovery And Debugging

Use CMake as the source of truth for targets and executable locations.

## Discover Targets

After configuring, inspect generated targets:

```sh
cmake --preset <configure-preset>
cmake --build out/build/<configure-preset> --target help
```

The top-level configure also generates a `targets` file in the build tree when CMake generation completes.

Common executable targets:

- Tools: `cbuildgen`, `packchk`, `packgen`, `projmgr`, `svdconv`
- Unit tests: `CbuildUnitTests`, `PackChkUnitTests`, `PackGenUnitTests`, `ProjMgrUnitTests`, `SVDConvUnitTests`
- Integration tests: `CbuildIntegTests`, `PackChkIntegTests`, `SvdConvIntegTests`
- Library tests: `CrossPlatformUnitTests`, `ErrLogUnitTests`, `RteFsUtilsUnitTests`, `RteModelUnitTests`, `RteUtilsUnitTests`, `XmlReaderUnitTests`, `XmlTreeUnitTests`, `XmlTreeSlimUnitTests`, `XmlValidatorUnitTests`, `YmlTreeUnitTests`, `YmlSchemaChkTests`

## Debug In VS Code

Recommended extensions are listed in `.vscode/extensions.json`.

1. Install recommended extensions.
2. Select a CMake configure preset with CMake Tools.
3. Configure the project.
4. Select a launch target with CMake Tools.
5. Start the CMake Tools debug command.

CMake Tools resolves the active launch target path. Do not hardcode paths unless investigating a path-specific issue.

## Debug From A Shell

Build the target first:

```sh
cmake --build --preset <build-preset> --target <target>
```

Then run the executable from the configured build tree. On Visual Studio generators, remember that executables are under the selected configuration, for example `Debug` or `Release`.

Use `ctest --preset <test-preset> -R <test-name> --output-on-failure` before opening an interactive debugger. It is the fastest way to confirm the failing test name and runtime environment.
