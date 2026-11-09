@echo off
echo ========================================
echo Executando teste de determinismo (Release)
echo ========================================
build\jogo2d001_release.console.exe --headless --test-determinism

echo.
echo ========================================
echo Teste finalizado.
echo ========================================
pause