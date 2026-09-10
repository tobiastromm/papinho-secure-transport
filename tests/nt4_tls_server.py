# SPDX-License-Identifier: MPL-2.0
import ssl
import socket
import sys

EXPECTED = b"pst-phase5-public-runtime"

if len(sys.argv) not in (9, 10):
    raise SystemExit("usage: bind port cert.pem key.pem ca.pem 12|13 alpn required|optional [expected-sni|-]")

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
version = sys.argv[6]
context.minimum_version = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.maximum_version = context.minimum_version
context.load_cert_chain(sys.argv[3], sys.argv[4])
context.load_verify_locations(sys.argv[5])
server_alpn = [] if sys.argv[7] == "-" else sys.argv[7].split(",")
if server_alpn:
    context.set_alpn_protocols(server_alpn)
context.verify_mode = ssl.CERT_REQUIRED if sys.argv[8] == "required" else ssl.CERT_NONE
check_sni = len(sys.argv) == 10
expected_sni = None if not check_sni or sys.argv[9] == "-" else sys.argv[9]
observed_sni = [None]
def observe_sni(ssl_socket, server_name, ssl_context):
    del ssl_socket, ssl_context
    observed_sni[0] = server_name
context.set_servername_callback(observe_sni)
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind((sys.argv[1], int(sys.argv[2])))
listener.listen(1)
print("READY %s:%s TLS%s ALPN=%s" %
      (sys.argv[1], sys.argv[2], version,
       ",".join(server_alpn) if server_alpn else "NONE"), flush=True)
try:
    raw, address = listener.accept()
    with context.wrap_socket(raw, server_side=True) as tls:
        tls.settimeout(30)
        sni_match = not check_sni or observed_sni[0] == expected_sni
        print("SNI OBSERVED=%s EXPECTED=%s MATCH=%s" %
              (observed_sni[0] if observed_sni[0] is not None else "NONE",
               (expected_sni if expected_sni is not None else "NONE") if check_sni else "NOT_CHECKED",
               sni_match), flush=True)
        if not sni_match:
            raise ssl.SSLError("unexpected SNI")
        peer = tls.getpeercert()
        print("CLIENT %s AUTH=%s ALPN=%s" % (address, bool(peer), tls.selected_alpn_protocol()), flush=True)
        data = b""
        while len(data) < len(EXPECTED):
            part = tls.recv(len(EXPECTED) - len(data))
            if not part:
                break
            data += part
        content_match = data == EXPECTED
        sent = 0
        if content_match:
            tls.sendall(EXPECTED)
            sent = len(EXPECTED)
        print("IO RECV=%d SEND=%d CONTENT_MATCH=%s" %
              (len(data), sent, content_match), flush=True)
        if content_match:
            try:
                closed = tls.recv(1)
                if closed == b"":
                    plain = tls.unwrap()
                    plain.close()
                    print("SHUTDOWN RECIPROCAL_CLOSE_NOTIFY=True", flush=True)
            except socket.timeout:
                print("SHUTDOWN_WAIT_TIMEOUT", flush=True)
except (ssl.SSLError, OSError) as error:
    print("TLS_ERROR", error, flush=True)
finally:
    listener.close()
