<!-- SPDX-License-Identifier: MPL-2.0 -->

# Auditoria da fronteira de ownership no fechamento nativo

Status: auditoria e recomendação; não aceita a ADR-0008 nem autoriza mudanças
de produção, API ou SPI.

## Baseline e evidência

Auditoria sobre `fix/idle-connection-waitset-close`, commit
`6cbfeeb2a5d3473d296e9119bd5775ae0333a5e8`, baseline histórica anterior à decisão Model E do candidato 0.6.5,
então API 2.2.0 e SPI 3.0. O candidato atual implementado é API 3.0.0/SPI 4.0. A release 0.6.4 permanece imutável e o WIP preexistente
foi preservado.

Fontes consultadas:

- Microsoft `closesocket`: <https://learn.microsoft.com/windows/win32/api/winsock2/nf-winsock2-closesocket>.
- NSPR `PR_Close`: <https://firefox-source-docs.mozilla.org/nspr/reference/pr_close.html>.
- Source correspondente governado
  `third_party/retrozilla-nss/source/retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip`,
  SHA-256 `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`.
- NSPR `priometh.c`, `prsocket.c`, `prlayer.c`, `ntio.c`, `w95sock.c`; NSS
  `ssldef.c`, `sslsecur.c`, `sslsock.c`; providers e transport Win32 PST.

O source ZIP comprova a correspondente origem governada, não reprodução
byte-exact da DLL sem a evidência de build associada.

## Matriz por provider

| Provider | Ownership e I/O | Close atual | Consequência da falha |
| --- | --- | --- | --- |
| OpenSSL | Consumer cria/conecta/aceita; PST assume após attach. PST aplica `FIONBIO`; não altera linger; I/O síncrono não bloqueante, sem overlapped no caminho auditado. | Libera `SSL`/`SSL_CTX`, chama `closesocket`, ignora resultado e libera conexão. | Estado TLS já destruído e outcome perdido; retry não é possível no desenho atual. |
| Schannel | Mesma transferência; `FIONBIO`; `send`/`recv`/`select`, sem overlapped no provider auditado; linger intocado. | Libera contexto/credencial SSPI, certs/stores/chaves, chama `closesocket`, ignora resultado e libera conexão. | Mesma lacuna do OpenSSL. |
| RetroZilla NSS | PST importa com `PR_ImportTCPSocket`, ativa nonblocking e cria camada SSL com `SSL_ImportFD`; `PR_Poll` segue autoridade NSS. | Chama `PR_Close(ssl_fd)`, ignora resultado, zera ponteiro e libera estado. | `ssl_DefClose` remove/destrói a camada SSL antes do close inferior. `SocketClose` mantém o descriptor inferior OPEN se `_PR_MD_CLOSE_SOCKET` falha. A camada NSPR pode sobreviver, mas o estado SSL já não existe. Retry genérico de `PR_FAILURE` não está provado. |
| Combined | Delega ao provider já selecionado. | A única raiz é o provider vinculado. | Sem fallback ou troca pós-binding. |

O shutdown TLS é separado do close nativo: `SSL_shutdown`, token SSPI e
`PR_Shutdown`/alerta TLS não provam sucesso posterior de `closesocket` ou
`PR_Close`. Falha do close nativo não altera retroativamente clean/truncated.

### Evidência NSPR/NSS

`PR_Close` retorna `PR_SUCCESS` somente se o descriptor foi fechado. No
snapshot, `ssl_DefClose` remove a camada SSL, fecha a camada inferior e libera
o `sslSocket` mesmo quando o close inferior falha. `SocketClose` só marca
CLOSED e libera o `PRFileDesc` após sucesso nativo; no Windows `_PR_MD_CLOSE`
chama `closesocket` e mapeia o erro. Isso permite preservar a camada inferior
em alguns erros, mas não autoriza tratar a repetição como o mesmo objeto TLS.

## Linger: prevenção não substitui correção

PST não configura `SO_LINGER`/`SO_DONTLINGER`; sockets externos podem conservar
configuração do caller. Nonblocking + linger positivo com timeout pode produzir
`WSAEWOULDBLOCK`. A documentação Microsoft afirma, somente nesse caso, que o
handle permanece válido, a desconexão não iniciou e `closesocket` deve ser
repetido. Outros erros, inclusive `WSAENOTSOCK`, não autorizam retry cego; o
valor do handle pode ser reutilizado imediatamente após close efetivo.

Desativar linger reduziria esse caso, mas mudaria silenciosamente a semântica
do transporte recebido e não elimina todo erro. Linger abortivo é rejeitado
porque enfraquece shutdown autenticado. Uma futura política de inspeção,
rejeição ou normalização exige contrato próprio; não é solução de correctness.

## Responsabilidades normativas

| Operação | PST | Browser | Accelerator |
| --- | --- | --- | --- |
| Criar/conectar/aceitar antes do attach | Não | Sim | Sim |
| Transferir ownership | Aceita/rejeita atomicamente | Solicita | Solicita |
| Nonblocking, TLS, provider e close nativo | **Único responsável** | Não | Não |
| `SSL_free`, SSPI, `PR_Close`, `closesocket`, erro nativo | **Único responsável** | Não | Não |
| Classificação/diagnóstico portátil | **Único responsável** | Consome | Consome |
| Invocar shutdown e fornecer deadline | Executa | Política/event loop | Política/event loop |
| Wait-set da aplicação | Expõe/valida | Opera | Opera |
| Reter referência em resultado RETAINED | Define/testa | Obrigatório | Obrigatório |
| Zerar referência após DESTROYED | Define/testa | Obrigatório | Obrigatório |
| Retry | Prova segurança e executa | Agenda somente se autorizado | Idem |
| Admission, orçamento, restart/isolation | Expõe fatos | Política própria | Política própria |

Browser e Accelerator não implementam close provider-native nem interpretam
erros de provider. Políticas operacionais podem diferir; a semântica PST não.

## Alternativas e decisão recomendada

- **A — referência consumer-held:** simples e portátil, mas depende da regra
  inequívoca de nunca descartar a referência em resultado não destrutivo.
- **B — registry global PST:** evita perda da referência pelo consumer, mas
  adiciona estado global, sincronização e lifecycle de processo. Um contador
  de quarantined não limita conexões já admitidas. Não é necessário para a
  correção.
- **C — híbrido (recomendado):** PST retém estado failure-aware por conexão e
  toda mecânica nativa; consumers retêm a referência e escolhem admission,
  restart/isolation. É o menor modelo sem duplicação provider-specific.
- **D — close nos consumers:** rejeitado; viola ownership e cria double close,
  use-after-free, handle reuse e incompatibilidade NSS/Combined.

Recomenda-se também helper pointer-to-pointer que zere a referência somente em
DESTROYED. Em RETAINED a perda da última referência é misuse do contrato; API,
docs, exemplos e testes devem tornar isso explícito.

## API 3 / SPI 4 mínimos

API 3 é necessária porque `pst_connection_release(void)` não informa se o
objeto foi destruído. SPI 4 é necessária porque `connection_destroy(void)` não
preserva estado nem retorna outcome.

- DESTROYED: close confirmado, objeto destruído, referência inválida.
- Todo RETAINED: mesma referência viva, terminal para I/O, fora de wait-set;
  query de release permanece válida.
- RETRYABLE_RETAINED: somente com identidade do mesmo lifetime, nenhuma ação
  concorrente e autoridade factual para retry; nunca significa polling.
- QUARANTINED_RETAINED: outcome ambíguo ou retry inseguro; proíbe I/O, wait,
  resurrection, fallback e nova tentativa nativa.
- Wait-set ainda vinculado: falha antes de iniciar destruição.
- `NULL`, sizing curto/incompatível e enum desconhecido são erros estruturais;
  cauda de estrutura maior compatível é ignorada.
- `release(void)` legado precisa de política de migração explícita na API 3.

Nenhum provider atual expõe evento portátil que torne seguro repetir um close
ambíguo. Assim, produção deve classificar ausência de prova como QUARANTINED.
RETRYABLE fica reservado a causa com autoridade explícita, como barreira
test-only anterior à chamada ou futura notificação factual do provider. Não se
usa timeout, sleep ou WRITE readiness como oracle de `closesocket`.

Quarentena irresolúvel dura até o fim do processo ou decisão operacional de
isolamento/restart. Sem nova evidência do provider não há `resolve` público
seguro; o consumer consulta/loga, mas não fecha o handle por fora.

## Limites e múltiplos runtimes

O número 64 não está aprovado. Contar já-quarantined não cria hard bound:
conexões admitidas podem falhar simultaneamente e múltiplos runtimes contornam
contador local. Garantia process-wide exigiria reserva anterior a todo attach,
contagem sincronizada entre runtimes e rollback completo — arquitetura nova.

Não foi provada necessidade de registry global para correctness. PST pode
expor contagem factual por runtime, sem hard bound; Browser e Accelerator usam
limites/restart próprios. Muitos runtimes permanecem independentes. Se houver
exigência de hard bound, ela requer decisão separada baseada em todas as
conexões admitidas, não magic 64.

## NT4/VC6

O híbrido não requer registry/thread global e é compatível em princípio com
C89/VC6/NT4, mas precisa de `/ML`, `/MD`, `/W4`, NT4 real e packages separados.
NSS continua fechando via `PR_Close`; o core nunca chama `closesocket` sobre o
descriptor em camadas.

## Gates futuros

1. Sucesso nativo: DESTROYED, exatamente um close/destruição.
2. Falha test-only pré-close: mesmo lifetime retido, retry autorizado e sucesso.
3. Falha ambígua: QUARANTINED, sem I/O/wait/resurrection/retry.
4. `WSAENOTSOCK`: sem retry cego nem decisão pelo valor do handle.
5. Wait-set vinculado: rejeição antes do provider.
6. Helper não zera em RETAINED e zera somente em DESTROYED.
7. Concorrência, repetição, callback tardio e shutdown sem double close/UAF.
8. Múltiplos runtimes/falhas: contagem factual, sem falso hard bound.
9. OpenSSL/Schannel: `WSAEWOULDBLOCK` e outros erros separados.
10. NSS: preservação da camada inferior e erro NSPR/nativo comprovados.
11. Combined: uma raiz, zero fallback.
12. close_notify/truncation e ADR-0007 inalterados.
13. VC6 `/ML`/`/MD`, NT4, x64 e consumers package-only.
14. Browser/Accelerator usam a mesma API portátil, sem lógica nativa.

Fault injection sintética pré-close prova state machine, não comportamento de
falha Winsock real; as evidências devem permanecer separadas.

## Amendments mínimos propostos à ADR-0008

Sem editar ou aceitar a ADR: escolher modelo híbrido; remover 64/hard bound;
fixar PST como único owner da mecânica nativa; tornar referência RETAINED
normativa; invalidar referência somente em DESTROYED; restringir RETRYABLE a
autoridade factual sem polling; definir quarentena irresolúvel; registrar a
desmontagem antecipada da camada SSL NSS; separar orçamento do consumer de
garantia PST; deixar eventual hard bound para decisão própria; definir migração
de `release(void)` e sizing API 3/SPI 4; exigir os gates acima.

Decisões ainda necessárias: política do release legado; nomes/valores finais;
se algum provider atual possui caso RETRYABLE de produção; mapeamento exato
NSPR/native e evidência binária; política de cada consumer; e eventual admission
process-wide. Nenhuma autoriza consumer-native close ou enfraquecimento de
ownership, shutdown, truncation ou no-post-binding-fallback.
