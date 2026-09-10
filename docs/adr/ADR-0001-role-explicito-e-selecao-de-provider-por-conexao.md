---
adr: ADR-0001
title: Role explícito de conexão e seleção de provider por conexão
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

# ADR-0001 — Role explícito de conexão e seleção de provider por conexão

## Contexto

PapinhoSecureTransport 0.4.0 e sua API 1.3 selecionam um provider durante a criação de `pst_runtime` e implementam somente o role TLS CLIENT. A evolução para CLIENT e SERVER precisa conhecer role e política completa antes da seleção, sem inferir semântica a partir do transporte ou de dados opcionais.

A auditoria arquitetural SS-0 concluiu que seleção no runtime ocorre cedo demais para filtrar corretamente role, identidade local, autenticação e trust do peer, nome esperado, ALPN e capabilities requeridas.

## Forças da decisão

- role explícito e verificável;
- seleção coerente com toda a política da conexão;
- controle do consumer sobre provider exato, ordem ou seleção automática;
- ausência de fallback após falha de segurança ou protocolo;
- preservação da fronteira entre secure transport e servidor TCP;
- suporte a múltiplas conexões e providers sem promessa implícita de thread safety;
- aderência ao vocabulário role-neutral transversal.

## Alternativas consideradas

### Alternativa A — Inferir o role

Inferir CLIENT ou SERVER pelo socket, hostname, certificado, ALPN, tráfego ou comportamento do provider.

**Vantagens**
- nenhuma configuração pública adicional.

**Desvantagens / trade-offs**
- comportamento ambíguo e dependente de heurística;
- políticas idênticas poderiam produzir roles diferentes;
- incompatível com seleção fail-closed e testes determinísticos.

### Alternativa B — Selecionar um provider por runtime antes de conhecer a conexão

Adicionar role ao runtime ou manter a seleção atual e validar a conexão posteriormente.

**Vantagens**
- menor mudança estrutural no modelo 0.4.0.

**Desvantagens / trade-offs**
- limita naturalmente um runtime a um role/provider;
- separa a seleção dos requisitos completos;
- pode escolher antecipadamente um provider inelegível.

### Alternativa C — Role explícito e seleção por conexão

Tratar o runtime como contexto/registry e selecionar o provider quando a conexão é criada com role e política completos.

**Vantagens**
- seleção factual e completa;
- um runtime pode servir logicamente várias conexões e roles;
- EXACT, ORDERED e AUTOMATIC permanecem sob controle do consumer;
- provider fica imutável durante a conexão.

**Desvantagens / trade-offs**
- exige redesign incompatível da API pública e da SPI;
- informações do provider selecionado passam a pertencer à conexão.

## Decisão

Cada conexão segura PST terá exatamente um role obrigatório e explícito: `CLIENT` ou `SERVER`. O role nunca será inferido do socket conectado ou aceito, hostname, identidade local ou do peer, ALPN, direção do tráfego, primeiro pacote ou comportamento do provider.

O provider será selecionado na criação da conexão, quando PST conhecer role, política TLS, Local Identity, Peer Authentication, Peer Trust, Expected Peer Name quando aplicável, ALPN e capabilities requeridas. `pst_runtime` poderá evoluir para contexto/registry de providers.

Os modos continuam conceitualmente `EXACT`, `ORDERED` e `AUTOMATIC`. EXACT falha se o provider indicado não for elegível. ORDERED respeita a ordem definida pelo consumer. AUTOMATIC considera somente providers elegíveis conforme política documentada.

Após a seleção, uma `pst_connection` usa exatamente um provider, fixo até sua destruição. Falha de handshake, autenticação do peer, trust, ALPN, política, I/O ou shutdown nunca provoca reseleção ou fallback.

No role SERVER, o consumer cria e administra listener, bind, listen, accept, admission, scheduling e backlog. Somente o transporte conectado resultante do accept é entregue ao PST.

## Justificativa

A conexão é o primeiro ponto em que todos os requisitos de elegibilidade estão disponíveis. Selecionar nesse ponto evita heurísticas, preserva controle explícito do consumer e impede que uma falha posterior seja reinterpretada como oportunidade de downgrade ou fallback.

## Consequências

### Positivas
- CLIENT e SERVER tornam-se explícitos e simétricos;
- seleção considera requisitos completos;
- Combined SDK não elimina pinning ou ordenação do consumer;
- falhas de segurança permanecem terminais para a conexão;
- PST não se transforma em framework de servidor TCP.

### Negativas / trade-offs
- API 1.3 e SPI 2.4 provavelmente exigirão versões major novas;
- runtime info e lifecycle precisarão refletir seleção por conexão;
- testes de seleção precisarão combinar role e capabilities.

### Neutras ou operacionais
- API 2.0.0, SPI 3.0 e library/package 0.5.0 são direção provável, não implementação concluída;
- compartilhamento lógico de runtime entre conexões não implica thread safety entre threads.

## Regras derivadas

1. Toda conexão possui exatamente um role explícito.
2. Nenhum dado indireto pode determinar role.
3. Provider selection considera role e todos os requisitos conhecidos da conexão.
4. EXACT nunca substitui silenciosamente o provider solicitado.
5. ORDERED preserva a ordem do consumer.
6. AUTOMATIC considera apenas providers elegíveis.
7. Provider selecionado não muda durante a conexão.
8. Falhas posteriores à seleção não causam fallback.
9. PST não possui listener nem política de accept.
10. `shareable across connections != thread-safe across threads`.

## Impacto

### Código
Futura API/SPI deverá mover a seleção efetiva para a criação da conexão e transportar role explicitamente.

### Build e toolchains
Nenhuma mudança nesta decisão documental. Implementações futuras devem preservar VC6/C89 e os targets modernos aplicáveis.

### Documentação
Arquitetura, API/ABI, SPI, providers, exemplos e matrizes futuras deverão refletir seleção por conexão.

### Compatibilidade
A direção é incompatível com os contratos API 1.3/SPI 2.4; versões e migração serão decididas no design correspondente.

### Testes
Testar EXACT, ORDERED e AUTOMATIC para ambos os roles, elegibilidade, ausência de fallback e múltiplas conexões por runtime.

## Verificação de conformidade

- role aparece como entrada obrigatória e estruturada;
- não existe código que derive role de hostname, transporte, identidade, ALPN ou tráfego;
- seleção ocorre com a política completa;
- provider ID da conexão permanece estável;
- falhas pós-seleção não consultam outro provider;
- APIs SERVER recebem somente transporte já conectado.

## Relações

### ADRs relacionadas
- OrganizationEngineering/ADR-0002 — Adoção e governança de Architecture Decision Records.
- PapinhoEngineering/ADR-0004 — Modelo compartilhado de configuração, fonte única de verdade e separação entre Core e Frontends.
- PapinhoEngineering/ADR-0007 — Fronteiras portáveis entre core, plataforma e backends substituíveis.
- PapinhoEngineering/ADR-0009 — Separação entre identidade local, autenticação, confiança e nome do peer.
- ADR-0002 — Taxonomia de capabilities de provider consciente de role.
- ADR-0003 — Semântica de seleção ALPN no role SERVER.

### Capability Documents relacionadas
Não se aplica atualmente.

### Documentos relacionados
- `docs/architecture.md`
- `docs/provider-evolution.md`
- `docs/provider-spi.md`
- `docs/public-api-abi.md`
- contexto da auditoria SS-0 CLIENT/SERVER, não registrado como documento normativo no repositório.

## Histórico de revisões

| Revisão | Data | Descrição |
|---:|---|---|
| 1 | 2026-09-07 | Registro inicial do role explícito e da seleção de provider por conexão. |
| 2 | 2026-09-09 | Migração de metadata para `scope-level: project` e `scope-target: PapinhoSecureTransport`, sem alteração da decisão técnica. |
| 3 | 2026-09-09 | Atualiza a referência de governança de ADRs para `OrganizationEngineering/ADR-0002`, sucessora da antiga `PapinhoEngineering/ADR-0001`, sem alteração técnica. |
