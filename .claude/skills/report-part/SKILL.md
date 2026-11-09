# report-part

Gera o relatório padronizado de conclusão de uma parte do comando da Fase 2.

## Uso
- Deve ser chamada ao final de cada parte.
- Analisa automaticamente:
    - Arquivos alterados desde o último commit (usando `git status` ou `git diff`).
    - Comandos executados (histórico da sessão).
    - Status dos critérios de pronto definidos no `COMANDO_FASE2.md`.
    - Hash atual e comparação com o anterior.

## Comportamento
O Claude deve compilar as informações e apresentar o bloco no formato fixo:
1. Arquivos criados ou alterados
2. Comandos executados e resultado
3. Critérios de pronto: ✓ ou ✗
4. Hash (antes e depois) e validade da mudança
5. O que não foi testado ou ficou pendente
6. Hash do commit e próxima parte
