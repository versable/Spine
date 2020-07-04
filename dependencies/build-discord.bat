@echo OFF

call build-common.bat %1 %2

Set ARCHIVE=discord_game_sdk.zip
Set BUILD_DIR=%TMP_DIR%/
Set PREFIX=%DEP_DIR%/%ARCH_DIR%/Discord

IF EXIST %PREFIX% EXIT /B

echo "Compile Discord"

echo "Extracting Discord"

call build-common.bat downloadAndUnpack %ARCHIVE% %BUILD_DIR% https://dl-game-sdk.discordapp.net/latest/

echo "Configuring Discord"

mkdir "%PREFIX%"
mkdir "%PREFIX%\include"
mkdir "%PREFIX%\lib"
mkdir "%PREFIX%\src"

xcopy /S /Y "%BUILD_DIR%\cpp\*.h" "%PREFIX%\include\"
xcopy /S /Y "%BUILD_DIR%\cpp\*.cpp" "%PREFIX%\src\"
xcopy /F "%BUILD_DIR%/lib/x86/discord_game_sdk.dll" "%PREFIX%/lib/discord_game_sdk.dll*"
xcopy /F "%BUILD_DIR%/lib/x86/discord_game_sdk.dll.lib" "%PREFIX%/lib/discord_game_sdk.dll.lib*"

echo "Cleaning up"

cd %DEP_DIR%
RD /S /Q "%TMP_DIR%"
