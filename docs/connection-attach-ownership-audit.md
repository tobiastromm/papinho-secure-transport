# Auditoria de ownership de `pst_connection_attach`

Baseline: PST 0.6.5 candidata
`da7190930700f54ecc92611f5edbcb0932caa5ec`, API 3.0.0, SPI 4.0.
Data: 2026-10-11.

## Core e contrato

O core inicia `ownership_accepted` em zero. Quando o provider o devolve como
um, o core armazena o transporte e entra em ATTACHED no sucesso ou FAILED no
erro. `pst_connection_try_release` chama o close root do provider; somente
close confirmado/ausência de recurso permite destruir estado do provider,
wrapper consumido e Connection.

## Fronteiras por provider

| Provider | Antes da aceitação | Fronteira irreversível | Erro posterior possível |
| --- | --- | --- | --- |
| OpenSSL | validação, FIONBIO e `SSL_set_fd` | `SSL_set_fd` bem-sucedido e socket marcado owned | nenhum passo atual depois de accepted=1 |
| Schannel | validação e FIONBIO | socket armazenado e marcado owned | nenhum passo atual depois de accepted=1 |
| RetroZilla NSS | validação, FIONBIO e falha em `PR_ImportTCPSocket` | `PR_ImportTCPSocket` bem-sucedido | opções, `SSL_ImportFD`, callback, versão/ALPN/auth/cert podem falhar |
| Combined | seleção prévia | fronteira do provider delegado | resultado/accepted delegado; nenhum fallback |

NSS é o path factual accepted+error. Conforme o ponto, conserva descritor NSPR
inferior ou agregado SSL para o close Model E one-shot. O core conserva o
wrapper consumido. Cleanup do consumer nesse estado cria risco de double close
e confusão por reutilização de handle.

## Algoritmo obrigatório do consumer

1. Ler `ownership_accepted` independentemente do resultado.
2. Se zero, manter a responsabilidade original de cleanup.
3. Se um, invalidar imediatamente todos os aliases de ownership do
   wrapper/socket.
4. No sucesso, prosseguir; no erro, reter o owner PST durável e executar Model
   E, finalizando somente com OK + DESTROYED.

O contrato já é inequívoco e nenhum defeito funcional upstream foi provado.
O achado do Browser é risco real no source se seu cleanup condicionado apenas
a sucesso alcançar o path NSS accepted+error. Esta tarefa não executou uma
reprodução runtime. A3 continua sendo evidência obrigatória na migração de
cada consumer.
