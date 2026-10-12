# Browser handoff: owner durável Model E

Autoridade: PST ADR-0008, ADR-0009, especificação Model E e auditoria de
ownership de attach deste repositório.

## Implementação requerida

O Browser deve criar um owner durável capaz de sobreviver ao retorno do
stack/worker iniciador. Ele retém Connection, leases de runtime/provider e
credential/trust, membership do wait-set, role, identidade/geração, causa
terminal, release disposition e bit de finalização.

Antes das demais mudanças API 3, o cleanup de attach deve consultar
`ownership_accepted`, não apenas `PST_RESULT_OK`:

- accepted=0: Browser mantém wrapper/socket e executa o cleanup original;
- accepted=1: Browser invalida imediatamente seus aliases de ownership;
- accepted=1 com erro: Connection permanece no owner durável e segue Model E;
- Browser nunca fecha o socket nem libera o wrapper já consumido.

## Evidência obrigatória

Mapear todas as linhas da matriz compartilhada, especialmente A2/A3, retorno
do stack/worker com retenção, leases de credencial/runtime/WinSock, callback e
geração obsoletos, PRECONDITION corrigida, contenção unresolved, no double
release, no early reuse e exactly-once depois de destruição factual.

Estados de UI/navegação e política continuam específicos do Browser. O pin de
produção permanece PST 0.6.4 até autorização separada. Este handoff não altera
Browser nem qualifica execução NT4.
