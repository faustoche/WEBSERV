#!/usr/bin/python3
import os, sys

sys.stdout.write("Content-Type: text/plain\r\n");
sys.stdout.write("\r\n");
for k, v in os.environ.items():
    print(f"{k} = {v}")
