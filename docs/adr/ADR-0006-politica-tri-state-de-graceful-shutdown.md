---
adr: ADR-0006
title: Politica tri-state de graceful shutdown como requisito de capability
status: accepted
decision-date: 2026-09-11
last-revised: 2026-09-11
revision: 1
scope-level: project
scope-target: PapinhoSecureTransport
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0006 - Politica tri-state de graceful shutdown como requisito de capability

## Contexto

`PST_TLS_POLICY.require_graceful_shutdown` existe no contrato publico desde a
API 1.3. Naquele contrato, o campo usava o dominio binario
`PST_REQUIREMENT_DISABLED`/`PST_REQUIREMENT_REQUIRED`. A migracao para API 2.0
removeu esses nomes, preservou a validacao numerica `0..1` e passou a publicar
um exemplo SERVER com `PST_FEATURE_REQUIRED`, cujo valor e `2`.

Assim, a release 0.6.0 rejeita como `PST_RESULT_INVALID_ARGUMENT` uma
configuracao expressa somente com constantes publicas e recomendada pelo
proprio exemplo. O campo tambem nao participa atualmente do calculo de
capabilities requeridas, embora todos os providers publicados possuam shutdown
incremental e classificacao estrita de fechamento.

## Forcas da decisao

- compatibilidade com consumers API 2.0/2.1 zero-inicializados;
- validacao fechada dos valores publicos `PST_FEATURE_*`;
- selecao de provider anterior ao binding;
- ausencia de fallback posterior ao binding;
- deadlines e lifecycle controlados pelo consumer;
- preservacao de fechamento clean versus truncated;
- API/SPI e providers factuais;
- preservacao do layout ABI de `PST_TLS_POLICY`.

## Alternativas consideradas

### Manter o campo binario

Rejeitada. Preservaria a validacao historica, mas manteria invalido o exemplo
publicado e nao expressaria OPTIONAL como os demais requisitos de feature.

### Aceitar `2` apenas como alias de `1`

Rejeitada. Faria o constructor passar sem definir ou aplicar a diferenca entre
OPTIONAL e REQUIRED.

### Transformar a policy em operacao automatica de lifecycle

Rejeitada. PST nao possui deadline interno para shutdown, release nao e uma
operacao TLS implicita e o consumer continua responsavel por dirigir o
lifecycle incremental.

### Adotar requisito tri-state de capability

Aceita. O campo usa o dominio fechado `PST_FEATURE_*`. REQUIRED participa da
elegibilidade pre-binding; OPTIONAL e DISABLED nao tornam um provider
inelegivel.

## Decisao

`PST_TLS_POLICY.require_graceful_shutdown` e uma declaracao tri-state de
requisito de capability do provider. Nao e um modo automatico de lifecycle ou
shutdown.

| Valor | Validacao | Elegibilidade do provider | Significado de lifecycle |
|---|---|---|---|
| `PST_FEATURE_DISABLED` | aceita | capability nao requerida | shutdown publico explicito permanece disponivel |
| `PST_FEATURE_OPTIONAL` | aceita | capability nao obrigatoria | shutdown publico explicito permanece disponivel |
| `PST_FEATURE_REQUIRED` | aceita | capability obrigatoria | semantica incremental e reciproca normal |
| valor desconhecido | rejeita | N/A | N/A |

### DISABLED

Graceful-shutdown capability nao e requisito de elegibilidade. DISABLED nao
desabilita a API publica de shutdown, nao transforma EOF/reset abrupto em
fechamento clean e nao altera truncation.

### OPTIONAL

A capability e desejavel, mas sua ausencia nao elimina o provider. OPTIONAL
nao cria deadline nem shutdown automatico. Providers aplicaveis que ja suportam
graceful shutdown podem ter o mesmo comportamento operacional observado em
DISABLED.

### REQUIRED

A capability e requisito obrigatorio e providers sem ela sao rejeitados antes
do binding. Depois do binding, aplicam-se as mesmas operacoes incrementais de
shutdown reciproco. Falha posterior nao seleciona outro provider.

### Regras comuns

O consumer decide quando chamar `pst_connection_shutdown()` e mantem seus
proprios deadlines. `pst_connection_release()` realiza cleanup de recursos e
nao executa implicitamente TLS shutdown. Falha terminal e truncation continuam
releasable sem leak. Nenhum valor permite plaintext fallback, provider fallback
pos-binding ou reclassificacao de EOF/reset sem `close_notify` reciproco como
clean.

## Justificativa

O modelo tri-state harmoniza o campo com as constantes publicas usadas no
exemplo e separa requisito de selecao de provider da politica operacional da
aplicacao. A separacao preserva a natureza incremental da API e permite que um
consumer exija capability sem transferir ao PST a responsabilidade por seu
deadline.

## Consequencias

### Positivas

- `PST_FEATURE_REQUIRED` torna-se um valor valido e aplicavel;
- OPTIONAL e DISABLED ficam semanticamente distintos na declaracao, sem
  inventar comportamento automatico;
- provider incapaz e filtrado antes do binding;
- layout e chamadas publicas existentes permanecem estaveis.

### Negativas / trade-offs

- REQUIRED necessita de uma capability factual representavel no mask;
- providers precisam anunciar essa capability somente apos evidencia por role;
- packages anteriores continuam contendo a divergencia historica.

### Neutras ou operacionais

- os tres providers atuais podem continuar executando o mesmo shutdown
  incremental quando a API e chamada;
- OPTIONAL nao promete tentativa por um intervalo de tempo;
- esta decisao nao altera bytes ou assets publicados da release 0.6.0.

## Regras derivadas

1. O dominio aceito e exatamente DISABLED, OPTIONAL e REQUIRED.
2. Valores desconhecidos sao rejeitados.
3. REQUIRED adiciona graceful-shutdown capability aos requisitos calculados.
4. OPTIONAL e DISABLED nao adicionam essa capability aos requisitos obrigatorios.
5. Capability e avaliada por role antes do binding.
6. Falha pos-binding nunca provoca reselecao.
7. Nenhum modo cria deadline interno ou shutdown automatico.
8. Release permanece cleanup local bounded, nao TLS shutdown implicito.
9. Truncation e fechamento clean mantem a classificacao existente.
10. O layout ABI de `PST_TLS_POLICY` permanece inalterado.

## Impacto

### Codigo

A validacao deve reutilizar o validador fechado de `PST_FEATURE_*`. O calculo
de requisitos deve representar graceful shutdown quando REQUIRED. Magic
literals com significado de feature devem usar as constantes publicas.

### API e SPI

O campo e seus valores ja sao publicos. Se o inventario de capabilities nao
encontrar bit existente, sera necessaria evolucao aditiva do mask publico e de
seu alias privado, sem alterar o layout da SPI ou adicionar hook de vtable.

### Providers

Cada provider anuncia a capability apenas nos roles em que a semantica completa
foi provada. A operacao continua no `shutdown_step` existente.

### Compatibilidade

Consumers zero-inicializados continuam em DISABLED. O valor historico `1`
passa a significar formalmente OPTIONAL; consumers que pretendem requisito
obrigatorio devem usar `PST_FEATURE_REQUIRED`.

### Testes

Testar os quatro casos de validacao, elegibilidade pre-binding, ausencia de
fallback, CLIENT/SERVER, shutdown reciproco, clean/truncated e os tres providers.

## Verificacao de conformidade

- DISABLED, OPTIONAL e REQUIRED sao aceitos;
- valor desconhecido e rejeitado;
- REQUIRED elimina provider sem capability antes do binding;
- OPTIONAL e DISABLED nao eliminam esse provider;
- release nao chama shutdown implicitamente;
- EOF/reset abrupto permanece TRUNCATED;
- clean exige `close_notify` reciproco;
- nao existe fallback pos-binding nem plaintext fallback;
- API/SPI layouts permanecem byte-identicos.

## Relacoes

### ADRs relacionadas

- ADR-0001 - Role explicito e selecao de provider por conexao.
- ADR-0002 - Taxonomia de capabilities consciente de role.
- PapinhoEngineering/ADR-0007 - Fronteiras portaveis entre core, plataforma e backends substituiveis.
- PapinhoEngineering/ADR-0010 - Fronteira de adaptacao de providers e honestidade de capabilities.

### Documentos relacionados

- `docs/api-2.0.md`
- `docs/public-api-abi.md`
- `docs/provider-spi.md`
- `docs/lifecycle-ownership.md`
- `examples/basic_server.c`

## Historico de revisoes

| Revisao | Data | Descricao |
|---:|---|---|
| 1 | 2026-09-11 | Adota semantica tri-state de requisito de graceful-shutdown capability e preserva lifecycle explicito controlado pelo consumer. |
