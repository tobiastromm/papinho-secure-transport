import socket
import ssl
import struct
import sys

EXPECTED = b"pst-phase7b-data-before-close"
CLIENT_WRITE = b"pst-phase7b-client-write"
ACCEPT_TIMEOUT_SECONDS = 120
OPERATION_TIMEOUT_SECONDS = 10
MODES = {
    "pre_tls_close", "non_tls", "handshake_close", "handshake_reset",
    "clean_close", "abrupt_close", "read_clean", "read_abrupt",
    "data_then_close", "close_around_write", "shutdown_abort"
}


def abort_socket(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("hh", 1, 0))
    sock.close()


def abort_tls(tls):
    fd = tls.detach()
    raw = socket.socket(fileno=fd)
    abort_socket(raw)


if len(sys.argv) != 10 or sys.argv[3] not in MODES:
    raise SystemExit(
        "usage: bind port mode cert.pem key.pem ca.pem 12|13 alpn required|optional"
    )

bind, port, mode = sys.argv[1], int(sys.argv[2]), sys.argv[3]
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.settimeout(ACCEPT_TIMEOUT_SECONDS)
listener.bind((bind, port))
listener.listen(1)
print("READY MODE=%s BIND=%s PORT=%d ACCEPT_TIMEOUT_SECONDS=%d" % (
    mode, bind, port, ACCEPT_TIMEOUT_SECONDS
), flush=True)

accepted = False
try:
    raw, address = listener.accept()
    accepted = True
    raw.settimeout(OPERATION_TIMEOUT_SECONDS)
    print("ACCEPT MODE=%s PEER=%s" % (mode, address), flush=True)

    if mode == "pre_tls_close":
        raw.close()
        print("CLOSE TYPE=TCP_FIN", flush=True)
        raise SystemExit(0)

    if mode == "non_tls":
        raw.sendall(b"NOT-TLS\r\n")
        raw.close()
        print("SEND=9 CLOSE=TCP_FIN", flush=True)
        raise SystemExit(0)

    if mode in ("handshake_close", "handshake_reset"):
        received = raw.recv(4096)
        print("CLIENT_HELLO_BYTES=%d" % len(received), flush=True)
        if mode == "handshake_reset":
            abort_socket(raw)
            print("CLOSE TYPE=TCP_RST", flush=True)
        else:
            raw.close()
            print("CLOSE TYPE=TCP_FIN", flush=True)
        raise SystemExit(0)

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = (
        ssl.TLSVersion.TLSv1_3 if sys.argv[7] == "13"
        else ssl.TLSVersion.TLSv1_2
    )
    context.maximum_version = context.minimum_version
    context.load_cert_chain(sys.argv[4], sys.argv[5])
    context.load_verify_locations(sys.argv[6])
    if sys.argv[8] != "-":
        context.set_alpn_protocols([sys.argv[8]])
    context.verify_mode = (
        ssl.CERT_REQUIRED if sys.argv[9] == "required" else ssl.CERT_NONE
    )

    tls = context.wrap_socket(raw, server_side=True)
    tls.settimeout(OPERATION_TIMEOUT_SECONDS)
    print("TLS MODE=%s VERSION=%s AUTH=%s ALPN=%s" % (
        mode, tls.version(), bool(tls.getpeercert()), tls.selected_alpn_protocol()
    ), flush=True)

    if mode in ("clean_close", "read_clean"):
        plain = tls.unwrap()
        plain.close()
        print("CLOSE TYPE=TLS_CLOSE_NOTIFY", flush=True)
    elif mode == "data_then_close":
        tls.sendall(EXPECTED)
        print("SEND=%d CONTENT=%s" % (len(EXPECTED), EXPECTED.decode("ascii")), flush=True)
        plain = tls.unwrap()
        plain.close()
        print("CLOSE TYPE=TLS_CLOSE_NOTIFY", flush=True)
    elif mode in ("abrupt_close", "read_abrupt"):
        fd = tls.detach()
        socket.socket(fileno=fd).close()
        print("CLOSE TYPE=TCP_FIN_NO_CLOSE_NOTIFY", flush=True)
    elif mode == "close_around_write":
        data = tls.recv(len(CLIENT_WRITE))
        print("RECV=%d CONTENT_MATCH=%s" % (len(data), data == CLIENT_WRITE), flush=True)
        abort_tls(tls)
        print("CLOSE TYPE=TCP_RST_NO_CLOSE_NOTIFY", flush=True)
    elif mode == "shutdown_abort":
        data = tls.recv(len(CLIENT_WRITE))
        print("RECV=%d CONTENT_MATCH=%s" % (len(data), data == CLIENT_WRITE), flush=True)
        listener.settimeout(OPERATION_TIMEOUT_SECONDS)
        print("CONTROL_ACCEPT_BOUND_SECONDS=%d" % OPERATION_TIMEOUT_SECONDS, flush=True)
        control, control_address = listener.accept()
        control.settimeout(OPERATION_TIMEOUT_SECONDS)
        marker = control.recv(1)
        print("CONTROL_ACCEPT PEER=%s MARKER=%s" % (
            control_address, marker == b"C"
        ), flush=True)
        control.sendall(b"R")
        print("CONTROL_READY", flush=True)
        try:
            closed = tls.recv(1)
            print("CLIENT_SHUTDOWN_OBSERVED TYPE=TLS_EOF CLOSE_NOTIFY=%s" % (
                closed == b""
            ), flush=True)
        except (ConnectionResetError, ssl.SSLError, OSError) as error:
            print("CLIENT_SHUTDOWN_OBSERVED TYPE=%s TEXT=%s" % (
                type(error).__name__,
                str(error).replace("\r", " ").replace("\n", " ")
            ), flush=True)
        abort_tls(tls)
        print("CLOSE TYPE=SERVER_ABORT_AFTER_CLIENT_SHUTDOWN", flush=True)
        control.sendall(b"A")
        control.close()
        print("CONTROL_ABORT_CONFIRMED", flush=True)
except socket.timeout as error:
    if not accepted:
        print("FIXTURE_TIMEOUT STAGE=ACCEPT REASON=NO_CLIENT "
              "ACCEPT_TIMEOUT_SECONDS=%d" % ACCEPT_TIMEOUT_SECONDS,
              flush=True)
    else:
        print("FIXTURE_ERROR TYPE=%s TEXT=%s" % (
            type(error).__name__, str(error).replace("\r", " ").replace("\n", " ")
        ), flush=True)
    raise SystemExit(3)
except (ssl.SSLError, OSError) as error:
    print("FIXTURE_ERROR TYPE=%s TEXT=%s" % (
        type(error).__name__, str(error).replace("\r", " ").replace("\n", " ")
    ), flush=True)
    raise SystemExit(3)
finally:
    listener.close()
