---
adr: ADR-0003
title: Semântica de seleção ALPN no role SERVER
status: accepted
decision-date: 2026-09-07
last-revised: 2026-09-09
revision: 3
scope-level: project
scope-target: PapinhoSecureTransport
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0003 — Semântica de seleção ALPN no role SERVER

## Contexto

Na API 1.3, a lista ALPN é uma oferta ordenada do CLIENT. SERVER precisa receber a oferta do peer e selecionar um protocolo permitido, com resultado consistente entre OpenSSL, Schannel e RetroZilla NSS.

Sem regra comum, providers poderiam escolher ordens distintas ou exigir callback público prematuramente. SS-0 determinou que a primeira API SERVER deve fixar uma regra simples e determinística.

## Forças da decisão

- comportamento idêntico entre providers;
- escolha determinística e testável;
- configuração copiada e sem callback/reentrância;
- fail-closed quando ALPN é obrigatório;
- ausência de protocolo de aplicação hardcoded;
- evolução futura deliberada.

## Alternativas consideradas

### Alternativa A — Preferência do CLIENT

Selecionar o primeiro protocolo da oferta do CLIENT aceito pelo SERVER.

**Vantagens**
- preserva preferência remota.

**Desvantagens / trade-offs**
- retira do servidor o controle de preferência;
- pode divergir do comportamento/configuração natural de providers.

### Alternativa B — Callback público de seleção

Entregar a oferta a callback da aplicação durante o handshake.

**Vantagens**
- máxima flexibilidade dinâmica.

**Desvantagens / trade-offs**
- acrescenta lifetime, reentrância, threading e normalização de erros;
- aumenta diferenças entre providers antes de existir requisito real.

### Alternativa C — Lista em ordem de preferência do SERVER

Selecionar o primeiro item da lista local também presente na oferta do CLIENT.

**Vantagens**
- determinística;
- simples de copiar, validar e testar;
- mantém policy no consumer sem callback.

**Desvantagens / trade-offs**
- não oferece algoritmos dinâmicos na primeira API.

## Decisão

No role CLIENT, a lista de protocolos é uma oferta ordenada.

No role SERVER, a lista é a ordem de preferência do servidor. A seleção é o primeiro protocolo da preferência SERVER que também aparece na oferta CLIENT.

Os modos são:

- `REQUIRED` sem interseção: handshake falha;
- `OPTIONAL` sem interseção: handshake pode continuar sem ALPN negociado;
- `DISABLED`: ALPN não é negociado.

A primeira API SERVER não exporá callback público nem um campo genérico `selection_policy` sem algoritmos reais adicionais. Preferência do CLIENT, callback de aplicação ou outro algoritmo poderão ser considerados por evolução deliberada futura.

Falha de ALPN ocorre depois da seleção do provider e nunca causa fallback.

## Justificativa

Uma lista local ordenada expressa policy suficiente para a primeira implementação, evita callbacks durante o handshake e produz comportamento portável verificável nos três providers.

## Consequências

### Positivas
- seleção reproduzível entre providers;
- configuração permanece simples, copiável e frozen;
- REQUIRED falha de forma inequívoca;
- nenhum protocolo de aplicação entra no core.

### Negativas / trade-offs
- aplicações que precisem de escolha dinâmica aguardarão evolução futura;
- adapters precisam normalizar APIs nativas que usem ordenação diferente.

### Neutras ou operacionais
- extensão futura não é automaticamente tarefa trivial ou `good-first-issue`;
- consistência cross-provider será requisito para qualquer algoritmo novo.

## Regras derivadas

1. Lista CLIENT representa oferta ordenada.
2. Lista SERVER representa preferência local ordenada.
3. SERVER escolhe a primeira preferência local presente na oferta.
4. REQUIRED sem match falha.
5. OPTIONAL sem match pode continuar sem ALPN.
6. DISABLED não negocia ALPN.
7. Não existe callback público inicial.
8. Não existe `selection_policy` especulativa inicial.
9. Falha ALPN nunca reseleciona provider.
10. O core não contém protocolo de aplicação hardcoded.

## Impacto

### Código
Futuros adapters SERVER deverão mapear a mesma regra para OpenSSL, Schannel e RetroZilla NSS.

### Build e toolchains
Nenhuma mudança nesta etapa; a implementação deverá respeitar capabilities reais por target.

### Documentação
API, provider docs e exemplos deverão diferenciar oferta CLIENT e preferência SERVER.

### Compatibilidade
A semântica SERVER fará parte do novo contrato API/SPI; API 1.3 permanece fato histórico CLIENT-only.

### Testes
Testar preferência SERVER, múltiplas interseções, REQUIRED/OPTIONAL/DISABLED, listas vazias, ausência e mismatch, sem fallback.

## Verificação de conformidade

- fixtures oferecem listas em ordens conflitantes e comprovam preferência SERVER;
- os três providers produzem o mesmo protocolo negociado;
- REQUIRED sem interseção termina em policy failure;
- OPTIONAL sem interseção não fabrica protocolo;
- nenhum callback ou policy especulativa aparece na primeira API.

## Relações

### ADRs relacionadas
- OrganizationEngineering/ADR-0002 — Adoção e governança de Architecture Decision Records.
- PapinhoEngineering/ADR-0004 — Modelo compartilhado de configuração, fonte única de verdade e separação entre Core e Frontends.
- PapinhoEngineering/ADR-0007 — Fronteiras portáveis entre core, plataforma e backends substituíveis.
- PapinhoEngineering/ADR-0009 — Separação entre identidade local, autenticação, confiança e nome do peer.
- ADR-0001 — Role explícito de conexão e seleção de provider por conexão.
- ADR-0002 — Taxonomia de capabilities de provider consciente de role.

### Capability Documents relacionadas
Não se aplica atualmente.

### Documentos relacionados
- `docs/architecture.md`
- `docs/tls-runtime.md`
- `docs/provider-spi.md`
- `docs/providers.md`
- contexto da auditoria SS-0 CLIENT/SERVER, não registrado como documento normativo no repositório.

## Histórico de revisões

| Revisão | Data | Descrição |
|---:|---|---|
| 1 | 2026-09-07 | Registro inicial da semântica ALPN para o role SERVER. |
| 2 | 2026-09-09 | Migração de metadata para `scope-level: project` e `scope-target: PapinhoSecureTransport`, sem alteração da decisão técnica. |
| 3 | 2026-09-09 | Atualiza a referência de governança de ADRs para `OrganizationEngineering/ADR-0002`, sem alteração técnica. |
