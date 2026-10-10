<!-- SPDX-License-Identifier: MPL-2.0 -->

# Native-close architecture alternatives audit

Status: auditoria comparativa para decisão do owner. Não aceita a ADR-0008 e
não altera API, SPI, produção, packages ou releases.

## Baseline e identidades

- Branch: `fix/idle-connection-waitset-close`.
- HEAD: `6cbfeeb2a5d3473d296e9119bd5775ae0333a5e8`.
- Tree: `37ab882ba9ac0a9e956c7a682773d81a714d0838`.
- Candidato não publicado: 0.6.5, API 2.2.0, SPI 3.0.
- Release publicada imutável: tag `v0.6.4`, commit
  `9e32c84dad6d8a84d6f1119ddde88ffe57583c89`.

Os seis hashes publicados 0.6.4 permanecem os registrados em
`dist/packages/0.6.4/SHA256SUMS-packages.txt`; o arquivo tem SHA-256
`ce553af8932d29b46cb1c9293342809d2cee9568a2e1d320761ecf0d15ec5200`.
Os seis ZIPs candidatos 0.6.5 permanecem os registrados em
`dist/packages/0.6.5/SHA256SUMS-packages.txt`; o arquivo tem SHA-256
`f4dda53f8ac4595f756644862f87e7ac309a24cdb39258ccb8b2341ce51b2e3c`.
Esta auditoria não recertifica nem promove esses bytes.

O WIP preexistente, incluindo `tests/test_connection_failures.c`, documentos
Mode C, ADR-0008 proposta e tooling test-only, foi preservado.

## Invariantes não negociáveis

1. Nunca retornar sucesso enquanto recurso PST-owned permanece sem close.
2. Não liberar estado necessário para fechar ou conter com segurança.
3. Sem double/stale-handle close, UAF ou callback tardio inseguro.
4. Retry somente com mesmo lifetime e autoridade do provider.
5. Sem fallback pós-binding ou close nativo oculto no consumer.
6. `close_notify` TLS e close TCP/nativo são fatos distintos.
7. Consumers não implementam `closesocket`, `PR_Close` ou retry específico.
8. Objeto retido permanece observável e tem lifetime documentado.
9. Caminho saudável e ownership do wait-set permanecem intactos.
10. Outcome irresolúvel permanece truthful, nunca falso sucesso.
11. Retenção não observada não pode ser ilimitada; hard bound exige prova.
12. C89/VC6/NT4, x64 e `/ML`/`/MD` continuam obrigatórios.

## Evidência de código e contratos externos

O core chama o hook `void connection_destroy`, destrói o transport consumido e
libera o objeto incondicionalmente em `src/pst_runtime.c:151-152`. A SPI 3
declara o hook `void` em `src/pst_backend.h:45-47`. Uma conexão ainda em
wait-set é rejeitada antes disso (`src/pst_runtime.c:151`).

Microsoft documenta que `closesocket` pode retornar `WSAEWOULDBLOCK` para socket
nonblocking com linger positivo e timeout, mantendo o handle válido e sem
iniciar disconnect; também documenta reutilização imediata do valor do handle,
proibição de close concorrente e conclusão posterior de I/O overlapped:
<https://learn.microsoft.com/windows/win32/api/winsock2/nf-winsock2-closesocket>.
NSPR documenta `PR_SUCCESS` somente para close bem-sucedido e `PR_FAILURE`
caso contrário: <https://firefox-source-docs.mozilla.org/nspr/reference/pr_close.html>.

### OpenSSL

`ossl_attach` aplica `FIONBIO`, chama `SSL_set_fd` e aceita ownership em
`src/backends/openssl/pst_backend_openssl.c:107`. `ossl_connection_destroy`
libera `SSL`, `SSL_CTX` e configuração antes de chamar `closesocket`, ignora o
resultado e libera o wrapper (`:99`). O shutdown TLS ocorre separadamente por
`SSL_shutdown` (`:122`). Não há `SO_LINGER` nem overlapped no provider
auditado.

Prevenção: PROPOSED. Rejeitar ou normalizar linger positivo após transferência
evitaria o caso documentado de `WSAEWOULDBLOCK`, mas mudaria comportamento
herdado e não elimina outros erros. Falhas residuais: `WSAENOTSOCK`, falha do
subsystem, erro ambíguo, concorrência indevida e eventual I/O assíncrono futuro.

### Schannel

`sch_attach` aplica `FIONBIO` e aceita o socket
(`src/backends/schannel/pst_backend_schannel.c:241`). O destroy apaga buffers,
contexto/credencial SSPI, certificados/stores/chaves antes de chamar
`closesocket`, cujo resultado é descartado (`:237`). O close TLS usa
`ApplyControlToken(SCHANNEL_SHUTDOWN)` e token SSPI (`:275`), separado do close
nativo. Não há linger nem overlapped no provider auditado.

Prevenção e falhas residuais são equivalentes ao OpenSSL. Mudar linger não
prova close infalível.

### RetroZilla NSS

O attach torna o socket nonblocking, importa-o por `PR_ImportTCPSocket`, aceita
ownership e cria a camada SSL por `SSL_ImportFD`
(`src/backends/nss/pst_backend_nss.c:703-767`). O destroy chama `PR_Close`,
ignora o resultado, zera `ssl_fd` e libera tudo (`:625-638`).

No source governado SHA-256
`5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`,
`ssl_DefClose` remove/destrói a camada SSL antes de fechar a camada inferior;
`SocketClose` só marca CLOSED e libera o `PRFileDesc` após sucesso de
`_PR_MD_CLOSE_SOCKET`; `_PR_MD_CLOSE` usa `closesocket`. Portanto a camada
inferior pode sobreviver, mas o descriptor SSL original já foi desmontado.

Prevenção por linger antes da importação é apenas PROPOSED e afeta contrato do
socket recebido. Depois da importação, PST deve respeitar NSPR. Falhas
residuais: todo `PR_FAILURE` cuja preservação/retry não esteja provada,
desmontagem parcial e erro nativo não recuperável. Retry cego é REJECTED.

### Combined

O core fixa `pst_connection.provider` na criação e despacha pelo mesmo vtable
até release (`src/pst_runtime.c:13,119,151`). Não há segunda raiz de close.
Combined significa conjunto de providers elegíveis, não fallback pós-binding.

## Auditoria de prevenção

| Provider | Desabilitar linger positivo | Efeito comprovado | Falhas residuais |
| --- | --- | --- | --- |
| OpenSSL | Não autorizado pelo contrato atual | Remove somente o caso Winsock documentado de WSAEWOULDBLOCK por linger | Sim |
| Schannel | Não autorizado pelo contrato atual | Igual ao OpenSSL | Sim |
| NSS | Exigiria ação antes/import-aware e prova NSPR | Pode reduzir o mesmo caso no socket inferior | Sim; inclui stack parcialmente desmontado |

`PREVENTION_EFFECTIVENESS=MITIGATION_ONLY`. Não se recomenda alteração
silenciosa de linger, abortive close ou enfraquecimento de `close_notify`.

## Alternativas

### A — SPI 4 `try_close` + API 3 structured `try_release`

PROVEN como capaz de representar os invariantes; PROPOSED como contrato.
Uniforme e extensível, porém o candidato atual de 64 bytes inclui retry,
contador e limite sem necessidade comprovada. Risco: generalizar antes de haver
provider com progresso de retry factual.

### B — resultado público simples + query separada

PROPOSED e viável. `try_release` retorna `OK` somente se destruiu,
`INVALID_STATE` para precondition retained e `RESOURCE_FAILURE` para native
retained; `get_release_info` fornece disposição e diagnóstico. Preserva a
distinção essencial, mas ainda requer record estruturado e SPI 4. Duas chamadas
não criam race se o objeto é terminal, consumer-owned e single-operation.

### C — socket nativo consumer-owned / PST borrowing

REJECTED. Invalida a transferência atual, exige consumers fecharem por fora e
não funciona uniformemente com `PRFileDesc` em camadas, SSL/SSPI teardown ou
Combined. Duplica justamente a mecânica que PST abstrai.

### D — registry/quarantine manager central PST

PROPOSED, mas desnecessário para correctness. Pode preservar referências
perdidas, ao custo de estado global, sincronização, shutdown de processo e
complexidade NT4. Sem reserva para toda conexão admitida não prova hard bound.

### E — política mínima por provider + boundary failure-aware

PROPOSED e **selecionada**. SPI 4 tenta o close e devolve apenas
`CLOSED_CONFIRMED`, `NO_NATIVE_RESOURCE` ou `RETAINED_UNRESOLVABLE` nesta
release. API 3 destrói somente nos dois primeiros; no terceiro retorna
`RESOURCE_FAILURE`, preserva a referência e expõe query factual. Não publica
retry até um provider demonstrar condição de progresso sem polling. É o modelo
híbrido reduzido à semântica atualmente demonstrável, sem manager global.

### F — normalizar linger e manter destroy void

REJECTED. É menos código, mas não satisfaz I1, I2, I10 nem falhas residuais.

## Consumer duplication test

Browser e Accelerator executam o mesmo pseudocódigo mecânico:

| Browser | Accelerator |
| --- | --- |
| `remove_from_wait_set(c)` | `remove_from_wait_set(c)` |
| `r = pst_connection_try_release(c, &info)` | `r = pst_connection_try_release(c, &info)` |
| `if (r == OK) c = NULL` | `if (r == OK) c = NULL` |
| `else retain(c); log(info); apply_browser_policy()` | `else retain(c); log(info); apply_accelerator_policy()` |

Ambos preservam o ponteiro em não sucesso. Nenhum interpreta
`WSAEWOULDBLOCK`, `PR_FAILURE`, linger, socket numérico ou detalhes OpenSSL,
Schannel/NSS. A única diferença legítima é política: Browser pode isolar uma
sessão/UI; Accelerator pode interromper admission ou substituir o worker.

Um helper pointer-to-pointer que zera somente em sucesso reduz o risco de
lost-reference, mas não substitui ownership disciplinado. A API C não impede
um caller incorreto de descartar o último ponteiro sem registry.

## Contrato de progresso e retry

`RETRYABLE_RETAINED` sozinho é insuficiente: ele não diz quando repetir e pode
criar hot loop. As categorias conceituais seriam:

- retry permitido imediatamente;
- retry após evento concreto READ/WRITE/provider signal;
- retry proibido.

Nenhum provider atual fornece sinal portátil pós-close que prove a segunda
categoria. Winsock exige repetir após `WSAEWOULDBLOCK`, mas não define uma
readiness de socket como autoridade para isso. Uma tentativa imediata pode
falhar repetidamente; Sleep/polling/deadline não prova elegibilidade. NSS ainda
tem a transformação de stack.

Assim, a decisão mínima 0.6.5 é **não expor retry de produção**: toda falha
nativa não conclusiva vira `RETAINED_UNRESOLVABLE`. Uma futura extensão só
pode adicionar retry quando o provider fornecer uma destas autoridades:

1. condição já satisfeita e tentativa única explicitamente autorizada;
2. interesse READ/WRITE factual registrado em um release-wait específico;
3. callback/evento provider-owned correlacionado ao mesmo generation ID.

Nesse futuro, PST decide elegibilidade e o consumer apenas agenda a operação
após o evento PST. Reusar o wait-set normal exigiria estado especial porque a
conexão deve ser removida antes do release; a menor adição provável seria um
evento release-progress provider-neutral ou uma operação de wait própria.
Isso tem impacto ABI/API e está deferido, não implicitamente aprovado.

Cancelamento nunca converte retained em destroyed. Deadline continua sendo
política do consumer e não autoriza close cego.

## Estado mínimo de contenção

Para a primeira implementação, o conjunto seguro e verificável é conservador:
reter o objeto core, runtime/provider, wrapper de conexão, transport, lifetime
generation, diagnóstico e todo estado provider ainda referenciado. Isso evita
partial free prematuro; otimização de memória vem somente após prova.

| Provider | Estado mínimo comprovadamente necessário |
| --- | --- |
| OpenSSL | socket, ownership/lifetime, wrapper e diagnóstico. Liberar `SSL`/`SSL_CTX` separadamente pode ser possível após unbind e sem callbacks, mas é UNKNOWN até teardown reorder testado; inicialmente reter tudo. |
| Schannel | socket, ownership/lifetime, wrapper e diagnóstico. Contexto/credencial/certs podem ter dependências de shutdown/cleanup; partial destruction é UNKNOWN; inicialmente reter tudo. |
| NSS | ponteiro que, após `ssl_DefClose`, pode representar a camada inferior, runtime NSPR carregado, ownership/lifetime, wrapper e diagnóstico. O SSL state pode já ter sido destruído internamente; PST não pode alegar que o ponteiro ainda é SSL. |
| Combined | exatamente o estado do provider vinculado mais sua identidade; nenhuma segunda raiz. |

Trust/config/identity só podem ser liberados separadamente se não houver
referência provider, callback tardio ou necessidade diagnóstica. Retenção
integral é maior, porém arquiteturalmente menor e mais verificável.

## Resolução e terminalidade

Transições permitidas no modelo selecionado:

```text
BOUND_OR_TERMINAL
  -> UNBOUND
  -> CLOSE_ATTEMPT
       -> CLOSED_CONFIRMED/NO_RESOURCE -> FINAL_DESTROY -> DESTROYED
       -> FAILURE_OR_UNKNOWN -> RETAINED_UNRESOLVABLE
```

`RETAINED_UNRESOLVABLE` é terminal para handshake, I/O, shutdown, wait-set,
attach, fallback e provider switch. Somente query/diagnóstico é permitido.
Não há resurrection, force-destroy ou segunda tentativa. Evidência tardia não
muda o estado nesta versão. Process termination contém o recurso pelo SO, mas
não prova close bem-sucedido nem exactly-once.

Uma futura transição para retryable exige evento provider-owned no mesmo
generation e regra específica aceita; valor de handle, `WSAENOTSOCK`, timeout
ou ausência de callback não bastam. Overlapped futuro exige drenar completions
antes de liberar suas estruturas. Os providers atuais auditados usam caminho
síncrono, mas a API não deve prometer isso universalmente.

## Lost reference e observabilidade

Sem registry, PST não pode impedir que C code descarte o último ponteiro. As
mitigações mínimas são: contrato normativo; helper pointer-to-pointer; exemplos
que só zeram após OK; testes de mutation/negative; generation opaco; logging de
retained; e runtime mantido vivo enquanto `connections != 0` (o core já adia
destroy via contagem em `src/pst_runtime.c:151`).

Para 0.6.5, uma nova API de counters não é necessária. `PST_RELEASE_INFO` por
objeto, log normalizado e contagem mantida pelo consumer são suficientes. Uma
contagem runtime pode ser extensão futura de observabilidade, mas não evita
lost reference nem cria bound global. Runtime teardown com filhos deve emitir
diagnóstico factual; não pode liberar módulos exigidos por objeto retido.

## Bounds e admission

Todas as conexões já admitidas podem falhar simultaneamente. Portanto:

- consumer admission: recomendado como política operacional;
- warning/count por runtime: observabilidade, não garantia;
- registry/counter process-wide: desnecessário para correctness;
- hard bound: somente com reservation para toda conexão antes de attach,
  sincronização process-wide e rollback completo;
- process isolation/restart: contenção legítima para retained irresolúvel.

Nenhum limite library-enforced, inclusive 64, é recomendado nesta release.
Browser e Accelerator podem escolher thresholds diferentes usando a mesma
semântica release.

## Matriz negativa futura

OpenSSL e Schannel devem cobrir close success, barreira sintética pré-close,
`WSAEWOULDBLOCK` real se reproduzível, `WSAENOTSOCK`, erro ambíguo, reuse do
número, ausência/presença futura de I/O pendente e teardown reorder. A injeção
sintética prova state machine, não Winsock real.

NSS deve cobrir `PR_Close` success, falha source-backed, descriptor inferior
retido, camada SSL já destruída, stack parcial e proibição de retry. É preciso
instrumentar a identidade factual do descriptor transformado antes de declarar
qualquer recuperação.

Todos cobrem wait-set bound precondition, close_notify recíproco, FIN/RST e
truncation, no resurrection, repeated release, shutdown de runtime/process e
exactly-once quando comprovável. Combined só propaga a disposição selecionada.

## API 3 e SPI 4: decisão mínima proposta

API 3 continua necessária porque o `void release` não pode comunicar retenção.
SPI 4 continua necessária porque `void connection_destroy` não pode retornar
outcome nem preservar estado. SPI 3 deve ser rejeitada por API 3 sem fallback.

Assinaturas candidatas mínimas:

```c
PST_RESULT pst_connection_try_release(
    pst_connection *connection,
    PST_RELEASE_INFO *release_info);

PST_RESULT pst_connection_get_release_info(
    const pst_connection *connection,
    PST_RELEASE_INFO *release_info);
```

`pst_connection_release(void)` é removida como breaking change expressa.
`try_release` retorna:

- `PST_RESULT_OK`: somente DESTROYED; ponteiro inválido;
- `PST_RESULT_INVALID_STATE`: PRECONDITION_RETAINED, incluindo wait-set;
- `PST_RESULT_RESOURCE_FAILURE`: UNRESOLVABLE_RETAINED após falha nativa.

A query não repete close. O caller preserva a referência em todo non-OK.

### Layout recomendado para decisão do owner

O record de 64 bytes da ADR proposta não é mínimo porque inclui
`retry_condition`, `quarantine_count` e `quarantine_limit` ainda não suportados
por contrato factual. Recomenda-se V1 de 48 bytes:

| Offset | Campo | Tipo |
| ---: | --- | --- |
| 0 | `struct_size` | `pst_u32` |
| 4 | `api_version` | `pst_u32` |
| 8 | `disposition` | `pst_u32` |
| 12 | `result` | `pst_i32` |
| 16 | `flags` | `pst_u32` |
| 20 | `native_domain` | `pst_u32` |
| 24 | `native_code` | `pst_i32` |
| 28 | `native_code_valid` | `pst_u32` |
| 32 | `terminal` | `pst_u32` |
| 36 | `generation_low` | `pst_u32` |
| 40 | `generation_high` | `pst_u32` |
| 44 | `reserved0` | `pst_u32` |

`PST_RELEASE_INFO_V1_MIN_SIZE=48UL`, nunca `sizeof`. Init rejeita NULL/short,
zera apenas os 48 bytes conhecidos, grava API 3 e preserva a cauda oversized.
Operações rejeitam major incompatível antes de close. `result` replica o
retorno. A cauda permite futura extensão minor para progresso/counters sem
redefinir V1. Offset/size precisam de probes VC6 `/ML`/`/MD` e x64; nenhum foi
executado nesta auditoria.

SPI 4 mínimo:

```text
connection_try_close(provider_connection, close_result)
  -> NO_NATIVE_RESOURCE
  -> CLOSED_CONFIRMED
  -> RETAINED_UNRESOLVABLE
```

O resultado contém domain/code somente quando factual. `connection_destroy`
torna-se cleanup infalível permitido apenas após NO_RESOURCE/CLOSED. NSS pode
usar estado privado específico; Combined delega. Não há hook opcional.

## Matriz de decisão

| Alternativa | Status | Invariantes | Escopo | Risco principal |
| --- | --- | --- | --- | --- |
| A structured generalizada | PROPOSED | Satisfaz | Maior | Publicar retry/limites prematuros |
| B result simples + query | PROPOSED | Satisfaz | Médio | Ainda requer record/SPI 4 |
| C transport borrowed | REJECTED | Viola I3/I5/I7 | Grande | Ownership duplicado/NSS inválido |
| D registry global | PROPOSED/DEFERRED | Pode satisfazer | Grande | Complexidade sem hard-bound proof |
| E provider policy + retained terminal | **SELECTED** | Satisfaz | Menor seguro | Retenção até process containment |
| F linger + destroy void | REJECTED | Viola I1/I10 | Pequeno aparente | Falhas silenciosas residuais |

Modelo E é a menor arquitetura correta porque implementa somente distinções
demonstradas: destruído ou retido irresolúvel. Ele mantém a fronteira híbrida,
mas não exige manager, retry scheduler, counter público ou magic limit.

## Amendments necessários à ADR-0008

Antes de aceitação, o owner deve autorizar revisão que:

1. selecione explicitamente o Modelo E;
2. remova `RETRYABLE_RETAINED` da entrega inicial, sem proibir extensão futura;
3. substitua `QUARANTINED_RETAINED` por nomenclatura factual
   `UNRESOLVABLE_RETAINED` ou aprove equivalência explícita;
4. remova limite 64, `quarantine_count/limit` e alegação de bound;
5. aprove ou rejeite o layout V1 de 48 bytes;
6. mantenha API 3 try-release-only e SPI 4 mandatory try-close;
7. estabeleça retained terminal, query-only e process containment;
8. registre o comportamento NSS de desmontagem da camada SSL;
9. mantenha PST como único owner de mecânica nativa;
10. exija gates VC6/NT4/x64/Combined/consumers antes de implementação/release.

## STOP e conclusão

Não há evidência suficiente para retry de produção cross-provider; por isso o
modelo selecionado não o oferece. Isso não bloqueia um contrato seguro de
retenção terminal. A identidade exata da camada inferior NSS após falha deve
ser qualificada durante implementação antes de qualquer partial destruction;
até lá, retenção conservadora é a única afirmação segura.

Nenhum build ou teste foi executado, conforme o escopo. Nenhuma claim de probe
PASS é feita. ADR-0008 continua PROPOSED.
