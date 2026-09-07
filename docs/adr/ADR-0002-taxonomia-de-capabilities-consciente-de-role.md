---
adr: ADR-0002
title: Taxonomia de capabilities de provider consciente de role
status: accepted
decision-date: 2026-09-07
last-revised: 2026-09-07
revision: 1
scope: project
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0002 — Taxonomia de capabilities de provider consciente de role

## Contexto

As capabilities API 1.3/SPI 2.4 descrevem TLS, trust, ALPN e `CLIENT_AUTH`, mas não declaram se um provider implementa CLIENT, SERVER ou ambos. Recursos existentes no backend nativo não equivalem a integração comprovada sob o contrato PST.

SS-0 identificou que `CLIENT_AUTH` colapsa apresentação de Local Identity e autenticação do peer, contrariando a separação semântica de PapinhoEngineering/ADR-0009.

## Forças da decisão

- claims factuais e testáveis;
- seleção segura por role e política;
- terminologia role-neutral;
- providers assimétricos e dependentes de OS/snapshot;
- representação simples enquanto suficiente;
- independência entre semântica e largura de armazenamento.

## Alternativas consideradas

### Alternativa A — Preservar capabilities atuais e inferir SERVER

Tratar TLS, certificado e ALPN como evidência implícita de SERVER.

**Vantagens**
- nenhuma capability nova.

**Desvantagens / trade-offs**
- produz claims falsos;
- ALPN client-side não prova seleção server-side;
- existência de API nativa não prova implementação PST.

### Alternativa B — Uma capability única por role

Declarar apenas CLIENT e SERVER, supondo que recursos auxiliares tenham a mesma semântica em ambos.

**Vantagens**
- máscara pequena.

**Desvantagens / trade-offs**
- não representa providers parciais;
- não permite filtrar trust, identidade, ALPN e autenticação separadamente.

### Alternativa C — Role e mecanismos como capabilities factuais independentes

Representar role e funcionalidades relevantes separadamente, sem implicações implícitas.

**Vantagens**
- seleção precisa;
- evolução incremental e honesta;
- suporta assimetrias entre roles e plataformas.

**Desvantagens / trade-offs**
- mais bits e combinações de teste;
- nomes públicos exatos precisam ser congelados no design API/SPI.

## Decisão

Suporte a role é capability factual. TLS 1.3, ALPN ou suporte a certificados não implica SERVER.

A taxonomia deverá distinguir conceitualmente `ROLE_CLIENT`, `ROLE_SERVER`, `LOCAL_IDENTITY`, `PEER_CERT_AUTH`, `PEER_CERT_OPTIONAL`, `ALPN_CLIENT`, `ALPN_SERVER`, `CUSTOM_TRUST`, `SYSTEM_TRUST`, `PEER_NAME_VERIFY`, `PEER_INFO`, `NONBLOCKING`, `BACKEND_WAIT`, `TLS_1_2`, `TLS_1_3`, `RESUMPTION` e `EARLY_DATA`.

Os nomes públicos exatos serão refinados na API 2.0/SPI 3.0. `CLIENT_AUTH` não sobreviverá onde misturar Local Identity, Peer Authentication e Peer Trust.

Uma bitmask compacta é aceitável enquanto suficiente, mas 32 bits são armazenamento, não arquitetura semântica. Não existem implicações entre capabilities salvo definição explícita.

## Justificativa

Capabilities são claims de comportamento PST comprovado, não inventário de símbolos do backend. Separar role e mecanismos permite seleção fail-closed e preserva diferenças reais entre OpenSSL, Schannel e RetroZilla NSS.

## Consequências

### Positivas
- seleção pode rejeitar providers incapazes antes da conexão;
- CLIENT e SERVER são anunciados honestamente;
- ALPN e autenticação podem variar por role;
- capabilities dependentes de OS/snapshot permanecem explícitas.

### Negativas / trade-offs
- ABI de capability atual provavelmente será substituída;
- cada claim precisará de teste específico;
- documentos e manifests precisarão declarar conjuntos mais detalhados.

### Neutras ou operacionais
- 32 bits podem continuar sendo usados inicialmente;
- aumentar a largura futuramente não muda o significado das capabilities existentes.

## Regras derivadas

1. `ROLE_CLIENT` não implica `ROLE_SERVER`.
2. `ALPN_CLIENT` não implica `ALPN_SERVER`.
3. `LOCAL_IDENTITY` não implica `PEER_CERT_AUTH`.
4. `TLS_1_3` não implica `ROLE_SERVER`.
5. Capability só é anunciada após implementação e validação sob PST.
6. Provider selection usa capabilities factuais e requisitos da conexão.
7. Nenhuma implication é presumida sem contrato explícito.
8. Terminologia deve seguir PapinhoEngineering/ADR-0009.

## Impacto

### Código
Futuras API/SPI e descriptors deverão substituir a taxonomia ambígua e calcular requisitos por conexão.

### Build e toolchains
Nenhuma mudança nesta etapa. Claims podem variar por target e runtime efetivo.

### Documentação
Provider docs, target matrix e capabilities vivas deverão separar implementado, viável e não suportado.

### Compatibilidade
A mudança de significado recomenda API 2.0/SPI 3.0, sem aliases semanticamente enganosos.

### Testes
Cada role/capability anunciada precisa de gates positivos e negativos, incluindo seleção e runtime-effective query.

## Verificação de conformidade

- descriptors distinguem CLIENT e SERVER;
- nenhuma seleção usa capability implícita;
- ALPN client/server e identidade/autenticação aparecem separadamente;
- masks estática e runtime-effective são honestas;
- documentação não converte viabilidade nativa em claim implementado.

## Relações

### ADRs relacionadas
- PapinhoEngineering/ADR-0001 — Adoção e governança de Architecture Decision Records.
- PapinhoEngineering/ADR-0005 — Governança da documentação viva de capabilities.
- PapinhoEngineering/ADR-0007 — Fronteiras portáveis entre core, plataforma e backends substituíveis.
- PapinhoEngineering/ADR-0009 — Separação entre identidade local, autenticação, confiança e nome do peer.
- ADR-0001 — Role explícito de conexão e seleção de provider por conexão.
- ADR-0003 — Semântica de seleção ALPN no role SERVER.

### Capability Documents relacionadas
Não se aplica atualmente.

### Documentos relacionados
- `docs/providers.md`
- `docs/provider-spi.md`
- `docs/provider-evolution.md`
- `docs/target-matrix.md`
- contexto da auditoria SS-0 CLIENT/SERVER, não registrado como documento normativo no repositório.

## Histórico de revisões

| Revisão | Data | Descrição |
|---:|---|---|
| 1 | 2026-09-07 | Registro inicial da taxonomia de capabilities consciente de role. |
