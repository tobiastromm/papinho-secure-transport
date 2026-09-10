---
adr: ADR-0005
title: Wait-set portavel e readiness multiplexada controlada pelo consumer
status: accepted
decision-date: 2026-09-09
last-revised: 2026-09-09
revision: 1
scope: project
decision-makers:
  - Tobias Tromm
supersedes: []
superseded-by: []
---

# ADR-0005 - Wait-set portavel e readiness multiplexada controlada pelo consumer

## Contexto

A API 2.0 oferece readiness incremental por conexao, mas obriga o consumer a
coordenar separadamente cada espera. Accelerator precisa esperar muitas
conexoes PST, listeners/fontes nativas emprestadas e uma solicitacao explicita
de wake em uma unica thread de I/O. Browser e LegacyMail tambem precisam que
cancelamento, deadlines e upgrade de um transporte previamente usado nao sejam
confundidos com semantica interna de um provider TLS.

Readiness TLS nao e necessariamente igual a readiness do socket. Em particular,
o backend RetroZilla NSS precisa continuar confirmando progresso por `PR_Poll`
no descriptor SSL. Ao mesmo tempo, tipos como `SOCKET`, `PRFileDesc *`, `SSL *`,
`BIO *` ou handles SSPI nao podem entrar no contrato portavel.

## Forcas da decisao

- uma unica espera controlada pela aplicacao;
- composicao entre providers e fontes externas;
- compatibilidade com VC6, NT4 e C89;
- ausencia de polling periodico obrigatorio;
- provider como autoridade final da readiness TLS;
- wake thread-safe e reutilizavel;
- ownership e lifetime deterministas;
- bounded enumeration sem perda silenciosa;
- evolucao aditiva da API 2.x e, se necessario, da SPI 3.x;
- separacao entre SNI e Expected Peer Name.

## Alternativas consideradas

### Expor handles nativos dos providers

Rejeitada: vazaria implementacao, impediria composicao portavel e permitiria ao
consumer contornar a semantica TLS do provider.

### Fazer polling sequencial periodico de cada conexao

Rejeitada como modelo normativo: aumenta latencia/custo e exige acordar sem um
evento real. Timeout zero continua valido como operacao explicita, nao como
obrigacao de busy polling do consumer.

### Wait-set portavel com adaptador de plataforma

Aceita. O core conserva membros e tokens opacos. O adaptador de plataforma
espera fontes observaveis e um wake privado. Quando uma fonte associada a PST
acorda, o provider confirma o interesse efetivo antes da publicacao do resultado.

## Decisao

### Modelo publico

`pst_wait_set` e um objeto opaco. Um membro recebe um `pst_wait_token` escolhido
pelo consumer e estavel enquanto registrado. Tokens nao derivam de sockets,
ponteiros de provider ou indices internos.

Existem dois tipos de membro:

1. conexao PST, cuja readiness publicada e a intersecao entre interesse atual e
   readiness confirmada pelo provider;
2. fonte nativa emprestada, criada por API de adaptador de plataforma e observada
   segundo interesses portaveis READ/WRITE.

O header portavel nao contem tipos nativos. O header Win32 pode receber o valor
opaco de um `SOCKET` como `pst_size`, seguindo a fronteira ja adotada para
transportes Win32.

### Enumeracao bounded

O caller fornece array e capacidade. A espera informa o numero total pronto e o
numero copiado. Se a capacidade for insuficiente, retorna
`PST_RESULT_INSUFFICIENT_CAPACITY`; os primeiros membros seguem a ordem estavel
de registro. Readiness nao e consumida pela enumeracao: nova espera com timeout
zero pode observar novamente membros ainda prontos. Nenhum estado e descartado
silenciosamente.

Membros terminais permanecem visiveis com `PST_WAIT_READY_TERMINAL` ate remocao.

### Wake e tempo

`pst_wait_set_wake()` e thread-safe, coalescente, nao terminal e nao cancela
conexoes. O resultado WAKE e distinto de TIMEOUT. Wake ocorrido antes da espera
permanece pendente para a proxima espera; uma espera que reporta wake consome a
pendencia observada.

Timeout de espera e relativo, em milissegundos, e aceita zero. Ele significa
somente que nenhuma fonte ficou pronta naquela chamada. Deadlines de handshake,
read, write, shutdown ou politica da aplicacao pertencem ao consumer e devem usar
tempo monotonico; PST nao inventa deadline global.

### Threading e lifetime

- operacoes handshake/read/write/shutdown da mesma conexao sao serializadas por
  um unico contexto de execucao de I/O;
- operacoes concorrentes na mesma conexao sao invalidas;
- `wake()` pode ser chamado concorrentemente;
- add/remove pertencem inicialmente a thread owner e nao ocorrem durante wait;
- conexao deve ser removida antes de `pst_connection_release()`;
- release de conexao registrada e invalido e nao libera a conexao;
- release durante wait e invalido;
- destroy de wait-set com membros registrados e invalido;
- fonte externa e emprestada, nunca fechada pelo PST;
- destroy/remove nunca fecha fonte externa;
- o contrato nao requer worker thread oculto.

O wait-set registra a conexao em exatamente um wait-set por vez. Duplicata,
remocao ausente e token duplicado sao erros deterministas.

### Fronteira provider/plataforma

Provider readiness permanece encapsulada. Em Win32, o adaptador pode usar a
fonte nativa conectada apenas para bloquear eficientemente; antes de publicar
readiness PST ele executa a verificacao nao bloqueante do provider. Assim,
`select()` nao substitui `PR_Poll`, SSPI ou OpenSSL como autoridade semantica.

Uma falha estrutural do wait-set, falha de readiness do provider, membro terminal,
fonte externa pronta, wake, timeout e violacao de membership/lifetime recebem
resultados/flags normalizados distintos. Erros nativos continuam diagnosticos,
nao controle obrigatorio do scheduler.

### SNI e Expected Peer Name

`server_name_indication` e metadata de roteamento enviada no ClientHello.
`expected_peer_name` e a identidade autenticada esperada no certificado. O
campo aditivo `server_name_indication_mode` possui tres estados:

```text
PST_SNI_MODE_COMPAT   = 0
PST_SNI_MODE_DISABLED = 1
PST_SNI_MODE_EXPLICIT = 2
```

COMPAT preserva consumidores zero-inicializados: Expected Peer Name, quando
presente, tambem pode ser usado como SNI, exatamente como em PST 2.0. Sem nome
esperado, COMPAT nao implica SNI. DISABLED solicita ausencia explicita de SNI,
sem retirar o significado independente de Expected Peer Name. EXPLICIT envia o
nome indicado, que pode ser igual ou diferente do nome autenticado. EXPLICIT
sem nome e invalido. Configuracao CLIENT SNI em role SERVER e invalida neste
contrato.

Controle independente completo e uma capability role-scoped factual,
`PST_CAP_SNI_CONTROL`. EXPLICIT e DISABLED combinado com Expected Peer Name
exigem essa capability antes do binding. COMPAT nao a exige. Falha posterior
nunca provoca fallback.

OpenSSL e Schannel podem anunciar a capability somente apos prova real de nome
enviado e nome autenticado independentes. O snapshot RetroZilla NSS oferece ao
PST CLIENT apenas `SSL_SetURL` para esse caminho e permanece PARTIAL: suporta o
comportamento COMPAT historico, mas nao anuncia controle independente. NSS/NSPR
nao sera alterado, nao recebera parser TLS provider-local e nao tera verificacao
de certificado substituida para fabricar paridade.

A matriz CLIENT R6 foi fechada factual e role-scoped:

| combinacao | OpenSSL | Schannel | RetroZilla NSS |
|---|---|---|---|
| COMPAT + Expected Peer Name | suporta; SNI historico observado | suporta; SNI historico observado | suporta; `SSL_SetURL`, SNI observado |
| COMPAT + sem Expected Peer Name | sem SNI pelo contrato; conexao sem identidade e rejeitada pela politica CLIENT atual | sem SNI pelo contrato; conexao sem identidade e rejeitada pela politica CLIENT atual | sem SNI implicito; configuracao sem autenticacao permanece sujeita a politica do provider |
| DISABLED + Expected Peer Name | suporta; ausencia de SNI observada e nome autenticado | suporta; ausencia de SNI observada e nome autenticado | nao suportado, filtrado antes do binding |
| DISABLED + sem Expected Peer Name | sem SNI pelo contrato; conexao sem identidade e rejeitada pela politica CLIENT atual | sem SNI pelo contrato; conexao sem identidade e rejeitada pela politica CLIENT atual | combinacao estreita nao exige controle independente; sujeita a politica de autenticacao do provider |
| EXPLICIT igual ao Expected Peer Name | suporta; valor observado | suporta; valor observado | nao suportado, filtrado antes do binding |
| EXPLICIT diferente do Expected Peer Name | suporta; SNI observado e certificado validado contra o nome esperado | suporta; SNI observado e certificado validado contra o nome esperado | nao suportado, filtrado antes do binding |

Assim, `SNI_CONTROL` e FULL para OpenSSL e Schannel e PARTIAL para RetroZilla
NSS. A classificacao nao enfraquece a politica CLIENT existente, que exige
autenticacao de identidade nos dois providers modernos. A selecao filtra a
capability antes de inicializar/vincular o provider; uma falha depois do
binding permanece terminal e nao tenta outro provider.

### Upgrade apos plaintext

O transporte conectado pode ser usado pelo consumer antes do attach. O consumer
deve parar exatamente na fronteira de upgrade, sem pre-ler bytes de records TLS,
e entao transferir ownership ao PST. Depois que ownership for aceito, falha nao
reverte para plaintext nem devolve ownership. PST nao interpreta SMTP, IMAP,
HTTP CONNECT ou outro protocolo de tunel.

`PRE_READ_TLS_BYTES_BEFORE_ATTACH` permanece nao suportado neste escopo.

### Isolamento, rotacao e metadata futura

Configuracoes continuam snapshots imutaveis por conexao. Rotacao cria novo
snapshot para novas conexoes; conexoes existentes conservam identidade/trust
anteriores. Multiplos runtimes nao implicam thread safety nem eliminam limites
globais factuais dos providers. Metadata futura sera aditiva e nunca incluira
handles nativos; nesta fase somente token/membership requerido pelo scheduler e
adicionado.

### Versionamento

As novas funcoes/tipos e o campo final de SNI sao aditivos, preservando o prefixo
ABI 2.0. A API passa a 2.1.0. O tamanho minimo aceito de
`PST_CONNECTION_CONFIG` continua sendo o tamanho publicado em 2.0; campos novos
so sao lidos quando `struct_size` os cobre.

O mecanismo nao requer novo hook obrigatorio de provider: a validacao final usa
o `wait(..., timeout=0)` existente. A SPI permanece 3.0. Se uma plataforma futura
precisar de hook provider adicional, ele devera ser opcional e motivar SPI 3.x,
sem alterar esta semantica publica.

A library version da implementacao sera 0.6.0.

## Consequencias

### Positivas

- Accelerator pode compor listeners, conexoes TLS e wake em uma espera;
- Browser ganha SNI independente de identidade autenticada;
- STARTTLS e CONNECT compartilham a mesma fronteira generica;
- providers continuam substituiveis e factuais;
- API/SPI major permanecem estaveis.

### Negativas / trade-offs

- cada plataforma precisa de adaptador real de multiplexacao;
- a primeira implementacao Win32 herda limites factuais de `select()`;
- add/remove concorrente fica fora da primeira versao;
- pre-read de records TLS nao e recuperado pelo PST.

## Regras derivadas

1. Wait-set core e tokens sao portaveis.
2. Fontes nativas entram somente por adapter publico de plataforma.
3. Handles privados de provider nunca sao publicos.
4. Fonte externa e sempre emprestada e consumer-owned.
5. Wake e thread-safe, reutilizavel e distinto de timeout.
6. Deadline de operacao pertence ao consumer.
7. Enumeracao insuficiente e explicita e repetivel.
8. Terminal permanece observavel ate remocao.
9. Remove precede release.
10. Readiness nativa nao substitui confirmacao do provider.
11. Trabalho apos readiness continua bounded e incremental.
12. SNI e Expected Peer Name sao independentes.
13. Upgrade exige fronteira limpa e transfere ownership sem rollback.
14. Nenhuma falha pos-binding seleciona outro provider.

## Verificacao de conformidade

- headers portaveis nao contem tipos Win32/provider;
- testes cobrem zero/um/muitos/mistos, overflow, terminal e lifecycle;
- testes cobrem fonte externa e wake antes/durante/depois de timeout;
- `timeout=0` nao bloqueia;
- NSS continua confirmando readiness por `PR_Poll`;
- release registrada falha sem liberar;
- fonte emprestada permanece aberta apos remove/destroy;
- SNI diferente do Expected Peer Name chega corretamente a cada provider;
- fixtures STARTTLS-style e CONNECT-style usam o mesmo transporte apos plaintext;
- builds VC6 `/W4` e MSVC moderno permanecem sem warnings.

## Relacoes

- PapinhoEngineering/ADR-0003 - logging estruturado e desacoplado.
- PapinhoEngineering/ADR-0007 - fronteiras portaveis.
- PapinhoEngineering/ADR-0009 - identidade, autenticacao, trust e nome do peer.
- PapinhoEngineering/ADR-0010 - adaptacao de providers e capability honesty.
- ADR-0001 - role e selecao por conexao.
- ADR-0002 - capabilities conscientes de role.
- `docs/readiness-progress.md`
- `docs/lifecycle-ownership.md`

## Historico de revisoes

| Revisao | Data | Descricao |
|---:|---|---|
| 1 | 2026-09-09 | Congela wait-set, wake, fonte externa, threading, SNI, upgrade apos plaintext e versionamento da trilha. |
