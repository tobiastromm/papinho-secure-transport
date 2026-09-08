# SPDX-License-Identifier: MPL-2.0
import socket
import ssl
import sys
import time

PAYLOAD = b"pst-phase5-public-runtime"

if len(sys.argv) != 10:
    raise SystemExit(
        "usage: host port tls ca.pem client.pem|- client.key|- "
        "alpn-list|- clean|data-abrupt expected-alpn|-"
    )

host, port_text, tls_text = sys.argv[1:4]
ca_path, cert_path, key_path = sys.argv[4:7]
alpn_text, close_mode, expected_alpn = sys.argv[7:10]
version = ssl.TLSVersion.TLSv1_2 if tls_text == "12" else ssl.TLSVersion.TLSv1_3

context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
context.minimum_version = version
context.maximum_version = version
context.check_hostname = True
context.verify_mode = ssl.CERT_REQUIRED
context.load_verify_locations(ca_path)
if cert_path != "-":
    context.load_cert_chain(cert_path, key_path)
if alpn_text != "-":
    context.set_alpn_protocols(alpn_text.split(","))

raw = socket.create_connection((host, int(port_text)), timeout=10)
tls = context.wrap_socket(raw, server_hostname="localhost")
selected = tls.selected_alpn_protocol()
if expected_alpn == "-":
    if selected is not None:
        raise SystemExit("unexpected ALPN %r" % selected)
elif selected != expected_alpn:
    raise SystemExit("ALPN mismatch expected=%s actual=%s" %
                     (expected_alpn, selected))

tls.sendall(PAYLOAD)
if close_mode == "data-abrupt":
    time.sleep(0.1)
    descriptor = tls.detach()
    socket.socket(fileno=descriptor).close()
    print("CLIENT TLS=%s WRITE=25 CLOSE=TCP_NO_CLOSE_NOTIFY ALPN=%s PASS=1" %
          (tls_text, selected or "NONE"), flush=True)
else:
    received = b""
    while len(received) < len(PAYLOAD):
        block = tls.recv(len(PAYLOAD) - len(received))
        if not block:
            break
        received += block
    if received != PAYLOAD:
        raise SystemExit("echo mismatch read=%d" % len(received))
    plain = tls.unwrap()
    plain.close()
    print("CLIENT TLS=%s WRITE=25 READ=25 CONTENT_MATCH=1 "
          "CLOSE=RECIPROCAL ALPN=%s PASS=1" %
          (tls_text, selected or "NONE"), flush=True)
