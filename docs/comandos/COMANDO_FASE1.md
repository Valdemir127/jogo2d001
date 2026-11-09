# Comando: montar a Fase 1 do Jogo2d001 (em partes)

Você é um engenheiro de software sênior (C++ e Godot). Junto com este comando está o
`CLAUDE.md` do projeto. **Leia-o inteiro antes de qualquer coisa.** Ele é a fonte de
verdade: se este comando e o `CLAUDE.md` divergirem, siga o `CLAUDE.md` e me avise.

## Como trabalhar (vale para todas as partes)
1. Execute **uma parte por vez**, na ordem. Não adiante trabalho das partes seguintes.
2. Ao terminar a parte, rode os critérios de pronto, faça o commit e **pare**.
3. Responda no formato fixo de relatório (seção "Relatório de cada parte") e espere eu escrever
   `continuar` antes de iniciar a próxima.
4. Se algo estiver ambíguo, errado ou impossível, pare e pergunte; não contorne em silêncio.
5. Se não conseguir rodar algo (build, editor, teste), diga claramente o que **não** foi testado.
6. Respostas curtas e diretas. Comentários no código e mensagens de commit em português.

## Objetivo da Fase 1
Projeto Godot + GDExtension em C++ que compila no Windows e mostra um grid pequeno com
areia, água e pedra simulados, desenhados como textura, com medição de desempenho e
teste de determinismo.

## Decisões já aprovadas (não rediscutir)
| Tema | Decisão |
|---|---|
| Godot | 4.7.2 (última estável). Confirmar que existe e registrar no README |
| godot-cpp | Submódulo fixado em **commit ou tag específico** (nunca `master` solto), com `api_version=4.7`. Registrar o commit no README |
| Grid | 512x288, configurável por constantes |
| IDs | 0=air, 1=stone, 2=sand, 3=water |
| Buffer | SoA em `SimCore`: `mat_id` (uint16), `temp_vida` (uint8), `flags` (uint8) |
| `get_ids()` | Retorna `PackedByteArray` com `uint16` little-endian (2 bytes/célula) |
| Textura | `Image.FORMAT_R16`. No shader: `int id = int(round(texture(tex, uv).r * 65535.0))` |
| Fallback | Constante que ativa modo R8 (IDs < 256, valor `* 255.0` no shader). Validar R16 em execução no renderer usado pelo projeto e avisar se falhar |
| Hash | FNV-1a 64 bits sobre os bytes de `mat_id` + `temp_vida` + `flags` |
| Valor dourado | `tests/determinism_golden.txt`, versionado. Debug e release devem dar o **mesmo** valor |
| Classes | `SimCore` (C++ puro, sem Godot), `SimGrid` (herda `RefCounted`, único binding), `MaterialTable` (indexada por ID, valores fixos agora, pronta para o registro de mods) |
| Pincel | Raio padrão 8, ajustável com `+` e `-` |
| Paleta | stone `#808080`, sand `#c2b280`, water `#3a7bd5`, com leve variação por célula derivada da coordenada (só no shader) |
| Fonte única de materiais | A cena lê nome e cor da `MaterialTable`; sem repetir IDs e cores à mão em vários lugares (se ficar complexo, avise e deixe como pendência) |

## Estrutura de arquivos esperada
```
/project.godot
/.gitignore
/README.md
/game/cenas/teste.tscn
/game/cenas/benchmark.tscn
/game/scripts/teste.gd
/game/scripts/benchmark.gd
/game/shaders/paleta.gdshader
/extension/SConstruct
/extension/godot-cpp/            (submódulo)
/extension/src/sim_core.{h,cpp}
/extension/src/sim_grid.{h,cpp}
/extension/src/material_table.{h,cpp}
/extension/src/register_types.{h,cpp}
/bin/jogo2d001.gdextension
/tests/                          (teste de hash e valor dourado)
/maps  /mods  /docs              (vazias, com .gitkeep)
```

## Regras técnicas obrigatórias
- C++17 ou superior; `snake_case` para funções/variáveis, `PascalCase` para classes
- Simulação só com **inteiros** (sem float); RNG próprio (xorshift) com seed fixa
- **Sem alocação** dentro de `step()`: buffers criados uma vez e reutilizados
- Flag por célula para não mover a mesma célula duas vezes no mesmo tick
- Alternar a direção de varredura a cada tick
- `powder`: cai e desliza na diagonal. `liquid`: cai, desliza e espalha na horizontal
- Troca por densidade (água = 1000, areia = 1500: areia afunda na água)
- Uma única chamada C++ → Godot por frame para entregar o buffer
- Nenhum Node por célula; nenhuma chamada à API do Godot em loops por célula
- Não adicionar dependências além de `godot-cpp` sem me perguntar
- Não alterar a estrutura de pastas sem avisar

## Fora do escopo da Fase 1 (não implemente)
Chunks, dirty rects, sono de chunks, multithread, streaming, salvamento, fase de dados e
mods, máquinas, jogador, outros materiais, GPU compute.

---

## Parte 0 — Verificação e preparo
**Objetivo:** confirmar versões e ambiente antes de criar código.
Ações:
- [ ] Ler o `CLAUDE.md` e resumir em até 10 linhas o que entendeu
- [ ] Confirmar que Godot 4.7.2 e `api_version=4.7` existem; escolher e informar o commit/tag do `godot-cpp`
- [ ] Verificar ferramentas disponíveis: Python, SCons, compilador MSVC (ou MinGW), Git
- [ ] Listar dúvidas realmente necessárias (máximo 5)
Pronto quando: resumo entregue, versões confirmadas, ferramentas verificadas.
Commit: nenhum.

## Parte 1 — Esqueleto do repositório
Ações:
- [ ] Criar a estrutura de pastas acima (pastas vazias com `.gitkeep`)
- [ ] Criar `.gitignore` (`*.dll`, `*.obj`, `*.lib`, `*.exp`, `.scons_cache`, `.sconsign.dblite`, `.godot/`, builds)
- [ ] Inicializar o Git e adicionar `godot-cpp` como submódulo no commit/tag fixado
- [ ] Criar `project.godot` mínimo e um `README.md` inicial (versões usadas)
Pronto quando: repositório limpo, submódulo no commit fixado, estrutura conferida.
Commit: `chore: esqueleto do projeto e submódulo godot-cpp`

## Parte 2 — Build e extensão vazia
Ações:
- [ ] Criar `extension/SConstruct` para Windows (debug e release), saída em `/bin`
- [ ] Criar `bin/jogo2d001.gdextension` apontando para as DLLs
- [ ] Criar `register_types.{h,cpp}` com uma classe mínima de teste (ex.: método que retorna uma string)
- [ ] Compilar `template_debug` e `template_release`
- [ ] Abrir o projeto no editor da Godot e confirmar que a extensão carrega sem erros
Pronto quando: as duas builds passam e a extensão carrega no editor.
Commit: `feat: build da extensão e carregamento no editor`

## Parte 3 — Núcleo da simulação (sem Godot)
Ações:
- [ ] Implementar `MaterialTable` (estado, densidade, comportamento) para air, stone, sand, water
- [ ] Implementar `SimCore` com buffers SoA, `step()`, `paint()` e acesso aos buffers
- [ ] Implementar `powder`, `liquid`, troca por densidade, flag anti-duplo-movimento e varredura alternada
- [ ] Implementar o hash FNV-1a 64 bits sobre `mat_id` + `temp_vida` + `flags`
- [ ] Criar o teste em `tests/` (executável sem Godot, compilado pelo SCons): mesma seed e mesma sequência de pincel, N ticks, roda 2 vezes e compara o hash, imprime em hexadecimal
- [ ] Gerar o valor dourado em `tests/determinism_golden.txt` e fazer o teste compará-lo
- [ ] Rodar o teste em debug **e** release; os valores devem ser idênticos
- [ ] Explicar como verificou que `step()` não aloca
Pronto quando: testes passam em debug e release com o mesmo valor dourado.
Commit: `feat: núcleo da simulação, tabela de materiais e teste de determinismo`

## Parte 4 — Binding para o Godot
Ações:
- [ ] Criar `SimGrid` (herda `RefCounted`) envolvendo o `SimCore`
- [ ] Expor: `step()`, `paint(x, y, raio, material_id)`, `get_ids()` (`PackedByteArray` uint16 LE), `get_largura()`, `get_altura()`
- [ ] Expor também nomes e cores dos materiais a partir da `MaterialTable` (para a cena usar)
- [ ] Registrar a classe em `register_types.cpp` e remover a classe mínima da Parte 2
- [ ] Compilar debug e release e confirmar no editor que `SimGrid` pode ser instanciada
Pronto quando: um script de teste no editor cria o `SimGrid`, pinta, dá `step()` e lê `get_ids()` com o tamanho esperado.
Commit: `feat: SimGrid exposto ao Godot`

## Parte 5 — Cena de teste e renderização
Ações:
- [ ] `game/shaders/paleta.gdshader`: lê o ID da textura R16 e aplica a paleta com variação por coordenada
- [ ] `game/scripts/teste.gd` e `game/cenas/teste.tscn`: cria o `SimGrid`, monta `Image.FORMAT_R16` e `ImageTexture`, atualiza 1 vez por frame
- [ ] Validar R16 em execução no renderer do projeto; implementar a constante de fallback R8 e avisar claramente se R16 falhar
- [ ] Pincel com o mouse; teclas `1`, `2`, `3`, `0` = pedra, areia, água, apagar; `+` e `-` mudam o raio (padrão 8)
- [ ] Label com FPS, tempo do `step()` em ms e quantidade de células não vazias
- [ ] Nome e cor dos materiais lidos da `MaterialTable` (via `SimGrid`)
Pronto quando: areia forma montes, água nivela, areia afunda na água, pedra não se move, sem erros no log.
Commit: `feat: cena de teste, shader de paleta, pincel e interface`

## Parte 6 — Benchmark, README e entrega
Ações:
- [ ] `game/cenas/benchmark.tscn` e `benchmark.gd`: preenche o grid com areia e água, roda 600 ticks, mede em release o tempo médio e máximo do `step()` e imprime no log
- [ ] Completar o `README.md`: pré-requisitos, comandos de build (debug e release), como rodar a cena de teste, o benchmark e o teste de determinismo, versões e commit do `godot-cpp`
- [ ] Revisar `.gitignore` e a estrutura final de pastas
Pronto quando: benchmark roda e imprime números, README permite a outra pessoa compilar do zero.
Commit: `feat: benchmark e documentação`

---

## Relatório de cada parte (formato fixo)
1. Arquivos criados ou alterados
2. Comandos executados e resultado
3. Critérios de pronto: ✓ ou ✗, um por linha
4. O que não foi testado ou ficou pendente
5. Hash do commit e próxima parte

## Entrega final (após a Parte 6)
Além do relatório da Parte 6, entregue:
- Números do benchmark: tamanho do grid, máquina, tempo médio e máximo do `step()` em release
- Lista de pendências e decisões que precisam de mim
- Sugestões para a Fase 2 (chunks, dirty rects, sono de chunks e multithread), **sem implementá-las**
