<!-- SPDX-License-Identifier: MPL-2.0 -->

# Começando com PST 0.5.0

Este guia usa a API pública 2.0.0. Escolha um SDK canônico na [Matriz de targets](../target-matrix.md) e trate `manifest.ini` e `consumer-link.ini` como contrato exato de build e implantação.

## Build e link

Adicione o diretório `include` do SDK e faça link com `lib/<target-id>/papinho_secure_transport.lib` e todas as bibliotecas de `consumer-link.ini`. Coloque as DLLs NSS/OpenSSL fornecidas pelo package ao lado do executável; não as instale em diretórios do sistema nem dependa de PATH global. Schannel usa bibliotecas do Windows e não inclui DLL TLS própria.

Builds de source usam `tools\build-vc6.bat` no target `win32-x86-vc6-retrozilla-nss` e os scripts modernos documentados para Schannel, OpenSSL e Combined. Identidade do toolchain não implica faixa de versões do sistema operacional.

## Configuração comum

1. Chame `pst_win32_register_builtin_providers()` uma vez durante setup serializado do processo.
2. Inicialize `PST_RUNTIME_OPTIONS` com `struct_size` e `PST_API_VERSION`; use `pst_runtime_create` ou sua variante de diagnóstico/logging.
3. Crie snapshots de credentials/trust com `pst_credentials_create` e `pst_trust_create`. Os buffers do chamador podem ser liberados após criação bem-sucedida.
4. Inicialize todos os campos aninhados de um `PST_CONNECTION_CONFIG` completo, incluindo tamanho e versão.
5. Chame `pst_connection_create`. A seleção ocorre transacionalmente aqui, não na criação do runtime.

Use `PST_CONNECTION_ROLE_CLIENT` em conexão de saída e `PST_CONNECTION_ROLE_SERVER` em conexão aceita. Veja [basic_client.c](../../examples/basic_client.c), [basic_server.c](../../examples/basic_server.c), [custom_trust.c](../../examples/custom_trust.c), [system_trust.c](../../examples/system_trust.c) e [mtls.c](../../examples/mtls.c).

## Transporte e ownership

A aplicação cria e conecta o socket CLIENT. Em SERVER, também cria o listener, executa bind/listen/accept e aplica sua política de admissão. PST recebe somente o socket CLIENT conectado ou SERVER aceito através de `pst_win32_socket_transport_create` e `pst_connection_attach`.

Antes de `ownership_accepted != 0`, o consumidor continua responsável por fechar/liberar após falha. Depois da aceitação, PST/provider é a única raiz de fechamento. O listener nunca é transferido ao PST.

## Identidade e trust

Local Identity é a cadeia de certificados e a chave privada PKCS#8 sem criptografia deste endpoint. Peer Authentication escolhe certificado DISABLED, OPTIONAL ou REQUIRED. Peer Trust é um conjunto explícito de âncoras CUSTOM ou o modo SYSTEM; PST nunca combina ou substitui os modos. Expected Peer Name é independente, pertence ao CLIENT e não se aplica ao SERVER.

Autenticação fornece fatos de identidade TLS, não autorização. A aplicação deve mapear certificado/Principal aceito para suas próprias permissões.

## TLS, ALPN e seleção

Defina mínimo/máximo TLS exatos. ALPN CLIENT é oferta ordenada. ALPN SERVER é preferência local: REQUIRED rejeita ausência de match, OPTIONAL permite ausência de protocolo negociado e DISABLED não envia/seleciona ALPN.

`PST_BACKEND_SELECTION_EXACT` tenta um ID. `ORDERED` copia e considera IDs na ordem informada. `AUTOMATIC` segue a ordem do target. Use `required_capabilities` explícito somente para requisitos não calculados pela configuração. Elegibilidade usa a máscara do role solicitado. Depois do binding, o provider fica fixado e nenhuma falha posterior causa fallback. Veja [provider_selection.c](../../examples/provider_selection.c) e [Providers](../providers.md).

## Operação incremental

Execute handshake/read/write/shutdown um passo por chamada. Em `NEED_READ`, `NEED_WRITE` ou `NEED_READ_WRITE`, consulte o interesse e use `pst_connection_wait` bounded ou o event loop da aplicação antes de tentar novamente. Respeite contagens parciais. Consulte Peer Info somente após estabelecimento.

Shutdown termina somente quando o provider retorna `PST_OPERATION_COMPLETE`; emitir close_notify local não basta. EOF/reset depois do estabelecimento sem close_notify do peer é `PST_RESULT_TRUNCATED`, mesmo se dados foram lidos antes. Libere connection, credentials, trust e runtime sempre pelas funções PST.

## Limites dos providers

OpenSSL implementa CLIENT/SERVER TLS 1.2/1.3 e ALPN SERVER completo. Schannel SERVER oferece TLS 1.2 no Windows 10 build 19045 validado e não anuncia ALPN SERVER completo nesse ambiente. NSS implementa CLIENT/SERVER TLS 1.2/1.3 e foi validado em NT4 SP6 x86 real, mas não anuncia SYSTEM_TRUST SERVER nem ALPN SERVER completo. Consulte sempre capabilities por role; não presuma simetria.

Continue em [API 2.0](../api-2.0.md), [SPI 3.0](../provider-spi-3.0.md), [migração](../api-1.3-to-2.0-migration.md), [segurança](../security-and-limitations.md) e [link do consumidor](../consumer-linking.md).
