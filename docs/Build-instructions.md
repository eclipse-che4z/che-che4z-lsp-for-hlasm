In this chapter, we describe how to build the project on different platforms. We only describe methods that we use and are guaranteed to work, but other platforms and versions may work as well.

The result of a build is the Visual Studio Code extension packed into a VSIX file, which can be found in the `bin/` subdirectory of the build folder.

Prerequisites
-------------

In order to build the project on any platform, the following software needs to be installed:

-   CMake 3.10 or higher

-   C++ compiler with support for C++17

-   Java Development Kit (JDK) 8 or higher (the ANTLR project written in Java is built from sources)

-   Maven (the build system of ANTLR)

-   Git (to download sources of the third party software)

-   npm (to compile the typescript parts of the VS Code extension)

Windows
-------

On Windows, we use Visual Studio Community 2019. We also have VS configurations for building and testing the project in WSL.

It is also possible to build the project from the command line:

    mkdir build && cd build
    cmake ../
    cmake --build .

Linux
-----
In addition to the prerequisites listed in \[prereq\], the Linux build has two more prerequisites:

-   pkg-config

-   UUID library

We build the project for Ubuntu 20.04 and for the Alpine Linux.

### Ubuntu

On Ubuntu 20.04 the following commands install all prerequisites and then build the project into the `build` folder:

    apt update && sudo apt install cmake g++-10 uuid-dev npm default-jdk
                           pkg-config maven
    mkdir build && cd build
    cmake -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10 ../
    cmake --build .

### Alpine Linux

The build works on Alpine linux version 3.10. The following commands install all prerequisites and then build the project into the `build` folder:

    apk update && apk add linux-headers git g++ cmake util-linux-dev npm ninja
                          pkgconfig openjdk8 maven
    mkdir build && cd build
    cmake ../
    cmake --build .

Mac OS
------

We have only built the project on MacOS 10.14. In order to successfully build, first install LLVM 8 using homebrew.

The project can be built with a snippet like this:

    mkdir build && cd build
    cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
          -DLLVM_PATH=<path-to-llvm-installation> ../
    cmake --build .

For instance, a possible path to LLVM is `/usr/local/opt/llvm8`

Running Tests
-------------

Once the project is built, there are two test executables in the `bin/` subdirectory of the build folder: `library_test` and `server_test`. Run both of them to verify the build.

Installation
------------

The built VSIX can be manually installed into VS Code by following these steps:

1.  Open the extensions tab (Ctrl + Shift + X)

2.  Select “More actions ...” (the ⋯ icon)

3.  Select “Install from VSIX...”

4.  Find the VSIX file and confirm the selection.

5.  The plugin is now installed.

Alternatively, the plugin can be installed with following command:

    code --install-extension <path-to-vsix>
