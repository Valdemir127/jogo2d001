# Jogo2d001 — Fase 1

Grid 512x288 com areia, água e pedra. Simulação em C++ via GDExtension, renderizada como textura R16 com paleta aplicada em shader.

## Versões fixadas
- Godot **4.7.2** (Forward+)
- `godot-cpp` branch `master`, commit **`b472519c42be09c1acec3fd63acd4010fc586ad4`** (submódulo)

## Build (Windows, PowerShell)

```powershell
git submodule update --init --recursive
cd extension
scons platform=windows target=template_debug   -j4
scons platform=windows target=template_release -j4