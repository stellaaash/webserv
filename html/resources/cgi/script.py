import os
import sys
import time

body = sys.stdin.read()
time.sleep(10)
print("Content-Type: text/plain")
print()
print("Hello from CGI")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING =", os.environ.get("QUERY_STRING"))
print("CONTENT_LENGTH =", os.environ.get("CONTENT_LENGTH"))
print("CONTENT_TYPE =", os.environ.get("CONTENT_TYPE"))
print("SCRIPT_FILENAME =", os.environ.get("SCRIPT_FILENAME"))
print("Body =", body)
