import socket
import ssl
import sys

port = int(sys.argv[1])
certificate = sys.argv[2]
private_key = sys.argv[3]
version = sys.argv[4]
exchanges = int(sys.argv[5])
close_mode = sys.argv[6] if len(sys.argv) > 6 else "client"
expected = b"pst-phase5-public-runtime"
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
selected = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.minimum_version = selected
context.maximum_version = selected
context.load_cert_chain(certificate, private_key)
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(1)
listener.settimeout(30)
print("READY 127.0.0.1:%d TLS%s EXCHANGES=%d" % (port, version, exchanges), flush=True)
try:
    raw, address = listener.accept()
    raw.settimeout(10)
    with context.wrap_socket(raw, server_side=True) as tls:
        print("ACCEPT %s TLS=%s CIPHER=%s" % (address, tls.version(), tls.cipher()[0]), flush=True)
        for index in range(exchanges):
            data = b""
            while len(data) < len(expected):
                part = tls.recv(len(expected) - len(data))
                if not part:
                    raise RuntimeError("unexpected eof")
                data += part
            match = data == expected
            if not match:
                raise RuntimeError("payload mismatch")
            tls.sendall(data)
            print("EXCHANGE=%d RECV=%d SEND=%d CONTENT_MATCH=1" % (index + 1, len(data), len(data)), flush=True)
        if close_mode == "peer-abrupt":
            descriptor = tls.detach()
            socket.socket(fileno=descriptor).close()
            print("CLOSE=ABRUPT", flush=True)
        else:
            try:
                tls.unwrap()
                print("CLOSE=CLEAN", flush=True)
            except (ssl.SSLError, OSError) as error:
                print("CLOSE=NOTIFY_SENT DETAIL=%s" % error, flush=True)
    print("PASS TLS%s EXCHANGES=%d CONTENT_MATCH=1" % (version, exchanges), flush=True)
finally:
    listener.close()