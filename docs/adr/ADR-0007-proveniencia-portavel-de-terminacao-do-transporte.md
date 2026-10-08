---
adr: ADR-0007
title: Proveniencia portavel e evidencia nativa da terminacao do transporte
status: accepted
decision-date: 2026-10-08
last-revised: 2026-10-08
revision: 3
scope-level: project
scope-target: PapinhoSecureTransport
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0007 - Proveniencia portavel e evidencia nativa da terminacao do transporte

## Contexto e motivacao

`PST_RESULT_TRUNCATED` responde se o encerramento TLS nao foi autenticado por
`close_notify`. FIN e RST sem alerta possuem a mesma conclusao TLS, mas sao
causas de transporte diferentes e uteis para suporte. Codigos nativos podem
acrescentar evidencia factual, desde que nunca se tornem controle TLS portavel.

O diagnostico publico 2.1 normaliza o resultado e deliberadamente nao expoe
codigos nativos. `PST_DIAGNOSTIC_INFO` possui 64 bytes congelados e seu minimo
depende de `sizeof`; estende-lo faria uma implementacao nova rejeitar storage
de consumers 2.1 antigos.

## Alternativas

### Alterar `PST_RESULT_TRUNCATED`

Rejeitada. Misturaria seguranca TLS com causa TCP e quebraria consumers.

### Acrescentar campos a `PST_DIAGNOSTIC_INFO`

Rejeitada. Recriaria a incompatibilidade de `MIN_SIZE == sizeof` e congelaria
metadata especializada no snapshot geral.

### Expor texto ou codigo nativo sem dominio

Rejeitada. Misturaria Winsock, NSPR, OpenSSL e Schannel, alem de introduzir
texto, locale e risco de disclosure.

### Estrutura separada em camadas

Proposta. Mantem resultado TLS, classificacao portavel e evidencia nativa
opcional como fatos distintos.

## Contrato candidato em tres camadas

```text
A. TLS/security result
   PST_RESULT_TRUNCATED permanece autoritativo.

B. Portable transport classification
   UNKNOWN / TCP_FIN / TCP_RST / SOCKET_ERROR.

C. Optional native observation
   dominio explicito + padrao numerico de 32 bits + disponibilidade.
```

Camadas B e C sao diagnosticas. Elas nunca alteram verificacao TLS, trust,
shutdown, retry, fallback, HTTP framing, ownership ou provider selection.

## API publica candidata

```c
#define PST_TRANSPORT_TERMINATION_UNKNOWN      0UL
#define PST_TRANSPORT_TERMINATION_TCP_FIN      1UL
#define PST_TRANSPORT_TERMINATION_TCP_RST      2UL
#define PST_TRANSPORT_TERMINATION_SOCKET_ERROR 3UL

#define PST_NATIVE_ERROR_DOMAIN_NONE     0UL
#define PST_NATIVE_ERROR_DOMAIN_WINSOCK  1UL
#define PST_NATIVE_ERROR_DOMAIN_NSPR     2UL
#define PST_NATIVE_ERROR_DOMAIN_OPENSSL  3UL
#define PST_NATIVE_ERROR_DOMAIN_SCHANNEL 4UL

typedef struct PST_TRANSPORT_TERMINATION_INFO {
    pst_u32 struct_size;
    pst_u32 api_version;
    pst_u32 valid;
    pst_u32 termination;
    pst_u32 operation;
    pst_u32 native_domain;
    pst_u32 native_code;
    pst_u32 native_code_valid;
} PST_TRANSPORT_TERMINATION_INFO;

#define PST_TRANSPORT_TERMINATION_INFO_MIN_SIZE 32UL

PST_API PST_RESULT PST_CALL
pst_transport_termination_info_init(
    PST_TRANSPORT_TERMINATION_INFO *out_info);

PST_API PST_RESULT PST_CALL
pst_connection_copy_transport_termination(
    const pst_connection *connection,
    PST_TRANSPORT_TERMINATION_INFO *out_info);
```

O minimo e o valor numerico congelado `32UL`, nunca `sizeof`, permitindo cauda
futura. A implementacao escreve somente o prefixo conhecido, aceita registros
maiores do mesmo major e preserva a cauda desconhecida.

## Decisao sobre `generation`

`generation` foi removido da proposta. O contrato preserva somente o primeiro
evento terminal e nunca o reseta durante a vida da conexao. Portanto nao existe
uma sucessao publica de eventos cuja observacao precise de contador. Manter o
campo seria especulativo. Uma futura API de historico/eventos deve definir seu
proprio identificador.

## Dominio e representacao nativa

`native_code` e um padrao opaco de 32 bits, interpretado somente no
`native_domain` correspondente. Valores negativos NSPR sao preservados por
conversao bit a bit para `pst_u32`; um consumer especializado pode reconverter
para `pst_i32` depois de verificar o dominio. Winsock usa seus valores positivos;
OpenSSL usa o reason numerico; Schannel usa o padrao de 32 bits de
`SECURITY_STATUS` quando esse for o sinal factual.

Nao existe dominio `OTHER`: ele nao fornece namespace interpretavel. Um sinal
sem dominio estavel usa `native_code_valid=0` e `native_domain=NONE`.

Um unico par dominio/codigo registra a evidencia decisiva usada para a
classificacao. Ele nao tenta ser uma cadeia causal. No OpenSSL, FIN pode usar o
reason OpenSSL `UNEXPECTED_EOF`, enquanto RST pode usar Winsock
`WSAECONNRESET`. Os dois namespaces nunca sao colocados no mesmo campo nem um
codigo posterior substitui o decisivo. Se mais de uma evidencia for necessaria
no futuro, sera preciso um novo formato bounded; esta proposta nao descarta
silenciosamente uma segunda evidencia necessaria para decidir o evento atual.

Dominio e codigo descrevem uma observacao, nao autoria. `TCP_RST` nao afirma
que o servidor remoto originou o reset.

## Consistencia e lifetime

- `valid=0`: nenhum evento terminal de transporte foi observado;
- `valid=1` e `termination=UNKNOWN`: houve evento, mas o tipo nao foi provado;
- `native_code_valid=0`: nao existe codigo confiavel para o mesmo evento;
- `native_code_valid=1`: dominio e codigo pertencem ao mesmo evento;
- o primeiro evento terminal vence e o snapshot inteiro fica imutavel;
- provider prepara um registro completo localmente e o publica de uma vez sob
  o contrato de chamada existente; nao ha nova promessa de thread safety;
- chamadas concorrentes sobre a mesma conexao continuam fora do contrato;
- antes do evento, a consulta retorna `OK` com `valid=0`;
- `close_notify` isolado nao cria FIN nem evento de transporte;
- durante shutdown reciproco, um FIN/RST realmente observado pode criar o
  primeiro evento, sem mudar CLEAN/TRUNCATED;
- depois de shutdown ou estado terminal, o snapshot permanece copiavel enquanto
  a conexao existir;
- timeout, wake, cancel local e release local nao criam provenance;
- depois de release, o ponteiro nao e valido e nenhuma consulta e permitida.

## Matriz de captura por provider

| Provider/evento | Site e operacao | Classificacao | Evidencia nativa |
|---|---|---|---|
| NSS `PR_Read==0` | read/shutdown, imediatamente apos chamada | TCP_FIN | indisponivel (`NONE`, valid=0) |
| NSS `PR_END_OF_FILE_ERROR` | `PR_GetError` no read/shutdown | TCP_FIN | NSPR, bit pattern do erro |
| NSS `PR_CONNECT_RESET_ERROR` | `PR_GetError` no read/shutdown | TCP_RST | NSPR, bit pattern do erro |
| NSS outro erro de transporte | `PR_GetError` no site da falha | SOCKET_ERROR ou UNKNOWN | NSPR quando confiavel |
| Schannel `recv==0` | `recv_input`, read/shutdown | TCP_FIN | indisponivel (`NONE`, valid=0) |
| Schannel `WSAECONNRESET` | `WSAGetLastError` imediatamente apos `recv` | TCP_RST | WINSOCK, 10054 |
| Schannel outro erro socket | `WSAGetLastError` imediatamente apos chamada | SOCKET_ERROR | WINSOCK |
| OpenSSL `UNEXPECTED_EOF` | `SSL_read_ex`/`SSL_shutdown`, fila capturada antes de limpar | TCP_FIN | OPENSSL, reason numerico |
| OpenSSL `SSL_ERROR_SYSCALL` + `WSAECONNRESET` | erro Winsock capturado antes de outra chamada | TCP_RST | WINSOCK, 10054 |
| OpenSSL sinal ambiguo | mesmo site, sem evidencia suficiente | UNKNOWN | indisponivel ou unico dominio confiavel |

`SEC_I_CONTEXT_EXPIRED` e `SSL_ERROR_ZERO_RETURN` provam `close_notify`, nao FIN;
nao criam provenance de transporte. Chamadas posteriores nao podem sobrescrever
o primeiro snapshot.

Os tres providers ja possuem sites privados de captura suficientes. OpenSSL
precisa preservar o Winsock error imediatamente antes de `SSL_get_error` ou
outra chamada que possa altera-lo, e preservar separadamente o reason OpenSSL
quando ele for a evidencia decisiva. Isso nao requer novo hook de vtable.

## Seguranca e privacidade

Nao sao expostos strings nativas, caminhos, ponteiros, handles, hostnames,
certificados, chaves, trust, tokens ou payload. Logging/UI comum nao deve exibir
codigo nativo por default. Controle portavel usa apenas `PST_RESULT` e estados
publicos; codigo nativo e troubleshooting opt-in.

## ABI e versionamento candidatos

A estrutura possui oito `pst_u32`, com tamanho esperado de 32 bytes,
alinhamento 4 e offsets 0,4,8,12,16,20,24,28 em VC6 x86 `/ML`, VC6 x86 `/MD`
e MSVC x64. A medicao faz parte do gate antes da aceitacao.

Os novos tipos e dois simbolos sao aditivos; nenhuma estrutura ou assinatura
2.1 muda. `PST_CALL` continua `__cdecl`. A distribuicao permanece static-lib e
nao declara ordinal/DLL ABI. API 2.2.0 e candidata; library/package 0.6.5 usa
composite `0x00000605UL`. Consumers binarios/headers 2.1 continuam usando os
simbolos e layouts anteriores.

SPI 3.0 pode permanecer porque a evidencia percorre somente estado privado e a
estrutura diagnostica interna reconstruida junto com os providers in-tree. PST
nao declara ABI de plugin dinamico e nenhum hook/layout publico de provider e
adicionado.

## Testes de aceitacao propostos

- tamanhos/offsets/alinhamento em VC6 `/ML`, VC6 `/MD` e MSVC x64;
- consumer 2.1 compila/linka sem mudanca e preserva todos os layouts/simbolos;
- close_notify sem FIN: `valid=0` e resultado CLEAN;
- FIN sem close_notify: TRUNCATED + TCP_FIN;
- RST sem close_notify: TRUNCATED + TCP_RST;
- erro socket nao FIN/RST: classificacao factual ou UNKNOWN;
- sinal ambiguo: UNKNOWN sem codigo fabricado;
- reset durante handshake captura o mesmo evento sem fallback;
- timeout/cancel/wake/release local nao criam evento;
- primeira captura nao e sobrescrita por cleanup ou erro posterior;
- consulta antes/depois de shutdown e terminal ate release;
- no-resurrection, exactly-once close e reciprocal shutdown preservados;
- nenhum secret, payload ou personal path em diagnostico/log.

## Questoes que exigem decisao do owner

1. Aceitar esta ADR e congelar a estrutura de 32 bytes sem `generation`?
2. Aceitar o par unico dominio/codigo como evidencia decisiva, deixando cadeia
   causal/multiplas evidencias para evolucao futura?
3. Autorizar API 2.2.0 aditiva e os dois novos simbolos?
4. Confirmar SPI 3.0 e library/package 0.6.5 com composite `0x00000605UL`?
5. Autorizar a Phase B B1-B4 antes de packages e recertificacao externa?

## Plano condicionado de release unica

1. B1: aceitacao do owner e freeze ABI;
2. B2: query publica e captura provider/core;
3. B3: regressao FIN/RST/UNKNOWN/native/lifecycle;
4. B4: integrar shutdown reciproco e metadata 0.6.5;
5. B5: freeze unico, consumers package-only e reproducao;
6. B6: NT4 `/ML`/`/MD`, clean machine, Browser `/MD` e Accelerator OpenSSL x64;
7. B7: autorizacao separada, publicacao e download-back.

## Relacoes

- OrganizationEngineering/ADR-0002 - governanca de ADRs.
- ADR-0005 - wait-set portavel e readiness multiplexada.
- ADR-0006 - graceful shutdown como requisito de capability.
- `docs/error-diagnostics.md`
- `docs/public-api-abi.md`

## Historico de revisoes

| Revisao | Data | Descricao |
|---:|---|---|
| 1 | 2026-10-08 | Proposta inicial de classificacao portavel. |
| 2 | 2026-10-08 | Adiciona evidencia nativa opcional em camadas, remove generation especulativo e detalha ABI/lifecycle/providers. |
| 3 | 2026-10-08 | Aceita o contrato para implementacao na Phase B por decisao explicita do owner. |
