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

A API 2.0 torna o **role da conexão explícito**.

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

A API pública atual é **2.0.0**, a SPI de providers é **3.0** e a versão do package/library é **0.5.0**.

### Estado atualmente validado

| Testado em | Provider | CLIENT | SERVER | TLS 1.2 | TLS 1.3 |
|---|---|:---:|:---:|:---:|:---:|
| Windows NT 4.0 SP6 x86 | RetroZilla NSS | ✅ | ✅ | ✅ | ✅ |
| Windows 10 build 19045 x64 | Schannel | ✅ | ✅ | ✅ | SERVER não anunciado |
| Windows 10 build 19045 x64 | OpenSSL 3.5.8 | ✅ | ✅ | ✅ | ✅ |
| Windows 10 build 19045 x64 | Combined Schannel/OpenSSL | ✅ | ✅ | ✅ | ✅ quando OpenSSL é elegível |

Esta tabela registra configurações efetivamente testadas, não todos os sistemas em que o PST ou um provider subjacente talvez funcione.

Os SDKs x64 empacotados também foram compilados e exercitados a partir dos packages extraídos em uma máquina física separada com Windows 10 Pro x64, sem utilizar o checkout de desenvolvimento nem uma instalação global do OpenSSL para o runtime dos testes.

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

Foi validado inclusive no Windows NT 4.0 SP6 x86.

A versão utilizada pelo PST deriva da linhagem RetroZilla NSS/NSPR, e o projeto preserva e documenta sua origem, versões, modificações, processo de build e licenças.

No role SERVER atual, `SYSTEM_TRUST` e ALPN SERVER com a semântica completa do PST **não são anunciados**.

---

## Schannel

Utiliza a infraestrutura de segurança fornecida pelo próprio Windows.

No ambiente atualmente validado pelo projeto — Windows 10 build 19045 — o role CLIENT oferece TLS 1.2, confiança SYSTEM/CUSTOM, validação de hostname, ALPN, mTLS, Peer Info e operações nonblocking de acordo com a capability mask publicada.

O role SERVER foi validado em TLS 1.2 com Local Identity, CUSTOM_TRUST, SYSTEM_TRUST, autenticação de certificado de cliente, I/O incremental, shutdown recíproco e truncation.

TLS 1.3 SERVER e ALPN SERVER com a semântica completa do PST não são anunciados nesse ambiente.

Para entregar a cadeia intermediária da Local Identity no role SERVER, o backend pode utilizar temporariamente `CurrentUser\CA`, com preservação de certificados preexistentes, refcount e remoção normal controlada. Existe uma limitação documentada de possível resíduo após crash.

Isso descreve o ambiente validado, não uma afirmação universal sobre todas as versões do Schannel.

---

## OpenSSL

O target atualmente validado utiliza **OpenSSL 3.5.8 LTS**.

CLIENT e SERVER foram validados em TLS 1.2 e TLS 1.3.

Entre as capacidades comprovadas estão CUSTOM_TRUST, SYSTEM_TRUST do Windows onde anunciado, hostname no CLIENT, ALPN, mTLS, Peer Info, operações nonblocking, I/O bidirecional, shutdown recíproco e truncation.

Para `SYSTEM_TRUST` no Windows, o PST combina o TLS do OpenSSL com a avaliação de confiança realizada pelas APIs de certificados do Windows.

No role SERVER, Expected Peer Name não se aplica; a validação de certificados de cliente usa a finalidade adequada de `clientAuth`.

---

# O que são certificados e CAs?

Antes de um programa confiar que está falando com o servidor correto — ou, quando configurado, antes de um servidor confiar no certificado apresentado por um cliente — ele precisa de uma forma de verificar a identidade TLS do peer.

Em TLS isso normalmente envolve um **certificado digital**.

De forma simplificada, o certificado funciona como uma identificação apresentada por um endpoint.

Mas simplesmente receber uma identificação não basta: é preciso saber **quem declarou que ela é confiável**.

É aí que entram as **Autoridades Certificadoras**, ou **CAs — Certificate Authorities**.

Podemos pensar assim:

```text
Autoridade Certificadora confiável
              │
              │ assina / valida
              ▼
      certificado do endpoint
              │
              │ apresentado durante TLS
              ▼
           outro endpoint
```

O sistema verifica se existe uma cadeia de confiança válida e, no CLIENT, quando solicitado, se a identidade apresentada corresponde ao nome esperado.

O PST separa quatro conceitos que não devem ser confundidos:

- **Local Identity**: cadeia + chave privada que este endpoint apresenta;
- **Peer Authentication**: certificado do peer DISABLED, OPTIONAL ou REQUIRED;
- **Peer Trust**: exatamente CUSTOM ou SYSTEM;
- **Expected Peer Name**: validação de nome no role CLIENT.

`CUSTOM_TRUST` e `SYSTEM_TRUST` não são unidos nem substituídos silenciosamente.

Autenticação TLS também não equivale à autorização da aplicação: um certificado válido não se torna automaticamente um usuário, conta ou Principal do sistema.

---

# Exemplo: rede corporativa com CA própria

Imagine novamente o sistema de estoque.

A empresa possui servidores internos e uma autoridade certificadora própria.

```text
             CA DA EMPRESA
                  │
             assina certificados
                  │
                  ▼
             Servidor ERP
                  ▲
                  │ TLS
                  │
             ┌────┴────┐
             │   PST   │
             └────┬────┘
                  │
             Cliente ERP
```

A aplicação pode fornecer essa CA ao PST através de `CUSTOM_TRUST`.

Se a empresa também exigir certificado do cliente, o servidor PST pode configurar `Peer Authentication=REQUIRED` e validar o certificado apresentado pelo cliente contra o trust configurado.

O provider então valida a conexão de acordo com a política solicitada, sem precisar modificar o protocolo empresarial.

---

# Selecionando providers

A seleção é **por conexão**.

## AUTOMATIC

O PST segue a ordem de registro do target e escolhe o primeiro provider elegível para o role e para **todas** as capabilities solicitadas.

Exemplo em um target combinado:

```text
Providers do target:

1. Schannel
2. OpenSSL
```

Pedido SERVER A:

```text
TLS 1.2
```

Se ambos forem elegíveis e Schannel aparecer primeiro, Schannel pode ser selecionado.

Pedido SERVER B:

```text
TLS 1.2 + ALPN SERVER
```

Como Schannel SERVER não anuncia ALPN SERVER com a semântica completa do PST no ambiente validado, ele é eliminado **antes do binding**. OpenSSL pode então ser escolhido.

## EXACT

A aplicação solicita explicitamente um provider.

Se ele não puder atender ao role/política solicitados, a operação falha.

## ORDERED

A aplicação fornece uma ordem própria de preferência. Providers inelegíveis podem ser descartados antes do binding e o próximo candidato pode ser considerado.

Depois que um provider aceita o binding/ownership do transporte, porém, ele fica **fixado**. Falha posterior de handshake, certificado, trust, ALPN, I/O, transporte ou shutdown não provoca troca para outro provider.

Isso é seleção pré-binding, não fallback pós-handshake.

---

# Inicializando os providers

Uma aplicação Win32 registra explicitamente os providers incluídos naquele target através do bootstrap público do PST.

A aplicação não precisa incluir headers privados de NSS, Schannel ou OpenSSL para realizar esse bootstrap.

Os providers disponíveis continuam sendo definidos pelo target que foi compilado.

```text
win32-x86-vc6-retrozilla-nss
    └── RetroZilla NSS

win32-x64-msvc-19.51-schannel
    └── Schannel

win32-x64-msvc-19.51-openssl3
    └── OpenSSL

win32-x64-msvc-19.51-schannel-openssl3
    ├── Schannel
    └── OpenSSL
```

O bootstrap é explícito: o PST não procura aleatoriamente bibliotecas instaladas no computador nem registra providers escondido da aplicação.

---

# TLS hoje; outras formas de transporte seguro talvez amanhã

**TLS é atualmente o único protocolo de transporte seguro implementado e contratado pelo PST.**

A arquitetura foi desenvolvida para evitar dependência de uma implementação TLS específica.

Isso também deixa espaço para que, **se houver interesse e colaboração da comunidade**, outras famílias de transporte seguro sejam estudadas no futuro.

Por exemplo:

- DTLS;
- transportes relacionados a QUIC;
- protocolos baseados em Noise;
- outras tecnologias que façam sentido para o projeto.

Nenhuma delas é suportada hoje.

Também não existe garantia de que a SPI atual possa recebê-las sem mudanças.

A inclusão de uma nova tecnologia dependeria de um caso de uso real, desenho arquitetural adequado, segurança, testes, manutenção e pessoas interessadas em desenvolvê-la.

---

# Novos providers também podem surgir com colaboração

Os três providers atuais não precisam representar para sempre todas as implementações possíveis.

A comunidade pode propor integrações com outras bibliotecas ou tecnologias de segurança.

Mas um novo provider não entra no projeto apenas porque “funciona”.

Também precisam ser considerados:

- segurança;
- manutenção;
- sistemas e compiladores suportados;
- licença;
- possibilidade de redistribuição;
- obrigações de código-fonte e notices;
- origem e histórico das dependências (provenance);
- capacidade de reproduzir o build;
- testes independentes de interoperabilidade.

Em alguns casos, pode fazer mais sentido que o usuário forneça separadamente a biblioteca necessária em vez de o PST redistribuí-la.

Cada caso precisa ser analisado individualmente.

---

# Nota para a comunidade de retrocomputação

Um dos objetivos importantes do projeto é ajudar a reduzir a distância entre software antigo e padrões modernos de segurança.

À medida que padrões de segurança evoluem, programas e sistemas antigos podem perder a capacidade de se comunicar com serviços atuais mesmo quando continuam sendo úteis para aquilo para o qual foram criados.

Uma maneira de contornar isso seria fazer o lado moderno voltar a aceitar versões antigas e menos seguras dos protocolos.

O PST ajuda a explorar outra direção:

> **até onde podemos levar padrões modernos de segurança às aplicações e sistemas antigos sem exigir que o outro lado reduza sua segurança?**

Isso permite investigar casos como:

- navegadores antigos;
- clientes de e-mail;
- aplicações corporativas;
- programas cliente-servidor;
- software especializado;
- outros sistemas preservados pela comunidade.

A mesma arquitetura que hoje permite estudar essa ponte para sistemas antigos também ajuda a reduzir o acoplamento de programas criados hoje às tecnologias de segurança disponíveis hoje.

Afinal, o que hoje chamamos de moderno também envelhece.

---

# Um convite especial: NSS, NSPR e TLS moderno em sistemas antigos

Existe uma área de contribuição particularmente interessante.

O provider para o target `win32-x86-vc6-retrozilla-nss` utiliza trabalho proveniente da linhagem **RetroZilla / Mozilla NSS / NSPR**.

Seria muito valioso para a comunidade ver pessoas interessadas em:

- NSS;
- NSPR;
- TLS 1.3;
- criptografia moderna;
- VC6 e outros compiladores históricos;
- Win32 em sistemas antigos;
- versões antigas do Windows;

estudando como manter, atualizar ou criar uma linhagem reproduzível e mantida dessas tecnologias para plataformas antigas.

O PST não promete criar ou manter sozinho essa futura linhagem. É justamente uma área onde **colaboração externa pode ampliar aquilo que o projeto consegue alcançar**.

---

# Onde o PST pode ser usado?

Alguns exemplos:

```text
Navegador
    │ HTTP
    ▼
   PST
    │ TLS
    ▼
Internet
```

```text
Cliente de e-mail
    │ SMTP / IMAP
    ▼
   PST
    │ TLS
    ▼
Servidor de e-mail
```

```text
Cliente ERP
    │ protocolo empresarial
    ▼
   PST
    │ TLS
    ▼
LAN / rede corporativa
    │
    ▼
Servidor ERP
```

```text
Aplicação própria
    │ protocolo próprio
    ▼
   PST
    │ transporte seguro
    ▼
Outro computador
```

HTTP, SMTP, IMAP e o protocolo empresarial **não fazem parte do PST**.

Eles aparecem apenas para mostrar o tipo de software que pode utilizar a camada de transporte seguro.

---

# PST no ecossistema Papinho

O PST é um projeto independente.

Alguns projetos do ecossistema Papinho ajudam a ilustrar usos diferentes.

### PapinhoBrowser

Usa o PST como camada segura abaixo de HTTP/HTTPS.

### PapinhoLegacyMail

Usa o PST abaixo de SMTP e IMAP.

OAuth, contas, providers de e-mail, XOAUTH2 e os próprios protocolos de e-mail continuam responsabilidade do PapinhoLegacyMail.

### PapinhoAccelerator

PapinhoAccelerator é um componente **específico do PapinhoBrowser**.

Usa o PST para estabelecer a conexão segura entre o PapinhoBrowser e o PapinhoAccelerator, protegendo os dados trocados entre eles.

Dependendo da configuração, o Accelerator também pode realizar conexões externas em nome do Browser, utilizando novamente o PST como camada de transporte seguro.

---

# Estado atual do projeto

O PST 0.5.0 possui três providers funcionais e roles CLIENT e SERVER explícitos por conexão através da API pública 2.0.0 e SPI de providers 3.0.

TLS 1.2 foi validado nos três providers. TLS 1.3 foi validado em RetroZilla NSS e OpenSSL. O role SERVER foi validado nos três providers dentro de suas capability masks factuais.

A validação inclui execução real no Windows NT 4.0 SP6 x86, execução no Windows 10 build 19045 x64, interoperabilidade cruzada entre providers, seleção EXACT/ORDERED/AUTOMATIC, matriz negativa de segurança/lifecycle, consumers extraídos dos packages e testes em uma máquina Windows limpa separada.

Plataformas fora da matriz documentada permanecem sem validação.

# Distribuição

A versão 0.5.0 possui um package de código-fonte e SDKs estáticos separados para:

- `win32-x86-vc6-retrozilla-nss`;
- `win32-x64-msvc-19.51-schannel`;
- `win32-x64-msvc-19.51-openssl3`;
- `win32-x64-msvc-19.51-schannel-openssl3`.

O Combined Schannel/OpenSSL é um package oficial opcional para seleção de providers; ele não representa uma quarta implementação TLS e não é uma recomendação padrão.

Consulte o [guia prático](getting-started.md), a [matriz de targets](../target-matrix.md), a [API 2.0](../api-2.0.md), a [SPI 3.0](../provider-spi-3.0.md), o [guia de migração](../api-1.3-to-2.0-migration.md) e a [documentação de packaging](../release-packaging.md).

# Segurança e limitações

Para conhecer as fronteiras de segurança, limitações por provider e detalhes como shutdown, truncation, trust e adaptação Schannel SERVER, consulte [Segurança e limitações](../security-and-limitations.md).

# Transparência no desenvolvimento

O PapinhoSecureTransport foi desenvolvido com o auxílio do OpenAI Codex, utilizado extensivamente como assistente de engenharia em atividades de implementação, testes, auditoria e documentação. As decisões arquiteturais, de produto e de release permaneceram sob responsabilidade do mantenedor do projeto.

O repositório preserva uma seleção do [histórico de engenharia e das evidências de release](../codex/README.md) para transparência e auditabilidade.

# Apoie o projeto

PapinhoSecureTransport é software livre e de código aberto sob a MPL-2.0. Se o projeto for útil para você e você quiser apoiar voluntariamente o trabalho realizado em torno dele, poderá fazê-lo pelo GitHub Sponsors.

O patrocínio não altera o acesso ao software nem os direitos concedidos por sua licença e não representa contratação de suporte, manutenção ou desenvolvimento futuro.

## Licença

O PapinhoSecureTransport é licenciado sob a [Mozilla Public License 2.0](../../LICENSE). Dependências redistribuídas preservam seus próprios termos; consulte os [avisos de terceiros](../../THIRD_PARTY_NOTICES.md).
