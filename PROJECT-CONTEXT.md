---
project: PapinhoSecureTransport
display-name: PapinhoSecureTransport

relationship: OWN_PRODUCT
customer: null

product-families:
  - papinho-ecosystem

technology-profiles:
  - native-c
  - library

shared-frameworks: []

components: []
---

# Project Context — PapinhoSecureTransport

## Resumo

PapinhoSecureTransport (PST) é uma biblioteca de secure transport que fornece
uma interface comum para comunicação segura e mantém código específico de
providers fora da lógica principal dos consumers.

O projeto pertence à Product Family `papinho-ecosystem` e é tratado como
`OWN_PRODUCT` dentro do modelo de governança do OrganizationEngineering.

A baseline atual é uma biblioteca C nativa distribuída por targets específicos,
com providers substituíveis e suporte CLIENT/SERVER. O projeto não é definido
por uma única versão do Windows ou por um único provider. Plataformas e targets
concretos permanecem documentados pela matriz factual do próprio projeto.

## Governança efetiva

```text
OrganizationEngineering
+
relationship: OWN_PRODUCT
+
product-family: papinho-ecosystem
+
technology-profile: native-c
+
technology-profile: library
+
PapinhoSecureTransport project-local governance
```

Nenhum Shared Framework organizacional é atualmente declarado para o PST.

## Componentes

O projeto não declara neste momento componentes com Project Context separado.

Providers, targets e backends não são automaticamente `components` desta
classificação. Suas diferenças continuam governadas pela arquitetura, ADRs,
Provider SPI, target matrix e documentação específica do PST.

Caso no futuro uma parte do repositório necessite de contexto de governança
próprio, `components` poderá ser refinado de forma explícita.

## Fontes canônicas de governança

```text
Organization Engineering:
  repository: tobiastromm/organization-engineering

Product Family — papinho-ecosystem:
  repository: tobiastromm/papinho-engineering

Project-local:
  repository: tobiastromm/papinho-secure-transport
```

Paths locais absolutos e a default branch não constituem identidade normativa.
Em uma tarefa concreta, a branch/ref/working tree efetivamente trabalhada é a
fonte factual local, conforme a governança aplicável de agentes.

## ADRs locais

```text
docs/adr/
```

ADRs locais do PST registram decisões específicas do projeto.

Novas ADRs e futuras migrações de metadata devem seguir a governança canônica de
ADRs do OrganizationEngineering e o template global aplicável.

## Capability Documents

Quando aplicável, documentação viva de capabilities deve permanecer separada de
ADRs e de evidência factual em Code/Tests.

O Project Context não redefine paths ou categorias documentais já governadas
pelos ADRs e documentos vivos do projeto.

## Workflows aplicáveis

Workflows organization-wide deverão ser utilizados quando forem criados e forem
aplicáveis ao projeto, incluindo conceitualmente:

```text
PROJECT-BOOTSTRAP
PROJECT-CONFORMANCE-AUDIT
PROJECT-RESUMPTION-AUDIT
PROJECT-RELEASE-AUDIT
PROJECT-ARCHITECTURE-HANDOFF
```

A enumeração acima registra aplicabilidade conceitual prevista; não declara que
todos esses workflows já estejam implementados no repositório de governança.

## Regras de resolução

Antes de uma tarefa de engenharia no PST:

```text
1. determinar active branch/ref/working tree;
2. carregar OrganizationEngineering aplicável;
3. carregar governança aplicável de papinho-ecosystem;
4. carregar Technology Profiles aplicáveis quando existirem canonicamente;
5. carregar Shared Framework governance aplicável, se houver;
6. carregar ADRs e documentação local do PST;
7. carregar phase/plan/roadmap relevante;
8. auditar Code/Tests/Build/scripts antes de assumir estado factual;
9. detectar conflitos antes de implementar.
```

## Conflitos

Se duas governanças aplicáveis entrarem em conflito:

```text
GOVERNANCE CONFLICT
```

O conflito deve ser reportado e resolvido no nível apropriado antes de uma
alteração arquitetural que dependa dele.

Não escolher silenciosamente uma regra apenas por estar mais próxima do código
ou por estar na default branch.

## Aplicabilidade de tecnologia

`native-c` e `library` descrevem o projeto de forma geral.

Eles não congelam:

- C89/VC6 como única toolchain;
- Windows como única plataforma;
- RetroZilla NSS, Schannel ou OpenSSL como provider universal;
- uma única ABI ou target;
- uma única geração de sistema operacional.

Targets concretos continuam sendo identificados factual e duravelmente segundo
a governança aplicável e a documentação do PST.

## Não objetivos

Este Project Context não:

- substitui ADRs locais;
- substitui `AGENTS.md`;
- substitui README, API, SPI ou provider docs;
- congela branch ativa;
- congela paths absolutos locais;
- transforma provider em Product Family;
- transforma target em Project Relationship;
- declara suporte a plataformas além da evidência do projeto;
- altera código, API, SPI, build ou comportamento funcional.
