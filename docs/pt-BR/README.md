<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport

[Início do projeto](../../README.md) · [Guia prático](getting-started.md) · [English](../en/README.md)

## Comunicação segura sem prender sua aplicação a uma única biblioteca de segurança

Quando dois programas se comunicam por uma rede, os dados passam por um caminho que nem sempre está sob o controle de quem desenvolveu a aplicação.

Sem proteção adequada, alguém com acesso ao caminho da comunicação pode tentar ler, alterar ou se passar por uma das partes envolvidas. É por isso que existem protocolos de transporte seguro como o **TLS**: eles permitem criptografar a comunicação e verificar, por meio de certificados digitais, se o programa está se comunicando com quem realmente deveria, ajudando a garantir que as informações não sejam lidas, alteradas ou entregues à pessoa ou ao sistema errado.

O problema para o desenvolvedor é que usar TLS normalmente significa integrar diretamente alguma implementação específica, como OpenSSL, Schannel ou NSS.

E essa escolha começa a entrar no restante do software.

```text
Aplicação ligada diretamente a uma implementação

┌────────────────────────────────────────┐
│            SUA APLICAÇÃO               │
│                                        │
│ protocolo da aplicação                 │
│ regras de negócio                      │
│ interface                              │
│                                        │
│ + APIs específicas de TLS              │
│ + certificados                         │
│ + lifecycle da biblioteca              │
│ + tratamento de erros                  │
│ + readiness / nonblocking              │
│ + detalhes específicos do provider     │
└──────────────────┬─────────────────────┘
                   │
                   ▼
              OpenSSL / NSS /
             Schannel / outro
```

Isso pode funcionar muito bem hoje.

Mas sistemas operacionais mudam. Bibliotecas mudam. Versões de TLS mudam. **Aquilo que hoje é atual também envelhece.**

O **PapinhoSecureTransport (PST)** foi criado para colocar uma fronteira entre a aplicação e essas implementações.

```text
┌────────────────────────────────────────────┐
│               SUA APLICAÇÃO                │
│                                            │
│ navegador • e-mail • ERP • mensageria     │
│ serviço • protocolo próprio • outro app   │
└────────────────────┬───────────────────────┘
                     │
                     │ "Preciso de uma conexão
                     │  segura com estas
                     │  características."
                     ▼
┌────────────────────────────────────────────┐
│         PapinhoSecureTransport             │
│                                            │
│             contrato comum                 │
└────────────────────┬───────────────────────┘
                     │
          ┌──────────┼──────────┐
          │          │          │
          ▼          ▼          ▼
   RetroZilla NSS  Schannel   OpenSSL
          │          │          │
          └──────────┼──────────┘
                     │
                     ▼
          sistema operacional / rede
```

A aplicação passa a dizer **do que precisa**, sem precisar conhecer os detalhes de NSS, Schannel ou OpenSSL.

O **PST fica entre a aplicação e essas implementações**. Ele oferece o contrato comum que a aplicação utiliza, verifica quais capacidades são necessárias para aquela conexão e encaminha o trabalho para um provider compatível disponível naquele target.

O provider cuida de **como aquilo é realizado** usando a implementação de segurança à qual está integrado.

Dessa forma, detalhes específicos de NSS, Schannel ou OpenSSL ficam concentrados atrás da fronteira do PST, em vez de se espalharem pelo código principal da aplicação.

---

# Um exemplo concreto

Imagine que você esteja desenvolvendo um **sistema de gestão de estoque para uma rede de lojas**.

Existe um programa cliente nos computadores das lojas e um servidor central.

O cliente precisa enviar informações como:

```text
CONSULTAR_PRODUTO 18472
ATUALIZAR_ESTOQUE 18472 35
REGISTRAR_ENTRADA 18472 10
```

Essas informações não deveriam circular pela rede de uma forma que permita a outra pessoa simplesmente lê-las ou modificá-las.

Você decide proteger a comunicação usando TLS.

Sem uma camada como o PST, seu programa pode acabar integrando diretamente uma biblioteca de segurança:

```text
Sistema de estoque
      │
      ├── protocolo do estoque
      ├── regras do negócio
      ├── SSL_CTX
      ├── SSL
      ├── X509
      ├── WANT_READ / WANT_WRITE
      ├── tratamento de erros OpenSSL
      └── lifecycle OpenSSL
```

A camada de segurança começa a fazer parte da implementação do próprio sistema de estoque.

Com PST:

```text
       APLICAÇÃO CLIENTE
            NA LOJA
              │
      protocolo do estoque
              │
              ▼
     ┌─────────────────┐
     │       PST       │
     │   role CLIENT   │
     └────────┬────────┘
              │
             TLS
              │
           rede/LAN
              │
             TLS
              ▼
     ┌─────────────────┐
     │       PST       │
     │   role SERVER   │
     └────────┬────────┘
              │
      SERVIDOR CENTRAL
```

O PST não sabe o que significa `ATUALIZAR_ESTOQUE`.

Ele não precisa saber.

O protocolo empresarial continua pertencendo à aplicação. O PST não precisa entender as mensagens trocadas: ele cuida da camada que estabelece a conexão TLS, protege os dados durante a comunicação e trata seu encerramento seguro.

E isso vale tanto para uma conexão pela Internet quanto para computadores dentro de uma LAN ou de uma rede corporativa privada.

---

# CLIENT e SERVER: o que muda?

A API 2.x torna o **role da conexão explícito**.

Em uma conexão **CLIENT**, a aplicação cria e conecta o socket nativo e então entrega ao PST esse transporte já conectado.

Em uma conexão **SERVER**, o PST não toma o lugar da arquitetura de servidor da aplicação. A aplicação continua responsável por:

```text
socket
  │
bind
  │
listen
  │
accept
  │
admissão / política da aplicação
  │
socket conectado
  │
  ▼
 PST (role SERVER)
  │
 TLS
```

Ou seja: o consumidor possui `bind`, `listen`, `accept`, admissão e sessões da aplicação. O PST recebe **apenas o transporte conectado retornado por `accept`** e, depois que o ownership é aceito, passa a dirigir o TLS daquela conexão.

Isso permite que o mesmo contrato sirva tanto para software cliente quanto para software servidor, sem transformar o PST em um servidor HTTP, framework de sessões ou sistema de autorização.

Uma aplicação também pode ter conexões CLIENT e SERVER no mesmo runtime e escolher providers diferentes para cada conexão, quando o target e as capabilities permitirem.

---

# Muitas conexões sem entregar o event loop ao PST

A evolução API 2.1 acrescenta uma peça importante sem mudar essa filosofia: o PST pode participar de um scheduler da aplicação sem virar o dono da arquitetura do programa.

O **wait-set portátil** reúne várias conexões PST através de tokens escolhidos pelo consumidor. No Win32, ele também pode observar fontes externas emprestadas, como o socket de um listener que continua pertencendo à aplicação.

```text
                 thread de I/O da aplicação
                           │
                           ▼
                      PST wait-set
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
     conexão PST A    conexão PST B    listener/socket
                                      da aplicação
```

O PST não executa `accept()` no listener e não fecha uma fonte externa emprestada. Ele apenas ajuda a aplicação a descobrir **onde existe trabalho possível**.

Uma espera finita pode bloquear até que uma conexão, uma fonte externa ou um `wake` explícito precise de atenção. O `wake` pode vir de outra thread e serve para acordar o scheduler; ele não cancela conexões e não é confundido com timeout.

Isso evita exigir um loop do tipo “testa tudo, dorme alguns milissegundos e tenta novamente”. Também evita esperar `N × timeout`, uma vez para cada conexão.

Ainda assim, o PST não inventa uma política de fairness para a aplicação. Read e write continuam incrementais e bounded: uma conexão muito ativa não autoriza o PST a drenar dados indefinidamente enquanto as outras esperam. A aplicação continua dona dos seus buffers, do restante ainda não enviado, de seus deadlines e de sua política de atendimento.

---

# Por que não usar simplesmente OpenSSL, Schannel ou NSS diretamente?

Você pode.

O PST não existe porque essas tecnologias sejam ruins. Pelo contrário: elas são justamente as tecnologias que tornam o PST possível.

A diferença está em **onde a dependência fica**.

Ao usar diretamente uma implementação, sua aplicação passa a depender de sua API, seus tipos, seu lifecycle e suas particularidades.

Com PST, essas diferenças ficam atrás de uma fronteira comum:

```text
                    SUA APLICAÇÃO
                          │
                    API do PST
                          │
                          ▼
               ┌──────────────────┐
               │       PST        │
               └────────┬─────────┘
                        │
          ┌─────────────┼─────────────┐
          │             │             │
          ▼             ▼             ▼
        NSS          Schannel       OpenSSL
          │             │             │
          └─────────────┼─────────────┘
                        │
                        ▼
               plataforma suportada
```

Trocar ou acrescentar um provider compatível ainda pode exigir um novo build, um novo target ou trabalho de integração no PST, mas essa mudança fica concentrada na camada de transporte seguro, sem exigir que a lógica principal da aplicação seja reescrita para conhecer a API nativa do novo provider.

Esse desacoplamento é uma das razões mais importantes para a existência do projeto.

---

# Segurança também é uma questão de longevidade

O PST também foi pensado para **reduzir o quanto um programa criado hoje fica preso às escolhas feitas para sua camada de segurança**, porque as bibliotecas, os sistemas operacionais e os padrões de segurança que hoje consideramos atuais também envelhecem.

Na prática, essa separação permite que a aplicação continue utilizando o mesmo contrato do PST enquanto a implementação responsável pela segurança pode evoluir ao longo do tempo:

```text
HOJE

Aplicação
    │
    ▼
   PST
    │
    ▼
provider adequado ao target atual


AMANHÃ

Aplicação
    │
    ▼
   PST
    │
    ▼
provider atualizado ou diferente
```

Se o novo provider implementa as capacidades de que aquela aplicação precisa, a mudança fica concentrada na camada de transporte seguro.

Essa separação não torna software automaticamente eterno, nem garante que qualquer provider futuro possa substituir qualquer outro sem trabalho.

Mas reduz um tipo importante de acoplamento que costuma tornar aplicações mais difíceis de manter à medida que segurança, sistemas operacionais e bibliotecas evoluem.

**E esse benefício pode crescer com a comunidade**: à medida que mais projetos utilizem e contribuam com o PST, novos providers e novas plataformas podem ser desenvolvidos uma única vez e aproveitados por diferentes aplicações. Um trabalho de compatibilidade que talvez fosse inviável para o desenvolvedor de um único programa passa a poder ser compartilhado por todos os projetos que utilizam o mesmo contrato.

---

# O que o PST implementa hoje?

Atualmente, **TLS é o único protocolo de transporte seguro implementado e contratado pelo PST**.

TLS permite, entre outras coisas:

- criptografar os dados transmitidos;
- verificar a identidade do servidor;
- opcionalmente verificar também a identidade do cliente;
- negociar parâmetros seguros antes de iniciar a comunicação da aplicação.

O PST atualmente trabalha com **TLS 1.2 e TLS 1.3**, dependendo das capacidades do provider, do role e do target utilizado.

A versão pública publicada continua sendo **0.5.0 / API 2.0.0 / SPI 3.0**. O desenvolvimento atual prepara **0.6.0 / API 2.1.0**, mantendo a SPI **3.0**.

### Estado validado no desenvolvimento 0.6.0 até M9

| Provider | CLIENT | SERVER | TLS 1.2 | TLS 1.3 elegível |
|---|:---:|:---:|:---:|:---:|
| RetroZilla NSS | ✅ | ✅ | ✅ | ✅ |
| Schannel | ✅ | ✅ | ✅ | não anunciado no ambiente validado |
| OpenSSL 3.5.8 | ✅ | ✅ | ✅ | ✅ |
| Combined Schannel/OpenSSL | ✅ | ✅ | ✅ | ✅ quando OpenSSL é elegível |

Na matriz M9, **todos os 9 pares CLIENT×SERVER passaram TLS 1.2** e **todos os 4 pares elegíveis passaram TLS 1.3**. Os cinco pares TLS 1.3 envolvendo Schannel foram considerados inelegíveis antes do binding, conforme as capabilities, em vez de serem tentados e depois “rebaixados”.

A validação física final de NT4, clean machine e packages da futura 0.6.0 pertence à M10. Até ela terminar, não transformamos a evidência de desenvolvimento em uma afirmação de release publicada.

---

# TLS 1.3 no Windows NT 4.0

Um dos resultados mais interessantes obtidos durante o desenvolvimento do PST foi validar conexões **TLS 1.3 em Windows NT 4.0** através do provider baseado na linhagem RetroZilla NSS/NSPR.

Isso não significa que o PST tenha criado a implementação criptográfica de TLS 1.3 utilizada nesse caminho.

A implementação de NSS/NSPR vem de trabalho upstream do ecossistema Mozilla/RetroZilla.

O trabalho do PST está em sua própria camada: integração com esse provider, abstração do transporte, definição dos contratos comuns, lifecycle, ownership, readiness, políticas, testes e validação da interoperabilidade dentro da arquitetura do projeto.

Essa distinção é importante tanto tecnicamente quanto historicamente.

---

# Providers

Um **provider** é a implementação que conecta o contrato comum do PST a uma tecnologia concreta de segurança.

Hoje existem três providers reais.

## RetroZilla NSS

Entre as capacidades comprovadas no role CLIENT estão TLS 1.2/TLS 1.3, validação de hostname, CUSTOM_TRUST, ALPN, mTLS, Peer Info e operações nonblocking.

O role SERVER também foi validado em TLS 1.2 e TLS 1.3, incluindo Local Identity, autenticação de certificado de cliente, CUSTOM_TRUST, mTLS, I/O incremental, shutdown recíproco e detecção de truncation.

Foi validado inclusive no Windows NT 4.0 SP6 x86 na release anterior; a revalidação física da 0.6.0 ocorrerá em M10.

No role SERVER atual, `SYSTEM_TRUST` e ALPN SERVER com a semântica completa do PST **não são anunciados**.

Na API 2.1 existe ainda uma assimetria CLIENT importante: `Expected Peer Name` e SNI são conceitos separados no contrato, mas o snapshot RetroZilla NSS associa hostname/SNI através de `SSL_SetURL`. Por isso, o provider suporta o modo compatível e permanece **parcial** para controle independente de SNI. O PST não modifica o NSS para fingir uma capability que ele não possui.

---

## Schannel

Utiliza a infraestrutura de segurança fornecida pelo próprio Windows.

No ambiente atualmente validado pelo projeto — Windows 10 build 19045 — CLIENT e SERVER foram validados em TLS 1.2 dentro das capability masks publicadas.

TLS 1.3 e ALPN SERVER com a semântica completa do PST não são anunciados nesse ambiente.

Para entregar a cadeia intermediária da Local Identity no role SERVER, o backend pode utilizar temporariamente `CurrentUser\CA`, com preservação de certificados preexistentes, refcount e remoção normal controlada. Existe uma limitação documentada de