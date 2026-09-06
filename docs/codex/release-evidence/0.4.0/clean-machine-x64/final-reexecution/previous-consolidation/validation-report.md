# PapinhoSecureTransport 0.4.0 — Clean-machine x64

**CLEAN_MACHINE_X64=FAIL** — falha reproduzida no shutdown online OpenSSL. Os handshakes SYSTEM_TRUST, a matriz de seleção e todos os testes locais passaram. O FAIL global é conservador e inclui o encerramento online adicional à prova mínima de SYSTEM_TRUST.

| Caso final | Provider | Handshake | Auth | I/O | Shutdown | Consumer |
|---|---|---|---|---|---|---|
| schannel-local-tls12 | schannel | PASS | PASS | PASS | PASS | PASS |
| openssl-local-tls12 | openssl | PASS | PASS | PASS | PASS | PASS |
| openssl-local-tls13 | openssl | PASS | PASS | PASS | PASS | PASS |
| combined-exact-local-tls13 | openssl | PASS | PASS | PASS | PASS | PASS |
| combined-exact-schannel-tls13 | UNSUPPORTED | NOT_PERFORMED | NOT_PERFORMED | NOT_PERFORMED | NOT_PERFORMED | PASS |
| openssl-system-tls13 | openssl | PASS | PASS | PASS | FAIL | FAIL |
| combined-automatic-system-tls12 | schannel | PASS | PASS | PASS | PASS | PASS |
| combined-automatic-system-tls13 | openssl | PASS | PASS | PASS | FAIL | FAIL |
| combined-ordered-system-tls12 | openssl | PASS | PASS | PASS | FAIL | FAIL |
| combined-exact-system-tls13 | openssl | PASS | PASS | PASS | FAIL | FAIL |

## Relatório A–AZ

### A. Objetivo

Validação externa CLEAN_MACHINE_X64 da distribuição 0.4.0; sem checkout ou reconstrução PST.

### B. Conclusão

CLEAN_MACHINE_X64=FAIL. Resultado global conservador: consumers online OpenSSL/Combined falham no shutdown; autenticação SYSTEM_TRUST e seleção passaram.

### C. Data

2026-09-05T19:28:29.844313-03:00

### D. Sistema

Windows 10 Pro 22H2 x64, build 19045.6332; inventário atualizado em machine-inventory.txt.

### E. Arquitetura

OS e processo de inventário x64; cl Hostx64/x64 e linker /MACHINE:X64.

### F. PATH original

machine-inventory.txt e runtime-parent-environment.json registram ambiente original. Ambiente de build e ambiente dos consumers foram limitados por processo; nenhuma alteração global.

### G. OpenSSL existente

Nenhuma instalação global encontrada nos locais usuais ou no PATH. Python tem sua própria implementação TLS para o servidor; ela não aparece nos módulos dos consumers.

### H. Variáveis OpenSSL

Nenhuma variável OPENSSL_* herdada registrada no inventário do processo. Nenhuma foi criada para fazer o teste passar.

### I. Checkout

Não encontrado nos locais óbvios consultados; nenhum conteúdo de checkout usado. A busca não é uma varredura integral do disco.

### J. MSVC

Visual Studio Build Tools 2022 17.14.37614.0, ferramentas MSVC 14.44.35207, dumpbin 14.44.35228.0. Instalados externamente pelo usuário.

### K. Windows SDK

Builds usaram explicitamente Windows 10 SDK 10.0.19041.0. SDK 10.0.26100.0 também está instalado; não usado como input destes builds.

### L. CRT

Consumers /MD; MSVC runtime x64 14.44.35211 e UCRT 10.0.19041.3636 encontrados. Somente runtime de sistema nos módulos dos consumers.

### M. Python

Python 3.14.7 x64 em AppData/Local/Python/pythoncore-3.14-64; ssl OpenSSL 3.5.7, TLS 1.3 disponível. Requisito dos servidores fixture e automação, não da API PST. Sem pip e sem instalação pelo agente.

### N. Packages

Quatro ZIPs source/Schannel/OpenSSL/Combined de 0.4.0 usados. NT4 fora do escopo.

### O. Hashes dos ZIPs

PASS: comparados diretamente com manifesto e valores históricos antes da compilação; conferidos novamente após testes. package-hashes-post-test.json contém todos os valores.

### P. Extração

Diretórios source, schannel, openssl e combined separados; nenhum arquivo de SDK modificado. Verificação final compara conjunto e bytes de todos os arquivos de cada SDK diretamente com seu ZIP.

### Q. Metadados

README, consumer-link.ini, manifest.ini, VERSION e instruções de integração lidos. Headers, .lib, runtime, LICENSE, THIRD_PARTY_NOTICES e exemplos inventariados.

### R. API pública

consumer.c inclui somente papinho_secure_transport.h e papinho_secure_transport_win32.h como headers PST; demais headers são Windows/MSVC. Sem SPI, headers privados, headers OpenSSL ou source interno no consumer.

### S. Comandos de compilação

compile-schannel.cmd, compile-openssl.cmd e compile-combined.cmd preservam vcvarsall x64 10.0.19041.0, /W4 /MD /TC /showIncludes e /VERBOSE:LIB.

### T. Includes

PASS: todas as inclusões efetivas de /showIncludes pertencem ao SDK correspondente ou Windows SDK/MSVC; lista completa em compile-and-dependency-audit.json.

### U. Link

PASS: papinho_secure_transport.lib e import libraries OpenSSL vêm do SDK correspondente; libs Windows e CRT somente da toolchain. Comandos usam caminhos absolutos para libraries dos packages.

### V. Avisos de compilação

Dois avisos C4996 por consumer para getenv/fopen no harness de validação. Não houve erro de compile/link; não se declara build sem warnings.

### W. Fixtures

root.der, server-chain.pem e server.key artificiais recebidos do usuário. Hashes registrados sem expor chave. load_cert_chain confirmou correspondência chave/certificado.

### X. Certificado local

CN/SAN localhost; validade 2026-09-02 22:31:53Z a 2026-10-02 22:31:53Z. Cadeia/trust comprovados pelos handshakes locais autenticados.

### Y. Servidor fixture

source/tests/schannel_backend_tls_server.py, bytes idênticos ao ZIP source. Sem modificar servidor, certificados ou gerar identidade nova. Endpoints 127.0.0.1:18472–18475, uma troca e ALPN fixture/1.

### Z. Schannel local

PASS: TLS 1.2, CUSTOM_TRUST root.der, CERT=1 CHAIN=1 HOSTNAME=1 AUTH=1, ALPN fixture/1, write/read 25 bytes iguais e shutdown completo; servidor registrou CLOSE=CLEAN.

### AA. Schannel dependências

PASS: dumpbin sem libssl/libcrypto; inspeção dos módulos durante o teste também não contém OpenSSL. PATH de execução: C:\Windows\System32;C:\Windows.

### AB. OpenSSL TLS 1.2 local

PASS: provider openssl, TLS=0x0303, autenticação completa, ALPN, 25 bytes com conteúdo igual, shutdown e CLOSE=CLEAN no servidor.

### AC. OpenSSL TLS 1.3 local

PASS: provider openssl, TLS=0x0304, autenticação completa, ALPN, 25 bytes com conteúdo igual, shutdown e CLOSE=CLEAN no servidor.

### AD. OpenSSL SYSTEM_TRUST

Handshake/autenticação PASS em Google e Cloudflare: TRUST=SYSTEM TLS=0x0304 CERT=1 CHAIN=1 HOSTNAME=1 AUTH=1. Nenhuma CA customizada fornecida; nenhuma fixture importada no trust store.

### AE. Online I/O

HEAD / HTTP/1.1 com Host e Connection: keep-alive; resposta HTTP 200 e headers completos antes do shutdown. Comandos, hostname, porta e bytes em logs/result JSON.

### AF. Online shutdown

FAIL para OpenSSL e Combined/OpenSSL: drive(c,1) retorna PST_RESULT_TRUNCATED (13), após HTTP HEAD completo. Reproduzido com www.cloudflare.com e www.google.com. Causa não isolada; não classificado como falha de autenticação ou indisponibilidade de DNS/Internet.

### AG. Combined AUTOMATIC TLS 1.2

PASS: SYSTEM_TRUST selecionou schannel, TLS 1.2 autenticado, HTTP e shutdown online passaram.

### AH. Combined AUTOMATIC TLS 1.3

Seleção e handshake/autenticação PASS: openssl, SYSTEM_TRUST, TLS 1.3. Consumer integrado FAIL no shutdown online.

### AI. Combined EXACT OpenSSL

PASS no teste local TLS 1.3 completo. Online: EXACT openssl + SYSTEM_TRUST/TLS 1.3 autentica; consumer integrado FAIL no shutdown.

### AJ. Combined EXACT Schannel

PASS da expectativa negativa: TLS 1.3 retorna UNSUPPORTED (código 3). Schannel em teste positivo anuncia 0x00000e7d, sem o bit TLS 1.3; OpenSSL anuncia 0x00000e7f.

### AK. Combined ORDERED

Seleção e handshake/auth PASS: [openssl, schannel] + TLS 1.2 + SYSTEM_TRUST seleciona openssl. Consumer integrado FAIL no shutdown online. Seleção de provider não é fallback pós-seleção.

### AL. Paths DLL OpenSSL

PASS: libssl-3-x64.dll e libcrypto-3-x64.dll carregadas de openssl/runtime/windows-x64-msvc-openssl-3.5.8 e combined/runtime/windows-x64-msvc-schannel-openssl-3.5.8, respectivamente.

### AM. Hashes DLL

PASS: arquivo carregado, arquivo do SDK e conteúdo distribuído coincidem. SHA-256 libssl=3fb3cd7804dbe3216c801b470e14461d80214ece99c637ae42ea3d8caf75d7ed; libcrypto=09eec573c9adea156ba2073f8cd61720d0aabeb7562d8498b4ecd21b710a3044.

### AN. Módulos

Toolhelp32 dentro de cada consumer registra paths completos antes de liberar runtime. Todos os módulos observados pertencem ao executable próprio, Windows ou runtime do SDK correspondente. loaded-module-paths.txt e dll-loaded-hashes.json.

### AO. Dependency inspection

dumpbin /DEPENDENTS para os três executables e para cada DLL empacotada. Logs dependencies-*.txt e resumo JSON; nenhuma DLL foi copiada ou substituída.

### AP. PATH de runtime

OpenSSL/Combined: somente runtime do SDK correspondente seguido de C:\Windows\System32;C:\Windows. Schannel: somente os dois diretórios Windows. Sem diretório Python ou OpenSSL global no PATH de consumer.

### AQ. Isolamento de inputs

PASS: compile/link sem C:\Projetos, checkout, dist/staging, build externo, source/third_party ou source/src. Apenas código do consumer local e SDK próprio mais toolchain.

### AR. Isolamento de runtime

PASS nos snapshots: nenhum módulo de checkout, build externo, dist/staging, third_party source ou instalação TLS Python foi carregado pelos consumers.

### AS. Sandbox

Primeira tentativa local no sandbox retornou UNSUPPORTED para Schannel TLS 1.2; repetição autorizada fora dele passou. Gates finais locais/online executados fora do sandbox. Restrição do sandbox não foi usada como resultado criptográfico.

### AT. Tentativas anteriores

Logs de GET com shutdown prematuro estão em online-before-response-drain; tentativa de drenar até fechamento em online-drain-attempt; HEAD Cloudflare em cloudflare-head-attempt. GET não integra o resultado final. Logs atuais online são HEAD Google. Não foram apagadas as falhas online anteriores.

### AU. Limite da classificação

O requisito mínimo de prova online SYSTEM_TRUST foi satisfeito; o encerramento online é cobertura adicional. Ainda assim, este relatório mantém FAIL global conservador porque os consumers completos retornam erro. Não se afirma que TLS/autenticação falharam, nem se converte truncamento em ENVIRONMENT_FAILURE sem prova.

### AV. Rastreabilidade

Cada caso final registra hash do executable e do source consumer; ambos conferem com os artefatos finais. SHA de fixtures e servidor preservado; scripts prepare.py, run-gates.py, audit.py e finalize-report.py reproduzem a preparação e análise.

### AW. Comandos e outputs

runtime-paths-and-commands.txt agrega comandos; *-consumer.log, *-server.log/.err e *-result.json contêm outputs e exit codes. local-gate-results.json e online-gate-results.json agregam as execuções.

### AX. Restrições

Nenhum clone, download PST/GitHub, rebuild PST, instalação OpenSSL global, alteração de ZIP/package, mistura de SDKs, publicação, commit, tag, push ou edição da documentação oficial. Nenhum secret de produção usado.

### AY. Próximo passo

Levar a reprodução de shutdown online e os logs ao projeto principal para determinar a causa e o critério de aceitação. Não há fixture/toolchain pendente e nenhum pedido de instalação. Não alterar SDKs nesta máquina.

### AZ. Estado final

SCHANNEL_CLEAN_MACHINE=PASS; OPENSSL_CLEAN_MACHINE=FAIL; COMBINED_CLEAN_MACHINE=FAIL; CLEAN_MACHINE_X64=FAIL. Todos os gates possíveis foram executados; nenhum PASS global emitido.

PAPINHOSECURETRANSPORT 0.4.0 CLEAN-MACHINE X64 VALIDATION BLOCKED
