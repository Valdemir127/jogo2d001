# CLAUDE.md — Jogo2d001

## Visão geral
Jogo 2D de simulação de partículas/materiais (falling sand), com mapa fixo e grande.
Inspirações: Noita (principal, base do mapa), Sandustry, Terraria, The Powder Toy.
Estilo: mistura de exploração/combate (Noita), sobrevivência/construção (Terraria),
sandbox criativo (Powder Toy) e automação/fábrica (Sandustry).
História: ainda não definida. Não inventar lore, personagens ou narrativa.

Fase atual: **modo livre** para testar mecânicas e materiais. A forma do jogador
(magias, armas ou ferramentas) será decidida depois. Não implementar combate,
inventário ou progressão até isso ser decidido.

Prioridades desde o início: **desempenho nível Factorio** e **suporte a mods**.

## Estado atual
**Fase 1 concluída.** Grid 512x288 com ar, pedra, areia e água; simulação em C++ puro
(`SimCore`) exposta ao Godot por `SimGrid` (`RefCounted`); textura R16 com shader de paleta;
pincel com teclado; `MaterialTable` com IDs fixos (0=air, 1=stone, 2=sand, 3=water).
- Hash de determinismo do cenário de teste: **`a2e6b9a4026a64b3`** (debug e release),
  guardado em `tests/determinism_golden.txt`
- Próximo: **Fase 2** (chunks, dirty rects, sono de chunks e multithread)

## Stack
- Engine: Godot (última versão estável 4.x), desenvolvimento em Windows
- Lógica pesada: C++ via GDExtension (godot-cpp), compilada com SCons
- Compilador: **MinGW (via MSYS2)**; `godot-cpp` fixado no commit
  `b472519c42be09c1acec3fd63acd4010fc586ad4` com `api_version=4.6` (este commit ainda não
  suporta a 4.7; a extensão roda no editor 4.7.2). Revisar quando o `godot-cpp` suportar a 4.7
- GDScript apenas para cola, UI e protótipos rápidos
- Mods: orientados a dados (JSON), carregados por um registro em C++ (ver seção Mods)

## Regra de ouro: C++ primeiro
Tudo que puder ser feito em C++ vai para a GDExtension:
- Simulação de células/materiais, física, fluidos, gases, fogo, reações
- Máquinas, itens e transporte de recursos (automação)
- Geração/carregamento do mapa, streaming e salvamento
- Colisão e consultas espaciais (raycast no grid)
- Sistema de mods (carregador e registro de dados)
Fica no Godot: input, UI, câmera, áudio, cenas, ferramentas de editor.
Em dúvida, perguntar antes de implementar em GDScript algo que roda por frame.

## Metas de desempenho (referência: Factorio)
Valores iniciais, ajustar após os primeiros benchmarks:
- Simulação: 60 ticks/s estáveis com a janela ativa cheia (ex.: 5x5 chunks)
- Thread principal: sem picos por carregamento ou save (alvo < 2 ms de trabalho em qualquer frame)
- Carregamento inicial: poucos segundos, independente do tamanho do mapa
- Automação: milhares de máquinas/itens ativos sem queda de ups
- Regra: nenhuma feature entra se estourar o orçamento sem decisão explícita
- Profiling desde o dia 1 (Tracy ou equivalente) e cena de benchmark reproduzível

## Arquitetura da simulação (desempenho)
- Dados em struct-of-arrays por chunk: `mat_id` (uint16), `temp/vida` (uint8), `flags` (uint8)
  em arrays separados (melhor cache, SIMD e compressão)
- Chunks de simulação 64x64 com dirty rects; chunk sem mudanças por N ticks **dorme** e custa zero
- Propriedades dos materiais em tabela indexada por ID, sem chamadas virtuais no loop
- Reações em matriz pré-computada [material A][material B] → reação (consulta O(1))
- Sem alocação dentro do tick: pools e arenas, buffers reutilizados
- Só inteiros na simulação (sem float): ganha velocidade, determinismo e testes reproduzíveis
- RNG próprio (xorshift) por chunk, com seed derivada de tick + coordenada do chunk
- Multithread: job system com chunks em padrão xadrez (fases), sem travas no meio do tick;
  evitar false sharing (alinhar dados por chunk a 64 bytes)
- Varreduras quentes escritas para auto-vetorização/SIMD (AVX2 com fallback)
- Orçamento de tempo por tick: se estourar, adia trabalho de baixa prioridade em vez de travar
- Alternar a direção de varredura a cada tick para evitar viés

## Mapa e chunks
- Fixo (não procedural), baseado em Noita, tamanho comparável (mais de 1 bilhão de células)
- Nunca carregar o mapa inteiro (seria ~1,5 GB só com 1 byte por célula)
- Coordenadas `int32` no mundo desde o início
- Chunks de carregamento 256x256; subchunks de simulação 64x64
- Autoria: PNGs por região/bioma, cor → material (só para editar)
- Jogo: conversor gera chunks binários comprimidos (RLE funciona muito bem aqui + LZ4/zstd)
- Mapa de teste inicial (ex.: 4096x2048), com código pronto para escalar
- Só os chunks perto do jogador ficam ativos; o resto fica congelado no disco

## Carregamento
- Arquivo de região com cabeçalho, índice e chunks de acesso direto (sem parse por célula)
- Leitura assíncrona (memory-mapped ou I/O em thread), descompressão em workers
- Prefetch na direção do movimento do jogador
- Chunks novos entram na simulação só entre dois ticks, via fila trocada no início do tick
- Nunca bloquear a thread principal esperando disco

## Salvamento
- Mapa base somente leitura; o save guarda apenas chunks modificados
- Snapshot no tick: copia só os chunks sujos e serializa em thread de fundo
  (no Windows não há fork, então a cópia é feita no tick e é barata por ser só o delta)
- Escrita em arquivo temporário + rename atômico (nunca corromper o save)
- Um arquivo por região, com índice, compressão (LZ4/zstd) e versão no cabeçalho
- Autosave contínuo e incremental, sem pausa perceptível
- Save usa nomes estáveis de material (`mod:nome`) com paleta por save, não IDs numéricos

## Automação: máquinas e recursos
- Máquinas, itens e esteiras como dados em arrays (SoA) em C++, **sem Node por entidade**
- Entidades inativas não custam nada (ativação por evento, não por polling)
- Esteiras/transporte em segmentos (linhas de itens), não item por item
- Atualização por eventos agendados (fila por tick futuro) em vez de checar tudo todo tick
- Renderização em lote (MultiMesh ou textura), sem nós individuais

## Renderização
- Janela de células vira uma textura de IDs (R16); um shader aplica a paleta na GPU
- Enviar só as regiões sujas (atualização parcial de textura)
- Um único ponto de chamada por frame entre C++ e Godot; sem chamar a API do Godot em loops quentes
- Viewport interno pequeno (sugestão: 640x360) com escala inteira; 1 célula = 1 pixel lógico (a confirmar)
- GPU compute só depois, se a CPU não bastar

## Mods (desde o início)
Abordagem escolhida: **mods orientados a dados**, estilo fase de dados do Factorio.
- Mods são arquivos de dados (JSON), sem código executável
- No carregamento, o registro em C++ lê todos os mods e compila tudo em tabelas
  (materiais, reações, máquinas, receitas) antes de o jogo começar: custo zero em runtime
- Sem scripts nem DLLs nativas: seguro por construção e determinístico
- O jogo base é implementado como um mod, para validar a API desde o começo
Formato:
- Pasta ou zip com `mod.json` (id, versão, dependências, versão do jogo), `data/` e `assets/`
- Definições em JSON: `materials`, `reactions`, `machines`, `recipes`
- Nomes de conteúdo com namespace: `mod_id:nome`
- Ordem de carga resolvida por dependências; conflitos e erros de validação vão para o log
- Validação rigorosa dos dados (tipos, limites, referências) com mensagens claras
- Documentar o formato e publicar um mod de exemplo
Preparação para o futuro:
- Scripting (Lua/Luau) fica **fora de escopo** por enquanto; se vier depois, só para eventos
  de baixa frequência, nunca por célula, e com sandbox. Manter o registro de dados
  independente disso para não precisar refazer

## Esquema de dados dos mods
Regra: JSON é só para **autoria**. No carregamento tudo vira inteiros e tabelas compactas
(a simulação continua sem float). Comportamentos são **primitivas declarativas** fixas
no C++ que os mods combinam; novos comportamentos entram no núcleo do jogo, não via mod.

`mod.json`:
```json
{ "id": "base", "version": "0.1.0", "game_version": ">=0.1", "dependencies": [] }
```

Material (`data/materials/*.json`):
```json
{
  "id": "base:sand",
  "name": "Areia",
  "state": "powder",
  "colors": ["#c2b280", "#b8a574", "#cbbd8c"],
  "density": 1500,
  "tags": ["sand"],
  "behavior": { "type": "powder", "slide": true },
  "flammability": 0,
  "hardness": 1
}
```
- `state`: `solid` | `powder` | `liquid` | `gas` | `ephemeral`
- `behavior.type`: `static` | `powder` | `liquid` (com `viscosity`) | `gas` (com `rise_speed`)
- Modificadores combináveis: `flammable` (`ignition_temp`, `burn_time`, `burn_to`),
  `corrosive` (`strength`), `conductive`, `evaporates_to`, `freezes_to`,
  `decays` (`lifetime` → `becomes`), `spreads` (cresce sobre materiais-alvo)
- `colors`: variações sorteadas por célula (visual no shader, sem custo na simulação)

Reação (`data/reactions/*.json`):
```json
{
  "id": "base:water_lava",
  "inputs": ["base:water", "base:lava"],
  "outputs": ["base:steam", "base:stone"],
  "chance": 0.5,
  "priority": 0
}
```
- `inputs` aceitam IDs ou tags (`"#liquid"`); tags são expandidas no carregamento
- `outputs` na mesma ordem dos inputs (posição de cada célula); `null` mantém a célula
- `chance` em 0–1 no JSON, convertido para inteiro 0–255 no carregamento
- Conflito entre reações do mesmo par: vence a maior `priority`; empate = erro no log,
  salvo `"override": true` explícito

Validação no carregamento (falhar com mensagem clara, nunca travar):
- IDs únicos e com namespace; toda referência existe; total de materiais ≤ 65535
- Campos fora do intervalo, tipos errados e ciclos de dependência entre mods
- Reações com ciclo infinito (A→B→A) geram aviso
- Valores numéricos limitados a faixas seguras (evita overflow e abuso)

Máquinas e receitas: esquema a definir depois do núcleo da simulação.

## Primitivas e materiais iniciais
Proposta inicial, ajustar durante o desenvolvimento (a lista de primitivas só cresce
quando um material novo realmente exigir).

Primitivas de comportamento: `static`, `powder`, `liquid`, `gas`.
Modificadores: `flammable`, `emits_heat`, `corrosive`, `conductive`, `evaporates_to`,
`freezes_to`, `decays`, `spreads`.
Convenção: `base:air` é o material 0 (vazio). Densidade: água = 1000. Gases ignoram
densidade e usam `rise_speed`.

| Material | Estado | Comportamento | Modificadores |
|---|---|---|---|
| `base:stone` | solid | static | hardness alta |
| `base:wood` | solid | static | flammable → `base:fire` |
| `base:dirt` | powder | powder | densidade 1400 |
| `base:sand` | powder | powder (desliza) | densidade 1500 |
| `base:water` | liquid | liquid (viscosidade baixa) | densidade 1000, evaporates_to `base:steam` |
| `base:oil` | liquid | liquid (viscosidade média) | densidade 800 (flutua na água), flammable → `base:fire` |
| `base:lava` | liquid | liquid (viscosidade alta) | densidade 2500, emits_heat, freezes_to `base:stone` |
| `base:fire` | ephemeral | gas | emits_heat, decays → `base:smoke` |
| `base:smoke` | ephemeral | gas | decays → `base:air` |
| `base:steam` | gas | gas | decays → `base:water` (condensa) |

Reações iniciais (só pares que os modificadores não cobrem):
- `base:water` + `base:lava` → `base:steam` + `base:stone`
- `base:water` + `base:fire` → `base:steam` + `base:air`

O resto emerge dos modificadores: o fogo e a lava aquecem os vizinhos, madeira e óleo
inflamam e viram fogo, o fogo vira fumaça, a fumaça some, o vapor condensa.

Critério de aceitação do núcleo: esses 10 materiais funcionando juntos, com a simulação
dentro das metas de desempenho, antes de adicionar qualquer material novo.

## Estrutura de pastas
```
/project.godot
/game/            # cenas, GDScript, assets
/extension/       # C++ (GDExtension)
  /src/  /godot-cpp/ (submódulo)  SConstruct
/bin/             # DLLs + .gdextension
/maps/            # PNGs de autoria
/mods/            # mods embutidos (incluindo o jogo base)
/docs/
```

## Build (Windows)
- Requisitos: Python, SCons, **MinGW via MSYS2**, Git
- Debug: `scons platform=windows use_mingw=yes api_version=4.6 target=template_debug`
- Release: `scons platform=windows use_mingw=yes api_version=4.6 target=template_release`
- Recarregar a extensão ao recompilar (reabrir o editor se necessário)
- `build_release.bat`: exporta o build e copia as DLLs (o editor precisa estar **fechado**)
- `test_determinism.bat`: roda o teste de determinismo no `.exe` exportado
- O Godot não exporta scripts soltos (`--script` falha no `.exe`): o teste de release usa a
  flag customizada `--test-determinism`, lida no `_ready()` do `teste.gd`

## Convenções
- C++17 ou superior; `snake_case` para funções/variáveis, `PascalCase` para classes
- Sem alocação dinâmica nem exceções no loop de simulação
- Interface exposta ao Godot pequena e estável (`ClassDB::bind_method`)
- Testes unitários para a simulação (possível por ser determinística)
- Hash dourado: só muda com mudança **proposital** de comportamento ou de ordem de
  atualização; nesse caso, o commit explica o motivo e atualiza `tests/determinism_golden.txt`
  (guardar o valor antigo em `docs/`). Refatoração sem mudança de comportamento mantém o hash
- Debug e release devem sempre dar o mesmo hash
- Comentários e commits em português
- Medir antes de otimizar: registrar tempo de tick, de render, de load e de save

## Ordem de desenvolvimento
1. ✅ Grid pequeno com areia e água, renderizado como textura (concluída)
2. Chunks, dirty rects, sono de chunks e multithread
3. Mais materiais e reações (pedra, madeira, terra, óleo, lava, fogo, fumaça, vapor)
4. Fase de dados + API de mods (o jogo base já carregado como mod)
5. Streaming e conversor do mapa
6. Salvamento incremental
7. Máquinas e automação
8. Jogador (só depois do modo livre)

## Como o Claude deve trabalhar
- Respostas curtas e diretas; analisar com cuidado antes de propor mudanças grandes
- Explicar a decisão de arquitetura em 1–2 linhas
- Toda proposta de código deve considerar o orçamento de desempenho
- Não adicionar dependências nem alterar a estrutura de pastas sem avisar
- Ao propor C++, indicar também o binding e o uso no Godot

## Fora de escopo (por enquanto)
- História, diálogos e lore
- Multiplayer (mas manter determinismo para não fechar a porta)
- Mapa procedural
- Mobile/Web (foco em Windows)
- Scripting em mods (Lua/Luau/WASM): só depois, se houver demanda

## Perguntas em aberto
- Dimensões finais do mapa
- Confirmar tamanho da célula na tela
- Migrar para `api_version=4.7` quando o `godot-cpp` suportar
- Forma final do jogador (decidir após o modo livre)
- Esquema de máquinas e receitas (depois do núcleo da simulação)
- Confirmar a lista de primitivas e modificadores após testar os 10 materiais iniciais
