# SPDX-License-Identifier: MPL-2.0
import select
import os
import socket
import sys
import time

if len(sys.argv) != 5 or sys.argv[4] not in ("starttls", "connect"):
    raise SystemExit("usage: listen-port upstream-host upstream-port starttls|connect")
listen_port = int(sys.argv[1])
upstream_host = sys.argv[2]
upstream_port = int(sys.argv[3])
style = sys.argv[4]

def send_fragmented(sock, data):
    for offset in range(0, len(data), 3):
        sock.sendall(data[offset:offset + 3])
        time.sleep(0.01)

listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", listen_port))
listener.listen(1)
listener.settimeout(30)
print("UPGRADE_PROXY_READY 127.0.0.1:%d STYLE=%s UPSTREAM=%s:%d" %
      (listen_port, style, upstream_host, upstream_port), flush=True)
client, address = listener.accept()
listener.close()
client.settimeout(15)
if style == "starttls":
    send_fragmented(client, b"PLAINTEXT_GREETING\r\n")
    expected = b"UPGRADE_REQUEST\r\n"
    ready = b"UPGRADE_READY\r\n"
else:
    expected = b"PLAINTEXT_TUNNEL_REQUEST\r\n"
    ready = b"PLAINTEXT_TUNNEL_READY\r\n"
request = b""
while len(request) < len(expected):
    part = client.recv(len(expected) - len(request))
    if not part:
        raise RuntimeError("plaintext request truncated")
    request += part
if request != expected:
    raise RuntimeError("plaintext request mismatch")
send_fragmented(client, ready)
if os.environ.get("PST_TEST_PREREAD_TLS"):
    client.sendall(b"\x15\x03\x03\x00\x02\x02\x28")
upstream = socket.create_connection((upstream_host, upstream_port), timeout=15)
client.setblocking(False)
upstream.setblocking(False)
print("PLAINTEXT_BOUNDARY STYLE=%s REQUEST=%d READY=%d EXACT=1 CONNECTIONS=1" %
      (style, len(request), len(ready)), flush=True)
sockets = [client, upstream]
while sockets:
    readable, _, _ = select.select(sockets, [], [], 30)
    if not readable:
        raise RuntimeError("relay timeout")
    for source in readable:
        target = upstream if source is client else client
        try:
            data = source.recv(16384)
        except BlockingIOError:
            continue
        except ConnectionResetError:
            data = b""
        if not data:
            sockets.remove(source)
            try:
                target.shutdown(socket.SHUT_WR)
            except OSError:
                pass
            continue
        target.setblocking(True)
        target.sendall(data)
        target.setblocking(False)
client.close()
upstream.close()
print("UPGRADE_PROXY_PASS STYLE=%s SAME_CLIENT_TRANSPORT=1 NO_RECONNECT=1" % style, flush=True)
