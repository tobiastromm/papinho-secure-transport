# PST 0.6.5 NSS candidate — real NT4 /ML consolidated evidence

Owner evidence archives:

- `novissimo.zip`: SHA-256
  `236F5815A4E83B8FC001EA31BADE8524F7C5F63F926C91709CEF33A28D4C66F6`.
- `restante_ml.zip`: SHA-256
  `E4A3B1BEACCEE3D7F160AC2210D7EB983CCD15C8682C2CA982F39089E440CB7F`.

Accepted real Windows NT 4.0 `/ML` results:

- TLS 1.2 mTLS, peer authentication, expected peer name, 25-byte write/read,
  content match and reciprocal shutdown: PASS.
- TLS 1.3 with the same security and I/O requirements: PASS.
- Real shutdown backpressure: three consecutive NEED_WRITE operations with
  WRITE interest, no premature completion, 32768-byte drain, exactly one
  close-notify and reciprocal shutdown: PASS.
- Cancellation while shutdown is pending: PASS; reset observed by the fixture
  and no false clean completion.
- Clean peer close: close-notify received, local close-notify sent, reciprocal
  close-notify confirmed, exit code zero: PASS.
- Idle TCP reset: normalized TRUNCATED terminal result, PASS=1, exit code zero.
- Data then abrupt FIN: 29 application bytes preserved before TRUNCATED,
  terminal result and exit code zero: PASS.
- API 3/SPI 4 Model E and NSS layered-close local contracts: PASS.

R2/R3 binary identity comparison:

- Candidate `ssl3.dll`:
  `68F20EDE3C71CBAA132EF9E1DA9F54CCF8FA5D96DA761E29892070511FED0FCE`.
- PST `/ML` library:
  `23A2E162D869E45DFC91274897FB685F0AA94E08AFFCB7E0280C09103C9B9D3C`.
- NSS integration executable:
  `C1E6EEC63F1DA69925F03C6D50E3309BDCD2D443844816B23215B455BF194C07`.
- Failure executable:
  `FE54973B6AB85F77D010AB90B249F841A89B120082F7D77A25FA3FE6774B04AB`.
- Model E executable:
  `6B3EFBF8114467C892C1A6256E7532A478CFD39F763800FB26268DA844A088D8`.
- Layered-close executable:
  `D3A29AD60D8F8702BF0E708EE66FF1C3023C916F98BA6705204054AE8E482A1B`.

All listed identities are byte-exact between R2 and R3. R3 changes only the
validation runner mapping/NT4 syntax, manifests, and validation-only log files;
no PST or NSS candidate binary changed.

Conclusion: `REAL_NT4_ML=PASS` for the governed matrix above. `/ML` does not
need repetition before executing the independent `/MD` matrix.
