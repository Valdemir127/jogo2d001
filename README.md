# Jogo2d001 — Fase 1

Grid 512×288 com areia, água e pedra. Simulação em C++ via GDExtension, renderizada como textura R16 com paleta aplicada em shader.

## Versões fixadas

- Godot **4.7.2** (renderer Forward+)
- `godot-cpp` branch `master`, commit **`b472519c42be09c1acec3fd63acd4010fc586ad4`** (submódulo)

## Clonar

```bash
git clone --recurse-submodules https://github.com/<seu-usuario>/jogo2d001.git
cd jogo2d001
```

Se você já clonou sem `--recurse-submodules`, rode depois:

```bash
git submodule update --init --recursive
```

## Build (Windows)

### Pré-requisitos

- [MSYS2](https://www.msys2.org/) com o toolchain MinGW-w64:
  ```bash
  pacman -S --needed mingw-w64-x86_64-gcc
  ```
- Python 3.10+
- SCons: `pip install scons`

### Compilação

Abra o terminal **"MSYS2 MinGW 64-bit"** (não use PowerShell/CMD) e execute, a partir da raiz do projeto:

```bash
cd extension
scons platform=windows target=template_debug   api_version=4.6 use_mingw=yes -j4
scons platform=windows target=template_release api_version=4.6 use_mingw=yes -j4
```

Os DLLs são gerados em `bin/`:

- `jogo2d001.windows.template_debug.x86_64.dll`
- `jogo2d001.windows.template_release.x86_64.dll`

A primeira compilação demora (10–25 min) porque o SCons compila o `godot-cpp` inteiro (~800–1000 arquivos `.cpp`). As seguintes recompilam só `extension/src/*.cpp` (~15 s).

### Flags

- **`api_version=4.6`**: o `godot-cpp` no commit fixado exige a flag explicitamente. `4.6` é compatível em runtime com o Godot 4.7; a Fase 1 não usa nenhuma API nova de 4.7.
- **`use_mingw=yes`**: força o uso do MinGW em vez de tentar o MSVC.
- **`-j4`**: 4 jobs paralelos. Ajuste conforme os núcleos da sua CPU.

## Execução

1. Abra o projeto no Godot 4.7.2 (Forward+).
2. Rode a cena principal `game/cenas/teste.tscn` (F5).

### Controles

- **Mouse** (segurar e arrastar): pintar
- **`1`** pedra · **`2`** areia · **`3`** água · **`0`** apagar
- **`+`** / **`-`**: raio do pincel

### HUD

Mostra FPS, tempo médio e máximo do `step()` em ms, material atual e raio do pincel.

## Determinismo

O teste roda o cenário fixo (`executar_cenario_teste`) duas vezes no mesmo processo, compara os hashes e valida contra o valor dourado versionado em `tests/determinism_golden.txt`.

```bash
godot --headless --path . --script res://game/scripts/teste_determinismo.gd
```

Saída esperada:

```
[det] hash1=a2e6b9a4026a64b3
[det] hash2=a2e6b9a4026a64b3
[det] OK
```

- **Se o golden estiver vazio** (`<preencher com o hash obtido>`), o script imprime o hash obtido. Copie para `tests/determinism_golden.txt` e rode de novo.
- **Se o hash divergir** do golden, algum commit anterior alterou o comportamento da simulação. Investigue antes de atualizar o golden.

> `godot` aqui é um atalho local. No Windows, use `.\godot` (wrapper `godot.cmd` na raiz) ou o caminho completo do executável.

## Benchmark

Roda 600 ticks no cenário fixo (`preencher_cenario_benchmark`), com tempo total, médio e máximo do `step()`.

```bash
godot --headless --path . --script res://game/scripts/benchmark.gd
```

Saída esperada:

```
[benchmark] grid=512x288 ticks=600
[benchmark] total=... ms  medio=... ms  max=... ms
[benchmark] hash final=...
```

> **Atenção**: o editor da Steam carrega a DLL `template_debug` (`-O0`). Para medir performance real, exporte o jogo em release (a DLL `template_release` é usada).

## Fallback R8

Se o renderer rejeitar `Image.FORMAT_R16` (não deve acontecer no Forward+), troque `USAR_R16 = true` para `false` no topo de `game/scripts/teste.gd`. Os IDs da Fase 1 são todos `< 256`, então R8 dá conta. O shader decodifica o ID com `round(raw * 255.0)` em vez de `round(raw * 65535.0)`.

## Estrutura

```
/project.godot
/game/            cenas, GDScript, shaders
/extension/       C++ (GDExtension) + submódulo godot-cpp
/bin/             DLLs + .gdextension
/maps/            PNGs de autoria (vazio na Fase 1)
/mods/            mods embutidos (vazio na Fase 1)
/docs/            documentação
/tests/           golden de determinismo
```