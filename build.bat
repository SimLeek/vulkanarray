if "%1" == "" (
    ECHO DEFAULTING CMAKE_GENERATOR TO Visual Studio 16 2019. Pass in an argument if you wish to change
    set argg="Visual Studio 16 2019"
) else (
    echo %1
    set argg="%1"
)


if "%2" == "" (
    ECHO DEFAULTING TARGET_PLATFORM TO x64. Pass in a second argument if you wish to change this.
    set arga="x64"
) else (
    echo %2
    set arga="%2"
)

RMDIR /Q /S build
MKDIR build
PUSHD build

cmake .. -G %argg% -A %arga%
cmake --build . --config Release

bin\md5.exe

PUSHD ..