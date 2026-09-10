<!-- SPDX-License-Identifier: MPL-2.0 -->

# Começando com o PapinhoSecureTransport

> **Este é o guia prático do PapinhoSecureTransport.**
>
> Se esta é a primeira vez que você encontra o projeto, recomendamos começar pela
> [apresentação completa em Português (Brasil)](README.md), que explica o problema
> que o PST resolve, sua arquitetura e os conceitos principais.
>
> [← Apresentação em Português (Brasil)](README.md) ·
> [README principal do repositório](../../README.md)

Este guia acompanha você desde a preparação do ambiente até suas primeiras conexões TLS **CLIENT e SERVER** utilizando apenas a API pública do PapinhoSecureTransport.

Ao final, você terá visto como:

- escolher o target adequado;
- compilar o PST;
- entender os arquivos gerados;
- integrar o PST ao seu programa;
- registrar os providers disponíveis;
- criar conexões com role CLIENT e SERVER;
- estabelecer uma conexão TLS;
- entender quem possui `bind`, `listen` e `accept` no lado SERVER;
- escolher entre confiança do sistema e uma CA própria;
- utilizar autenticação mútua (mTLS);
- selecionar providers por conexão;
- entender capabilities por role;
- tratar diagnósticos e logging;
- realizar shutdown TLS corretamente.

Você não precisa conhecer OpenSSL, Schannel ou NSS para começar. Quando algum conceito de TLS for necessário, ele será explicado no ponto em que aparecer.

---

# 1. O que você precisa

Antes de compilar o PST, é importante entender que **não existe um único build para todos os computadores**.

O projeto possui targets diferentes porque plataformas, arquiteturas, compiladores e implementações de segurança também são diferentes.

Você não precisa preparar todos os ambientes. **Escolha o target que pretende utilizar e instale apenas as ferramentas necessárias para ele.**

Os quatro targets publicados na 0.5.0 e mantidos na trilha de desenvolvimento 0.6.0 são:

| Target | Provider(s) | Arquitetura | Ambiente efetivamente validado |
|---|---|---:|---|
| `win32-x86-vc6-retrozilla-nss` | RetroZilla NSS/NSPR | x86 | Windows NT 4.0 SP6 x86 |
| `win32-x64-msvc-19.51-schannel` | Schannel | x64 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-openssl3` | OpenSSL 3.5.8 LTS | x64 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-schannel-openssl3` | Schannel + OpenSSL 3.5.8 | x64 | Windows 10 build 19045 x64 |

A identidade do target descreve fatos duráveis de build — plataforma/ABI, arquitetura, toolchain e provider. Ela **não é** uma declaração da versão do sistema operacional.

Por exemplo:

```text
win32-x86-vc6-retrozilla-nss
```

não significa “NT4-only”. Windows NT 4.0 aparece na documentação porque é um sistema no qual esse target foi efetivamente validado.

As dependências necessárias aos providers, como a versão utilizada de RetroZilla NSS/NSPR e o OpenSSL 3.5.8, já possuem versões preparadas e mantidas pelo projeto.

> **Este guia utiliza as dependências versionadas e preparadas pelo projeto.** Reconstruir OpenSSL, RetroZilla NSS/NSPR ou outras dependências a partir de seus respectivos códigos-fonte não é necessário para começar a usar o PST.

Assim, para seguir este guia, você precisa preparar **o compilador correspondente ao target escolhido**. Nas próximas etapas utilizaremos as dependências já fornecidas pelo projeto.

---

# 2. Escolha seu target

Se você ainda não sabe qual target utilizar, não comece escolhendo uma biblioteca.

Comece perguntando:

> **Em qual plataforma minha aplicação vai executar e de quais recursos de segurança ela precisa?**

Essa é justamente uma das ideias centrais do PST.

Em vez de a aplicação começar dizendo:

```text
"Quero OpenSSL."
```

ela pode começar pelas suas necessidades:

```text
"Preciso de TLS 1.3."
"Quero utilizar os certificados confiáveis do sistema."
"Preciso autenticar também o cliente."
"Preciso aceitar conexões como SERVER."
"Preciso de ALPN no lado SERVER."
```

A partir dessas necessidades, você pode escolher um target que contenha um provider capaz de atendê-las.

## `win32-x86-vc6-retrozilla-nss`

Esse é o target x86/VC6 com RetroZilla NSS/NSPR.

Na configuração efetivamente validada no Windows NT 4.0 SP6 x86, foram comprovados:

```text
CLIENT
  ├── TLS 1.2
  └── TLS 1.3

SERVER
  ├── TLS 1.2
  └── TLS 1.3
```

Também foram validados, conforme o role, CUSTOM_TRUST, Local Identity, autenticação de certificado de peer, mTLS, Peer Info, I/O incremental, shutdown recíproco e detecção de truncation.

No role SERVER atual:

```text
SYSTEM_TRUST       → não anunciado
ALPN SERVER completo → não anunciado
```

Isso descreve capacidades factuais do provider no contrato PST atual.

## `win32-x64-msvc-19.51-schannel`

Schannel utiliza a infraestrutura de segurança fornecida pelo próprio Windows.

No ambiente Windows 10 build 19045 x64 validado pelo projeto:

```text
CLIENT
  └── TLS 1.2

SERVER
  └── TLS 1.2
```

SYSTEM_TRUST e CUSTOM_TRUST estão disponíveis conforme as capability masks publicadas para cada role.

TLS 1.3 SERVER e ALPN SERVER com a semântica completa do PST não são anunciados nesse ambiente.

Isso **não** significa que Schannel seja universalmente limitado a TLS 1.2. É uma afirmação sobre o ambiente e o comportamento efetivamente validados pelo PST.

## `win32-x64-msvc-19.51-openssl3`

O target OpenSSL usa **OpenSSL 3.5.8 LTS**.

Foram validados:

```text
CLIENT
  ├── TLS 1.2
  └── TLS 1.3

SERVER
  ├── TLS 1.2
  └── TLS 1.3
```

O provider também oferece, onde anunciado, CUSTOM_TRUST, SYSTEM_TRUST do Windows, ALPN, mTLS, Peer Info, operações nonblocking, shutdown recíproco e truncation.

## `win32-x64-msvc-19.51-schannel-openssl3`

Também existe um target combinado:

```text
              Aplicação
                  │
        configuração da conexão
                  │
                  ▼
                 PST
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
    Schannel             OpenSSL
```

Esse target é útil quando a aplicação quer que o PST selecione entre mais de um provider.

A seleção acontece **por conexão**.

Exemplo SERVER:

```text
TLS 1.2
```

pode deixar Schannel e OpenSSL elegíveis.

Já:

```text
TLS 1.2 + ALPN SERVER
```

elimina Schannel antes do binding no ambiente validado, porque Schannel SERVER não anuncia ALPN SERVER com a semântica completa do PST. OpenSSL pode então ser selecionado.

O Combined é um package oficial opcional. Ele não é uma quarta implementação TLS e não é a recomendação padrão para todo usuário.

## E Windows 11 ou Windows Server?

Essas plataformas ainda precisam passar pela matriz formal de validação do projeto.

Por isso, este guia não vai assumir que determinada configuração é suportada apenas porque tecnicamente esperamos que funcione.

Hoje podemos afirmar:

```text
Windows NT 4.0 SP6 x86     → validado para o target NSS/VC6
Windows 10 build 19045 x64 → validado para os targets x64 atuais

Windows 11                 → ainda não validado formalmente nesta versão
Windows Server             → ainda não validado formalmente nesta versão
```

À medida que novas plataformas forem efetivamente testadas, esta documentação poderá ser ampliada.

---

# 3. Compile o PST

Agora que você escolheu um target, vamos transformar o código-fonte do PST na biblioteca que sua aplicação poderá utilizar.

O processo é, conceitualmente, o mesmo para todos os targets:

```text
repositório do PST
        │
        │ escolhe o target
        ▼
script / sistema de build
        │
        │ compila PST + provider daquele target
        ▼
biblioteca do PST
        │
        └── dependências de runtime, quando necessárias
```

Os builds são mantidos separados. Isso evita misturar artefatos incompatíveis produzidos por arquiteturas e toolchains diferentes.

> Execute os comandos abaixo **a partir da raiz do repositório PapinhoSecureTransport**.

## x86 / VC6 / RetroZilla NSS

O wrapper de build prepara o ambiente VC6 através de `tools\vc6-env.bat` e chama o `Makefile.vc6`.

Para limpar:

```bat
tools\build-vc6.bat clean
```

Para compilar e executar a suíte canônica:

```bat
tools\build-vc6.bat test
```

A biblioteca PST desse target fica em:

```text
build\win32-x86-vc6-retrozilla-nss\papinho_secure_transport.lib
```

As dependências RetroZilla NSS/NSPR desse caminho já estão preparadas e versionadas pelo projeto.

## x64 / Schannel

Para limpar:

```bat
tools\build-win32-x64-msvc-19.51-schannel.bat clean
```

Para compilar e testar:

```bat
tools\build-win32-x64-msvc-19.51-schannel.bat test
```

A biblioteca fica em:

```text
build\win32-x64-msvc-19.51-schannel\papinho_secure_transport.lib
```

Como Schannel faz parte do Windows, esse target não distribui uma DLL Schannel fornecida pelo PST.

## x64 / OpenSSL 3.5.8

Para limpar:

```bat
tools\build-win32-x64-msvc-19.51-openssl3.bat clean
```

Para compilar e testar:

```bat
tools\build-win32-x64-msvc-19.51-openssl3.bat test
```

A biblioteca fica em:

```text
build\win32-x64-msvc-19.51-openssl3\papinho_secure_transport.lib
```

O build também copia para o diretório de saída as DLLs OpenSSL necessárias em runtime:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

## x64 / Combined Schannel + OpenSSL

Para limpar:

```bat
tools\build-win32-x64-msvc-19.51-schannel-openssl3.bat clean
```

Para compilar e executar o teste específico do target combinado:

```bat
tools\build-win32-x64-msvc-19.51-schannel-openssl3.bat combined-test
```

A biblioteca fica em:

```text
build\win32-x64-msvc-19.51-schannel-openssl3\papinho_secure_transport.lib
```

Esse target também utiliza:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

para o provider OpenSSL.

## Preciso executar os testes sempre?

Durante o desenvolvimento do PST, usamos os targets de teste para confirmar que o build continua saudável.

Para alguém começando com o projeto, executar os testes na primeira compilação é uma boa forma de verificar que:

```text
toolchain
   +
dependências
   +
PST
   +
provider
   │
   ▼
ambiente funcionando
```

Alguns testes de integração dependem de condições específicas, como rede, fixtures ou execução em outro sistema, e podem ficar separados da suíte offline normal.

---

# 4. O que foi gerado

Depois de uma compilação bem-sucedida, cada target produz sua própria biblioteca PST:

```text
build\
├── win32-x86-vc6-retrozilla-nss\
│   └── papinho_secure_transport.lib
│
├── win32-x64-msvc-19.51-schannel\
│   └── papinho_secure_transport.lib
│
├── win32-x64-msvc-19.51-openssl3\
│   └── papinho_secure_transport.lib
│
└── win32-x64-msvc-19.51-schannel-openssl3\
    └── papinho_secure_transport.lib
```

Embora o nome da `.lib` seja o mesmo, **essas bibliotecas não são intercambiáveis**.

Cada uma foi construída para um target diferente e contém os providers correspondentes àquele target.

```text
papinho_secure_transport.lib
          │
          ├── target NSS / x86 / VC6
          │      └── RetroZilla NSS
          │
          ├── target Schannel / x64
          │      └── Schannel
          │
          ├── target OpenSSL / x64
          │      └── OpenSSL
          │
          └── target Combined / x64
                 ├── Schannel
                 └── OpenSSL
```

## Headers públicos

Os headers públicos ficam em:

```text
include\
├── papinho_secure_transport.h
└── papinho_secure_transport_win32.h
```

Uma aplicação Win32 normalmente começa com:

```c
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
```

Sua aplicação deve utilizar **somente os headers públicos em `include\`**.

Ela não deve precisar incluir arquivos de:

```text
src\
src\backends\
third_party\
```

para utilizar o PST.

## Para que serve a `.lib`?

Durante a compilação da sua aplicação:

```text
seu_programa.c
      │
      │ compilação
      ▼
seu_programa.obj
      │
      ├──── papinho_secure_transport.lib
      │
      ▼
    linker
      │
      ▼
seu_programa.exe
```

A `.lib` é utilizada durante a **linkedição** do programa.

Ela não é, por si só, um arquivo que o usuário precisa colocar ao lado do `.exe`.

## E as DLLs?

Isso depende do provider.

### Schannel

Schannel é fornecido pelo próprio Windows:

```text
sua_aplicação.exe
        │
        ▼
       PST
        │
        ▼
 Schannel do Windows
```

### OpenSSL

O target OpenSSL atualmente validado utiliza:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

Essas DLLs precisam estar disponíveis durante a execução da aplicação que utiliza esse provider.

### RetroZilla NSS/NSPR

O target NSS utiliza o runtime NSS/NSPR preparado pelo projeto. Você não precisa reconstruí-lo para seguir este guia.

---

# 5. A API pública: o modelo mental da versão 2.1

A API 2.1 preserva os conceitos da API 2.0 e acrescenta readiness multiplexada sem entregar o event loop ao PST.

```text
providers registrados
        │
        ▼
     runtime
        │
        ├── conexão CLIENT
        ├── conexão SERVER
        └── outras conexões
```

O runtime funciona como contexto/registro compartilhável entre conexões.

Isso **não** significa que runtime ou connection sejam automaticamente thread-safe para chamadas concorrentes em várias threads.

Cada conexão recebe sua própria configuração completa.

Entre os elementos principais estão:

- role CLIENT ou SERVER;
- política TLS;
- política ALPN;
- Local Identity;
- Peer Authentication;
- Peer Trust;
- Expected Peer Name, quando CLIENT;
- política de seleção do provider.

A configuração é copiada/snapshotada pelo PST; a aplicação não deve depender de mutá-la depois como mecanismo para alterar uma conexão já criada.

---

# 6. Registrando os providers

No Win32, a aplicação registra explicitamente os providers que foram compilados naquele target através do bootstrap público do PST.

Conceitualmente:

```c
pst_win32_register_builtin_providers();
```

A aplicação não precisa incluir headers privados de NSS, Schannel ou OpenSSL.

O PST também não sai procurando bibliotecas aleatórias instaladas na máquina.

O target define quais providers estão disponíveis:

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

Depois do bootstrap, crie o runtime e então as conexões.

---

# 7. Sua primeira conexão CLIENT

No role CLIENT, a aplicação continua responsável por criar e conectar o transporte TCP.

O fluxo geral é:

```text
socket()
   │
connect()
   │
socket conectado
   │
   ▼
PST connection (CLIENT)
   │
attach transport
   │
handshake TLS
   │
read / write
   │
shutdown TLS
```

O PST não faz DNS, HTTP, SMTP, IMAP ou qualquer protocolo da aplicação por você.

Um exemplo mínimo real está em:

```text
examples\basic_client.c
```

A ideia geral é:

1. registrar os providers;
2. criar o runtime;
3. criar uma configuração com `role=CLIENT`;
4. definir política TLS/trust/hostname;
5. criar a conexão;
6. entregar o socket já conectado;
7. avançar o handshake incrementalmente;
8. trocar dados;
9. consultar informações negociadas se necessário;
10. realizar shutdown recíproco;
11. liberar conexão e runtime.

---

# 8. Sua primeira conexão SERVER

No role SERVER, existe uma fronteira importante:

> **PST não cria o listener e não executa `bind`, `listen` ou `accept`.**

Essas operações pertencem à aplicação.

O fluxo é:

```text
socket()
   │
bind()
   │
listen()
   │
accept()
   │
socket conectado
   │
   ▼
PST connection (SERVER)
   │
attach transport
   │
handshake TLS
   │
read / write
   │
shutdown TLS
```

Um exemplo mínimo real está em:

```text
examples\basic_server.c
```

A aplicação decide:

- em qual interface/porta ouvir;
- quantos listeners criar;
- quais conexões admitir;
- como organizar threads/event loop/processos;
- qual protocolo de aplicação existe acima do TLS;
- como autorizar usuários ou sessões.

O PST começa **depois que já existe um transporte conectado**.

## Local Identity no SERVER

Um servidor TLS normalmente precisa apresentar um certificado e uma chave privada.

No PST, isso é **Local Identity**.

Ela é separada de:

- Peer Authentication;
- Peer Trust;
- autorização da aplicação.

Se o servidor apenas apresenta seu próprio certificado e não exige certificado de cliente:

```text
Local Identity      → configurada
Peer Authentication → DISABLED
```

Se deseja mTLS:

```text
Local Identity      → certificado/chave do servidor
Peer Authentication → REQUIRED
Peer Trust          → CUSTOM ou SYSTEM, conforme provider/capability
```

---

# 9. Local Identity, Peer Authentication, Peer Trust e Expected Peer Name

Esses conceitos são independentes.

## Local Identity

É aquilo que **este endpoint apresenta**:

```text
certificado/cadeia
+
chave privada
```

Pode existir em CLIENT, em SERVER ou nos dois, dependendo do uso.

## Peer Authentication

Controla se o certificado do outro lado é:

```text
DISABLED
OPTIONAL
REQUIRED
```

## Peer Trust

Quando o certificado do peer precisa ser validado, o trust é configurado como:

```text
CUSTOM
```

ou:

```text
SYSTEM
```

O PST não faz união silenciosa entre os dois e não troca automaticamente um pelo outro.

## Expected Peer Name

É a verificação independente do nome esperado do peer no role CLIENT.

Exemplo:

```text
servidor apresenta certificado
        │
        ├── cadeia confiável?  → Peer Trust
        └── é para example.com? → Expected Peer Name
```

No role SERVER, Expected Peer Name não se aplica.

## Autenticado não significa autorizado

Mesmo que o TLS confirme que o peer apresentou um certificado válido, a aplicação ainda precisa decidir o que aquele peer pode fazer.

```text
TLS:
"certificado válido e autenticado"

Aplicação:
"esse certificado corresponde a qual conta?"
"essa conta pode executar esta operação?"
```

PST não transforma certificado automaticamente em usuário, conta ou Principal da aplicação.

---

# 10. SYSTEM_TRUST e CUSTOM_TRUST

## SYSTEM_TRUST

Usa a política/lista de confiança do sistema operacional quando o provider e o role anunciam essa capability.

É útil quando você quer integrar a aplicação ao trust administrado pelo Windows.

## CUSTOM_TRUST

A aplicação fornece explicitamente sua própria CA ou conjunto de CAs.

Exemplo:

```text
CA DA EMPRESA
     │
     ├── servidor interno
     └── cliente corporativo
```

Isso é especialmente útil em redes privadas, laboratórios e PKIs empresariais.

Nem todos os providers oferecem SYSTEM_TRUST em todos os roles.

Por exemplo, no PST 0.5.0:

```text
RetroZilla NSS SERVER SYSTEM_TRUST → não anunciado
```

Sempre consulte a capability mask ou a [matriz de targets](../target-matrix.md).

---

# 11. mTLS

TLS pode autenticar apenas o servidor:

```text
CLIENT
   │
   │ verifica servidor
   ▼
SERVER
```

ou os dois lados:

```text
CLIENT ⇄ SERVER
 certificado  certificado
```

Esse segundo caso é normalmente chamado de **mTLS — mutual TLS**.

No PST, mTLS emerge da configuração separada de identidade e autenticação.

No SERVER:

```text
Local Identity      → identidade do servidor
Peer Authentication → REQUIRED
Peer Trust          → trust usado para validar o cliente
```

No CLIENT:

```text
Local Identity      → identidade do cliente
Peer Authentication → REQUIRED
Peer Trust          → trust usado para validar o servidor
Expected Peer Name  → nome esperado do servidor
```

A autenticação do certificado de cliente no SERVER usa a finalidade apropriada de `clientAuth`.

---

# 12. Selecionando providers

A API 2.x mantém a seleção **por conexão**.

Existem três modos.

## EXACT

Você solicita um provider específico.

```text
EXACT = openssl
```

Se ele não for elegível para aquela configuração:

```text
falha
```

PST não troca silenciosamente para outro.

## ORDERED

Você fornece sua própria lista:

```text
1. openssl
2. schannel
```

PST considera os providers nessa ordem e pode pular um candidato **antes do binding** quando ele não é elegível para o role/capabilities solicitados.

## AUTOMATIC

PST usa a ordem de registro do target.

No Combined atual:

```text
1. Schannel
2. OpenSSL
```

e seleciona o primeiro provider elegível.

## Capabilities por role

Um provider pode oferecer determinada capability em CLIENT e não em SERVER.

Por isso o PST mantém masks separadas por role.

Exemplo:

```text
SERVER + TLS 1.2 + ALPN SERVER
```

No Combined validado:

```text
Schannel → inelegível antes do binding
OpenSSL  → elegível
```

## Não existe fallback depois do binding

Esse ponto é importante.

Depois que o provider aceita o binding/ownership do transporte, ele fica fixado.

Se ocorrer depois:

- erro de handshake;
- certificado inválido;
- trust failure;
- ALPN failure;
- erro de I/O;
- truncation;
- falha de transporte;
- erro de shutdown;

PST **não tenta outro provider**.

Isso evita transformar uma falha de segurança ou protocolo em downgrade implícito.

---

# 13. ALPN

ALPN permite negociar um protocolo de aplicação durante o handshake TLS.

Exemplo típico:

```text
CLIENT oferece:
h2
http/1.1
```

No role CLIENT, a lista representa uma **oferta ordenada**.

No role SERVER, a lista representa a **preferência local do servidor**.

O servidor seleciona o primeiro item de sua preferência que também foi oferecido pelo cliente.

Os modos públicos são:

```text
REQUIRED
OPTIONAL
DISABLED
```

Nem todos os providers oferecem ALPN completo nos dois roles.

No PST 0.5.0:

```text
OpenSSL SERVER ALPN → suportado
Schannel SERVER ALPN completo → não anunciado
RetroZilla NSS SERVER ALPN completo → não anunciado
```

---

# 14. Handshake incremental e readiness

O PST foi projetado para operação incremental/nonblocking.

Uma chamada de handshake pode dizer, conceitualmente:

```text
NEED_READ
NEED_WRITE
NEED_READ_WRITE
```

Isso não significa falha e também não significa conclusão.

Significa que a operação precisa ser tentada novamente quando houver a condição apropriada.

O backend pode ter necessidades de readiness próprias. Por isso, a aplicação não deve assumir que observar diretamente o socket nativo é sempre suficiente para representar o interesse TLS interno de todos os providers.

Use as operações públicas de interesse/wait do PST.

---

# 15. Wait-set, fontes externas, wake e backpressure

A API 2.1 oferece um **wait-set portátil** para observar várias conexões usando tokens definidos pelo consumidor. No Win32, `pst_external_source` permite incluir uma fonte nativa emprestada, como um listener, sem transferir seu ownership.

`timeout=0` faz poll imediato; timeout positivo é uma espera máxima do scheduler. `wake` pode ser chamado por outra thread e é diferente de timeout e cancelamento. A aplicação continua dona de seus deadlines.

Read/write podem fazer progresso parcial. Se `pst_write` aceitar apenas parte do buffer, a aplicação mantém o sufixo ainda não enviado e o retoma quando houver readiness. PST não implementa `send-all` implícito e não drena uma conexão indefinidamente.

---

# 15. Read e write

Depois do handshake:

```text
aplicação
   │
pst_write
   │
 TLS
   │
rede
```

e:

```text
rede
   │
 TLS
   │
pst_read
   │
aplicação
```

PST protege bytes.

Ele não interpreta:

- HTTP;
- SMTP;
- IMAP;
- JSON;
- protocolo empresarial;
- mensagens da aplicação.

Esse framing continua responsabilidade da aplicação.

---

# 16. Shutdown e truncation

Encerrar o TCP não é a mesma coisa que encerrar TLS corretamente.

TLS possui o alerta:

```text
close_notify
```

Para shutdown gracioso, o PST espera encerramento TLS recíproco.

```text
lado A ── close_notify ──► lado B
lado A ◄─ close_notify ─── lado B
```

Emitir apenas o seu próprio `close_notify` não significa que o shutdown recíproco terminou.

Se uma conexão TLS já estabelecida recebe EOF/reset sem `close_notify` do peer, o PST classifica isso como:

```text
TRUNCATED
```

inclusive quando dados autenticados foram entregues antes do EOF.

Isso permite que a aplicação diferencie um fechamento TLS limpo de um encerramento abrupto do transporte.

---

# 17. Peer Info

Depois de uma conexão estabelecida, a aplicação pode consultar fatos normalizados sobre o peer e a sessão.

Dependendo do provider/capabilities, isso pode incluir:

- versão TLS;
- cipher;
- presença de certificado;
- cadeia;
- estado de autenticação;
- hash/fingerprint;
- certificado leaf copiado;
- ALPN negociado.

Essas informações são fatos da camada TLS.

A decisão de associá-las a uma identidade da aplicação continua fora do PST.

---

# 18. Diagnósticos e logging

Quando algo falha, saber apenas:

```text
"erro TLS"
```

não é suficiente.

PST oferece diagnósticos normalizados e logging para ajudar a distinguir causas como:

- certificado ausente;
- certificado inválido;
- trust failure;
- Expected Peer Name;
- capability ausente;
- provider não elegível;
- falha de handshake;
- truncation;
- erro de transporte.

O logging não deve expor material sensível, payload da aplicação ou chave privada.

Os testes de release incluem gates explícitos para evitar esse tipo de vazamento.

---

# 19. Ownership do transporte

Uma regra importante é saber quem fecha o socket.

Antes de PST aceitar ownership do transporte:

```text
falha
  │
  ▼
chamador ainda é responsável por fechar
```

Depois de ownership aceito:

```text
PST/provider
     │
     └── única raiz de fechamento
```

Isso evita double-close e ambiguidades de lifecycle.

No SERVER, essa regra vale para o **socket conectado retornado por `accept`**, não para o listener. O listener continua pertencendo à aplicação.

---

# 20. TLS depois de plaintext: STARTTLS e CONNECT

PST pode receber o **mesmo transporte já conectado** depois de a aplicação usá-lo em plaintext e alcançar uma boundary limpa de upgrade.

```text
SMTP/IMAP-like: plaintext -> STARTTLS aceito -> attach PST -> TLS
proxy-like:     plaintext -> CONNECT aceito  -> attach PST -> TLS
```

A aplicação continua responsável por SMTP, IMAP, HTTP e pela detecção da boundary. PST não reconecta e não faz rollback para plaintext depois de aceitar ownership. Bytes TLS pré-lidos antes do attach não são suportados nesta versão.

---

# 20. Exemplos públicos

O repositório inclui exemplos mínimos destinados a mostrar o contrato público sem acoplamento às APIs privadas dos providers.

Comece por:

```text
examples\basic_client.c
examples\basic_server.c
```

Depois consulte os exemplos de:

- trust;
- mTLS;
- seleção de provider;
- logging;
- diagnostics.

O objetivo desses exemplos é mostrar o PST, não ensinar toda a API nativa de NSS, Schannel ou OpenSSL.

---

# 21. Usando um SDK em vez de compilar o repositório

A versão 0.5.0 é distribuída com SDKs específicos por target.

Um SDK contém, conforme aplicável:

- headers públicos;
- `papinho_secure_transport.lib`;
- manifest;
- README/documentação;
- bibliotecas de link necessárias;
- runtime DLLs necessárias;
- notices/licenças;
- material de corresponding source.

Escolha o SDK cujo Target ID corresponde ao artefato que deseja integrar.

Os packages oficiais são:

```text
win32-x86-vc6-retrozilla-nss
win32-x64-msvc-19.51-schannel
win32-x64-msvc-19.51-openssl3
win32-x64-msvc-19.51-schannel-openssl3
```

O target Combined é opcional.

---

# 22. Qual caminho escolher?

Uma forma prática de pensar:

```text
Precisa rodar na configuração NT4/x86 validada?
        │
        └── win32-x86-vc6-retrozilla-nss

Windows x64 + quer caminho TLS nativo do Windows validado?
        │
        └── win32-x64-msvc-19.51-schannel

Windows x64 + precisa TLS 1.3 / OpenSSL 3?
        │
        └── win32-x64-msvc-19.51-openssl3

Windows x64 + precisa escolher Schannel/OpenSSL por conexão?
        │
        └── win32-x64-msvc-19.51-schannel-openssl3
```

Mas lembre:

> Target, provider e sistema operacional testado são fatos diferentes.

Consulte sempre a [Target Matrix](../target-matrix.md).

---

# 23. Onde continuar

Depois deste guia, os documentos mais importantes são:

- [Apresentação completa em Português](README.md)
- [Target Matrix](../target-matrix.md)
- [API 2.0](../api-2.0.md)
- [SPI 3.0](../provider-spi-3.0.md)
- [Migração API 1.3/SPI 2.4 → API 2.0/SPI 3.0](../api-1.3-to-2.0-migration.md)
- [Segurança e limitações](../security-and-limitations.md)
- [Packaging de release](../release-packaging.md)

Se você estiver integrando PST a uma aplicação real, uma boa ordem é:

```text
README do idioma
      │
      ▼
getting-started
      │
      ▼
basic_client.c / basic_server.c
      │
      ▼
API 2.0
      │
      ▼
security-and-limitations
```

A ideia continua sendo a mesma desde o início do projeto:

> **a aplicação diz de quais propriedades de transporte seguro precisa; o PST mantém os detalhes específicos dos providers atrás de uma fronteira comum.**
