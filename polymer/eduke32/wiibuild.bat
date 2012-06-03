@echo off
setlocal ENABLEEXTENSIONS DISABLEDELAYEDEXPANSION

set PATH=C:\devkitPro\devkitPPC\bin;C:\devkitPro\msys\bin;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%

:: Detect versioning systems and pull the revision number:
for /f "delims=" %%G in ('svn info 2^>^&1 ^| grep Revision ^| cut -d " " -f 2') do @set rev=r%%G
if not "%rev%"=="" set vc=svn
if "%rev%"=="" for /f "delims=" %%G in ('git svn info 2^>^&1 ^| grep Revision ^| cut -d " " -f 2') do @set rev=r%%G
if not "%rev%"=="" set vc=git
if "%rev%"=="" set vc=none
if not "%rev%"=="" echo const char *s_buildRev = "%rev%";>source\rev.h
if "%rev%"=="" set rev=rXXXX

:: Get the current date:
for /f "delims=" %%G in ('"C:\MinGW\msys\1.0\bin\date.exe" +%%Y%%m%%d') do @set currentdate=%%G

:: Build:
set buildparameters=PLATFORM=WII %*

make veryclean %buildparameters%
make OPTLEVEL=2 LTO=0 %buildparameters%

if not exist "eduke32.elf" goto end

:: Package data:
xcopy /e /q /y Wii\apps apps\
for %%G in (eduke32) do for %%H in (.elf) do if exist "%%~G%%~H" move /y "%%~G%%~H" "apps\%%~G\boot%%~H"
for %%G in (eduke32) do for %%H in (.elf.map) do if exist "%%~G%%~H" del /f /q "%%~G%%~H"
"echo.exe" -e "    <version>%rev%</version>\n    <release_date>%currentdate%</release_date>" | "cat.exe" Wii\meta_1.xml - Wii\meta_2.xml >"apps\eduke32\meta.xml"
if exist "*.txt" copy /y "*.txt" "apps\eduke32\"
"ls.exe" -l -R apps

:end

:: Clean up revision number:
if "%vc%"=="svn" svn revert source\rev.h
if "%vc%"=="git" git checkout source\rev.h

endlocal
