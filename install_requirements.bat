@echo off

goto check_Permissions

:check_Permissions
	:: https://stackoverflow.com/a/11995662/782170
    echo Administrative permissions required. Detecting permissions...

    net session >nul 2>&1
    if %errorLevel% == 0 (
        echo Success: Administrative permissions confirmed.
    ) else (
        echo Failure: Current permissions inadequate.
		exit /B
    )

call RefreshEnv.cmd

:: where /q choco
:: IF ERRORLEVEL 1 (
::     echo installing choco
::     Set-ExecutionPolicy Bypass -Scope Process
::     Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
::     call RefreshEnv.cmd
:: )
:: exit /b 0
:: where /q make
:: IF ERRORLEVEL 1 (
::     echo installing make
::     choco install make
::     call RefreshEnv.cmd
:: )

:: todo, get gfortran installing so we can actually use blas
:: where /q gfortran

if "%1" == "" (
    ECHO DEFAULTING CMAKE_GENERATOR TO Visual Studio 16 2019. Pass in an argument if you wish to change
    set argg="Visual Studio 16 2019"
) else (
    echo %1
    set argg="%1"
)


if "%2" == "" (
    ECHO DEFAULTING TARGET_PLATFORM TO Win32. Pass in a second argument if you wish to change this.
    set arga="Win32"
) else (
    echo %2
    set arga="%2"
)

if "%VULKAN_SDK%" == "" (
    curl --output vulkan-sdk.exe --url https://vulkan.lunarg.com/sdk/home#sdk/downloadConfirm/latest/windows/vulkan-sdk.exe
    vulkan-sdk.exe
    exit /B
)

set res=F
if not exist "bin\Win32\escapi.dll" set res=T
if not exist "bin\x64\escapi.dll" set res=T
if "%res%"=="T" (
    echo Could not fine escapi win32 and x64 dlls. Please place in .\bin\Win32 and .\bin\x64 respectively.
    echo Download from here: https://github.com/jarikomppa/escapi#binaries
    exit /B
)

vcpkg install sdl2[vulkan]:x86-windows vulkan:x86-windows
vcpkg install sdl2[vulkan]:x64-windows vulkan:x64-windows
vcpkg install catch2:x86-windows
vcpkg install catch2:x64-windows

if not exist "build_requirements" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

    RMDIR /Q /S build_requirements
    MKDIR build_requirements
    PUSHD build_requirements

    set arga="Win32"

    git clone https://github.com/xtensor-stack/xsimd.git
    PUSHD xsimd
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    git clone https://github.com/xtensor-stack/xtl.git
    PUSHD xtl
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    git clone https://github.com/xtensor-stack/xtensor.git
    PUSHD xtensor
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    git clone https://github.com/xtensor-stack/xtensor-blas.git
    PUSHD xtensor-blas
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    git clone https://github.com/xianyi/OpenBLAS.git
    PUSHD OpenBLAS
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    git clone https://github.com/Reference-LAPACK/lapack
    PUSHD lapack
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    set arga="x64"

    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    PUSHD xsimd
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD xtl
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD xtensor
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD xtensor-blas
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD OpenBLAS
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD lapack
    cmake -G %argg% -A %arga% -D CMAKE_TOOLCHAIN_FILE=D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake .
    msbuild INSTALL.vcxproj
    PUSHD ..

    PUSHD ..
) else (
    echo build_requirements folder detected. If you wish to reinstall requirements, please delete that folder.
)
