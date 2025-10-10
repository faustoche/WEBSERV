#!/usr/bin/python3
import sys

sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n");
sys.stdout.write("\r\n");

if len(sys.argv) < 2:
    print("Error: no argument.")
    sys.exit(1)

file_path = sys.argv[1]

try:
    with open(file_path, "r") as f:
        print(f.read())
except Exception as e:
    print(f"Error: cannot read file: {e}")


