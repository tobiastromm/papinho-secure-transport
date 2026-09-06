# SPDX-License-Identifier: MPL-2.0
import socket
import ssl
import struct
import sys
import time

port = int(sys.argv[1])
certificate = sys.argv[2]
private_key = sys.argv[3]
minimum = sys.argv[4]
maximum = sys.argv[5]
exchanges = int(sys.argv[6])
close_mode = sys.argv[7]
payload_size = int(sys.argv[8])
expect_failure = len(sys.argv) > 9 and sys.argv[9] == "expect-failure"
pattern = b"pst-phase5-public-runtime"
expected = bytes(pattern[i % len(pattern)] for i in range(payload_size))
versions = {"12": ssl.TLSVersion.TLSv1_2, "13": ssl.TLSVersion.TLSv1_3}
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.minimum_version = versions[minimum]
context.maximum_version = versions[maximum]
context.load_cert_chain(certificate, private_key)
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(1)
listener.settimeout(30)
print("READY 127.0.0.1:%d TLS_MIN=%s TLS_MAX=%s EXCHANGES=%d PAYLOAD=%d" % (port, minimum, maximum, exchanges, payload_size), flush=True)
passed = False
try:
    raw, address = listener.accept()
    raw.settimeout(15)
    if payload_size > 65536:
        raw.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
    try:
        with context.wrap_socket(raw, server_side=True) as tls:
            print("ACCEPT %s TLS=%s CIPHER=%s" % (address, tls.version(), tls.cipher()[0]), flush=True)
            if expect_failure:
                raise RuntimeError("handshake unexpectedly succeeded")
            if payload_size > 65536:
                time.sleep(1.5)
            chunk_size = 7 if payload_size <= 65536 else 4096
            for index in range(exchanges):
                data = bytearray()
                while len(data) < payload_size:
                    part = tls.recv(min(chunk_size, payload_size - len(data)))
                    if not part:
                        raise RuntimeError("unexpected eof")
                    data.extend(part)
                if bytes(data) != expected:
                    raise RuntimeError("payload mismatch")
                for offset in range(0, payload_size, 3):
                    tls.sendall(data[offset:offset + 3])
                print("EXCHANGE=%d RECV=%d SEND=%d CONTENT_MATCH=1" % (index + 1, len(data), len(data)), flush=True)
            if close_mode == "peer-reset":
                descriptor = tls.detach()
                reset_socket = socket.socket(fileno=descriptor)
                reset_socket.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                                        struct.pack("hh", 1, 0))
                reset_socket.close()
                print("CLOSE=RESET", flush=True)
            elif close_mode == "peer-abrupt":
                descriptor = tls.detach()
                socket.socket(fileno=descriptor).close()
                print("CLOSE=ABRUPT", flush=True)
            else:
                try:
                    tls.unwrap()
                    print("CLOSE=CLEAN", flush=True)
                except (ssl.SSLError, OSError) as error:
                    if close_mode in ("peer-clean", "data-then-close", "wait-fatal"):
                        print("CLOSE=NOTIFY_SENT", flush=True)
                    else:
                        raise error
            passed = True
    except ssl.SSLError as error:
        if not expect_failure:
            raise
        print("EXPECTED_HANDSHAKE_FAILURE TYPE=%s" % type(error).__name__, flush=True)
        passed = True
finally:
    listener.close()
print("SERVER_PASS=%d" % int(passed), flush=True)
if not passed:
    raise SystemExit(20)