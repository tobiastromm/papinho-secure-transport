<!-- SPDX-License-Identifier: MPL-2.0 -->

# PapinhoSecureTransport 0.5.0

PapinhoSecureTransport (PST) oferece às aplicações um contrato API 2.0 para conexões TLS CLIENT e SERVER, implementado pelos providers através da SPI 3.0. Ele separa os protocolos da aplicação das APIs nativas dos providers. PST não implementa HTTP, bind/listen/accept, admissão, sessões nem autorização da aplicação.

## Usando CLIENT e SERVER

Registre os providers incluídos no target, crie um runtime compartilhável e crie cada conexão com role explícito e configuração completa e imutável. Um runtime pode ser compartilhado entre conexões; isso não significa que um runtime ou uma conexão seja seguro para chamadas concorrentes em várias threads.

Em CLIENT, o consumidor cria e conecta o socket antes de entregá-lo ao PST. Em SERVER, o consumidor cria o listener, executa bind, listen e accept e decide a admissão; somente o socket conectado retornado por accept é entregue ao PST. Antes de `ownership_accepted`, o chamador fecha o transporte após falha. Depois da aceitação, o provider selecionado é a única raiz de fechamento.

Handshake, read, write, wait e shutdown são incrementais. `NEED_READ`, `NEED_WRITE` e `NEED_READ_WRITE` pedem nova tentativa bounded, não indicam conclusão. Fechamento gracioso exige close_notify recíproco. EOF/reset depois do estabelecimento sem close_notify do peer é `TRUNCATED`, inclusive quando dados autenticados foram entregues antes.

Veja os exemplos [CLIENT](../../examples/basic_client.c), [SERVER](../../examples/basic_server.c) e o [guia de integração](getting-started.md).

## Seleção e capabilities

A seleção é por conexão. EXACT nomeia um provider, ORDERED recebe uma lista de preferência copiada e AUTOMATIC segue a ordem de registro do target. Máscaras de capability por role filtram candidatos antes do binding do transporte. Conexões CLIENT e SERVER podem, portanto, selecionar providers diferentes. Depois do binding, o provider fica fixado; falha de handshake, autenticação, trust, ALPN, I/O, transporte ou shutdown nunca causa fallback.

A máscara agregada é somente a união para descoberta. Elegibilidade sempre usa a máscara do role solicitado. Combined é um target opcional de seleção Schannel/OpenSSL, não uma quarta implementação TLS.

## Identidade, autenticação e trust

- Local Identity é a cadeia de certificados e a chave privada que este endpoint apresenta.
- Peer Authentication controla certificado DISABLED, OPTIONAL ou REQUIRED.
- Peer Trust é exatamente CUSTOM ou SYSTEM; PST não une nem substitui os modos silenciosamente.
- Expected Peer Name é validação independente do nome do peer no lado CLIENT. Não se aplica a SERVER.
- Peer Info fornece fatos TLS/certificado copiados. Autenticado não significa autorizado, e certificado não é automaticamente um Principal da aplicação.

ALPN CLIENT é uma oferta ordenada. ALPN SERVER seleciona o primeiro item da preferência local também oferecido pelo cliente. REQUIRED rejeita ausência de interseção, OPTIONAL permite estabelecer sem seleção e DISABLED não negocia ALPN.

## Matriz de providers

| Provider | CLIENT | SERVER | TLS | Trust e observações SERVER |
|---|---:|---:|---|---|
| RetroZilla NSS/NSPR | sim | sim | 1.2/1.3 | CUSTOM; SYSTEM_TRUST SERVER e ALPN SERVER completo ausentes; NT4 SP6 x86 real validado |
| Schannel | sim | sim | CLIENT 1.2; SERVER 1.2 no Win10 19045 validado | CUSTOM/SYSTEM conforme máscara; TLS 1.3 SERVER e ALPN SERVER completo ausentes nesse ambiente; adaptação CurrentUser\\CA possui cleanup normal seguro e limitação documentada de resíduo após crash |
| OpenSSL 3.5.8 | sim | sim | 1.2/1.3 | CUSTOM e SYSTEM do Windows onde anunciado; ALPN SERVER completo |

Capabilities são factuais e separadas por role; não se presume simetria. Consulte [Providers](../providers.md), [API 2.0](../api-2.0.md), [SPI 3.0](../provider-spi-3.0.md), [Matriz de targets](../target-matrix.md), [Segurança](../security-and-limitations.md) e [Migração](../api-1.3-to-2.0-migration.md).

## Distribuição

Os SDKs estáticos 0.5.0 usam IDs canônicos para x86 VC6/NSS, x64 Schannel, x64 OpenSSL e x64 Combined. Cada manifesto informa toolchain, arquitetura, providers, capabilities, bibliotecas e runtimes adjacentes. O nome do toolchain não é uma identificação da versão do sistema operacional. Escolha o target cuja matriz factual e requisitos de implantação atendam à aplicação.

PST é software MPL-2.0. Componentes de terceiros preservam suas licenças e obrigações de código-fonte correspondente.
