@echo off
echo ========================================
echo 1. Limpando a pasta build...
echo ========================================
if exist build rmdir /s /q build
mkdir build
mkdir build\bin

echo.
echo ========================================
echo 2. Exportando o projeto (Release)...
echo ========================================
call godot.cmd --headless --export-release "Windows Desktop" build\jogo2d001_release.exe

echo.
echo ========================================
echo 3. Copiando arquivos da GDExtension...
echo ========================================
copy bin\jogo2d001.gdextension build\
copy bin\jogo2d001.windows.template_release.x86_64.dll build\bin\

echo.
echo ========================================
echo Exportacao concluida com sucesso!
echo ========================================
pause