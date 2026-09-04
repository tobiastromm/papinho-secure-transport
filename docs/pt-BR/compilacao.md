# Compilação

Execute a partir da raiz. Use `tools\build-vc6.bat clean` e `tools\build-vc6.bat test test-nss-unit` para VC6/NSS (`build\vc6`); `tools\build-modern-msvc.bat clean` e `test` para Schannel x64; `tools\build-modern-msvc-openssl.bat clean` e `test` para OpenSSL x64; e `tools\build-modern-msvc-combined.bat clean` seguido de `combined-test` para o target combinado isolado.

VC6 usa C89 `/W4`. SDK, runtime, hashes, fontes, patches e avisos NSS/NSPR estão preservados no repositório; não existe dependência ativa de `C:\PSTW`. A reprodutibilidade é nível B, não promessa byte a byte.

Os builds modernos usam x64 `/MD /W4`. Schannel vem do Windows; a evidência cobre TLS 1.2 no Windows 10 build 19045, onde TLS 1.3 não é anunciado pelo adapter. OpenSSL usa somente a dependência 3.5.8 preparada e seus manifests. Testes públicos com SYSTEM_TRUST dependem do ambiente. O target combinado é validação, não pacote de release prometido; Phase 9.E decidirá isso.