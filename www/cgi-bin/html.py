#!/usr/bin/python3
import os
import sys

sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n");
sys.stdout.write("\r\n");

print("<!DOCTYPE html>")
print("<html lang='fr'>")
print("<head><meta charset='UTF-8'><title>Test CGI Python</title></head>")
print("<body>")
print("<h1>Bonjour depuis Python CGI !</h1>")


print("<h2>Variables d'environnement CGI</h2>")
print("<ul>")
for key in [
    "REQUEST_METHOD", 
    "SCRIPT_NAME", 
    "SCRIPT_FILENAME", 
    "PATH_INFO", 
    "QUERY_STRING", 
    "REMOTE_ADDR", 
    ]:
    value = os.environ.get(key, "")
    if value:
        print(f"<li>{key} = {value}</li>")
print("</ul>")

print("</body></html>")
