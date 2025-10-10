#!/usr/bin/python3

import sys

response_body = "Fast response\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Content-Length: " + str(len(response_body)) + "\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(response_body)
sys.stdout.flush()