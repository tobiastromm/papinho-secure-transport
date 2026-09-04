# Começando com o PapinhoSecureTransport

> **Este é o guia prático do PapinhoSecureTransport.**
>
> Se esta é a primeira vez que você encontra o projeto, recomendamos começar pela
> [apresentação completa em Português (Brasil)](README.md), que explica o problema
> que o PST resolve, sua arquitetura e os conceitos principais.
>
> [← Apresentação em Português (Brasil)](README.md) ·
> [README principal do repositório](../../README.md)

Este guia acompanha você desde a preparação do ambiente até uma primeira conexão TLS utilizando apenas a API pública do PapinhoSecureTransport.

Ao final, você terá visto como:

- escolher o target adequado;
- compilar o PST;
- entender os arquivos gerados;
- integrar o PST ao seu programa;
- registrar os providers disponíveis;
- estabelecer uma conexão TLS;
- escolher entre confiança do sistema e uma CA própria;
- utilizar autenticação mútua (mTLS);
- selecionar providers;
- tratar diagnósticos e logging.

Você não precisa conhecer OpenSSL, Schannel ou NSS para começar. Quando algum conceito de TLS for necessário, ele será explicado no ponto em que aparecer.

---

# 1. O que você precisa

Antes de compilar o PST, é importante entender que **não existe um único build para todos os computadores**.

O projeto possui targets diferentes porque plataformas, arquiteturas, compiladores e implementações de segurança também são diferentes.

Você não precisa preparar todos os ambientes. **Escolha o target que pretende utilizar e instale apenas as ferramentas necessárias para ele.**

Atualmente, os ambientes validados pelo projeto são:

| Quero compilar para... | Provider | Ferramentas utilizadas pelo projeto |
|---|---|---|
| Windows NT 4.0 SP6 x86 | RetroZilla NSS | Visual C++ 6 SP5 + Processor Pack; `cl.exe` 12.00.8804; `link.exe` 6.00.8447 |
| Windows 10 build 19045 x64 | Schannel | Visual Studio Build Tools 2026 18.9.2; `cl.exe` 19.51.36256; NMAKE 14.51.36256.0; Windows SDK 10.0.26100.0 |
| Windows 10 build 19045 x64 | OpenSSL 3.5.8 | Visual Studio Build Tools 2026 18.9.2; `cl.exe` 19.51.36256; NMAKE 14.51.36256.0; Windows SDK 10.0.26100.0 |
| Windows 10 build 19045 x64 | Schannel + OpenSSL | Visual Studio Build Tools 2026 18.9.2; `cl.exe` 19.51.36256; NMAKE 14.51.36256.0; Windows SDK 10.0.26100.0 |

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
```

A partir dessas necessidades, você pode escolher um target que contenha um provider capaz de atendê-las.

## Windows NT 4.0 SP6 x86

Para o ambiente legado atualmente validado pelo projeto:

```text
Windows NT 4.0 SP6 x86
        │
        ▼
 RetroZilla NSS
        │
        ├── TLS 1.2
        └── TLS 1.3
```

Esse é o caminho atualmente utilizado e validado pelo PST para Windows NT 4.0.

Além de TLS 1.2 e TLS 1.3, esse provider oferece outras capacidades, como confiança customizada, validação de hostname, ALPN e autenticação mútua, apresentadas no README principal do idioma.

## Windows 10 build 19045 x64

Nesse ambiente, o projeto validou duas opções de provider.

### Schannel

Schannel utiliza a infraestrutura de segurança fornecida pelo próprio Windows.

```text
Windows 10 build 19045 x64
        │
        ▼
     Schannel
        │
        ├── TLS 1.2
        └── SYSTEM_TRUST
```

É uma opção especialmente interessante quando as capacidades necessárias à aplicação estão disponíveis no Schannel e você deseja utilizar a infraestrutura de segurança e confiança fornecida pelo próprio Windows.

No ambiente validado pelo projeto, TLS 1.3 não ficou disponível através desse provider. Isso não significa que Schannel seja limitado a TLS 1.2 em todas as versões do Windows.

### OpenSSL

O projeto também validou OpenSSL 3.5.8 nesse mesmo ambiente:

```text
Windows 10 build 19045 x64
        │
        ▼
 OpenSSL 3.5.8
        │
        ├── TLS 1.2
        ├── TLS 1.3
        └── SYSTEM_TRUST
```

Esse é atualmente o caminho validado pelo projeto quando uma aplicação no Windows 10 build 19045 precisa, por exemplo, combinar **TLS 1.3 com a confiança de certificados do Windows**.

### Schannel + OpenSSL

Também existe um target combinado:

```text
              Aplicação
                  │
                  │ capacidades necessárias
                  ▼
                 PST
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
    Schannel             OpenSSL
```

Nesse caso, mais de um provider fica disponível para o PST.

Por exemplo, no ambiente validado:

```text
TLS 1.2 + SYSTEM_TRUST
        │
        ▼
     Schannel
```

enquanto:

```text
TLS 1.3 + SYSTEM_TRUST
        │
        ▼
     OpenSSL
```

A aplicação continua utilizando a mesma API pública do PST. O que muda é o provider capaz de atender às capacidades solicitadas.

## E Windows 11 ou Windows Server?

Essas plataformas ainda precisam passar pela matriz formal de validação do projeto.

Por isso, este guia não vai assumir que determinada configuração é suportada apenas porque tecnicamente esperamos que funcione.

Hoje podemos afirmar:

```text
Windows NT 4.0 SP6 x86     → validado
Windows 10 build 19045 x64 → validado

Windows 11                 → validação pendente
Windows Server             → validação pendente
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

Os builds são mantidos separados. Isso evita, por exemplo, misturar em um mesmo diretório artefatos produzidos pelo Visual C++ 6 para x86 com arquivos produzidos pelo MSVC usado no target x64.

> Execute os comandos abaixo **a partir da raiz do repositório PapinhoSecureTransport**.

## Windows NT 4.0 x86 — RetroZilla NSS

O wrapper de build prepara o ambiente VC6 através de `tools\vc6-env.bat` e chama o `Makefile.vc6`.

Para limpar o target:

```bat
tools\build-vc6.bat clean
```

Para compilar e executar a suíte canônica:

```bat
tools\build-vc6.bat test
```

Os artefatos são produzidos em:

```text
build\
└── vc6\
```

A biblioteca PST desse target fica em:

```text
build\vc6\papinho_secure_transport.lib
```

As dependências RetroZilla NSS/NSPR utilizadas por esse caminho já estão preparadas e versionadas pelo projeto.

## Windows 10 x64 — Schannel

Para limpar:

```bat
tools\build-modern-msvc.bat clean
```

Para compilar e testar:

```bat
tools\build-modern-msvc.bat test
```

Os artefatos ficam em:

```text
build\
└── win64-modern-msvc\
```

A biblioteca PST fica em:

```text
build\win64-modern-msvc\papinho_secure_transport.lib
```

Como Schannel faz parte do Windows, esse target não precisa distribuir uma DLL Schannel fornecida pelo PST.

## Windows 10 x64 — OpenSSL 3.5.8

Para limpar:

```bat
tools\build-modern-msvc-openssl.bat clean
```

Para compilar e testar:

```bat
tools\build-modern-msvc-openssl.bat test
```

Os artefatos ficam em:

```text
build\
└── win64-modern-msvc-openssl\
```

A biblioteca PST fica em:

```text
build\win64-modern-msvc-openssl\papinho_secure_transport.lib
```

O próprio build copia para esse diretório as DLLs OpenSSL necessárias em runtime:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

## Windows 10 x64 — Schannel + OpenSSL

Para limpar:

```bat
tools\build-modern-msvc-combined.bat clean
```

Para compilar e executar o teste específico do target combinado:

```bat
tools\build-modern-msvc-combined.bat combined-test
```

Os artefatos ficam em:

```text
build\
└── win64-modern-msvc-combined\
```

A biblioteca PST fica em:

```text
build\win64-modern-msvc-combined\papinho_secure_transport.lib
```

Esse target também copia para o diretório de saída:

```text
libssl-3-x64.dll
libcrypto-3-x64.dll
```

O target Combined é especialmente útil para exercitar a seleção pública entre Schannel e OpenSSL.

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

Alguns testes de integração dependem de condições específicas, como acesso à rede ou fixtures próprias, e podem ser separados da suíte offline normal.

---

# 4. O que foi gerado

Depois de uma compilação bem-sucedida, cada target produz sua própria biblioteca PST:

```text
build\
├── vc6\
│   └── papinho_secure_transport.lib
│
├── win64-modern-msvc\
│   └── papinho_secure_transport.lib
│
├── win64-modern-msvc-openssl\
│   └── papinho_secure_transport.lib
│
└── win64-modern-msvc-combined\
    └── papinho_secure_transport.lib
```

Embora o nome seja o mesmo, **essas bibliotecas não são intercambiáveis**.

Cada uma foi construída para um target diferente e contém os providers correspondentes àquele target.

```text
papinho_secure_transport.lib
          │
          ├── target VC6
          │      └── RetroZilla NSS
          │
          ├── target Schannel
          │      └── Schannel
          │
          ├── target OpenSSL
          │      └── OpenSSL
          │
          └── target Combined
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

Essas DLLs precisam estar disponíveis durante a execução da aplicação que utiliza esse target.

### RetroZilla NSS/NSPR

O target legado utiliza o runtime NSS/NSPR preparado pelo projeto em:

```text
third_party\
└── retrozilla-nss\
    └── prebuilt\
        └── win32-x86-vc6\
            └── runtime\
```

Você não precisa reconstruí-lo para seguir este guia.

## Resumo

| Target | Biblioteca PST | Runtime adicional |
|---|---|---|
| RetroZilla NSS / x86 | `papinho_secure_transport.lib` | DLLs NSS/NSPR |
| Schannel / x64 | `papinho_secure_transport.lib` | componentes do próprio Windows |
| OpenSSL / x64 | `papinho_secure_transport.lib` | `libssl-3-x64.dll` + `libcrypto-3-x64.dll` |
| Combined / x64 | `papinho_secure_transport.lib` | OpenSSL DLLs + componentes do Windows |

---

# 5. Integre o PST ao seu projeto

Para utilizar PST, seu projeto precisa saber três coisas:

```text
onde estão os headers
        │
        ▼
onde está a .lib
        │
        ▼
quais DLLs precisam existir em runtime
```

A forma definitiva do pacote público do PST ainda será decidida antes da primeira distribuição estabilizada.

Durante o desenvolvimento, portanto, sua aplicação pode apontar diretamente para o build do target escolhido.

## Uma dependência organizada

Em um projeto consumidor, uma organização possível é:

```text
MeuProjeto\
│
├── src\
│
├── docs\
│
├── third_party\
│   └── pst\
│       ├── README.md
│       └── version.txt
│
└── build\
```

Essa pasta não precisa conter o código-fonte inteiro do PST.

Ela pode apenas registrar qual versão/target do PST o projeto utiliza e como o sistema de build localiza seu SDK.

Quando existir uma distribuição estabilizada, `version.txt` poderá registrar a versão exata utilizada.

## Inclua apenas a API pública

```c
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
```

Não inclua headers privados do PST ou headers de NSS, Schannel/OpenSSL apenas para operar a API pública.

## Registre os providers do target

```c
PST_RESULT result;

result = pst_win32_register_builtin_providers();

if (result != PST_RESULT_OK)
{
    /* tratar erro */
}
```

Essa chamada registra somente os providers compilados naquele target.

Os IDs públicos canônicos atualmente são:

```text
retrozilla-nss
schannel
openssl
```

Eles aparecem principalmente quando a aplicação utiliza seleção `EXACT`, `ORDERED` ou consulta qual backend foi selecionado.

---

# 6. Sua primeira conexão

Antes do TLS existir, sua aplicação precisa ter uma conexão de rede.

No Windows, isso normalmente significa criar um socket TCP e conectá-lo ao servidor desejado.

```text
sua aplicação
     │
     │ cria e conecta socket TCP
     ▼
socket conectado
     │
     │ entrega ao PST
     ▼
    PST
     │
     │ handshake TLS
     ▼
conexão segura
```

O PST não decide para qual servidor sua aplicação deve se conectar. DNS, endereço, porta e criação da conexão TCP continuam sob responsabilidade da aplicação.

A partir de um socket já conectado, a sequência PST é:

```text
1. registrar providers
        │
        ▼
2. criar runtime
        │
        ▼
3. criar trust
        │
        ▼
4. criar config
        │
        ▼
5. definir identidade/hostname
        │
        ▼
6. definir política TLS
        │
        ▼
7. congelar config
        │
        ▼
8. criar conexão
        │
        ▼
9. transformar socket em transporte PST
        │
        ▼
10. anexar transporte
        │
        ▼
11. executar handshake
        │
        ▼
12. read / write
        │
        ▼
13. shutdown
```

## O modelo é incremental

Handshake, leitura, escrita e shutdown não fazem esperas escondidas indefinidas.

Uma operação pode retornar estados como:

```text
PST_OPERATION_NEED_READ
PST_OPERATION_NEED_WRITE
PST_OPERATION_NEED_READ_WRITE
```

Isso significa que a operação ainda não terminou e precisa de progresso de rede.

Para esperar de maneira limitada:

```c
PST_WAIT_RESULT wait_result;

memset(&wait_result, 0, sizeof(wait_result));

result = pst_connection_wait(connection, 5000UL, &wait_result);

if (result != PST_RESULT_OK)
{
    /* erro de transporte/wait */
}

if (wait_result.timed_out)
{
    /* timeout definido pela aplicação */
}
```

## Helper de handshake limitado

```c
static PST_RESULT drive_handshake(pst_connection *connection)
{
    pst_u32 operation;
    PST_RESULT error;
    PST_RESULT result;
    unsigned int steps;

    for (steps = 0; steps < 100U; ++steps)
    {
        result = pst_connection_handshake(connection, &operation, &error);

        if (result != PST_RESULT_OK)
            return result;

        if (operation == PST_OPERATION_COMPLETE)
            return PST_RESULT_OK;

        if (operation == PST_OPERATION_FAILED)
            return error;

        {
            PST_WAIT_RESULT wait_result;

            memset(&wait_result, 0, sizeof(wait_result));

            result = pst_connection_wait(connection, 5000UL, &wait_result);

            if (result != PST_RESULT_OK)
                return result;

            if (wait_result.timed_out)
                return PST_RESULT_TRANSPORT_FAILURE;
        }
    }

    return PST_RESULT_RESOURCE_FAILURE;
}
```

O número de passos e o timeout pertencem à política da aplicação. Os valores acima são apenas exemplos limitados.

## Transformando o socket conectado em transporte PST

```c
pst_transport *transport = NULL;
pst_u32 ownership_accepted = 0;

result = pst_win32_socket_transport_create(
    (pst_size)native_socket,
    &transport);

if (result != PST_RESULT_OK)
{
    /* o socket ainda pertence à aplicação */
}
```

Depois:

```c
result = pst_connection_attach(
    connection,
    transport,
    PST_OWNERSHIP_TRANSFERRED,
    &ownership_accepted);
```

A regra de ownership é importante:

```text
ownership_accepted == 0
        │
        └── a aplicação continua responsável pelo transporte/socket

ownership_accepted == 1
        │
        └── PST assumiu ownership e realizará o fechamento correspondente
```

Se a chamada falhar e `ownership_accepted` continuar em zero, a aplicação deve liberar o transporte que ainda possui.

---

# 7. Exemplo com SYSTEM_TRUST

`SYSTEM_TRUST` significa usar a política de confiança fornecida pelo sistema operacional.

No Windows 10 validado pelo projeto, Schannel e OpenSSL oferecem essa capacidade.

## Crie a fonte de trust

```c
PST_TRUST_SOURCE trust_source;
pst_trust *trust = NULL;

memset(&trust_source, 0, sizeof(trust_source));

trust_source.struct_size = sizeof(trust_source);
trust_source.api_version = PST_API_VERSION;
trust_source.kind = PST_TRUST_SOURCE_SYSTEM;
trust_source.data = NULL;
trust_source.data_size = 0;

result = pst_trust_create(&trust_source, &trust);

if (result != PST_RESULT_OK)
{
    /* tratar erro */
}
```

## Peça as capacidades necessárias ao runtime

```c
PST_RUNTIME_OPTIONS options;
pst_runtime *runtime = NULL;

memset(&options, 0, sizeof(options));

options.struct_size = sizeof(options);
options.api_version = PST_API_VERSION;
options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
options.required_capabilities =
    PST_CAP_TLS_1_2 |
    PST_CAP_SYSTEM_TRUST |
    PST_CAP_HOSTNAME_VERIFY |
    PST_CAP_NONBLOCKING |
    PST_CAP_BACKEND_WAIT;

result = pst_runtime_create(&options, &runtime);
```

## Quais `required_capabilities` existem?

A API pública atual define:

| Capability | Significado |
|---|---|
| `PST_CAP_TLS_1_2` | provider suporta TLS 1.2 |
| `PST_CAP_TLS_1_3` | provider suporta TLS 1.3 |
| `PST_CAP_CLIENT_AUTH` | autenticação de cliente / mTLS |
| `PST_CAP_ALPN` | negociação ALPN |
| `PST_CAP_CUSTOM_TRUST` | CA/trust fornecida explicitamente pela aplicação |
| `PST_CAP_SYSTEM_TRUST` | política de confiança do sistema |
| `PST_CAP_HOSTNAME_VERIFY` | validação do hostname esperado |
| `PST_CAP_RESUMPTION` | session resumption |
| `PST_CAP_EARLY_DATA` | early data / 0-RTT |
| `PST_CAP_PEER_INFO` | informações normalizadas sobre o peer |
| `PST_CAP_NONBLOCKING` | operações incrementais/nonblocking |
| `PST_CAP_BACKEND_WAIT` | provider suporta o mecanismo PST de wait |

`PST_CAP_RESUMPTION` e `PST_CAP_EARLY_DATA` fazem parte do contrato público, mas **nenhum dos três providers atuais as anuncia**.

As máscaras atualmente validadas são:

| Provider | Capabilities |
|---|---:|
| RetroZilla NSS | `0x00000e5f` |
| Schannel | `0x00000e7d` |
| OpenSSL | `0x00000e7f` |

## Configure identidade e hostname

Trust e hostname respondem a perguntas diferentes.

```text
TRUST
"Eu confio na cadeia deste certificado?"

HOSTNAME
"Este certificado pertence ao servidor que eu queria acessar?"
```

Por isso os dois são configurados.

```c
PST_IDENTITY_CONFIG identity;

memset(&identity, 0, sizeof(identity));

identity.struct_size = sizeof(identity);
identity.api_version = PST_API_VERSION;
identity.credentials = NULL;
identity.trust = trust;
identity.expected_hostname = hostname;
identity.expected_hostname_size = strlen(hostname);
identity.require_peer_authentication = PST_REQUIREMENT_REQUIRED;
identity.require_client_authentication = PST_REQUIREMENT_DISABLED;
```

Depois:

```c
pst_config *config = NULL;

result = pst_config_create(&config);

if (result == PST_RESULT_OK)
    result = pst_config_set_identity(config, &identity);
```

## Defina a política TLS

Para TLS 1.2:

```c
PST_TLS_POLICY policy;

memset(&policy, 0, sizeof(policy));

policy.struct_size = sizeof(policy);
policy.api_version = PST_API_VERSION;
policy.minimum_version = PST_TLS_VERSION_1_2;
policy.maximum_version = PST_TLS_VERSION_1_2;
policy.alpn_protocols = NULL;
policy.alpn_protocol_count = 0;
policy.alpn_requirement = PST_FEATURE_DISABLED;
policy.resumption = PST_FEATURE_DISABLED;
policy.early_data = PST_FEATURE_DISABLED;
policy.require_graceful_shutdown = PST_REQUIREMENT_DISABLED;

result = pst_config_set_tls_policy(config, &policy);
```

Para exigir somente TLS 1.3:

```c
policy.minimum_version = PST_TLS_VERSION_1_3;
policy.maximum_version = PST_TLS_VERSION_1_3;
```

Depois congele:

```c
result = pst_config_freeze(config);
```

Um config congelado não deve mais ser modificado.

## Crie a conexão

```c
pst_connection *connection = NULL;

result = pst_connection_create(runtime, config, &connection);
```

A partir daí, anexe o transporte e conduza o handshake conforme mostrado na seção anterior.

---

# 8. Exemplo com CUSTOM_TRUST

`CUSTOM_TRUST` é útil quando a aplicação precisa confiar explicitamente em uma CA fornecida por ela mesma, por exemplo numa infraestrutura corporativa.

```text
       CA DA EMPRESA
             │
             ▼
    certificado do servidor
             │
             ▼
          cliente
```

## Crie trust a partir de DER

```c
PST_TRUST_SOURCE source;
pst_trust *trust = NULL;

memset(&source, 0, sizeof(source));

source.struct_size = sizeof(source);
source.api_version = PST_API_VERSION;
source.kind = PST_TRUST_SOURCE_CUSTOM_CA_DER;
source.data = ca_der;
source.data_size = ca_der_size;

result = pst_trust_create(&source, &trust);
```

DER é uma representação binária padronizada usada, entre outras coisas, para certificados.

Para este guia, basta pensar:

```text
certificado da CA
       │
       │ DER
       ▼
bytes fornecidos ao PST
```

A aplicação é responsável por obter esses bytes de uma fonte confiável.

## Exija a capability correspondente

```c
options.required_capabilities =
    PST_CAP_TLS_1_3 |
    PST_CAP_CUSTOM_TRUST |
    PST_CAP_HOSTNAME_VERIFY |
    PST_CAP_NONBLOCKING |
    PST_CAP_BACKEND_WAIT;
```

## CUSTOM_TRUST não cai silenciosamente para SYSTEM_TRUST

```text
CUSTOM_TRUST
      │
      ├── CA fornecida valida o servidor → continua
      │
      └── CA fornecida não valida         → falha
```

O PST não faz:

```text
CUSTOM falhou
      │
      ▼
"vamos tentar SYSTEM_TRUST"
```

Essa separação evita enfraquecer silenciosamente a política solicitada pela aplicação.

---

# 9. Exemplo com mTLS

No TLS mais comum, o cliente autentica o servidor.

No **mTLS — mutual TLS**, o servidor também exige que o cliente apresente sua própria identidade.

```text
        ambos autenticam
              │
      ┌───────┴───────┐
      ▼               ▼
   cliente         servidor
 certificado      certificado
```

No PST, a identidade do cliente pode ser fornecida como:

```text
certificado DER
      +
chave privada PKCS#8 DER
```

## Crie as credenciais

```c
PST_CREDENTIAL_SOURCE source;
pst_credentials *credentials = NULL;

memset(&source, 0, sizeof(source));

source.struct_size = sizeof(source);
source.api_version = PST_API_VERSION;
source.kind = PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;
source.certificate_der = certificate_der;
source.certificate_der_size = certificate_der_size;
source.private_key_der = private_key_der;
source.private_key_der_size = private_key_der_size;

result = pst_credentials_create(&source, &credentials);
```

## Exija client authentication

No runtime:

```c
options.required_capabilities =
    PST_CAP_TLS_1_3 |
    PST_CAP_CLIENT_AUTH |
    PST_CAP_CUSTOM_TRUST |
    PST_CAP_HOSTNAME_VERIFY |
    PST_CAP_NONBLOCKING |
    PST_CAP_BACKEND_WAIT;
```

Na identidade:

```c
identity.credentials = credentials;
identity.trust = trust;
identity.expected_hostname = hostname;
identity.expected_hostname_size = strlen(hostname);
identity.require_peer_authentication = PST_REQUIREMENT_REQUIRED;
identity.require_client_authentication = PST_REQUIREMENT_REQUIRED;
```

mTLS **não substitui** a autenticação do servidor.

Ainda temos duas perguntas:

```text
CLIENTE:
"Este é realmente o servidor esperado?"

SERVIDOR:
"Este é realmente um cliente autorizado?"
```

## Proteja a chave privada

A chave privada é material sensível.

Ela não deve ser colocada em logs, publicada no repositório ou embutida no código-fonte sem uma decisão explícita de segurança para o ambiente em questão.

---

# 10. Seleção de provider

A aplicação possui três modos públicos de seleção:

```text
PST_BACKEND_SELECTION_AUTOMATIC
PST_BACKEND_SELECTION_EXACT
PST_BACKEND_SELECTION_ORDERED
```

## AUTOMATIC

```c
memset(&options, 0, sizeof(options));

options.struct_size = sizeof(options);
options.api_version = PST_API_VERSION;
options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
options.required_capabilities =
    PST_CAP_TLS_1_3 |
    PST_CAP_SYSTEM_TRUST;
```

O PST percorre a ordem do target e escolhe o primeiro provider compatível.

No target Combined validado:

```text
ordem:
1. schannel
2. openssl
```

Para TLS 1.3 + SYSTEM_TRUST:

```text
schannel  ✗
openssl   ✓

→ openssl
```

## EXACT

Os IDs públicos atuais são:

```text
"retrozilla-nss"
"schannel"
"openssl"
```

Para exigir OpenSSL:

```c
memset(&options, 0, sizeof(options));

options.struct_size = sizeof(options);
options.api_version = PST_API_VERSION;
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "openssl";
options.required_capabilities =
    PST_CAP_TLS_1_3 |
    PST_CAP_SYSTEM_TRUST;
```

Se OpenSSL não estiver disponível naquele target ou não possuir as capabilities exigidas, a criação falha.

**EXACT não faz fallback.**

Exemplo com Schannel:

```c
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "schannel";
```

Exemplo com RetroZilla NSS:

```c
options.selection = PST_BACKEND_SELECTION_EXACT;
options.exact_backend_id = "retrozilla-nss";
```

## ORDERED

ORDERED permite à aplicação fornecer sua própria ordem.

```c
static const char *preferred[] =
{
    "openssl",
    "schannel"
};

memset(&options, 0, sizeof(options));

options.struct_size = sizeof(options);
options.api_version = PST_API_VERSION;
options.selection = PST_BACKEND_SELECTION_ORDERED;
options.preferred_backend_ids = preferred;
options.preferred_backend_count =
    sizeof(preferred) / sizeof(preferred[0]);
options.required_capabilities =
    PST_CAP_TLS_1_2 |
    PST_CAP_SYSTEM_TRUST;
```

Nesse exemplo, OpenSSL é avaliado antes de Schannel.

A seleção acontece na criação do runtime. Uma falha posterior de TLS, autenticação ou I/O **não provoca troca transparente de provider**.

## Descubra qual provider foi selecionado

```c
PST_RUNTIME_INFO info;

memset(&info, 0, sizeof(info));

info.struct_size = sizeof(info);
info.api_version = PST_API_VERSION;

result = pst_runtime_get_info(runtime, &info);

if (result == PST_RESULT_OK)
{
    printf("Provider selecionado: %s\n", info.backend_id);
}
```

---

# 11. Diagnóstico e logging

Quando uma operação falha, a aplicação recebe primeiro um `PST_RESULT`.

Alguns resultados públicos importantes incluem:

```text
PST_RESULT_OK
PST_RESULT_INVALID_ARGUMENT
PST_RESULT_INVALID_STATE
PST_RESULT_UNSUPPORTED
PST_RESULT_UNAVAILABLE
PST_RESULT_RESOURCE_FAILURE
PST_RESULT_TRANSPORT_FAILURE
PST_RESULT_PROTOCOL_FAILURE
PST_RESULT_AUTH_FAILURE
PST_RESULT_HOSTNAME_MISMATCH
PST_RESULT_POLICY_VIOLATION
PST_RESULT_BACKEND_FAILURE
PST_RESULT_TRUNCATED
PST_RESULT_CLOSED
```

Para obter uma descrição textual:

```c
printf("%s\n", pst_result_string(result));
```

A intenção é permitir que a aplicação trate categorias estáveis do PST sem depender diretamente dos códigos internos de NSS, Schannel ou OpenSSL.

## Diagnóstico copiado

```c
PST_DIAGNOSTIC_INFO diagnostic;

result = pst_diagnostic_info_init(&diagnostic);

if (result == PST_RESULT_OK)
{
    result = pst_connection_copy_diagnostic(
        connection,
        &diagnostic);
}
```

Quando `diagnostic.valid` for verdadeiro, o snapshot contém informações normalizadas, como:

- resultado;
- operação;
- backend ID.

O snapshot possui seus próprios dados e pode ser usado sem expor handles ou ponteiros nativos do provider.

Para erros durante a própria criação do runtime ou conexão, existem também:

```c
pst_runtime_create_ex(...)
pst_connection_create_ex(...)
```

## Níveis de logging

```text
PST_LOG_LEVEL_OFF
PST_LOG_LEVEL_ERROR
PST_LOG_LEVEL_WARN
PST_LOG_LEVEL_INFO
PST_LOG_LEVEL_DEBUG
PST_LOG_LEVEL_TRACE
```

## Callback

```c
static void PST_CALL on_log(
    void *user_context,
    const PST_LOG_EVENT *event)
{
    (void)user_context;

    printf(
        "level=%lu event=%lu result=%ld operation=%lu backend=%s\n",
        (unsigned long)event->level,
        (unsigned long)event->event_id,
        (long)event->normalized_result,
        (unsigned long)event->operation,
        event->backend_id);
}
```

## Configuração

```c
PST_LOG_CONFIG logging;

result = pst_log_config_init(&logging);

if (result == PST_RESULT_OK)
{
    logging.level = PST_LOG_LEVEL_INFO;
    logging.callback = on_log;
    logging.user_context = NULL;
}
```

Para criar um runtime com logging:

```c
PST_DIAGNOSTIC_INFO diagnostic;
pst_runtime *runtime = NULL;

pst_diagnostic_info_init(&diagnostic);

result = pst_runtime_create_with_logging(
    &options,
    &logging,
    &runtime,
    &diagnostic);
```

O callback é síncrono e o `PST_LOG_EVENT` recebido é efêmero. Se a aplicação quiser guardar dados do evento, deve copiar aquilo de que precisa durante a chamada.

O logging público foi desenhado para não expor automaticamente chaves privadas, payload da aplicação, conteúdo de certificados, handles, ponteiros ou códigos nativos arbitrários do provider.

---

# 12. Próximos passos

Se você chegou até aqui, já conhece o fluxo principal:

```text
escolher target
      │
      ▼
compilar PST
      │
      ▼
integrar headers + .lib + runtime
      │
      ▼
registrar providers
      │
      ▼
definir capabilities
      │
      ▼
configurar trust + hostname
      │
      ▼
anexar socket
      │
      ▼
handshake
      │
      ▼
read / write
      │
      ▼
shutdown
```

A partir daí, **o protocolo da sua aplicação volta a ser protagonista**.

PST não precisa saber se os bytes transportados representam:

```text
HTTP
SMTP
IMAP
mensagens de um ERP
mensageria
protocolo próprio
ou outro fluxo da aplicação
```

## Exemplos do repositório

A pasta:

```text
examples\
```

contém exemplos públicos destinados a mostrar casos específicos:

```text
basic_client.c
system_trust.c
custom_trust.c
mtls.c
provider_selection.c
diagnostics_logging.c
```

Eles incluem somente headers públicos do PST.

A ideia é que você possa estudar apenas o exemplo correspondente ao problema que deseja resolver.

## Fluxo mental para sua própria aplicação

```text
"Preciso proteger esta conexão."
          │
          ▼
"Qual versão TLS e qual trust eu preciso?"
          │
          ▼
"Quais capabilities representam isso?"
          │
          ▼
"Qual target contém provider compatível?"
          │
          ▼
"Crio runtime + config + conexão."
          │
          ▼
"Entrego meu socket conectado ao PST."
          │
          ▼
"Conduzo handshake/read/write/shutdown."
          │
          ▼
"Meu protocolo continua acima dessa camada."
```

Se você estiver tentando integrar o PST em um cenário não coberto por este guia, uma issue no repositório é um bom lugar para perguntar, documentar o caso de uso e, quando fizer sentido, transformar a resposta em nova documentação para a comunidade.

---

# Resumo rápido

```text
QUERO USAR PST
      │
      ▼
Escolho minha plataforma
      │
      ▼
Escolho um target
      │
      ▼
Compilo
      │
      ▼
Integro headers / .lib / runtime
      │
      ▼
Registro providers
      │
      ▼
Configuro segurança
      │
      ▼
Estabeleço TLS
      │
      ▼
Minha aplicação troca seus próprios dados
```

O objetivo do PST é justamente permitir que a implementação da camada de segurança possa evoluir sem obrigar o restante da aplicação a conhecer diretamente NSS, Schannel ou OpenSSL.
