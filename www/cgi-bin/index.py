#!/usr/bin/python3
import sys

# Préparer tout le corps de la réponse
body = (
    """<h1>★ Hello from CGI! ★</h1>
    <style>
        @font-face {
            font-family: "ComicNeue";
            src: url("/fonts/ComicNeue-Regular.otf") 
            format("opentype");
            font-weight: 400;
            font-style: normal;
        }
        body {
            margin:0; 
            font-family: "ComicNeue", 
            cursive, sans-serif; 
            font-weight: 700; 
            background: url("https://www.transparenttextures.com/patterns/asfalt-dark.png"); 
            text-align:center;
        }
        h1 { 
            font-weight: 700; 
            font-size:3rem; 
            color:yellow; 
            text-shadow:2px 2px 0 #000, 0 0 10px #ff00aa; 
            margin-top:2rem;
        }
        .back-link {
            display: inline-block;
            background: rgb(21, 255, 0);
            margin-top: 1rem;
            color:rgb(0, 0, 0);
            text-decoration: none;
            border: 2px solid rgb(21, 255, 0);
            padding: 10px 20px;
            font-weight: 700;
        }
        .back-link:hover {
            background: rgb(252, 248, 17);
            border: 2px solid rgb(252, 248, 17);
            color: #000000;
        }
    </style>
    <body>
        <a href="javascript:history.back()" class="back-link">← Go back to the main page</a>
    </body"""
)

# Calculer la longueur en octets
content_length = len(body.encode("utf-8"))

sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n");
sys.stdout.write("Content-Length: {content_length}\r\n");
sys.stdout.write("\r\n");
sys.stdout.write(body);

