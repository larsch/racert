@echo off
setlocal
set root=%cd%
for /F "tokens=* USEBACKQ" %%F IN (`git describe`) DO set version=%%F
rmdir /q/s release-build
mkdir release-build
pushd release-build
mkdir x86_64
pushd x86_64
cmake -G "Visual Studio 15 2017 Win64" %root% || exit /b 1
cmake --build . --config Release || exit /b 1
zip -j %root%/racert-%version%-win64.zip Release\racert.exe || exit /b 1
popd
mkdir x86
pushd x86
cmake -G "Visual Studio 15 2017" %root% || exit /b 1
cmake --build . --config Release || exit /b 1
zip -j %root%/racert-%version%-win32.zip Release\racert.exe || exit /b 1
popd
popd
