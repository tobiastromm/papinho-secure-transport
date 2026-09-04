# Primeiros passos

O PST protege um fluxo de bytes; o protocolo da aplicação continua sendo responsabilidade do programa. O fluxo normal é:

1. Chamar `pst_win32_register_builtin_providers()` antes de criar qualquer runtime.
2. Criar o runtime com seleção AUTOMATIC, EXACT ou ORDERED e informar as capacidades exigidas.
3. Criar confiança e, para mTLS, credenciais; configurar hostname e política TLS; congelar a configuração.
4. Conectar um socket TCP, envolvê-lo com `pst_win32_socket_transport_create`, criar a conexão e anexar com `PST_OWNERSHIP_TRANSFERRED`.
5. Avançar handshake, leitura, escrita e shutdown incrementalmente. `NEED_READ`, `NEED_WRITE` ou `NEED_READ_WRITE` pede uma espera limitada pela readiness indicada e nova tentativa. Readiness permite tentar; não garante progresso.
6. Liberar conexão, configuração, entradas e runtime pelas funções do PST.

Uma rede local segue o mesmo modelo: uma estação de filial pode se conectar a `erp.internal`, usar uma CA corporativa explícita por CUSTOM_TRUST, verificar esse hostname e trocar o protocolo do ERP sobre o PST. Isso não exige Internet pública nem HTTP.`r`n`r`nNunca desative autenticação apenas para o teste passar. CUSTOM_TRUST usa uma CA DER explícita e não cai silenciosamente para confiança do sistema. SYSTEM_TRUST exige um provider que anuncie a capacidade. Sempre informe o hostname esperado.

O conjunto embutido pertence ao target ligado: VC6/NT4 usa RetroZilla NSS; o target Schannel usa Schannel; o target OpenSSL usa OpenSSL; o target combinado de validação registra Schannel antes de OpenSSL. O bootstrap não procura providers no PATH ou no disco.