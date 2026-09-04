# PapinhoSecureTransport

## Comunicação segura sem prender sua aplicação a uma única biblioteca de segurança

Quando dois programas se comunicam por uma rede, os dados passam por um caminho que nem sempre está sob o controle de quem desenvolveu a aplicação.

Sem proteção adequada, alguém com acesso ao caminho da comunicação pode tentar ler, alterar ou se passar por uma das partes envolvidas. É por isso que existem protocolos de transporte seguro como o **TLS**: eles permitem criptografar a comunicação e verificar com quem o programa está falando.

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

Mas sistemas operacionais mudam. Bibliotecas mudam. Versões de TLS mudam. Aquilo que hoje é atual um dia também será legado.

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

A aplicação passa a dizer **do que precisa**.

O provider cuida de **como aquilo é realizado** usando a implementação de segurança disponível naquele target.

Isso reduz a quantidade de código específico de NSS, Schannel ou OpenSSL que precisa se espalhar pela aplicação principal.

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
       CLIENTE DA LOJA
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
              │
     ┌────────▼────────┐
     │       PST       │
     └────────┬────────┘
              │
      protocolo do estoque
              │
              ▼
        SERVIDOR CENTRAL
```

O PST não sabe o que significa `ATUALIZAR_ESTOQUE`.

Ele não precisa saber.

O protocolo empresarial continua pertencendo à aplicação. O PST cuida da parte responsável por estabelecer e manter a comunicação segura.

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
          Windows NT 4.0 / Windows 10 /
       outras versões e sistemas suportados
```

Trocar ou acrescentar um provider compatível continua podendo exigir um novo build, um novo target ou trabalho de integração no PST.

O que muda é que **a lógica principal da aplicação não precisa ser reescrita para conhecer a API nativa daquele provider**.

Esse desacoplamento é uma das razões mais importantes para a existência do projeto.

---

# Segurança também é uma questão de longevidade

O PST não foi pensado apenas para computadores que **já são antigos**.

Ele também tenta reduzir o quanto um programa criado **hoje** fica preso às escolhas tecnológicas disponíveis hoje.

Windows 10 já é um bom exemplo dessa passagem do tempo: durante anos foi uma plataforma atual; progressivamente passa a ocupar o lugar de uma plataforma legada.

O mesmo acontecerá com sistemas que hoje consideramos novos.

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

| Sistema / target validado | Provider | TLS 1.2 | TLS 1.3 | Confiança do sistema |
|---|---|:---:|:---:|:---:|
| Windows NT 4.0 SP6 x86 | RetroZilla NSS | ✅ | ✅ | — |
| Windows 10 build 19045 x64 | Schannel | ✅ | não anunciado nesse ambiente | ✅ |
| Windows 10 build 19045 x64 | OpenSSL 3.5.8 | ✅ | ✅ | ✅ |
| Windows 11 | — | ⏳ validação pendente | ⏳ validação pendente | ⏳ |
| Windows Server | — | ⏳ validação pendente | ⏳ validação pendente | ⏳ |

A tabela representa **o que o projeto efetivamente validou**, e não tudo que cada sistema operacional ou biblioteca pode teoricamente suportar.

Por exemplo, o fato de TLS 1.3 não ter ficado disponível no Schannel do Windows 10 build 19045 usado nos testes **não significa que Schannel seja limitado a TLS 1.2 em todas as versões do Windows**.

Windows 11, Windows Server e outros targets precisam ser testados explicitamente antes de aparecerem como suportados/validados pelo projeto.

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

A versão utilizada pelo PST possui provenance documentada e deriva da linhagem RetroZilla NSS/NSPR.

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

TLS 1.3 não é anunciado pelo PST nesse ambiente porque a validação funcional não passou.

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

Pedido:

```text
TLS 1.2 + SYSTEM_TRUST
```

Schannel consegue atender:

```text
→ Schannel
```

Pedido:

```text
TLS 1.3 + SYSTEM_TRUST
```

No ambiente Windows 10 atualmente validado, Schannel não anuncia TLS 1.3.

OpenSSL anuncia:

```text
Schannel  ✗ TLS 1.3
OpenSSL   ✓ TLS 1.3 + SYSTEM_TRUST

→ OpenSSL
```

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
- provenance;
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

Esse trabalho não precisaria beneficiar apenas o PST.

Poderia ser útil para navegadores, clientes de e-mail e inúmeros outros projetos de preservação e retrocomputação.

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

# Projetos Papinho que podem utilizar PST

O PST é um projeto independente.

Alguns projetos do ecossistema Papinho ajudam a ilustrar usos diferentes.

### PapinhoBrowser

Pode usar PST como camada segura abaixo de HTTP/HTTPS.

### PapinhoLegacyMail

Pode usar PST abaixo de SMTP e IMAP.

OAuth, contas, providers de e-mail, XOAUTH2 e os próprios protocolos de e-mail continuam responsabilidade do PapinhoLegacyMail.

### PapinhoAccelerator

PapinhoAccelerator é um componente **específico do PapinhoBrowser**.

Ele não faz parte da arquitetura genérica do PST e não é requisito para outros consumidores.

---

# O que o PST não tenta fazer?

PST não é:

- um navegador;
- um cliente de e-mail;
- um proxy HTTP;
- uma implementação SMTP/IMAP;
- uma implementação de protocolo empresarial;
- um gerenciador de contas;
- uma autoridade certificadora;
- um substituto para NSS, Schannel ou OpenSSL.

Ele ocupa a fronteira responsável pelo **transporte seguro**.

---

# Como contribuir

Há espaço para contribuições em várias direções.

Você não precisa ser especialista em criptografia.

São úteis contribuições em:

- documentação;
- português e inglês;
- outras traduções;
- testes em máquinas reais;
- Windows legado;
- VC6 e C89;
- Windows 11 e Windows Server;
- reprodução de builds;
- NSS/NSPR;
- Schannel;
- OpenSSL;
- TLS moderno em plataformas antigas;
- novos providers;
- novos sistemas operacionais;
- exemplos de integração;
- aplicações corporativas;
- navegadores;
- e-mail;
- mensageria;
- protocolos próprios;
- provenance;
- análise de licenças.

E, se houver pessoas interessadas em pesquisar novas famílias de transporte seguro, esse trabalho também pode ser discutido — sem que isso implique que tais tecnologias já sejam suportadas pelo PST.

---

# Estado atual do projeto

O PST possui atualmente:

- três providers funcionais;
- TLS 1.2 validado nos três;
- TLS 1.3 validado nos providers RetroZilla NSS e OpenSSL;
- API pública 1.3.0;
- SPI de providers 2.4;
- builds separados para targets diferentes;
- testes de interoperabilidade entre implementações diferentes;
- validação real no Windows NT 4.0 e Windows 10 build 19045.

A preparação da primeira distribuição pública estabilizada ainda está em andamento.

Por isso, algumas plataformas e formatos de pacote ainda aparecem como **pendentes de validação**, em vez de serem apresentados como suporte já garantido.

---

# Comece por aqui

- **Primeiros passos**
- **Providers e capacidades**
- **Compilação**
- **Segurança, certificados e trust**
- **Exemplos**
- **Como contribuir**
- **API pública**
- **SPI para criação de providers**
- **Plataformas validadas e limitações**

> Os links acima serão apontados para a estrutura definitiva dos documentos quando fecharmos a reorganização da documentação.

---

## Uma última observação de linguagem

Eu manteria **provider** como termo técnico do projeto, explicando na primeira ocorrência. “Provedor” em português pode soar como “provedor de Internet”; depois que a pessoa entende “provider = implementação que liga PST a NSS/Schannel/OpenSSL”, o termo fica natural.

E esta versão, para mim, finalmente junta as coisas que estavam aparecendo separadamente: **segurança básica → problema do desenvolvedor → desacoplamento → longevidade → TLS 1.2/1.3 real → providers → extensibilidade por colaboração → dimensão histórica/retrocomputação**.

Não mandaria isso ainda para o Codex. Primeiro fecharia essa apresentação com você; depois usamos **este texto como fonte canônica**, em vez de pedir que ele “interprete a essência do projeto” novamente.
