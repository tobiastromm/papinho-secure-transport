import ssl
import socket
import sys

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
version = sys.argv[5] if len(sys.argv) > 5 else "12"
context.minimum_version = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.maximum_version = context.minimum_version
context.load_cert_chain(sys.argv[2], sys.argv[3])
context.load_verify_locations(sys.argv[4])
if len(sys.argv) > 6 and sys.argv[6] != "-":
    context.set_alpn_protocols([sys.argv[6]])
context.verify_mode = ssl.CERT_REQUIRED if len(sys.argv) < 8 or sys.argv[7] == "required" else ssl.CERT_NONE
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", int(sys.argv[1])))
listener.listen(1)
print("READY", flush=True)
try:
    raw, _ = listener.accept()
    with context.wrap_socket(raw, server_side=True) as tls:
        data = tls.recv(4096)
        if data:
            tls.sendall(data)
except ssl.SSLError as error:
    print("TLS_ERROR", error, flush=True)
finally:
    listener.close()
