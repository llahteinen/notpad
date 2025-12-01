@echo off
echo Setting up environment for Qt usage...
:: Qt6_DIR e.g. D:\devtools\Qt\6.10.1\msvc2022_64
set PATH=%Qt6_DIR%\bin;%PATH%
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC

::set BUILD_TYPE=Debug
set BUILD_TYPE=Release
set EXE_NAME=NotPad.exe
set DEPLOY_PATH=.\install_%BUILD_TYPE%
set COMPILE_PATH=.\build\Desktop_Qt_6_10_1_MSVC2022_64bit-%BUILD_TYPE%\src

echo Copying executable...
xcopy /Y %COMPILE_PATH%\%EXE_NAME% %DEPLOY_PATH%\

echo Deploying Qt libraries...
windeployqt %DEPLOY_PATH%\%EXE_NAME% --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw --no-network --skip-plugin-types imageformats,iconengines
pause
