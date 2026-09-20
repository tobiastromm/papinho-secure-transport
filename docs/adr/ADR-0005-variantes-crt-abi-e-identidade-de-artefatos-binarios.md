---
adr: ADR-0005
title: Variantes CRT/ABI e identidade de artefatos binários
status: accepted
decision-date: 2026-09-20
last-revised: 2026-09-20
revision: 2
scope-level: project
scope-target: PapinhoSecureTransport
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0005 — Variantes CRT/ABI e identidade de artefatos binários

## Contexto

A integração do SDK VC6 do PST com PapinhoBrowser expôs uma incompatibilidade factual que não pertence à API TLS/PST:

```text
PapinhoBrowser
VC6 /MD
    ×
PST VC6 package
CRT estático /ML
```

A mesma base factual `win32-x86-vc6-retrozilla-nss` não é suficiente para declarar esses artefatos intercambiáveis. Testar o Browser com `/MT` também não torna uma biblioteca `/ML` compatível.

O Browser possui worker threads e não deve ser convertido para `/ML` apenas para acomodar o package atual.

PapinhoEngineering/ADR-0002 revision 3 estabelece transversalmente que CRT/runtime model é uma dimensão ABI discriminante quando necessária para distinguir artefatos incompatíveis.

## Decisão

PST deve tratar o CRT/runtime model como parte explícita da identidade de variantes VC6 quando mais de uma variante incompatível for suportada/distribuída ou quando consumers precisarem selecionar entre elas.

\`\`\`text
mesmo source / commit / API / SPI
        │
        ├── CRT /ML
        │      ↓
        │   win32-x86-vc6-retrozilla-nss-ml
        │
        ├── CRT /MD
        │      ↓
        │   win32-x86-vc6-retrozilla-nss-md
        │
        └── CRT /MT, se algum dia suportada
               ↓
            win32-x86-vc6-retrozilla-nss-mt
\`\`\`

Para o consumer PapinhoBrowser, a variante requerida é VC6 + RetroZilla NSS + `/MD`.

### Naming

Quando a dimensão CRT discriminar o artefato, o Target/Variant ID e package name devem tornar a diferença factual e mecanicamente selecionável.

Forma canônica aprovada quando as variantes coexistem/são suportadas:

\`\`\`text
win32-x86-vc6-retrozilla-nss-ml
win32-x86-vc6-retrozilla-nss-md

papinho-secure-transport-<version>-win32-x86-vc6-retrozilla-nss-ml.zip
papinho-secure-transport-<version>-win32-x86-vc6-retrozilla-nss-md.zip
\`\`\`

O token `md` identifica a variante CRT dinâmica do VC6. Isso não transforma `-md` em sufixo obrigatório para targets onde CRT não seja uma dimensão discriminante.

O identificador histórico sem sufixo é preservado como evidência congelada, mas **não é a identidade canônica da variante /ML daqui para frente**.

\`\`\`text
historical ID
win32-x86-vc6-retrozilla-nss
        ↓ crosswalk factual
canonical CRT-qualified variant
win32-x86-vc6-retrozilla-nss-ml
\`\`\`

Releases/packages/evidências já congelados mantêm seus nomes originais. Novos artefatos /ML, caso continuem suportados/distribuídos, usam `-ml`.

### Imutabilidade e proveniência

Um package já publicado/frozen não pode ter sua `.lib` substituída por outra compilada com CRT diferente mantendo version/filename/hash conceitualmente iguais.

```text
source + build config
        ↓
build variante CRT
        ↓
testes PST
        ↓
package novo
        ↓
SHA-256 novo
        ↓
release governada
        ↓
consumer pin
```

Não é permitido resolver a integração copiando uma `.lib` recompilada localmente para dentro de um package histórico.

### Consumer requirements

O consumer não deve alterar seu CRT apenas para acomodar PST quando isso contrariar seus requisitos próprios.

```text
consumer requirement
        ↓
select compatible PST artifact

não:

PST artifact happens to be /ML
        ↓
force multithreaded consumer to /ML
```

### Ownership e CRT boundaries

PST continua responsável por liberar seus próprios handles/allocations através da API PST. Consumers não devem liberar memória PST diretamente com seu CRT.

Mesmo com essa boundary, o CRT do static library continua sendo requisito de integração/link compatível e deve ser documentado.

Uma futura DLL continua exigindo auditoria específica de ABI/loader/CRT; esta ADR não declara DLL support.

## Consequências

- a Target Matrix deve registrar CRT explicitamente;
- release packaging deve distinguir variantes incompatíveis;
- consumer-linking deve dizer qual CRT cada package requer;
- build/release tooling deve produzir e validar cada variante suportada separadamente;
- historical packages permanecem imutáveis;
- a variante /ML canônica usa `-ml` em novos artefatos; o ID sem sufixo fica apenas como identificador histórico;
- o Browser pode continuar `/MD` e selecionar package PST `/MD`.

## Não objetivos

Esta ADR não:

- obriga PST a distribuir /ML, /MT e /MD simultaneamente;
- declara /ML adequado a aplicações multithread;
- muda Public API ou Provider SPI;
- define por si só a próxima package version;
- autoriza substituir 0.6.1 in-place;
- cria DLL support.

## Verificação

Para cada variante CRT distribuída:

- build flags mostram o CRT esperado;
- library/package metadata registra CRT;
- extracted consumer compile/link usa o CRT compatível;
- testes funcionais relevantes passam;
- package possui nome/ID não ambíguo quando coexistem variantes;
- SHA-256 é calculado sobre o package final;
- nenhum package histórico é alterado.

## Relações

- `PapinhoEngineering/ADR-0002` revision 3 — identificação factual e durável de build targets; CRT/runtime model como dimensão ABI discriminante.
- `OrganizationEngineering/ADR-0004` — nomenclatura de projetos/produtos/artefatos.
- `docs/target-matrix.md`
- `docs/release-packaging.md`
- `docs/consumer-linking.md`

## Princípio

**Toolchain igual não garante compatibilidade binária completa. Quando CRT muda a compatibilidade do artefato, PST torna essa diferença explícita, testável e selecionável.**

## Histórico de revisões

| Revisão | Data | Descrição |
|---:|---|---|
| 1 | 2026-09-20 | Registra CRT como dimensão ABI discriminante e a necessidade da variante VC6 /MD. |
| 2 | 2026-09-20 | Torna a nomenclatura CRT simétrica: novos artefatos /ML usam `-ml`, /MD usa `-md`; o ID VC6/NSS sem sufixo é preservado somente como identificador histórico via crosswalk. |
