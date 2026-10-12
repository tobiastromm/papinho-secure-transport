# Accelerator handoff: conformidade Model E compartilhada

O Accelerator pode preservar seu owner-thread/Connection Manager qualificado;
o contrato não exige biblioteca compartilhada. A evidência ADR-0032/ADR-0033
deve ser mapeada à matriz por artefato PST exato, provider, identidade/geração
e classe de evidência. Evidência histórica API 2 não vira qualificação API 3.

Adicionar evidência A2/A3 explícita se ainda ausente. Em accepted+error,
invalidar ownership de wrapper/socket no Accelerator e preservar a Connection
no mesmo owner durável Model E. Manter ausência de fallback pós-binding,
autoridade exclusiva do close root, leases, rejeição de geração obsoleta e
finalização exactly-once.

Este handoff não altera Accelerator, PACC wire, pin de produção PST 0.6.4 nem
o status da obrigação externa de proveniência R7.
