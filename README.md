Small and snappy notepad app with Win32 API, no WinUI, no AI slop, no bloat.

The idea is to have just enough features and remain lightweight and fast.

## Building

Compiles for Windows with MSVC and MinGW.

Linux or macOS not tested (so far).

### Requirements

- Qt 6.8+ and possibly compatible with some other versions
  - Originally developed using Qt 6.10.1, 6.11.1
- Qt 5 not compatible

### Shared linked (Qt default)

Install Qt 6 and build the project with Qt Creator or the IDE of your choice.

### Static linked

You need to build Qt from sources first. Static libraries are not provided by Qt.
- Download Qt source code using Qt Maintenance Tool
- Setup your command line environment
  - For example with MSVC:
    - Launch your Qt command line from the provided shortcut e.g. Qt 6.10.1 (MSVC 2022 64-bit)
    - Run vcvarsall.bat that is located in e.g. C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
- Configure the build
  - `cd /path/to/Qt/6.10.1` (sources would be located in /path/to/Qt/6.10.1/Src)
  - `mkdir build`
  - `cd build`
  - `..\Src\configure.bat -prefix /some/path/static_install_dir -static -submodules qtbase`
    - `-submodules qtbase` can be omitted if you want to build the whole Qt universe
    - This configures the release variant only
  - `cmake --build . --parallel` (or --parallel 8 for 8 threads etc)
  - `cmake --install .`
- Qt static libraries should be now installed in /some/path/static_install_dir

Now you are ready to build the project with Qt Creator or the IDE of your choice.
- Set CMAKE_PREFIX_PATH to /some/path/static_install_dir
  - Qt Creator uses CMAKE_PREFIX_PATH for setting all the required path variables.
- Run CMake and build.

## Licenses
This application uses the following open-source libraries:
- [Qt](https://www.qt.io/) (LGPL v3) – Copyright (C) The Qt Company Ltd. and other contributors
- [Notepad icons created by NX Icon - Flaticon](https://www.flaticon.com/free-icons/notepad)
- [Microsoft Fluent System Icons](https://github.com/microsoft/fluentui-system-icons)
- [SingleApplication](https://github.com/itay-grudev/singleapplication)
