@echo off
echo Adding Pico SDK environment variables...

:: Set the core SDK paths
set PICO_SDK_PATH=%USERPROFILE%\.pico-sdk\sdk\2.2.0
set PICO_TOOLCHAIN_PATH=%USERPROFILE%\.pico-sdk\toolchain\14_2_Rel1

:: Add the toolchain, CMake, Ninja, and picotool to the command PATH
:: This is the most critical step
set PATH=%USERPROFILE%\.pico-sdk\toolchain\14_2_Rel1\bin;%USERPROFILE%\.pico-sdk\cmake\v3.31.5\bin;%USERPROFILE%\.pico-sdk\ninja\v1.12.1;%USERPROFILE%\.pico-sdk\picotool\2.2.0\picotool;%PATH%

echo.
echo Pico environment is ready.
echo.