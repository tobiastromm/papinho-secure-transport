# Model E consumer conformance matrix

Esta matriz é reutilizável por consumers. A presença de uma linha não declara
que seu teste foi executado.

| ID | Caso | Oracle obrigatório | Classe mínima |
| --- | --- | --- | --- |
| A1 | Attach bem-sucedido | accepted=1; aliases antigos invalidados | integração |
| A2 | Erro antes do consumo | accepted=0; somente consumer limpa | negativo/provider |
| A3 | Erro depois do consumo | accepted=1; consumer não fecha; release Model E | sintético e path real quando disponível |
| R1 | Close confirmado/sem recurso | OK + DESTROYED; finalização uma vez | integração |
| R2 | Precondição wait-set | INVALID_STATE + PRECONDITION_RETAINED; zero close | sintético/integração |
| R3 | Precondição corrigida | mesma identidade removida e então DESTROYED | integração |
| R4 | Close não resolvido | RESOURCE_FAILURE + retenção query-only | falha governada |
| O1 | Escopo iniciador retorna | owner e dependências continuam alcançáveis | integração consumer |
| O2 | Credential/runtime lease | leases sobrevivem ao filho retido | integração consumer |
| O3 | Callback/geração tardio | ação obsoleta rejeitada | negativo/integração |
| O4 | Stop/cancel repetido | nenhuma segunda release/finalização | integração |
| O5 | Múltiplas conexões | identidades e dispositions independentes | stress/integração |
| N1 | Double release | impossível após limpar referência DESTROYED | negativo |
| N2 | Resurrection | operações terminais rejeitadas | negativo |
| N3 | Early reuse | slot/token não reutilizado prematuramente | negativo |
| N4 | Native fallback | consumer não chama closesocket/PR_Close | audit/negativo |
| N5 | Process containment | separado de resource destruction | audit de evidência |

Cada evidência registra versão/API/SPI, hash do artefato normal ou test-only,
provider, role, plataforma/toolchain, operação/geração, resultado,
disposition, transição de owner e contagem de finalização. Evidência sintética,
integração normal, test-only, falha nativa real e execução real de plataforma
não se substituem.
