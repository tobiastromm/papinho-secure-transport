# Readiness and progress audit

Phase 7.C is in progress. This document records the audit baseline before any
readiness or progress behavior is changed. Public API 1.2.0, library 0.3.0 and
SPI 2.3 remain unchanged.

## Readiness is not progress

Readiness says that a backend descriptor may be attempted without an ordinary
blocking wait. It does not promise that the current TLS operation will consume
or produce application bytes, complete, change its required interest, or enter
a terminal state. In particular, a TLS read may report `NEED_READ_WRITE`, see
only WRITE ready, and still return zero bytes with the same operation and
interest. Repeating that immediately is a spin, not progress.

For an application read, READ is the primary interest and WRITE is auxiliary.
For an application write, WRITE is primary and READ is auxiliary. Handshake is
not assigned one fixed primary direction because a TLS stack may legitimately
alternate reads and writes. The current RetroZilla NSS shutdown completes
locally in one provider step, so no pending shutdown direction is observable in
that provider.

## Current core guard

The guard in `src/pst_runtime.c` is private and stored in each
`pst_connection`: pending I/O kind, pending interest, last ready mask,
suppressed interest and whether a non-empty wait result was observed. It has no
global or thread-local state.

After READ or WRITE returns zero bytes in a nonterminal pending operation, the
core remembers the operation kind and mapped interest. If the next wait reports
only auxiliary readiness and retrying the same operation produces the same
zero-byte state, that observed auxiliary bit is suppressed from the next wait.
If suppression would remove every requested bit, the core falls back to the
provider's complete interest mask rather than issuing an empty wait.

The guard resets on transferred bytes, COMPLETE, CLOSED, FAILED, provider
failure, a normal wait timeout, handshake entry and shutdown entry. A changed
I/O kind or changed interest cannot satisfy the equality needed to carry stale
suppression forward. This implementation is symmetric between application READ
and WRITE; it is not a generic guard applied blindly to handshake and shutdown.

A timeout is bounded normal progress control, not a failure. It clears the
temporary suppression so an auxiliary dependency can become eligible again.
`NEED_*`, timeout, partial I/O and suppression do not create failure diagnostics
or WARN/ERROR logging. A fatal backend wait does transition the connection to
FAILED and retains the WAIT diagnostic.

## Operation and interest matrix

`NONE` is meaningful only for complete or terminal provider state. Returning a
zero-byte pending operation with `NONE` is not a useful wait contract and is not
produced by the current NSS provider.

| Operation | Provider interest | Readiness result | Core action and expected progress |
| --- | --- | --- | --- |
| HANDSHAKE | READ | timeout/NONE | remain pending; later wait remains bounded |
| HANDSHAKE | READ | READ or READ/HUP | retry handshake; HUP is not TLS close classification |
| HANDSHAKE | WRITE | timeout/NONE | remain pending; later wait remains bounded |
| HANDSHAKE | WRITE | WRITE or WRITE/HUP | retry handshake |
| HANDSHAKE | READ/WRITE | READ, WRITE, READ/WRITE, or those masks with HUP | retry; either direction may be a legitimate dependency, so no auxiliary suppression is applied |
| READ | READ | READ or HUP-derived READ | retry secure read; bytes, interest/state change, close or failure determine progress |
| READ | WRITE | WRITE | retry secure read because TLS may need to emit protocol data |
| READ | READ/WRITE | READ or READ/WRITE | retry; primary READ is present and is never suppressed |
| READ | READ/WRITE | WRITE only | retry once; if zero bytes and the same pending state recur, suppress WRITE for the next wait |
| WRITE | WRITE | WRITE | retry secure write; bytes or state/interest change determine progress |
| WRITE | READ | READ or READ/HUP | retry secure write because TLS may need inbound protocol data |
| WRITE | READ/WRITE | WRITE or READ/WRITE | retry; primary WRITE is present and is never suppressed |
| WRITE | READ/WRITE | READ only | retry once; if zero bytes and the same pending state recur, suppress READ for the next wait |
| SHUTDOWN, current NSS | NONE/COMPLETE | no wait | `PR_Shutdown` completes locally in one step; connection becomes CLOSED |
| SHUTDOWN, generic pending provider | READ, WRITE, or READ/WRITE | matching readiness or timeout | SPI can represent this, but the current core has no operation-specific no-progress suppression proof for pending shutdown |
| Any pending operation | any | timeout/NONE | no failure; clear temporary suppression; retain operation and permit later auxiliary readiness |
| Any operation | any | ERR or NVAL | fatal transport/readiness failure; transition to FAILED; no resurrection |

The RetroZilla NSS adapter maps `PR_POLL_HUP` to READ readiness, including
WRITE/HUP becoming READ/WRITE. This deliberately permits one secure read so the
provider can classify CLOSE/CLEAN versus FAILED/TRUNCATED from observed TLS
close_notify state. HUP alone never means CLOSED. `PR_POLL_ERR` and
`PR_POLL_NVAL` take fatal precedence even when useful readiness bits coexist.

## Progress and reset rules

The following are useful progress and reset the no-progress history:

- any positive byte count, including a partial read or partial write;
- COMPLETE, CLOSED or FAILED;
- a provider call failure;
- an operation-kind transition such as READ to WRITE;
- an interest transition, because stale suppression no longer matches;
- a normal timeout, which deliberately re-enables auxiliary readiness.

The same ready auxiliary bit, zero transferred bytes, the same pending
operation, the same interest and another immediate poll are not progress.
Accumulated byte totals remain the caller's responsibility and must use the
provider's truthful per-call `bytes_transferred` value.

## Existing evidence

Deterministic SPI tests currently prove the READ-side `NEED_READ_WRITE` case in
which WRITE-only readiness produces no progress and is removed from the next
wait, the case where WRITE-only readiness does produce a byte, combined
READ/WRITE readiness, timeout as nonfailure, and fatal wait transition with no
terminal-state resurrection. NSS unit tests prove READ, WRITE, READ/WRITE,
HUP, READ/HUP, WRITE/HUP, ERR and NVAL classification.

Real TLS 1.2/TLS 1.3 host and NT4 evidence proves nonblocking WOULD_BLOCK,
provider `PR_Poll`, authenticated ALPN, 25-byte bidirectional secure I/O and
content matching. Phase 7.B proves clean close, data before clean close,
provider-observed close_notify, abrupt-close truncation and fatal wait/terminal
behavior. Phase 6 also proved the original WRITE-only readiness spin correction
on real NT4.

## Coverage gaps identified by the audit

The implementation was not changed by this audit. Phase 7.C remains in progress
until deterministic coverage addresses these gaps:

- mirror the no-progress and real-progress sequences for WRITE with READ as the
  auxiliary direction;
- prove timeout re-enable end to end: suppress auxiliary, time out on the
  primary wait, then permit auxiliary readiness that makes real progress;
- assert that partial READ and partial WRITE clear stale suppression even when
  the provider remains pending;
- assert interest transitions `READ/WRITE -> READ` and `READ/WRITE -> WRITE`;
- assert READ/WRITE operation transitions cannot inherit suppression;
- use distinct mock connection states to prove connection A cannot affect B;
- cover alternating handshake readiness and fatal/timeout outcomes without
  adding speculative handshake suppression;
- document/test the generic pending-shutdown path if a provider that exposes it
  is introduced; the current NSS one-step completion cannot prove it;
- explicitly cover ready masks containing READ/WRITE/HUP together where the
  provider classifier can report them.

These are test-evidence gaps, except that generic pending-shutdown resilience is
an unproved capability. No current failing behavior justifies a core, backend,
SPI or public API change. Deterministic mock evidence must precede any such
correction.

## NT4 decision and housekeeping

Because this audit changes documentation only, existing real-NT4 Phase 6 and
7.B evidence remains applicable. If later 7.C work changes core readiness or
progress behavior, a targeted real-NT4 regression becomes mandatory.

Deferred housekeeping remains unchanged: preserve and vendor the exact
RetroZilla NSS source/provenance so `C:\PSTW` can eventually be disposable. It
is not part of this audit.
## Phase 7.C2 deterministic regression matrix

The existing SPI mock now exercises the application-I/O guard symmetrically.
Deterministic sequences prove WRITE NEED_READ_WRITE plus READ-only readiness
suppresses auxiliary READ after a zero-byte retry; positive WRITE progress
clears suppression; timeout re-enables auxiliary WRITE for READ; partial READ
and WRITE clear stale suppression; and changes to NEED_WRITE or NEED_READ take
effect immediately. Assertions use call counts and requested-interest masks,
not wall-clock timing.

The first run exposed a test-provider defect: its new WRITE script returned
NEED_READ_WRITE but updated mock interest as NONE. The mock now maps all three
pending operations consistently. No production behavior changed.

The completed matrix also proves two-connection isolation, READ-to-WRITE
transition reset, alternating handshake readiness, and stale-guard clearing on
entry to generic pending shutdown. Existing fatal-wait, provider HUP/ERR/NVAL
classification and no-resurrection tests remain passing. No production behavior
changed, so existing NT4 evidence remains applicable. Fresh host TLS 1.2/TLS
1.3 and failure-fixture execution remains mandatory before the closure audit.
