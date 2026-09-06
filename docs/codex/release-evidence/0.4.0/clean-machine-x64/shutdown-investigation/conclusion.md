# Phase 9.G-R2 — Investigação do shutdown OpenSSL

**Classificação C: EXPECTED_PEER_BEHAVIOR.** A causa observada é EOF TCP sem `close_notify` recíproco nos casos online que falham. O PST normaliza corretamente o erro nativo; não foi demonstrado bug de produção. Isso não torna o encerramento TLS completo nem autoriza converter `TRUNCATED` em sucesso.

## Evidência decisiva

Em `google-h11-keep.log`, a resposta HEAD terminou em 1.372 bytes, sem bytes extras e com corpo esperado de zero bytes. Na sequência 43 o cliente emite `close_notify`. `SSL_shutdown` retorna 0; PST retorna `result=0`, `operation=NEED_READ (1)`, `operation_error=0`. Depois do wait, `recv(MSG_PEEK)` retorna 0: EOF TCP, nenhum ciphertext pendente. A segunda chamada nativa retorna -1, `SSL_get_error=SSL_ERROR_SSL (1)` e a fila contém `0A000126: unexpected eof while reading`. Não houve alerta de leitura `close_notify` nem `SSL_ERROR_ZERO_RETURN`.

O retorno direto de `pst_connection_shutdown` continua sendo **PST_RESULT_OK (0)**; a operação é **FAILED (5)** e seu erro é **TRUNCATED (13)**. O helper `drive` do harness propagava esse erro como seu próprio retorno. O snapshot público registra `valid=1`, `generation=1`, `normalized=13`, `operation=10` (SHUTDOWN), `backend=openssl`. Essa distinção corrige a descrição abreviada do relatório anterior.

## Comparação

| Cenário | Peer close_notify | TCP EOF | PST shutdown | Classificação |
|---|---:|---:|---|---|
| local-tls13 | 1 | 1 | PASS / OK | reciprocal close_notify |
| google-h11-close | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |
| google-h11-keep | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |
| cloudflare-h11-close | 1 | 1 | PASS / OK | reciprocal close_notify |
| cloudflare-h11-keep | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |
| google-h11-keep-graceful0 | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |
| cloudflare-h11-keep-graceful0 | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |
| google-tls12-keep | 0 | 1 | FAIL / TRUNCATED (13) | EXPECTED_PEER_BEHAVIOR |

Nos oito casos o cliente emitiu `close_notify`. EOF também aparece nos casos bem-sucedidos, **depois** de o alerta recíproco ser processado; EOF isoladamente não caracteriza truncamento. As observações descrevem estas conexões, não garantem comportamento permanente de Google ou Cloudflare. Não foi feita captura em outro ponto da rede; a conclusão é sobre o peer TLS observado pelo cliente.

## HTTP e auditoria do harness

O harness anterior de GET lia inicialmente apenas um buffer; essa tentativa realmente podia iniciar shutdown com corpo HTTP pendente. Ela já havia sido substituída antes do blocker atual. A variante posterior que lia até EOF também podia tentar shutdown depois de Schannel já ter transitado a CLOSED, explicando INVALID_STATE naquela tentativa histórica.

O harness final sob investigação usa HEAD e acumula headers até `CRLF CRLF`; todos os casos controlados retornaram HTTP 200, headers completos e zero bytes além deles. Uma resposta HEAD não possui corpo, mesmo quando anuncia Transfer-Encoding: chunked. Não é necessário esperar Content-Length bytes, chunk terminator ou EOF para declarar esse HEAD completo. Não houve shutdown prematuro por um buffer incompleto nos casos atuais. O novo harness registra o framing explicitamente e preserva os requests exatos em `*.request.txt`, sem cookies, chaves ou corpo de resposta.

Com HTTP/1.1 + Connection: close, Google já apresentava EOF TCP sem alerta ao fim dos headers. Cloudflare anunciou Connection: close, deixou o alerta TLS pendente, e completou o shutdown recíproco. Com keep-alive, ambos mantinham o socket aberto/sem dados após os headers; o EOF sem alerta apareceu depois do close_notify enviado pelo cliente. Assim, o Connection header alterou o resultado Cloudflare, mas não tornou Google um peer de shutdown recíproco neste teste. HTTP/1.0 e endpoints adicionais não foram necessários.

## Local versus Internet

Foi repetida somente uma fixture local TLS 1.3 como controle da instrumentação. O servidor original empacotado ecoa os 25 bytes e chama `tls.unwrap()` imediatamente; ele inicia seu encerramento após a resposta. Havia 24 bytes cifrados pendentes antes da chamada de shutdown do cliente. A instrumentação confirmou alertas WRITE e READ; SSL_shutdown retornou 0 e depois 1, PST completou em duas etapas e o servidor registrou CLOSE=CLEAN. O controle local e Cloudflare/close demonstram que o mesmo SDK recebe e reconhece o alerta recíproco quando ele existe.

## Contrato e política

`source/src/backends/openssl/pst_backend_openssl.c:101` implementa shutdown incremental: retorno nativo 1 completa; 0 solicita NEED_READ; retorno negativo passa por SSL_get_error, WANT_READ/WANT_WRITE e classificação de falha. A função na linha 94 mapeia unexpected EOF para TRUNCATED e SSL_ERROR_ZERO_RETURN para CLOSED/CLEAN. A linha 83 remove explicitamente SSL_OP_IGNORE_UNEXPECTED_EOF. `source/src/pst_runtime.c:139` preserva a separação entre retorno da API, estado da operação e erro. Trechos numerados estão em `source-contract-excerpts.txt`.

O caminho real de shutdown observado foi 0 → wait → 1 ou -1. WANT_READ foi observado no handshake/read; WANT_WRITE não ocorreu nesta amostra, embora o mapeamento esteja no source. Nenhum SSL_ERROR_ZERO_RETURN foi observado: nos controles limpos SSL_shutdown retornou 1 e não exigiu SSL_get_error.

O gate usava `require_graceful_shutdown=1` (PST_REQUIREMENT_REQUIRED). Repetir keep-alive com valor 0 em Google e Cloudflare não mudou o resultado. O source armazena esse campo em `pst_identity.c:52`, mas o core/provider inspecionados não o consultam ao encerrar. Não há comportamento efetivo de “tolerar EOF e retornar OK” demonstrado por esse campo. A documentação de exemplo usa DISABLED, porém não promete reclassificar EOF cru como fechamento autenticado. A prosa de desenho em `openssl-provider.md:62` é menos precisa que a implementação incremental; a linha 128 e a matriz de hardening explicitam raw EOF como FAILED/TRUNCATED. Não alterar a política para fazer o teste passar.

## Integridade e limites da instrumentação

Foram adicionados apenas um harness e wrappers de encaminhamento no executable de investigação. O linker resolve SSL_new/shutdown/get_error/read/write/handshake e ERR_get_error nesses wrappers; eles chamam as funções originais das DLLs empacotadas exatamente uma vez. A callback observa apenas alertas; a fila é espiada sem consumo adicional e cada ERR_get_error original é encaminhado. errno/WSA error são preservados; MSG_PEEK não consome bytes. O MAP registra essa ligação. Não houve patch de DLL/IAT, header privado PST no harness, nova API/SPI, recompilação de produção ou alteração de packages.

Os hashes de .lib, DLLs, source inspecionado e consumer anterior são idênticos ao início (`immutable-after.json`). Todos os processos carregaram as DLLs do SDK OpenSSL extraído e seus hashes conferem. Os ZIPs/manifests não foram escritos. O compile /W4 teve avisos C4996 de getenv/fopen no harness, sem erro; nenhum gate já aprovado foi repetido além do único controle local necessário.

## Conclusão e próximo passo

Os packages continuam íntegros e este achado não demonstra necessidade de mudar ou regenerar produção. A condição de erro é corretamente preservada. A expectativa do Validation Kit de obter graceful shutdown de todo endpoint público precisa ser explícita; o fato de uma resposta HTTP completa ser útil à aplicação não transforma EOF sem alerta em TLS shutdown completo.

O CLEAN_MACHINE_X64 permanece FAIL/BLOCKED no estado anterior; nenhum critério foi redefinido nem FAIL convertido em PASS. O próximo passo concreto é repetir **somente os casos online afetados**, com **HEAD HTTP/1.1 para www.cloudflare.com, Connection: close e require_graceful_shutdown=1**, exigindo `close_notify` recíproco e sucesso real. Esse cenário já passou uma vez nesta investigação. Se os casos afetados passarem assim, a evidência pode fechar o gate sem afrouxar o contrato e sem repetir os gates já aprovados. Se o peer voltar a omitir o alerta, preservar TRUNCATED; uma alternativa de separar autenticação online de fechamento em fixture controlada exige uma decisão explícita sobre o critério, não foi aplicada aqui.

Evidências principais: `investigation-results.json`, `local-vs-online.csv`, logs/requests/sequências por caso, `classification.json`, `source-contract-excerpts.txt`, `compile.cmd/.log`, `shutdown-trace.map`, `harness-before.c`, `harness-investigation.c`, `native_trace.c`, `immutable-before` via `immutable-baseline.json` e `immutable-after.json`.

OPENSSL_SHUTDOWN_CLASSIFICATION=EXPECTED_PEER_BEHAVIOR
