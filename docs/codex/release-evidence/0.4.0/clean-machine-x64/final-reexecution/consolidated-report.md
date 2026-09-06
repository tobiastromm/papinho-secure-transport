# PapinhoSecureTransport 0.4.0 — Clean-machine x64 final

**CLEAN_MACHINE_X64=PASS**

A reexecução final passou com `www.cloudflare.com:443`, HEAD HTTP/1.1, `Connection: close`, `SYSTEM_TRUST`, TLS 1.3 e `require_graceful_shutdown=1`. O cliente recebeu a resposta HEAD completa (4.921 bytes de headers, zero corpo), enviou e recebeu `close_notify`; `SSL_shutdown` retornou 0 → 1. O PST completou o shutdown com retorno da API 0, operação COMPLETE (0), erro 0 e exit code 0. Nenhum TRUNCATED foi aceito ou observado neste gate.

```text
TRUST=SYSTEM
TLS=0x0304
CERT=1
CHAIN=1
HOSTNAME=1
AUTH=1
CLIENT_CLOSE_NOTIFY=1
PEER_CLOSE_NOTIFY=1
SHUTDOWN=COMPLETE
TRUNCATED=0
OPENSSL_ONLINE_GATE=PASS
```

## Gates consolidados

| Gate | Resultado | Base |
|---|---|---|
| PACKAGE_HASHES | PASS | resultado anterior preservado |
| COMPILE_LINK_SCHANNEL | PASS | resultado anterior preservado |
| COMPILE_LINK_OPENSSL | PASS | resultado anterior preservado |
| COMPILE_LINK_COMBINED | PASS | resultado anterior preservado |
| COMPILE_INPUT_PROVENANCE | PASS | resultado anterior preservado |
| SCHANNEL_TLS12 | PASS | resultado anterior preservado |
| OPENSSL_TLS12 | PASS | resultado anterior preservado |
| OPENSSL_TLS13 | PASS | resultado anterior preservado |
| OPENSSL_TLS13_SYSTEM_TRUST | PASS | reexecutado nesta etapa |
| OPENSSL_GRACEFUL_SHUTDOWN | PASS | reexecutado nesta etapa; alertas recíprocos obrigatórios |
| COMBINED_AUTOMATIC | PASS | seleção/TLS/autenticação já aprovados; antigos shutdowns online não são reclassificados |
| COMBINED_EXACT | PASS | resultado anterior preservado, incluindo expectativa UNSUPPORTED |
| COMBINED_ORDERED | PASS | seleção/TLS/autenticação já aprovados; antigo shutdown online não é reclassificado |
| OPENSSL_DLL_PROVENANCE | PASS | paths e hashes confirmados também na reexecução |
| NO_CHECKOUT_DEPENDENCY | PASS | resultado anterior preservado |
| NO_EXTERNAL_STAGING_BUILD_DEPENDENCY | PASS | resultado anterior preservado |
| RUNTIME_ISOLATION | PASS | módulos somente executable local, Windows e SDK correspondente |

Somente o teste online afetado foi reexecutado. Compilações, testes locais, seleção Combined e isolamento já aprovados não foram repetidos. A consolidação usa esses resultados conforme autorização expressa da reexecução final. As execuções antigas que terminaram em TRUNCATED permanecem com seus resultados originais; o novo gate de graceful shutdown é o cenário explícito acima.

## Contrato de encerramento

Google e keep-alive não são usados como gate de graceful shutdown porque os peers observados podem fechar TCP sem close_notify recíproco. A investigação classificou esse comportamento como EXPECTED_PEER_BEHAVIOR. Isso não torna TRUNCATED aceitável como PASS: o runner final exige ambos os alertas, retorno nativo 1, operação PST COMPLETE, resposta HEAD completa, sucesso do consumer e ausência de truncamento. A política continuou obrigatória, com valor 1.

## Proveniência e imutabilidade

A única alteração do Validation Kit foi adicionar o runner `final-online-gate.py`, que fixa o cenário solicitado e reutiliza o executable de observação já validado. Não houve rebuild PST, alteração de produção/API/SPI/package, troca de DLL, instalação global OpenSSL, clone, publicação ou alteração de documentação oficial. A investigação e seus logs foram preservados integralmente.

O executable usado está em `shutdown-investigation/shutdown-trace.exe`; os hashes do executable, harness e observer conferem com a investigação. Os comandos e cópias do source de validação estão em `final-reexecution/`. A compilação original do observer está registrada em `shutdown-investigation/compile.cmd` e `compile.log`; nenhuma nova compilação foi necessária.

PATH do consumer: `C:\pse-clean-code\clean-validation\openssl\runtime\windows-x64-msvc-openssl-3.5.8;C:\Windows\System32;C:\Windows`. Os módulos OpenSSL carregados vieram desse SDK; os outros módulos pertencem ao executable de validação ou Windows.

| DLL carregada do SDK | SHA-256 |
|---|---|
| libssl-3-x64.dll | 3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed |
| libcrypto-3-x64.dll | 09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044 |

Os hashes de .lib, DLLs, consumer anterior e source inspecionado continuam idênticos à investigação (`final-reexecution/unchanged-inputs.json`). O PASS dos quatro ZIPs/manifests é consolidado da evidência já aprovada em `package-hashes-post-test.json`; os ZIPs não foram escritos.

## Ambiente e evidências

Ambiente previamente inventariado: Windows 10 Pro 22H2 x64 build 19045.6332; MSVC v143 14.44.35207, Windows SDK 10.0.19041.0 e runtime MSVC x64 14.44.35211. Python 3.14.7 foi usado para automação, sem fornecer DLLs aos consumers PST.

- [Resultado e checks do gate](result.json)
- [Sequência nativa e pública](stdout.log)
- [Prova resumida](gate-proof.txt)
- [Comando e PATH](command.txt)
- [Request HTTP exata](request.txt)
- [Matriz consolidada e fontes](consolidated-gates.json)
- [Investigação preservada](../shutdown-investigation/conclusion.md)
- [Relatório A–AZ anterior, preservado como histórico](previous-consolidation/validation-report.md)

Os arquivos `final-stage-matrix.json` e `online-gate-results.json` pertencem à rodada anterior. O resultado vigente é `final-summary.json`, sustentado pela matriz e pelo gate em `final-reexecution/`.

CLEAN_MACHINE_X64=PASS

PAPINHOSECURETRANSPORT 0.4.0 CLEAN-MACHINE X64 VALIDATION PASS
