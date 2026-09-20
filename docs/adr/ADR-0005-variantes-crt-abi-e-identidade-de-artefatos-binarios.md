---
adr: ADR-0005
title: Variantes CRT/ABI e identidade de artefatos binários
status: accepted
decision-date: 2026-09-20
last-revised: 2026-09-20
revision: 1
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

```text
base técnica
win32-x86-vc6-retrozilla-nss
        │
        ├── CRT /ML → variante distinta
        ├── CRT /MD → variante distinta
        └── CRT /MT → variante distinta, se algum dia suportada
```

Para o consumer PapinhoBrowser, a variante requerida é VC6 + RetroZilla NSS + `/MD`.

### Naming

Quando a dimensão CRT discriminar o artefato, o Target/Variant ID e package name devem tornar a diferença factual e mecanicamente selecionável.

Exemplo aprovado de forma:

```text
win32-x86-vc6-retrozilla-nss-md

papinho-secure-transport-<version>-win32-x86-vc6-retrozilla-nss-md.zip
```

O token `md` identifica a variante CRT dinâmica do VC6. Isso não transforma `-md` em sufixo obrigatório para targets onde CRT não seja uma dimensão discriminante.

A variante histórica/publicada sem esse sufixo não deve ser retroativamente renomeada ou reinterpretada.

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
