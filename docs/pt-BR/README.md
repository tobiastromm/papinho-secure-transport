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

Mas sistemas operacionais mudam. Bibliotecas mudam. Versões de TLS mudam. Aquilo que hoje é atual passa a ser legado.

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

O **PST fica entre a aplicação e essas implementações**. Ele oferece o contrato comum que a aplicação utiliza, verifica quais capacidades são necessárias e encaminha o trabalho para um provider compatível disponível naquele target.

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
              │
      protocolo do estoque
              │
              ▼
     ┌─────────────────┐
     │       PST       │
     │                 │
     │ protege os dados│
     └────────┬────────┘
              │
             TLS
              │
           rede/LAN
              │
             TLS
              ▼
     ┌─────────────────┐
     │ SERVIDOR CENTRAL│
     │                 │
     │   servidor TLS  │
     └─────────────────┘
```

O PST não sabe o que significa `ATUALIZAR_ESTOQUE`.

Ele não precisa saber.

O protocolo empresarial continua pertencendo à aplicação. O PST não precisa entender as mensagens trocadas: ele cuida da camada que estabelece a conexão TLS, protege os dados durante a comunicação e trata seu encerramento seguro.

E isso vale tanto para uma conexão pela Internet quanto para computadores dentro de uma LAN ou de uma rede corporativa privada.

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

O PST atualmente trabalha com **TLS 1.2 e TLS 1.3**, dependendo das capacidades do provider e do target utilizado.

### Estado atualmente validado

| Testado em | Provider | TLS 1.2 | TLS 1.3 | Confiança do sistema |
|---|---|:---:|:---:|:---:|
| Windows NT 4.0 SP6 x86 | RetroZilla NSS | ✅ | ✅ | — |
| Windows 10 build 19045 x64 | Schannel | ✅ | não anunciado nesse ambiente | ✅ |
| Windows 10 build 19045 x64 | OpenSSL 3.5.8 | ✅ | ✅ | ✅ |

Esta tabela registra configurações efetivamente testadas, não todos os sistemas em que o PST ou um provider subjacente talvez funcione. Windows 11, Windows Server, outros builds do Windows e plataformas não Windows não foram validados para esta versão.

Os SDKs x64 empacotados também foram compilados e exercitados em uma máquina física separada com Windows 10 Pro 22H2 x64, build 19045.6332, sem utilizar o checkout de desenvolvimento nem uma instalação global do OpenSSL.

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

Voltado ao caminho legado atualmente validado pelo projeto.

Entre as capacidades comprovadas estão:

- TLS 1.2;
- TLS 1.3;
- validação de hostname;
- confiança customizada;
- ALPN;
- autenticação mútua (mTLS);
- informações normalizadas sobre o peer;
- operações nonblocking.

Foi validado inclusive no Windows NT 4.0.

A versão utilizada pelo PST deriva da linhagem RetroZilla NSS/NSPR, e o projeto preserva e documenta sua origem, versões, modificações, processo de build e licenças.

O provider atual não oferece `SYSTEM_TRUST` pelo contrato PST e possui uma limitação de singleton própria de sua implementação.

---

## Schannel

Utiliza a infraestrutura de segurança fornecida pelo próprio Windows.

No ambiente atualmente validado pelo projeto — Windows 10 build 19045 — oferece:

- TLS 1.2;
- confiança do sistema Windows;
- confiança customizada;
- validação de hostname;
- ALPN;
- mTLS;
- peer info;
- operações nonblocking.

TLS 1.3 não é anunciado pelo PST nesse ambiente porque essa capacidade não foi validada na configuração testada.

Isso é uma propriedade do ambiente testado, não uma afirmação universal sobre Schannel.

---

## OpenSSL

O target atualmente validado utiliza **OpenSSL 3.5.8 LTS**.

Entre as capacidades comprovadas:

- TLS 1.2;
- TLS 1.3;
- confiança customizada;
- confiança do sistema Windows;
- hostname;
- ALPN;
- mTLS;
- peer info;
- operações nonblocking.

Para `SYSTEM_TRUST` no Windows, o PST combina o TLS do OpenSSL com a avaliação de confiança realizada pelas APIs de certificados do Windows.

---

# O que são certificado e CA?

Antes de um programa confiar que está falando com o servidor correto, ele precisa de alguma forma de verificar sua identidade.

Em TLS isso normalmente envolve um **certificado digital**.

De forma simplificada, o certificado funciona como uma identificação apresentada pelo servidor.

Mas simplesmente receber uma identificação não basta: é preciso saber **quem declarou que ela é confiável**.

É aí que entram as **Autoridades Certificadoras**, ou **CAs — Certificate Authorities**.

Podemos pensar assim:

```text
Autoridade Certificadora confiável
              │
              │ assina / valida
              ▼
      certificado do servidor
              │
              │ apresentado durante TLS
              ▼
           aplicação
```

O sistema verifica se o certificado apresentado pertence ao servidor esperado e se existe uma cadeia de confiança válida até uma autoridade em que ele confia.

O PST permite trabalhar, dependendo do provider, tanto com:

**SYSTEM_TRUST**

A lista/política de confiança fornecida pelo sistema operacional.

quanto com:

**CUSTOM_TRUST**

Uma autoridade ou conjunto de autoridades fornecido explicitamente pela aplicação.

Isso é útil, por exemplo, para uma empresa que possui sua própria CA interna.

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

O provider então valida a conexão de acordo com a política solicitada, sem precisar modificar o protocolo empresarial.

---

# Selecionando providers

A aplicação pode escolher como o PST procura um provider.

## AUTOMATIC

O PST usa a ordem definida pelo target e escolhe o primeiro provider disponível que possui **todas** as capacidades solicitadas.

Exemplo em um target combinado:

```text
Providers do target:

1. Schannel
2. OpenSSL
```

Pedido A:

```text
TLS 1.2 + SYSTEM_TRUST
```

Schannel possui as capacidades necessárias e, como aparece primeiro na ordem do target, é selecionado:

```text
→ Schannel
```

Pedido B:

```text
TLS 1.3 + SYSTEM_TRUST
```

No ambiente Windows 10 validado pelo projeto, o provider Schannel não possui suporte a TLS 1.3 comprovado pelo PST. O provider OpenSSL possui as duas capacidades necessárias:

```text
Schannel  ✗ TLS 1.3
OpenSSL   ✓ TLS 1.3 + SYSTEM_TRUST

→ OpenSSL
```

Assim, o PST ignora o primeiro provider por ele não atender a todos os requisitos daquele pedido e seleciona o próximo provider compatível.

## EXACT

A aplicação solicita explicitamente um provider.

Se ele não puder atender à política solicitada, a operação falha.

O PST **não troca silenciosamente para outro provider**.

## ORDERED

A aplicação fornece uma ordem própria de preferência.

---

# Inicializando os providers

Desde a API 1.3.0, uma aplicação Win32 pode registrar os providers incluídos naquele target através de:

```c
pst_win32_register_builtin_providers();
```

A aplicação não precisa incluir headers privados de NSS, Schannel ou OpenSSL para realizar esse bootstrap.

Os providers disponíveis continuam sendo definidos pelo target que foi compilado.

```text
Target legado
    └── RetroZilla NSS

Target Schannel
    └── Schannel

Target OpenSSL
    └── OpenSSL

Target combinado
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

Um dos objetivos importantes do projeto é ajudar a reduzir a distância entre software legado e padrões modernos de segurança.

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

O provider legado atual do PST utiliza trabalho proveniente da linhagem **RetroZilla / Mozilla NSS / NSPR**.

Seria muito valioso para a comunidade ver pessoas interessadas em:

- NSS;
- NSPR;
- TLS 1.3;
- criptografia moderna;
- VC6 e outros compiladores históricos;
- Win32 legado;
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

O PST 0.4.0 possui três providers funcionais. TLS 1.2 foi validado nos três; TLS 1.3 foi validado nos providers RetroZilla NSS e OpenSSL. A versão utiliza API pública 1.3.0 e SPI de providers 2.4 e é distribuída por bibliotecas estáticas e SDKs específicos para cada target.

A validação inclui execução real no Windows NT 4.0 SP6 x86, execução no Windows 10 build 19045 x64, seleção no target combinado e testes isolados dos packages em uma máquina limpa. Plataformas fora da matriz documentada permanecem sem validação.

# Distribuição

A versão 0.4.0 possui um package de código-fonte e SDKs estáticos separados para os targets RetroZilla NSS, Schannel e OpenSSL 3.5.8. Também existe um SDK combinado Schannel/OpenSSL como package oficial opcional; ele não é uma recomendação padrão.

Consulte o [guia prático](getting-started.md) e a [documentação de packaging](../release-packaging.md) para integração e seleção do target.

# Segurança e limitações

Para conhecer as fronteiras de segurança e limitações documentadas, consulte [Segurança e limitações](../security-and-limitations.md).

# Transparência no desenvolvimento

O PapinhoSecureTransport foi desenvolvido com o auxílio do OpenAI Codex, utilizado extensivamente como assistente de engenharia em atividades de implementação, testes, auditoria e documentação. As decisões arquiteturais, de produto e de release permaneceram sob responsabilidade do mantenedor do projeto.

O repositório preserva uma seleção do [histórico de engenharia e das evidências de release](../codex/README.md) para transparência e auditabilidade.

# Apoie o projeto

PapinhoSecureTransport é software livre e de código aberto sob a MPL-2.0. Se o projeto for útil para você e você quiser apoiar voluntariamente o trabalho realizado em torno dele, poderá fazê-lo pelo GitHub Sponsors.

O patrocínio não altera o acesso ao software nem os direitos concedidos por sua licença e não representa contratação de suporte, manutenção ou desenvolvimento futuro.
## Licença

O PapinhoSecureTransport é licenciado sob a [Mozilla Public License 2.0](../../LICENSE). Dependências redistribuídas preservam seus próprios termos; consulte os [avisos de terceiros](../../THIRD_PARTY_NOTICES.md).