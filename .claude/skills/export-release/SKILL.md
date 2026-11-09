---
name: export-release
description: Exporta o Jogo2d001 para um executável Windows de release de forma segura (editor fechado, DLL de release atualizada, .gdextension e DLL nas pastas corretas) e valida com o teste de determinismo usando timeout. Use quando o usuário pedir para "exportar o jogo", "gerar o executável" ou "testar em release".
---

# Exportar Release do Jogo2d001

Gera `build\jogo2d001_release.exe` com a GDExtension funcionando, evitando: "DLL não encontrada",
editor bloqueando a DLL e travamento em loop no modo headless.

## Passo 0: A DLL de release existe e está atualizada?
```powershell
$dll = Get-Item bin\jogo2d001.windows.template_release.x86_64.dll -ErrorAction SilentlyContinue
$src = Get-ChildItem extension\src -Recurse -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $dll) { "DLL de release NAO EXISTE" }
elseif ($dll.LastWriteTime -lt $src.LastWriteTime) { "DLL de release DESATUALIZADA (fonte mais novo: $($src.Name))" }
else { "DLL de release OK" }
```
- Se não existe ou está desatualizada: **PARE**. Use a skill `build` (alvo release) ou peça ao usuário
  para compilar no terminal MSYS2 MinGW 64-bit. Não exporte com DLL velha.

## Passo 1: Nada pode estar aberto
```powershell
Get-Process godot*, jogo2d001* -ErrorAction SilentlyContinue
```
- Se retornar algum processo (editor **ou** o jogo exportado): **PARE** e avise:
  "Feche o Godot e o jogo exportado antes de exportar (eles bloqueiam as DLLs e a pasta build)."
  Aguarde a confirmação do usuário. Não encerre processos por conta própria.

## Passo 2: Executar o script de build e checar o código de saída
```powershell
.\build_release.bat
$LASTEXITCODE
```
O script limpa `build\`, exporta o projeto e copia o `.gdextension` e a DLL de release.
- Código diferente de 0: mostre as últimas linhas do log e **PARE**. Não siga para o passo 3.

## Passo 3: Validar a estrutura gerada
```powershell
Test-Path build\jogo2d001_release.exe
Test-Path build\jogo2d001.gdextension
Test-Path build\bin\jogo2d001.windows.template_release.x86_64.dll
```
Os três devem ser `True`. Se algum for `False`, o script falhou: investigue o log antes de continuar.

## Passo 4: Teste de determinismo com timeout (recomendado)
O `.exe` pode travar em loop se a DLL não carregar. **Nunca rode sem limite de tempo.**
```powershell
$out = "build\det_out.txt"; $err = "build\det_err.txt"
$p = Start-Process cmd.exe -ArgumentList "/c","test_determinism.bat" -PassThru -NoNewWindow `
     -RedirectStandardOutput $out -RedirectStandardError $err
if (-not $p.WaitForExit(60000)) {
  taskkill /PID $p.Id /T /F | Out-Null
  Get-Process jogo2d001* -ErrorAction SilentlyContinue | Stop-Process -Force
  "TIMEOUT: teste travou (provavel falha ao carregar a DLL)"
}
Get-Content $out; Get-Content $err
```
**Comparação com o valor dourado (não use hash escrito de memória):**
1. Leia `tests\determinism_golden.txt` para ver o formato e os cenários.
2. Compare **cada** linha `[det] hash=...` da saída com a entrada correspondente do arquivo.
3. Todos iguais = sucesso. Qualquer diferença = **falha**: reporte os valores obtidos e esperados,
   sem editar o arquivo dourado. Só a skill `update-golden` pode alterá-lo.
4. Timeout ou saída vazia = falha de carregamento (volte ao Passo 3).

## Relatório final (curto)
- Exportação: ✓/✗ e código de saída
- Arquivos validados: ✓/✗ por item
- Determinismo: ✓/✗, hash(es) obtido(s) vs. esperado(s)
- Hash do commit atual (`git rev-parse --short HEAD`)

## Erros comuns
- `Failed to open ...template_debug.dll`: editor aberto em segundo plano. Peça ao usuário para encerrá-lo
  no Gerenciador de Tarefas.
- `Can't open GDExtension dynamic library` no `.exe`: o `.gdextension` ou a DLL não foram para `build\`.
  Leia `build_release.bat` para confirmar o que ele copia e, se precisar, repita manualmente:
  ```powershell
  New-Item -ItemType Directory -Force build\bin | Out-Null
  Copy-Item bin\jogo2d001.gdextension build\jogo2d001.gdextension -Force
  Copy-Item bin\jogo2d001.windows.template_release.x86_64.dll build\bin\ -Force
  ```
- Jogo trava no modo headless: a DLL não carregou e o Godot ficou em loop. Volte ao Passo 3.

## Manutenção
Se o `build_release.bat`, os nomes das DLLs ou o formato do arquivo dourado mudarem, atualize esta skill.
