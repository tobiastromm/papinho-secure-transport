import socket
import sys
port = int(sys.argv[1])
listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(1)
listener.settimeout(30)
print("READY 127.0.0.1:%d PLAINTEXT" % port, flush=True)
client, address = listener.accept()
client.settimeout(10)
try:
    client.recv(4096)
    client.sendall(b"HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\nplain")
finally:
    client.close()
    listener.close()
print("SERVER_PASS=1", flush=True)
