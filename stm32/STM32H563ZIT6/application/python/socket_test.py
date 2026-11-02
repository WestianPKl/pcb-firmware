import socket

HOST = "192.168.18.200"
PORT = 5001

with socket.create_connection((HOST, PORT), timeout=2) as s:
    s.sendall(b"hello")
    data = s.recv(1024)

    print(data)
