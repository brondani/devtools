# [DRAFT] CMSIS Project Manager - Software Design 

## Table of Contents

[Introduction](#introduction)</br>

[Design Overview](#design-overview)</br>

[Use cases](#use-cases)</br>
- [ Use Case 1](#use-case-1)</br>
- [ Use Case 2](#use-case-2)</br>

[System Architecture](#system-architecture)</br>

[System Interfaces](#system-interfaces)</br>


## Introduction

The CMSIS Project Manager is a C++ utility provided as binary and as library with interfaces for the most common programming languages and platforms.
It leverages open source C++ libraries available in the [Open-CMSIS-Pack devtools](https://github.com/Open-CMSIS-Pack/devtools) repository.</br>
The tool assists the embedded software developer in the project creation by exposing available features
from installed CMSIS Packs such as devices and components, allowing to search them using free text filters in addition to standard PDSC attributes. It also validates input files that are written in a
human friendly YAML format following pre-defined schemas and it checks the correctness of components selection and unresolved missing dependencies.</br>

It accepts several input files:
| File             | Description
|:-----------------|:---------------------------------------------------------------------------------
| `.csolution.yml` | Defines the complete scope of the application and the build order of sub-projects
| `.cproject.yml`  | Defines the content of an independent build - directly relates to a `cprj` file
| `.clayer.yml`    | Defines pre-configured files and components for reusing in different solutions
| `.rzone`         | Defines memory and peripheral resources.


## Design Overview

The following diagram illustrates inputs and outputs of the `projmgr` processing:
</br></br>![Overview](images/Overview.svg)


## Use Cases

The CMSIS Project Manager has two main use cases:
- Backend to synchronize YAML and CPRJ files and related resources.
- Utility to discover available resources and evaluate selected items during project conception.

### Use case 1:
### Synchronize YAML and CPRJ files and related resources

When used as a backend for other tools, such as an IDE or a Build Manager, the most typical use is to generate CPRJs and configuration/resources files, creating complete projects.

</br></br>![UseCase1](images/UseCase1.svg)

### Use case 2:
### Discover project resources and evaluate selected items

</br></br>![UseCase2](images/UseCase2.svg)


## System Architecture

According to the typical use cases some workflows can be defined.</br>
Creating complete projects:
</br></br>![GenerateCPRJ](images/GenerateCPRJ.svg)

Recreating the `cproject.YAML` files:
</br></br>![RecreateYAML](images/RecreateYAML.svg)

Discovering resources (devices, components, dependencies) for assisting the project conception:

List Devices:
</br></br>![ListDevices](images/ListDevices.svg)

List Components:
</br></br>![ListComponents](images/ListComponents.svg)

List Dependencies:
</br></br>![ListDependencies](images/ListDependencies.svg)


## System Interfaces

The CMSIS Project Manager binaries and libraries are pre-compiled for Windows, Linux and MacOS. The core functions are written in C++ to leverage the already available [Open-CMSIS-Pack/devtools](https://github.com/Open-CMSIS-Pack/devtools) code base.</br>
The library interfaces are generated by the Simplified Wrapper and Interface Generator ([SWIG](http://www.swig.org)). Initially Python interface is provided. SWIG supports several other [scripting languages](http://www.swig.org/compat.html). 