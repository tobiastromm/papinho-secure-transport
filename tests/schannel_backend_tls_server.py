# SPDX-License-Identifier: MPL-2.0
import socket
import hashlib
import os
import ssl
import sys
import time

port = int(sys.argv[1])
certificate = sys.argv[2]
private_key = sys.argv[3]
version = sys.argv[4]
exchanges = int(sys.argv[5])
close_mode = sys.argv[6] if len(sys.argv) > 6 else "client"
alpn_protocols = sys.argv[7] if len(sys.argv) > 7 else "-"
client_ca = sys.argv[8] if len(sys.argv) > 8 else "-"
expected_client_sha256 = sys.argv[9].lower() if len(sys.argv) > 9 else "-"
expected = b"pst-phase5-public-runtime"
fragment_size = int(os.environ.get("PST_TEST_FRAGMENT_SIZE", "0"))
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
selected = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.minimum_version = selected
context.maximum_version = selected
context.load_cert_chain(certificate, private_key)
if alpn_protocols != "-":
    context.set_alpn_protocols(alpn_protocols.split(","))
if client_ca != "-":
    context.verify_mode = ssl.CERT_REQUIRED
    context.load_verify_locations(client_ca)
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
        print("ACCEPT %s TLS=%s CIPHER=%s ALPN=%s AUTH=%s" % (address, tls.version(), tls.cipher()[0], tls.selected_alpn_protocol(), bool(tls.getpeercert())), flush=True)
        if expected_client_sha256 != "-":
            actual_client_sha256 = hashlib.sha256(tls.getpeercert(binary_form=True)).hexdigest()
            if actual_client_sha256 != expected_client_sha256:
                raise RuntimeError("client identity fingerprint mismatch")
            print("CLIENT_IDENTITY_SHA256=%s MATCH=1" % actual_client_sha256, flush=True)
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
            if fragment_size > 0:
                for offset in range(0, len(data), fragment_size):
                    tls.sendall(data[offset:offset + fragment_size])
                    time.sleep(0.02)
            else:
                tls.sendall(data)
            print("EXCHANGE=%d RECV=%d SEND=%d CONTENT_MATCH=1" % (index + 1, len(data), len(data)), flush=True)
        if close_mode == "client-abrupt":
            descriptor = tls.detach()
            transport = socket.socket(fileno=descriptor)
            received_close = transport.recv(4096)
            transport.close()
            print("CLOSE=ABRUPT_AFTER_CLIENT_NOTIFY RECV_TLS_BYTES=%d" %
                  len(received_close), flush=True)
        elif close_mode == "client-timeout":
            descriptor = tls.detach()
            transport = socket.socket(fileno=descriptor)
            time.sleep(2)
            transport.close()
            print("CLOSE=NO_RESPONSE_TIMEOUT_FIXTURE", flush=True)
        elif close_mode == "peer-abrupt":
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
