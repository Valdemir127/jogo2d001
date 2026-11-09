---
name: benchmark
description: Roda o benchmark da simulação do Jogo2d001 e registra os resultados de forma padronizada em docs/benchmarks.md, para comparar com a linha de base. Use quando o usuário pedir "benchmark", "medir desempenho" ou ao concluir uma parte que afete o desempenho (chunks, sono, multithread).
---

# Benchmark da simulação

Objetivo: números **comparáveis** entre commits. Sem padrão, a comparação não vale nada.

## Regra principal: números oficiais são de RELEASE
- A DLL de **debug** é muito mais lenta e **não** conta como número oficial.
- Rodar `godot --headless --path . --script res://game/scripts/benchmark.gd` usa o editor, que normalmente
  carrega a DLL de **debug**. Esses números servem só para desenvolvimento e devem ser marcados como `debug`.
- Para número oficial em release, o benchmark precisa rodar no `.exe` exportado (como o teste de determinismo,
  por uma flag lida no `_ready()`, ex.: `--benchmark`). Se essa flag ainda não existe, **avise o usuário**
  e proponha criá-la; não registre debug como se fosse release.

## Passo 1: Preparar
- Garantir que a DLL do alvo está atualizada (skill `build`) e, para release, exportar (skill `export-release`).
- Fechar programas pesados. Se for notebook, ligado na tomada e sem modo de economia.
- Pegar o commit: `git rev-parse --short HEAD`
- Pegar a máquina:
  ```powershell
  Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors
  ```

## Passo 2: Executar
- Rodar o cenário **3 vezes** e usar a **mediana** das médias. Registrar também o máximo e o p99 quando o
  benchmark os fornecer.
- Usar sempre o mesmo número de ticks (padrão atual: 600) e o mesmo cenário.
- Aplicar timeout de 5 minutos por execução; se estourar, encerrar o processo e reportar.
- Cenários da Fase 2 (quando existirem): pior caso (tudo ativo), caso típico (mundo quase parado com uma
  região ativa) e janela ativa cheia (5x5 chunks), com 1, 2, 4 threads e o máximo da máquina.

## Passo 3: Registrar em `docs/benchmarks.md`
Acrescente linhas (não apague as antigas) nesta tabela; crie o arquivo com o cabeçalho se não existir:

| Data | Commit | Build | Máquina | Cenário | Mundo | Threads | Ticks | Média (ms) | Máx (ms) | P99 (ms) | Obs |
|---|---|---|---|---|---|---|---|---|---|---|---|

- `Build` = `release` ou `debug`.
- Use ponto como separador decimal e 3 casas.
- Na coluna `Obs`, anote se o sono de chunks estava ligado e qualquer anomalia.

## Passo 4: Comparar e reportar
- Compare com a linha de base (primeira linha `release` do mesmo cenário) e com a medição anterior.
- Reporte: variação percentual da média e do máximo.
- Variação menor que ~5% entre execuções é ruído; não declare melhora ou piora nesse caso.
- Meta do `CLAUDE.md`: 60 ticks/s estáveis com a janela ativa cheia (tick abaixo de 16,6 ms, idealmente
  com folga). Diga se a meta foi atingida.

## Manutenção
Se o script do benchmark, a flag ou o formato da tabela mudarem, atualize esta skill.
