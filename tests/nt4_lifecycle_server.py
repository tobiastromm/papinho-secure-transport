import ssl
import socket
import sys

EXPECTED = b"pst-phase5-public-runtime"

if len(sys.argv) != 10:
    raise SystemExit("usage: bind port cert.pem key.pem ca.pem 13 alpn required connections")

count = int(sys.argv[9])
if count < 1:
    raise SystemExit("connections must be positive")
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.minimum_version = ssl.TLSVersion.TLSv1_3
context.maximum_version = ssl.TLSVersion.TLSv1_3
context.load_cert_chain(sys.argv[3], sys.argv[4])
context.load_verify_locations(sys.argv[5])
context.set_alpn_protocols([sys.argv[7]])
context.verify_mode = ssl.CERT_REQUIRED
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind((sys.argv[1], int(sys.argv[2])))
listener.listen(1)
print("READY %s:%s TLS13 CONNECTIONS=%d" % (sys.argv[1], sys.argv[2], count), flush=True)
try:
    for cycle in range(1, count + 1):
        raw, address = listener.accept()
        with context.wrap_socket(raw, server_side=True) as tls:
            tls.settimeout(30)
            peer = tls.getpeercert()
            print("CLIENT %d %s AUTH=%s ALPN=%s" %
                  (cycle, address, bool(peer), tls.selected_alpn_protocol()), flush=True)
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
            print("CLIENT %d RECV=%d SEND=%d CONTENT_MATCH=%s" %
                  (cycle, len(data), sent, content_match), flush=True)
            if content_match:
                try:
                    part = tls.recv(1)
                    print("CLIENT %d SHUTDOWN_COMPLETE=%s" %
                          (cycle, part == b""), flush=True)
                except socket.timeout:
                    print("CLIENT %d SHUTDOWN_WAIT_TIMEOUT" % cycle, flush=True)
finally:
    listener.close()
