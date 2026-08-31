import ssl
import socket
import sys

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.minimum_version = ssl.TLSVersion.TLSv1_2
context.maximum_version = ssl.TLSVersion.TLSv1_2
context.load_cert_chain(sys.argv[2], sys.argv[3])
context.load_verify_locations(sys.argv[4])
context.verify_mode = ssl.CERT_REQUIRED
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
