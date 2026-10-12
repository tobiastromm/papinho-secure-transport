# PST 0.6.5 NSS candidate — real NT4 /MD consolidated evidence

Owner evidence archive:

- `md.zip`: SHA-256
  `650C6EF2DD46218A6DB46A13CE59F0F4350BF1720A2A50F2CA925FA507C646C4`.

Accepted real Windows NT 4.0 `/MD` results:

- TLS 1.2 mTLS, peer authentication, expected peer name, 25-byte write/read,
  content match and reciprocal shutdown: PASS.
- TLS 1.3 with the same security and I/O requirements: PASS.
- Real shutdown backpressure resume: 32767 bytes accepted before pending
  shutdown, `PST_OPERATION_NEED_WRITE` with WRITE interest, 32768 bytes
  drained, exactly one close-notify and reciprocal shutdown: PASS.
- Cancellation while shutdown is pending: PASS; the fixture observed the
  expected connection reset and no false shutdown completion was reported.
- Clean peer close: close-notify received, the local close-notify was sent,
  reciprocal close-notify was confirmed, `CLOSE_KIND=1`, and exit code was
  zero: PASS.
- Idle TCP reset: wait/read woke and reported terminal `OPERATION=5`,
  `CLOSE_KIND=2`, with exit code zero: PASS (TRUNCATED).
- Data then abrupt FIN: 29 application bytes were preserved with
  `CONTENT_MATCH=1` before terminal `OPERATION=5`, `CLOSE_KIND=2`; PASS=1 and
  exit code zero.
- API 3/SPI 4 Model E and NSS layered-close local contracts: PASS, including
  DESTROYED, PRECONDITION_RETAINED, CLOSE_UNRESOLVED_RETAINED, public retry
  disabled, synthetic layered-close failures, normal PR close success and no
  second PR close.

Validation-kit and candidate identity:

- Governed R3 `/MD` kit:
  `B76919B5720F35C6ACC56B7904BFA74D4FA07C8835EC64F34E864741FE1158EC`.
- Candidate `ssl3.dll`:
  `68F20EDE3C71CBAA132EF9E1DA9F54CCF8FA5D96DA761E29892070511FED0FCE`.
- PST `/MD` library:
  `801685EFD9772373F46402423FADAD961716934E1E8113D821B2016D30BB149B`.
- Candidate source commit:
  `da7190930700f54ecc92611f5edbcb0932caa5ec`.
- Candidate patch:
  `EE6A0D70F04E01D1D6747389F2ECDAA5995C75E99D4BA1FCD2855363660DE65F`.

The R3 correction is confined to validation-kit batch syntax, case-name
mapping, per-case trace isolation, manifests and instructions. It does not
change the candidate PST or NSS binaries. The returned archive identifies the
executed R3 `/MD` kit and supplies the functional logs/screenshots, but does
not independently repeat loaded-module hashes from inside NT4; module identity
therefore remains grounded in the already frozen and locally verified R3 kit.

Evidence hygiene:

- private-key material hits: 0;
- personal-path hits: 0;
- unintended application-payload log hits: 0.

The owner used ports 18841–18847 instead of the suggested independent `/MD`
range. Each fixture and matching client used the same port sequentially, so
this does not alter the functional result or mix cases.

Conclusion: `REAL_NT4_MD=PASS` for the governed matrix above.

Combined conclusion: `REAL_NT4_ML_MD_PARITY=PASS`; the accepted `/ML` evidence
does not need repetition, and no further NT4 rerun is required for this
candidate unless its production binaries or governed kit identity changes.
