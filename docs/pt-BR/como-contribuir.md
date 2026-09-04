# Como contribuir

Abra uma questão objetiva com plataforma, target, provider, comando exato, resultado esperado e observado, logs sem segredos e indicação de hardware/OS real. Faça mudanças pequenas e preserve trabalho não relacionado.

Targets afetados devem compilar sem warnings: VC6 C89 `/W4` por `tools\build-vc6.bat`, ou x64 moderno `/MD /W4` pelos scripts documentados. Separe diretórios de saída. Mudanças de segurança precisam de testes negativos determinísticos e validação real proporcional ao risco. Não enfraqueça confiança, hostname, política TLS, ownership, estados terminais, readiness ou redação de diagnósticos.

API 1.3.0 e SPI 2.4 estão congeladas. Novas propostas devem ser explícitas; layouts, valores, assinaturas e semântica não podem mudar casualmente. Código legado deve continuar C89/VC6 e alegações sobre sistemas antigos exigem testes reais.

Dependências/providers exigem revisão de licença, redistribuição, avisos, obrigações de fonte/modificação, proveniência, manutenção e modelo externo ou vendorizado. Compatibilidade técnica não concede permissão de redistribuição e isto não é aconselhamento jurídico. Veja o [checklist de provider](../provider-contributions.md).

Pesquisa e reprodução de builds são bem-vindas, especialmente sobre NSS/NSPR, VC6 e Win32 legado, além de linhagens mantidas, atualizadas ou novas capazes de TLS moderno — inclusive TLS 1.3 quando viável. O RetroZilla NSS é evidência histórica preservada; este convite não promete um fork mantido pelo PST. Documentação, traduções, testes em máquinas reais, adapters, integrações, proveniência e revisão de licenças também ajudam; não é preciso ser criptógrafo para contribuir.
