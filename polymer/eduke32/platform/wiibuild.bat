@echo off
setlocal ENABLEEXTENSIONS DISABLEDELAYEDEXPANSION

set targets=eduke32 mapster32
set PATH=C:\devkitPro\devkitPPC\bin;C:\devkitPro\msys\bin;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%

pushd "%~dp0.."
set wiidir=platform\Wii

:: Detect versioning systems and pull the revision number:
for /f "delims=" %%G in ('svn info 2^>^&1 ^| grep Revision ^| cut -d " " -f 2') do @set rev=%%G
if not "%rev%"=="" set vc=svn
if "%rev%"=="" for /f "delims=" %%G in ('git svn info 2^>^&1 ^| grep Revision ^| cut -d " " -f 2') do @set rev=%%G
if not "%rev%"=="" set vc=git
if "%rev%"=="" set vc=none
if not "%rev%"=="" echo s_buildRev = "r%rev%";>source\rev.h
if "%rev%"=="" set rev=XXXX

:: Get the current date:
for /f "delims=" %%G in ('"C:\MinGW\msys\1.0\bin\date.exe" +%%Y%%m%%d') do @set currentdate=%%G

:: Build:
set commandline=make veryclean all OPTLEVEL=2 LTO=0 PLATFORM=WII %*
echo %commandline%
%commandline%

for %%G in (%targets%) do if not exist "%%~G.elf" goto end

:: Package data:
if not exist apps mkdir apps
for %%G in (%targets%) do xcopy /e /q /y %wiidir%\apps\%%~G apps\%%~G\
for %%G in (%targets%) do for %%H in (.elf) do if exist "%%~G%%~H" move /y "%%~G%%~H" "apps\%%~G\boot%%~H"
for %%G in (%targets%) do for %%H in (.elf.map) do if exist "%%~G%%~H" del /f /q "%%~G%%~H"
for %%G in (%targets%) do "echo.exe" -e "    <version>r%rev%</version>\n    <release_date>%currentdate%</release_date>" | "cat.exe" "%wiidir%\%%~G_meta_1.xml" - "%wiidir%\%%~G_meta_2.xml" >"apps\%%~G\meta.xml"

xcopy /e /q /y /EXCLUDE:%wiidir%\xcopy_exclude.txt package\common apps\eduke32\

xcopy /e /q /y /EXCLUDE:%wiidir%\xcopy_exclude.txt package\common apps\mapster32\
xcopy /e /q /y /EXCLUDE:%wiidir%\xcopy_exclude.txt package\sdk apps\mapster32\

"ls.exe" -l -R apps
7z.exe a -mx9 -t7z eduke32-wii-r%rev%.7z apps -xr!*.svn*

:end

:: Clean up revision number:
if "%vc%"=="svn" svn revert source\rev.h
if "%vc%"=="git" git checkout source\rev.h

endlocal
goto :eof
