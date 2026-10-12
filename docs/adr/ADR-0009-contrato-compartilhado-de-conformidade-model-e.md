---
title: "Contrato compartilhado de conformidade Model E para consumers"
status: accepted
date: 2026-10-11
decision-makers: [Tobias Tromm]
scope: project
---

# ADR-0009: Contrato compartilhado de conformidade Model E para consumers

## Status e autoridade

Aceita por decisão explícita do owner em 2026-10-11 para a candidata PST
0.6.5. Esta ADR complementa, sem alterar, a ADR-0008. Ela não muda API 3.0,
SPI 4.0, packages ou autorização de publicação.

## Decisão

`MODEL_E_CONSUMER_CONFORMANCE=SHARED_NORMATIVE_CONTRACT`.

A distribuição normativa é contrato mais suíte/matriz reutilizável de
conformidade. Não haverá biblioteca compartilhada de lifecycle. Browser e
Accelerator mantêm owners e políticas próprios, mas obedecem aos mesmos fatos
publicados pelo PST.

Somente `PST_RESULT_OK` junto com
`PST_RELEASE_DISPOSITION_DESTROYED` invalida a referência. Em
`PRECONDITION_RETAINED`, o mesmo owner e suas dependências permanecem vivos
para corrigir apenas a precondição governada. Em
`CLOSE_UNRESOLVED_RETAINED`, a referência permanece terminal e somente para
consulta; retry, I/O, rebind, fallback, troca de provider, resurrection e
reuso antecipado são proibidos.

No attach, `ownership_accepted` é a autoridade da transferência,
independentemente do resultado da operação. Se for zero, o consumer mantém o
ownership. Se for um, PST/provider mantém o ownership e o consumer invalida
seus aliases de wrapper/socket, inclusive quando o attach retorna erro.

## Responsabilidades e evidência

PST define as dispositions, preserva a autoridade exclusiva do close root e
publica os fatos de release. O consumer mantém owner durável, correlação de
identidade/geração, leases de runtime/provider/credencial/trust, ordenação de
wait-set e política de contenção. O consumer nunca substitui o close nativo.

A especificação normativa está em
[`../model-e-consumer-lifecycle-contract.md`](../model-e-consumer-lifecycle-contract.md)
e a matriz em
[`../model-e-consumer-conformance-matrix.md`](../model-e-consumer-conformance-matrix.md).
Evidência sintética, package normal, test-only, falha nativa real e plataforma
real permanecem classes distintas.

As transições são implementáveis em C89 e não exigem atomics modernos, thread
pool ou event loop específico. Compatibilidade NT4 exige execução real.
Consumers legados e seus pins não são alterados por esta decisão.

## Consequências

O Browser precisa implementar um owner durável antes de concluir a migração
API 3. O Accelerator pode preservar seu manager qualificado, mapeando a
evidência para a matriz compartilhada. Nenhum código funcional, API, ABI, SPI,
SDK, package, consumer ou wire é modificado por esta ADR.
