# Comando: Fase 2 do Jogo2d001 (chunks, dirty rects, sono e multithread)

Você é um engenheiro de software sênior (C++ e Godot). Junto com este comando estão o
`CLAUDE.md` atualizado e o repositório da Fase 1 concluída. **Leia o `CLAUDE.md` inteiro e
explore o código existente antes de qualquer coisa.** O `CLAUDE.md` é a fonte de verdade: se
este comando e ele divergirem, siga o `CLAUDE.md` e me avise.

## Como trabalhar (vale para todas as partes)
1. Execute **uma parte por vez**, na ordem. Não adiante trabalho das partes seguintes.
2. Ao terminar, rode os critérios de pronto, faça o commit e **pare**.
3. Responda no formato fixo de relatório e espere eu escrever `continuar`.
4. Se algo estiver ambíguo, errado ou impossível, pare e pergunte; não contorne em silêncio.
5. Diga claramente o que **não** foi testado.
6. Respostas curtas e diretas. Comentários e commits em português.
7. Use o build da Fase 1: MinGW (MSYS2), `use_mingw=yes api_version=4.6`, scripts
   `build_release.bat` e `test_determinism.bat`. Não mude de compilador nem de versão do
   `godot-cpp` sem me perguntar.

## Objetivo da Fase 2
Tornar a simulação escalável: mundo maior que a tela dividido em chunks, processando só o
que muda, em várias threads, mantendo o determinismo. Metas do `CLAUDE.md`: 60 ticks/s
estáveis com a janela ativa cheia e custo próximo de zero para chunks parados.

## Regra de ouro desta fase: o hash é a prova
Cada parte tem uma regra clara sobre o hash dourado (`tests/determinism_golden.txt`,
valor atual `a2e6b9a4026a64b3`):
- **Refatoração sem mudança de comportamento:** o hash **deve continuar igual**. Se mudar,
  há um bug; corrija antes de seguir.
- **Mudança proposital de ordem ou de RNG:** o hash muda. Nesse caso, o commit explica o
  motivo, guarda o valor antigo em `docs/` e atualiza o arquivo dourado.
- Debug e release devem sempre dar o mesmo hash. O teste de release continua pela flag
  `--test-determinism` (`test_determinism.bat`).

## Decisões já aprovadas (não rediscutir)
| Tema | Decisão |
|---|---|
| Chunk de simulação | 64x64 células |
| Mundo de teste | 2048x1152 (32x18 chunks), configurável por constantes; coordenadas `int32` |
| Armazenamento | Por chunk, em SoA (`mat_id`, `temp_vida`, `flags`), alinhado a 64 bytes, pool único alocado uma vez |
| Sono de chunk | Chunk sem mudanças por N ticks dorme (N configurável, sugestão 8) e custa zero |
| Acordar vizinhos | Mudança perto da borda (ou célula entrando de fora) acorda o chunk vizinho |
| RNG | Sem estado por célula: derivado de `seed + tick + x + y` (permite pular células sem alterar o resultado) |
| Multithread | Fases 2x2 (4 fases) de chunks para evitar conflito nas bordas, inclusive diagonais |
| Threads | Número configurável; resultado **idêntico** com 1 thread e com N threads |
| Renderização | Textura da **janela da câmera** (não do mundo inteiro), R16 com shader de paleta |
| Dependências | Nenhuma nova sem me perguntar (usar `std::thread` ou o job system do próprio código) |

## Regras técnicas obrigatórias
- Simulação só com inteiros; sem alocação dentro de `step()`
- Nenhum Node por célula nem por chunk; nenhuma chamada à API do Godot em loops quentes
- Uma única chamada C++ → Godot por frame para entregar a janela renderizada
- Sem travas (mutex) no meio do tick; sincronização só entre fases
- Evitar false sharing (dados de cada chunk alinhados a 64 bytes)
- `SimCore` continua C++ puro, sem dependência do Godot
- Não alterar a estrutura de pastas sem avisar

## Fora do escopo da Fase 2 (não implemente)
Streaming do disco, conversor de mapa, salvamento, fase de dados e mods, máquinas, jogador,
novos materiais, SIMD manual, GPU compute.

---

## Parte 0 — Leitura e linha de base
Ações:
- [ ] Ler o código da Fase 1 e resumir em até 10 linhas como está organizado
- [ ] Rodar o benchmark atual em release e registrar os números em `docs/benchmarks.md`
  (máquina, grid 512x288, tempo médio e máximo do `step()`)
- [ ] Confirmar que o hash atual (`a2e6b9a4026a64b3`) bate em debug e release
- [ ] Listar dúvidas realmente necessárias (máximo 5)
Pronto quando: resumo, linha de base registrada e hash confirmado.
Commit: `docs: linha de base de desempenho da fase 1`

## Parte 1 — Armazenamento em chunks (mesmo comportamento)
Regra do hash: **deve continuar `a2e6b9a4026a64b3`.**
Ações:
- [ ] Reorganizar o `SimCore` para guardar o mundo em chunks de 64x64 (pool único, SoA, alinhado a 64 bytes)
- [ ] Manter exatamente a mesma ordem de varredura e de RNG da Fase 1
- [ ] Acesso a vizinhos entre chunks correto (células na borda leem e escrevem no chunk vizinho)
- [ ] Manter o cenário de teste de 512x288 (8x4.5 chunks: se não for múltiplo, definir a regra de borda e documentar)
- [ ] Atualizar `SimGrid` mantendo a interface atual funcionando
Pronto quando: hash idêntico em debug e release, cena de teste funciona igual.
Commit: `refactor: mundo armazenado em chunks de 64x64`

## Parte 2 — RNG sem estado
Regra do hash: **muda de propósito.**
Ações:
- [ ] Trocar o xorshift sequencial por RNG derivado de `seed + tick + x + y` (função hash de inteiros, rápida)
- [ ] Guardar o hash antigo em `docs/` e gerar o novo valor dourado, explicando o motivo no commit
- [ ] Adicionar um segundo cenário de teste com valor dourado próprio (ex.: lago de água sobre areia com pedra)
- [ ] Verificar visualmente na cena de teste que o comportamento continua correto
Pronto quando: hashes dos dois cenários iguais em debug e release, comportamento visual correto.
Commit: `feat: RNG sem estado derivado da posição e do tick`

## Parte 3 — Dirty rects e sono de chunks
Regra do hash: **o resultado com sono deve ser idêntico ao resultado sem sono.**
Ações:
- [ ] Cada chunk guarda um retângulo sujo (área que mudou no tick) e um contador de ticks parado
- [ ] Chunk sem mudanças por N ticks dorme e é pulado no `step()`
- [ ] Mudança perto da borda, pincel e células vindas de fora acordam o chunk (e os vizinhos necessários)
- [ ] Constante de debug que desliga o sono (varredura total) para comparação
- [ ] Teste de equivalência: mesmos cenários rodam com e sem sono e **os hashes são idênticos**
- [ ] Exibir na interface: chunks ativos / chunks totais e tempo do `step()`
Pronto quando: teste de equivalência passa nos dois cenários, em debug e release.
Commit: `feat: dirty rects e sono de chunks`

## Parte 4 — Mundo grande e janela da câmera
Regra do hash: cenários antigos continuam iguais; novo cenário no mundo 2048x1152.
Ações:
- [ ] Mundo de teste 2048x1152 criado por constantes
- [ ] Câmera simples: mover (WASD ou setas) e zoom inteiro (roda do mouse)
- [ ] Renderizar só a janela da câmera: copiar a região visível para a textura R16 (tamanho fixo, ex.: 640x360) e entregar numa única chamada por frame
- [ ] Pincel funciona em coordenadas do mundo
- [ ] Interface mostra posição da câmera, chunks ativos e tempo do `step()`
- [ ] Novo cenário de teste no mundo grande com valor dourado
Pronto quando: dá para navegar o mundo grande, pintar e simular fora da tela, sem erros, com hash estável.
Commit: `feat: mundo grande e renderização da janela da câmera`

## Parte 5 — Multithread por fases
Regra do hash: **mesmo valor com 1 thread e com N threads.** O valor muda em relação à
Parte 4 por causa da nova ordem; atualizar o dourado com explicação.
Ações:
- [ ] Dividir o tick em 4 fases (chunks com `x%2` e `y%2` iguais rodam juntos)
- [ ] Distribuir os chunks ativos de cada fase entre as threads; barreira entre as fases
- [ ] Número de threads configurável (inclusive 1, que serve de referência)
- [ ] Sem travas dentro do tick; sem alocação; sem false sharing
- [ ] Teste: 1 thread vs N threads (ex.: 2, 4, 8) dão o **mesmo hash** em todos os cenários, em debug e release
- [ ] Teste de estresse repetido (ex.: 50 execuções) sem divergência
Pronto quando: hashes idênticos independentemente do número de threads, sem travamentos.
Commit: `feat: simulação multithread em 4 fases`

## Parte 6 — Benchmark, documentação e entrega
Ações:
- [ ] Benchmarks em release no mundo 2048x1152, com 1, 2 e 4 threads (e o máximo da máquina):
  1. pior caso: todos os chunks ativos (areia e água caindo em todo o mundo)
  2. caso típico: mundo quase todo parado com uma região ativa
  3. janela ativa cheia (5x5 chunks) para comparar com a meta de 60 ticks/s
- [ ] Registrar tempo médio, máximo e percentil 99 do `step()` em `docs/benchmarks.md`, comparando com a linha de base
- [ ] Atualizar o `README.md` (como rodar testes, benchmarks, opções de threads e sono)
- [ ] Revisar o `.gitignore` e a estrutura final
Pronto quando: números registrados e README permite reproduzir tudo.
Commit: `docs: benchmarks da fase 2 e documentação`

---

## Relatório de cada parte (formato fixo)
1. Arquivos criados ou alterados
2. Comandos executados e resultado
3. Critérios de pronto: ✓ ou ✗, um por linha
4. Hash (antes e depois) e se a mudança era esperada
5. O que não foi testado ou ficou pendente
6. Hash do commit e próxima parte

## Entrega final (após a Parte 6)
- Tabela de benchmarks (linha de base vs Fase 2, por número de threads)
- Se as metas do `CLAUDE.md` foram atingidas ou não, e os gargalos que apareceram
- Lista de pendências e decisões que precisam de mim
- Sugestões para a Fase 3 (materiais restantes e reações, depois fase de dados dos mods),
  **sem implementá-las**
