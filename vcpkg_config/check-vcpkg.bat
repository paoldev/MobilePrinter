@echo off
setlocal enabledelayedexpansion

set VCPKG_DIR=%~dp0..\vcpkg

rem This sha1 commit hash MUST match '"builtin-baseline":"hashcode"' in all vcpkg.json manifest files.
rem To use latest vcpkg version, set VCPKG_COMMIT_ID=latest and remove "builtin-baseline" and "version>=" properties from any vcpkg.json manifest file.
set VCPKG_COMMIT_ID=04f7d34c26defd9a54397dcbbb26b1ce3573614d

if "%VCPKG_INSTALLATION_ROOT%" equ "" (
call :should_install_vcpkg
if "!ERRORLEVEL!"=="0" goto :eof
if exist "%VCPKG_DIR%\" rmdir /s /q "%VCPKG_DIR%\"
mkdir "%VCPKG_DIR%\"
cd /D "%VCPKG_DIR%\..\"
if /i "%VCPKG_COMMIT_ID%" equ "latest" (
  call :vcpkg_latest
) else (
  call :vcpkg_commit %VCPKG_COMMIT_ID%
)
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics
echo. > "%VCPKG_DIR%\vcpkg_my_id.%VCPKG_COMMIT_ID%.txt"
)

goto :eof

:should_install_vcpkg
if not exist "%VCPKG_DIR%\vcpkg.exe" exit /b 1
if not exist "%VCPKG_DIR%\vcpkg_my_id.%VCPKG_COMMIT_ID%.txt" exit /b 1
exit /b 0

:vcpkg_latest
git clone --depth 1 https://github.com/microsoft/vcpkg
exit /b

:vcpkg_commit
git clone https://github.com/microsoft/vcpkg
pushd vcpkg
git checkout %1
popd
exit /b
