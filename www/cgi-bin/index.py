#!/usr/bin/python3
import sys
sys.stdout.write("Content-Type: text/plain\r\n");
sys.stdout.write("Content-Length: 15\r\n");
sys.stdout.write("Connection: close\r\n");
sys.stdout.write("\r\n");
sys.stdout.write("Hello from CGI!")

