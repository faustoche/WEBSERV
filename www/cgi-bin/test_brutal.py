#!/usr/bin/env python3
import socket
import time

print("Connexion au serveur...")
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 8080))
print("Connecté!")

# Envoyer une requête partielle
print("Envoi de la requête partielle...")
sock.send(b'GET / HTTP/1.1\r\nHost: localhost\r\n')
time.sleep(0.1)

# Fermeture brutale (RST au lieu de FIN)
print("Fermeture brutale (RST)...")
sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b'\x01\x00\x00\x00\x00\x00\x00\x00')
sock.close()
print("Déconnecté brutalement!")