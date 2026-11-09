# Jogo2d001 — Fase 1

Grid 512×288 com areia, água e pedra. Simulação em C++ via GDExtension, renderizada como textura R16 com paleta aplicada em shader.

## Versões fixadas

- Godot **4.7.2** (renderer Forward+)
- `godot-cpp` branch `master`, commit **`b472519c42be09c1acec3fd63acd4010fc586ad4`** (submódulo)

## 1. Clonar

Como o projeto usa submódulos, o clone deve ser feito com a flag `--recurse-submodules`:

```bash
git clone --recurse-submodules https://github.com/<seu-usuario>/jogo2d001.git
cd jogo2d001
```

Se você já clonou sem a flag, rode depois:
```bash
git submodule update --init --recursive
```

## 2. Pré-requisitos (Windows)

- [MSYS2](https://www.msys2.org/) com o toolchain MinGW-w64:
  ```bash
  pacman -S --needed mingw-w64-x86_64-gcc
  ```
- Python 3.10+
- SCons: `pip install scons`
- **Godot 4.7.2 (Forward+)** instalado.

### Configurando o comando `godot` (Obrigatório para automação)
Os scripts `.bat` deste projeto usam o comando `godot`. Se o executável do Godot não estiver no PATH do sistema, crie um arquivo `godot.cmd` na raiz do projeto com o seguinte conteúdo (ajuste o caminho do executável se necessário):

```cmd
@echo off
"C:\Program Files (x86)\Steam\steamapps\common\Godot Engine\godot.windows.opt.tools.64.exe" %*
```

## 3. Compilação (Build)

Abra o terminal **"MSYS2 MinGW 64-bit"** (não use PowerShell/CMD) e execute, a partir da raiz do projeto:

```bash
cd extension
scons platform=windows target=template_debug   api_version=4.6 use_mingw=yes -j4
scons platform=windows target=template_release api_version=4.6 use_mingw=yes -j4
```

Os DLLs são gerados em `bin/`:
- `jogo2d001.windows.template_debug.x86_64.dll`
- `jogo2d001.windows.template_release.x86_64.dll`

*Nota: A primeira compilação demora (10–25 min) porque o SCons compila o `godot-cpp` inteiro (~800–1000 arquivos `.cpp`). As seguintes recompilam só `extension/src/*.cpp` (~15 s).*

## 4. Execução (Editor)

1. Abra o projeto no Godot 4.7.2.
2. Rode a cena principal `game/cenas/teste.tscn` (F5).

### Controles
- **Mouse** (segurar e arrastar): pintar
- **`1`** pedra · **`2`** areia · **`3`** água · **`0`** apagar
- **`+`** / **`-`**: raio do pincel

### HUD
Mostra FPS, tempo médio e máximo do `step()` em ms, material atual e raio do pincel.

## 5. Teste de Determinismo e Release

A simulação é validada contra o hash dourado **`a2e6b9a4026a64b3`** (versionado em `tests/determinism_golden.txt`).

### 5.1 Teste no Editor (Desenvolvimento)
Roda o cenário fixo duas vezes no mesmo processo e compara com o golden:
```powershell
.\godot --headless --path . --script res://game/scripts/teste_determinismo.gd
```
*Saída esperada: `[det] hash1=a2e6b9a4026a64b3` e `[det] OK`.*

### 5.2 Teste na Build Exportada (Release)
Para provar que as otimizações do compilador (`-O2`) não alteram a simulação, exporte o jogo e rode o teste no executável.

1. **Feche o editor Godot** (isso libera as DLLs para não corromper a exportação).
2. No PowerShell ou CMD, na raiz do projeto, rode:
   ```cmd
   .\build_release.bat
   ```
   *Esse script limpa a pasta `build/`, exporta o `.exe`, cria a subpasta `bin` e copia o `.gdextension` e a DLL de release para o local correto.*
3. Rode o script de teste:
   ```cmd
   .\test_determinism.bat
   ```
   *Saída esperada: `[det] hash=a2e6b9a4026a64b3`*

## 6. Benchmark
Roda 600 ticks no cenário fixo, com tempo total, médio e máximo do `step()`.
```powershell
.\godot --headless --path . --script res://game/scripts/benchmark.gd
```
*Saída esperada: `[benchmark] grid=512x288 ticks=600` e os tempos em ms.*

## 7. Solução de Problemas (Troubleshooting)

- **Erro ao exportar: "Failed to open ...template_debug.dll"**
  O editor do Godot está aberto. Feche-o e rode o `build_release.bat` novamente.
- **Erro "Can't open dynamic library" ao rodar o `.exe` exportado**
  A DLL não está na subpasta correta. O `build_release.bat` já resolve isso, mas confira se o arquivo `.gdextension` está na raiz de `build/` e a DLL em `build/bin/`.
- **O executável de release trava no modo `--headless`**
  Se o `.gdextension` ou a DLL estiverem faltando, o Godot entra em loop infinito ao tentar carregar a cena principal sem interface. Certifique-se de que o `build_release.bat` rodou sem erros.
- **Tela cinza ao abrir a cena no editor**
  Certifique-se de que a DLL de debug foi compilada e está em `bin/`. O `TextureRect` precisa carregar a textura sem falhas.

## 8. Estrutura de Pastas
```
/project.godot
/game/            cenas, GDScript, shaders
/extension/       C++ (GDExtension) + submódulo godot-cpp
/bin/             DLLs + .gdextension
/build/           (gerado pelos scripts, ignorado no git)
/maps/            PNGs de autoria (vazio na Fase 1)
/mods/            mods embutidos (vazio na Fase 1)
/docs/            documentação
/tests/           golden de determinismo
```