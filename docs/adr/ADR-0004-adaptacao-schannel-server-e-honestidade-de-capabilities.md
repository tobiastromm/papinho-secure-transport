---
adr: ADR-0004
title: Adaptação Schannel SERVER e honestidade de capabilities
status: accepted
decision-date: 2026-09-08
last-revised: 2026-09-09
revision: 2
scope-level: project
scope-target: PapinhoSecureTransport
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0004 — Adaptação Schannel SERVER e honestidade de capabilities

## Contexto

Na branch `feature/server-side`, o PST já possui três ADRs locais accepted:

- ADR-0001 — Role explícito de conexão e seleção de provider por conexão;
- ADR-0002 — Taxonomia de capabilities consciente de role;
- ADR-0003 — Semântica de seleção ALPN no role SERVER.

Durante a implementação SERVER do Schannel surgiram duas limitações concretas.

### Caso A — cadeia da Local Identity

A Local Identity provider-neutral contém leaf + intermediários + private key.

OpenSSL recebe a cadeia diretamente. No Schannel SERVER, sem os intermediários
disponíveis no Windows CA store, o handshake podia enviar apenas o leaf.

Foi aceita uma adaptação provider-local: disponibilizar temporariamente somente
os intermediários em `CurrentUser\CA`.

### Caso B — ALPN SERVER OPTIONAL

ADR-0003 define:

```text
REQUIRED  -> mismatch falha
OPTIONAL  -> mismatch pode continuar sem ALPN
DISABLED  -> não negocia ALPN
```

No Schannel, quando `SEC_APPLICATION_PROTOCOLS` é fornecido sem interseção, o
backend retorna `SEC_E_APPLICATION_PROTOCOL_MISMATCH`, inclusive no caso em que
PST deseja OPTIONAL.

Emular OPTIONAL exigiria parsear ClientHello TLS manualmente antes do Schannel.

Essa adaptação foi rejeitada.

## Forças da decisão

- preservação do contrato provider-neutral;
- capability honesty;
- segurança;
- Local Identity separada de Peer Trust;
- provider pinning e seleção pré-binding;
- ownership/refcount/cleanup explícitos;
- ausência de parser TLS paralelo.

## Alternativas consideradas

### Caso A — exigir instalação prévia pelo consumer

**Vantagens**
- Schannel não altera store.

**Desvantagens / trade-offs**
- peculiaridade Schannel vaza pela abstração;
- consumer precisa conhecer provider;
- substituibilidade piora.

### Caso A — bridging provider-local em CurrentUser\CA

**Vantagens**
- preserva contrato de Local Identity;
- usa mecanismo oficial do Windows;
- não interpreta TLS hostil;
- ownership/refcount/cleanup são testáveis.

**Desvantagens / trade-offs**
- mantém estado externo temporário;
- crash abrupto pode deixar intermediário público residual.

### Caso B — parser ClientHello

**Vantagens**
- poderia tentar reproduzir OPTIONAL.

**Desvantagens / trade-offs**
- interpreta input TLS hostil antes do security engine;
- exige framing, lengths, fragmentation e malformed-input handling;
- duplica responsabilidade profunda do backend;
- aumenta superfície de ataque.

### Caso B — capability ausente

**Vantagens**
- contrato permanece verdadeiro;
- seleção pode escolher outro provider;
- PST não cria parser TLS paralelo.

**Desvantagens / trade-offs**
- Schannel SERVER permanece capability-limited;
- EXACT Schannel pode retornar `UNSUPPORTED`.

## Decisão

### 1. Chain delivery Schannel — adaptação aceita

O provider Schannel SERVER pode disponibilizar temporariamente os
**intermediários** da Local Identity em:

```text
CurrentUser\CA
```

quando isso for necessário para entrega da cadeia pelo backend.

### 2. Root não é promovida

Esse mecanismo nunca deve inserir root em Trusted Root nem alterar Peer Trust.

```text
LOCAL IDENTITY
!=
PEER TRUST
```

### 3. Preexisting preserve

Certificado intermediário preexistente:

```text
→ nunca é removido pelo PST
```

### 4. Ownership/refcount

Certificado inserido pelo PST:

```text
→ possui ownership explícito
→ pode ser compartilhado com refcount
→ é removido no último release
→ é limpo em failure paths
```

### 5. Crash residue

Crash abrupto pode impedir cleanup.

Isso permanece limitação factual documentada. O PST não fará recovery agressivo
capaz de remover material que não possa provar ser seu.

O material residual desse mecanismo é certificado público intermediário, não
private key.

### 6. Evidência atual

A baseline validada inclui:

```text
SCHANNEL_SERVER_CHAIN_DELIVERY=PASS
SCHANNEL_INTERMEDIATE_STORE_SCOPE=CurrentUser\CA
SCHANNEL_INTERMEDIATE_PREEXISTING_PRESERVED=PASS
SCHANNEL_INTERMEDIATE_SHARED_REFCOUNT=PASS
SCHANNEL_INTERMEDIATE_LAST_REF_CLEANUP=PASS
SCHANNEL_INTERMEDIATE_FAILURE_CLEANUP=PASS
SCHANNEL_INTERMEDIATE_NORMAL_RESIDUE_COUNT=0
SCHANNEL_INTERMEDIATE_CRASH_RECOVERY=DOCUMENTED_LIMITATION
```

### 7. Parser ClientHello para OPTIONAL — rejeitado

PST não implementará, como workaround ordinário do Schannel:

- parser de TLS records;
- parser de ClientHello;
- reconstrução de fragmentação de handshake;
- validação manual de ALPN structures;
- pre-processing de input TLS hostil para decidir se deve fornecer
  `SECBUFFER_APPLICATION_PROTOCOLS`.

### 8. Capability honesty

A capability `ALPN_SERVER` definida pela ADR-0002 deve representar a semântica
SERVER completa definida pela ADR-0003.

Como o Schannel não satisfaz REQUIRED/OPTIONAL/DISABLED de forma completa:

```text
Schannel mechanism partial support
!=
PST ALPN_SERVER complete support
```

Logo, Schannel SERVER não anuncia `ALPN_SERVER`.

### 9. Provider selection usa a assimetria

Exemplo:

```text
SERVER + ALPN
ORDERED [Schannel, OpenSSL]

Schannel ALPN_SERVER absent
→ ineligible before binding

OpenSSL ALPN_SERVER present
→ selected
```

Sem requisito ALPN SERVER, Schannel pode continuar elegível conforme suas demais
capabilities.

### 10. Falha pós-binding não troca provider

Nada nesta decisão altera ADR-0001:

```text
provider selected
→ pinned to connection
→ no fallback after handshake/auth/ALPN/policy/I/O failure
```

## Justificativa

A adaptação da cadeia usa mecanismo oficial do OS, permanece provider-local, não
interpreta protocolo hostil e possui ownership/cleanup verificáveis.

O workaround ALPN exigiria que PST começasse a interpretar ClientHello TLS apenas
para equalizar Schannel. Isso cruza a fronteira de adaptação e expande a trust
boundary.

Portanto a capability ausente é a representação correta.

## Consequências

### Positivas

- Local Identity continua provider-neutral;
- peculiaridade Schannel fica encapsulada;
- Peer Trust não é alterado;
- ALPN capability permanece factual;
- provider selection pode escolher outro backend;
- PST não vira parser TLS paralelo.

### Negativas / trade-offs

- Schannel SERVER não possui ALPN_SERVER completo;
- EXACT Schannel pode retornar `UNSUPPORTED`;
- store bridging exige refcount/cleanup;
- crash pode deixar certificado público residual.

### Neutras ou operacionais

- outros providers podem usar mecanismos de chain delivery diferentes;
- outros providers podem possuir ALPN_SERVER completo;
- granularidade futura de capability pode evoluir se houver requisito real.

## Regras derivadas

1. CurrentUser\CA é usado apenas para intermediários necessários à Local Identity.
2. Root não é tornada trusted por esse mecanismo.
3. Local Identity e Peer Trust permanecem separados.
4. Preexisting certificates não são removidos.
5. PST-inserted certificates têm ownership/refcount/cleanup.
6. Failure paths limpam state criado pelo PST.
7. Crash residue é limitação documentada; não autoriza cleanup destrutivo.
8. Schannel SERVER não anuncia ALPN_SERVER enquanto não cumprir ADR-0003 completa.
9. Não implementar parser ClientHello ordinário para fabricar OPTIONAL.
10. Capability ausente pode tornar Schannel inelegível antes do binding.
11. Falha pós-binding não provoca reseleção.
12. Parsing TLS deliberado futuro exigirá decisão arquitetural/security review própria.

## Impacto

### Código
Schannel mantém store bridging provider-local e ALPN SERVER ausente.

### Build e toolchains
Nenhuma nova regra de target.

### Documentação
Provider docs e capability matrix devem registrar a adaptação e a ausência
factual de ALPN_SERVER.

### Compatibilidade
Outros providers não precisam usar Windows stores.

### Testes
Preservar gates de chain delivery, preserve/refcount/cleanup e provider
eligibility por ALPN_SERVER.

## Verificação de conformidade

- nenhum root é promovido por esse path;
- preexisting certificates sobrevivem;
- normal lifecycle deixa zero residue;
- private key não é gravada em CurrentUser\CA por essa adaptação;
- Schannel SERVER não anuncia ALPN_SERVER;
- não existe parser ClientHello para workaround OPTIONAL;
- ORDERED/AUTOMATIC podem selecionar provider alternativo por capability.

## Relações

### ADRs relacionadas

- ADR-0001 — Role explícito de conexão e seleção de provider por conexão.
- ADR-0002 — Taxonomia de capabilities de provider consciente de role.
- ADR-0003 — Semântica de seleção ALPN no role SERVER.
- PapinhoEngineering/ADR-0007 — Fronteiras portáveis entre core, plataforma e backends substituíveis.
- PapinhoEngineering/ADR-0009 — Separação entre identidade local, autenticação, confiança e nome do peer.
- PapinhoEngineering/ADR-0010 — Fronteira de adaptação de providers e honestidade de capabilities, se aprovada.

### Capability Documents relacionadas

Não se aplica atualmente.

### Documentos relacionados

- `docs/schannel-backend.md`
- `docs/provider-evolution.md`
- documentação SERVER/SS-0 da branch `feature/server-side`

## Histórico de revisões

| Revisão | Data | Descrição |
|---:|---|---|
| 1 | 2026-09-08 | Registra bridging de intermediários Schannel SERVER e rejeição de parser ClientHello para fabricar ALPN_SERVER OPTIONAL. |
| 2 | 2026-09-09 | Migração de metadata para `scope-level: project` e `scope-target: PapinhoSecureTransport`, sem alteração da decisão técnica. |
