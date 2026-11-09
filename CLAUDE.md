# CLAUDE.md

Instruções persistentes para o Claude Code neste repositório. Leia antes de qualquer ação.

## Contexto
- Engine: **Godot 4.7.2** (Windows, versão estável)
- Tipo: jogo 2D de simulação de materiais (sandbox, sobrevivência, mundo aberto — visão de longo prazo)
- GDScript para gameplay/UI; C++ (godot-cpp/GDExtension) para simulação de alto desempenho
- Desenvolvedor solo, iniciante em Godot e no Claude Code
- SO: **Windows**

## Modo de trabalho — OBRIGATÓRIO
- Modo **assistente cirúrgico**: uma tarefa = um arquivo (ou poucos) = uma verificação = um commit.
- **NUNCA** opere em modo agente automático.
- Não edite múltiplos arquivos sem pedido explícito.
- Não rode `git add`, `git commit` ou `git push` — o desenvolvedor faz manualmente.
- Prefira mudanças pequenas e verificáveis a soluções "elegantes".

## Estilo de resposta
- Explicações curtas (2–4 frases).
- Português do Brasil.
- Blocos de código curtos e focados.
- Ao final de qualquer edição: liste o que mudou e **como verificar** (comando, clique, ou o que observar na tela).

## Estrutura de pastas
- `src/` — código GDScript do jogo
- `src/sim/` — código C++ (GDExtension)
- `tests/` — testes unitários (doctest para C++, GUT para GDScript — quando chegar a hora)
- `addons/` — plugins Godot
- `assets/` — texturas, sons, dados
- `docs/` — documentação do projeto

Não crie arquivos em pastas fora desta lista sem confirmar.

## Convenções
- GDScript: indentação de 4 espaços; `snake_case` para arquivos e variáveis; `PascalCase` para `class_name`.
- C++: seguir `.clang-format` do repo; preferir `std::` e RAII.
- Não introduza dependências novas sem pedido explícito.

## Incerteza
- Não invente APIs da Godot 4.7. Se não tiver certeza de uma API/flag, diga "não tenho certeza, verifique na documentação oficial".
- Se um passo depender de versão específica da Godot, sinalize.
- Nota: Godot 4.7 introduziu mudanças (ex.: Asset Library → Asset Store, `AreaLight3D`). Trate APIs de versões anteriores com suspeita.

## Proibições
- Não configurar subagentes, hooks, MCP servers, worktrees ou automações multi-agente.
- Não refatorar arquivos fora do escopo pedido.
- Não "construir o jogo inteiro" em uma sessão.