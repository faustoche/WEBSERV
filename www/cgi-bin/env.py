#!/usr/bin/python3
import os
import sys

# Entête HTTP obligatoire pour CGI
sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n")
sys.stdout.write("\r\n")

# Début du HTML
print("""
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8" />
  <title>HTML.PHP</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Comic+Neue:ital,wght@0,300;0,400;0,700;1,300;1,400;1,700&display=swap" rel="stylesheet">
  <style>
    body {
      margin:0;
      font-family: "Comic Neue", cursive, sans-serif;
      font-weight: 700;
      background: url('https://www.transparenttextures.com/patterns/asfalt-dark.png');
      color:#fff;
      text-align:center;
    }
    .container {
      max-width: 600px;
      margin: 2rem auto;
      background: rgba(0, 0, 0, 0.5);
      padding: 2rem;
      border: 3px solid #ff00aa;
      border-radius: 10px;
    }
    h1 {
      font-weight: 700;
      font-size:3rem;
      color:yellow;
      text-shadow:2px 2px 0 #000, 0 0 10px #ff00aa;
      margin-top:2rem;
    }
    p {
      color:rgb(56, 36, 122);
      font-size:1.2rem;
      margin:1rem 0;
    }
    img {
      margin-top:1rem;
      border:5px ridge #ff00aa;
      max-width:300px;
    }
    ul {
      text-align: left;
    }
      
  </style>
</head>
<body>
  <h1 id="title">★ Trying to do some HTML with CGI ★</h1>
  <img id="image" src="https://images.ctfassets.net/denf86kkcx7r/2Ghp9VQgpJxmXD14Bb3mva/43352c7e00f45cf55c464ef46f8d44ac/quelle-est-esperance-vie-chiens-13?fm=webp&w=913" alt="Dog sleeping">

  <div class="container">
    <h1>Environment variables !</h1>
    <ul>
""")

# Boucle Python pour afficher les variables CGI
for key in [
    "REQUEST_METHOD", 
    "SCRIPT_NAME", 
    "SCRIPT_FILENAME", 
    "PATH_INFO", 
    "QUERY_STRING", 
    "REMOTE_ADDR",
    "CONTENT-LENGT",
    "CONTENT_TYPE",
    "SERVER_PROTOCOL",
    "GATEWAY_INTERFACE",
    "REDIRECT_STATUS",
    "HTTP_ACCEPT",
    "HTTP_USER_AGENT",
    "HTTP_ACCEPT_LANGUAGE",
    "HTTP_COOKIE",
    "HTTP_REFERER"
    ]:
    value = os.environ.get(key, "")
    if value:
        print(f"<li>{key} = {value}</li>")
print("</ul>")

# Fin du HTML
print("""
    </ul>
  </div>
</body>
</html>
""")
