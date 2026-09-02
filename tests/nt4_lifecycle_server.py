import ssl
import socket
import struct
import sys
import time

CANONICAL = b"pst-phase5-public-runtime"
RECORD_SIZE = 64
ACCEPT_TIMEOUT = 120
CLIENT_TIMEOUT = 10


def abort_tls(tls):
    fd = tls.detach()
    raw = socket.socket(fileno=fd)
    raw.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("hh", 1, 0))
    raw.close()


def read_exact(tls, size):
    data = b""
    while len(data) < size:
        part = tls.recv(size - len(data))
        if not part:
            break
        data += part
    return data


if len(sys.argv) == 10:
    bind, port, cert, key, ca = sys.argv[1:6]
    version, alpn, auth = sys.argv[6:9]
    mode, count, exchanges = "legacy", int(sys.argv[9]), 1
elif len(sys.argv) == 12:
    bind, port, cert, key, ca = sys.argv[1:6]
    version, alpn, auth, mode = sys.argv[6:10]
    count, exchanges = int(sys.argv[10]), int(sys.argv[11])
else:
    raise SystemExit("usage: bind port cert key ca 12|13 alpn|- required|optional [mode count exchanges]")

port = int(port)
if mode not in ("legacy", "echo", "io", "noalpn", "clean", "abrupt", "mixed", "chain"):
    raise SystemExit("invalid mode")
if count < 1 or exchanges < 1:
    raise SystemExit("count/exchanges must be positive")

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.minimum_version = ssl.TLSVersion.TLSv1_3 if version == "13" else ssl.TLSVersion.TLSv1_2
context.maximum_version = context.minimum_version
context.load_cert_chain(cert, key)
context.load_verify_locations(ca)
if alpn != "-":
    context.set_alpn_protocols([alpn])
context.verify_mode = ssl.CERT_REQUIRED if auth == "required" else ssl.CERT_NONE

tls12_context = None
if mode == "chain":
    tls12_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls12_context.minimum_version = ssl.TLSVersion.TLSv1_2
    tls12_context.maximum_version = ssl.TLSVersion.TLSv1_2
    tls12_context.load_cert_chain(cert, key)
    tls12_context.load_verify_locations(ca)
    tls12_context.set_alpn_protocols([alpn])
    tls12_context.verify_mode = context.verify_mode
noalpn_context = None
if mode == "mixed":
    noalpn_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    noalpn_context.minimum_version = context.minimum_version
    noalpn_context.maximum_version = context.maximum_version
    noalpn_context.load_cert_chain(cert, key)
    noalpn_context.load_verify_locations(ca)
    noalpn_context.verify_mode = context.verify_mode
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.settimeout(ACCEPT_TIMEOUT)
listener.bind((bind, port))
listener.listen(8)
print("READY MODE=%s BIND=%s PORT=%d TLS=%s ALPN=%s CLIENTS_EXPECTED=%d ACCEPT_TIMEOUT_SECONDS=%d" %
      (mode, bind, port, version, alpn if alpn != "-" else "NONE", count, ACCEPT_TIMEOUT), flush=True)

accepted = passed = failed = total_recv = total_send = clean = abrupt = 0
started = time.monotonic()
try:
    for cycle in range(1, count + 1):
        try:
            raw, address = listener.accept()
        except socket.timeout:
            print("FAILURE_STAGE=ACCEPT ITERATION=%d" % cycle, flush=True)
            failed += count - accepted
            break
        accepted += 1
        raw.settimeout(CLIENT_TIMEOUT)
        slot = ((cycle - 1) % 8) + 1
        try:
            active_context = tls12_context if mode == "chain" and slot == 1 else (noalpn_context if mode == "mixed" and slot == 7 else context)
            tls = active_context.wrap_socket(raw, server_side=True)
            tls.settimeout(CLIENT_TIMEOUT)
            if mode == "noalpn" or (mode == "mixed" and slot in (3, 7)):
                print("CLIENT=%d EXPECTED=WRONG_HOSTNAME TLS_UNEXPECTED_SUCCESS=1" % cycle, flush=True)
                tls.close()
                failed += 1
                continue
            if mode == "noalpn" or (mode == "mixed" and slot == 7):
                try:
                    tls.recv(1)
                except (ssl.SSLError, OSError):
                    pass
                passed += 1
                continue
            if mode == "clean" or (mode == "mixed" and slot == 7):
                plain = tls.unwrap()
                plain.close()
                clean += 1
                passed += 1
                continue
            if mode == "abrupt" or (mode == "mixed" and slot == 5) or (mode == "chain" and slot == 3):
                abort_tls(tls)
                abrupt += 1
                passed += 1
                continue
            loops = exchanges if mode == "io" else 1
            ok = True
            for exchange in range(loops):
                size = len(CANONICAL) if mode == "legacy" else RECORD_SIZE
                data = read_exact(tls, size)
                total_recv += len(data)
                if len(data) != size:
                    ok = False
                    break
                tls.sendall(data)
                total_send += len(data)
            if ok:
                passed += 1
            else:
                failed += 1
            try:
                plain = tls.unwrap()
                plain.close()
                clean += 1
            except (ssl.SSLError, OSError):
                tls.close()
        except (ssl.SSLError, OSError) as error:
            if mode == "noalpn" or (mode == "mixed" and slot in (3, 7)):
                passed += 1
            else:
                failed += 1
                print("CLIENT_ERROR ITERATION=%d TYPE=%s" % (cycle, type(error).__name__), flush=True)
finally:
    listener.close()

elapsed = int((time.monotonic() - started) * 1000)
ok = accepted == count and passed == count and failed == 0
print("MODE=%s CLIENTS_EXPECTED=%d CLIENTS_ACCEPTED=%d CLIENTS_PASS=%d CLIENTS_FAIL=%d TOTAL_RECV=%d TOTAL_SEND=%d CLEAN=%d ABRUPT=%d CONTENT_MATCH=%d ELAPSED_MS=%d PASS=%d" %
      (mode, count, accepted, passed, failed, total_recv, total_send, clean, abrupt, 1 if total_recv == total_send else 0, elapsed, 1 if ok else 0), flush=True)
raise SystemExit(0 if ok else 3)
