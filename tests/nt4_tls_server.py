import ssl
import socket
import sys

EXPECTED = b"pst-phase5-public-runtime"

if len(sys.argv) != 9:
    raise SystemExit("usage: bind port cert.pem key.pem ca.pem 12|13 alpn required|optional")

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
version = sys.argv[6]
context.minimum_version = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.maximum_version = context.minimum_version
context.load_cert_chain(sys.argv[3], sys.argv[4])
context.load_verify_locations(sys.argv[5])
if sys.argv[7] != "-":
    context.set_alpn_protocols([sys.argv[7]])
context.verify_mode = ssl.CERT_REQUIRED if sys.argv[8] == "required" else ssl.CERT_NONE
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind((sys.argv[1], int(sys.argv[2])))
listener.listen(1)
print("READY %s:%s TLS%s" % (sys.argv[1], sys.argv[2], version), flush=True)
try:
    raw, address = listener.accept()
    with context.wrap_socket(raw, server_side=True) as tls:
        tls.settimeout(30)
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
                tls.recv(1)
            except socket.timeout:
                print("SHUTDOWN_WAIT_TIMEOUT", flush=True)
except (ssl.SSLError, OSError) as error:
    print("TLS_ERROR", error, flush=True)
finally:
    listener.close()
