---
name: update-golden
description: Procedimento para atualizar o hash dourado de determinismo (tests/determinism_golden.txt) quando uma mudança de comportamento, de ordem de atualização ou de RNG da simulação for PROPOSITAL. Use quando o teste de determinismo falhar após uma mudança intencional, ou quando a parte do plano disser que o hash muda de propósito. Nunca use para "consertar" um teste que falha sem explicação.
---

# Atualizar o hash dourado (somente mudança proposital)

O hash dourado é a prova de que a simulação não mudou. Atualizá-lo sem motivo destrói essa prova.

## Passo 0: A mudança é mesmo proposital?
Só continue se **uma** destas for verdadeira:
- A parte do plano em execução diz que o hash muda (ex.: troca do RNG, nova ordem de atualização, multithread).
- O usuário disse explicitamente que mudou o comportamento da simulação.
Se for refatoração que deveria manter o comportamento: o hash **tem** que continuar igual. Se mudou, há um
bug. **PARE** e investigue; não atualize nada.

## Passo 1: Obter os novos hashes
Rode o teste de determinismo em **debug** (editor) e em **release** (skill `export-release`).
Condições obrigatórias antes de aceitar o novo valor:
- Debug e release dão o **mesmo** hash, em todos os cenários.
- Duas execuções seguidas dão o mesmo hash.
- Se existir opção de threads: 1 thread e N threads dão o **mesmo** hash.
Se qualquer condição falhar, é bug de determinismo. **PARE** e reporte; não atualize.

## Passo 2: Guardar o valor antigo
- Crie (ou complete) `docs/golden_history.md` com uma linha por mudança:
  `data | commit anterior | cenário | hash antigo | hash novo | motivo`
- Leia `tests/determinism_golden.txt` antes e preserve o formato e todos os cenários.

## Passo 3: Atualizar o arquivo dourado
- Edite **somente** `tests/determinism_golden.txt` com os novos valores.
- Não altere o código do teste só para fazê-lo passar.

## Passo 4: Confirmar
- Rode o teste de novo em debug e em release: ambos devem dar OK contra o novo arquivo.

## Passo 5: Commit
Mensagem em português, explicando o motivo, por exemplo:
`test: atualiza hash dourado (RNG sem estado por célula)`
No corpo: hash antigo, hash novo e por que o comportamento mudou. Se a mudança alterou também o
`CLAUDE.md` (seção "Estado atual"), atualize o hash lá no mesmo commit.

## Proibido
- Atualizar o hash porque o teste falhou "sem motivo claro".
- Aceitar hash diferente entre debug e release.
- Atualizar o hash sem registrar o valor antigo em `docs/golden_history.md`.

## Manutenção
Se o formato do arquivo dourado ou a forma de rodar o teste mudar, atualize esta skill.
