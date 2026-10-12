# Model E consumer lifecycle contract

Status: normativo para consumers PST 0.6.5 API 3.0.0 / SPI 4.0 sob a
ADR-0009. A ADR-0008 permanece autoridade sobre o comportamento de release.

## Resultados factuais

| Disposition | Fato PST | Ação obrigatória do consumer |
| --- | --- | --- |
| `DESTROYED` | Destruição final confirmada; ponteiro inválido | Limpar a referência uma vez e finalizar dependências na ordem do owner |
| `PRECONDITION_RETAINED` | Close não tentado; mesma referência válida | Preservar owner/dependências e corrigir somente a precondição indicada |
| `CLOSE_UNRESOLVED_RETAINED` | Resolução segura não comprovada; referência somente para consulta | Entrar em contenção terminal e preservar dependências |

`RELEASE_ATTEMPT`, `RELEASE_RESULT`, `RELEASE_DISPOSITION`,
`RESOURCE_DESTRUCTION` e `OWNER_FINALIZATION` são fatos distintos. READY ou
PRE_CLOSE autorizam uma tentativa; não comprovam destruição. Somente OK junto
com DESTROYED permite descartar a referência.

## Invariantes universais

- um único owner durável permanece alcançável além do stack/thread iniciador;
- runtime/provider e credential/trust leases sobrevivem aos filhos retidos;
- identidade e geração acompanham o lifetime retido;
- finalização ocorre exatamente uma vez, depois da destruição factual;
- não há double release, resurrection, early reuse, fallback destrutivo ou
  native close pelo consumer;
- callbacks e gerações obsoletos não podem atingir um novo lifetime;
- encerramento do processo é contenção, não prova de close bem-sucedido.

`PRECONDITION_RETAINED` permite apenas a correção controlada pelo owner (por
exemplo, remover membership do wait-set) e nova tentativa governada.
`CLOSE_UNRESOLVED_RETAINED` permite apenas consulta de release info.

## Ownership no attach

O resultado da operação e `ownership_accepted` são independentes.

| Fronteira | Resultado | `ownership_accepted` | Owner após retorno | Ação do consumer |
| --- | --- | ---: | --- | --- |
| Antes de consumo irreversível | erro | 0 | consumer | Executar cleanup original do wrapper/recurso nativo |
| Depois do consumo, setup posterior falha | erro | 1 | PST/provider | Invalidar aliases; reter Connection; usar Model E |
| Attach completo | OK | 1 | PST/provider | Invalidar aliases e continuar lifecycle |

Um attach aceito que termina em erro impede operações normais, mas continua
sendo lifetime PST-owned que exige `pst_connection_try_release`.

## Owner do consumer

O owner durável registra no mínimo Connection, leases de runtime/provider e
credential/trust, membership do wait-set, role, identidade/operação/geração,
causa terminal, tentativa/resultado/disposition de release e bit de
finalização. A representação permanece específica de cada consumer; nenhuma
biblioteca compartilhada é exigida.

As regras usam transições compatíveis com C89 e são aplicáveis por design a
VC6, NT4, Windows 2000/XP e MSVC x64. Qualificação de plataforma permanece
factual e separada.
