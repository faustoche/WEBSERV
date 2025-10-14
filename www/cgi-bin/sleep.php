#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html; charset=UTF-8");

ob_implicit_flush(true);
while (ob_get_level()) ob_end_flush();

?>
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8" />
  <title>Sleeping</title>
  <style>
    @font-face {
      font-family: "ComicNeue";
      src: url("/fonts/ComicNeue-Regular.otf") format("opentype");
      font-weight: 400;
      font-style: normal;
    }
    @font-face {
      font-family: "ComicNeue";
      src: url("../fonts/ComicNeue_Bold.otf") format("opentype");
      font-weight: 700;
      font-style: normal;
    }
    body{
      margin:0;
      font-family: "ComicNeue", cursive, sans-serif;
      font-weight: 700;
      background: url('https://www.transparenttextures.com/patterns/asfalt-dark.png');
      color:#fff;
      text-align:center;
    }
    h1{
      font-weight: 700;
      font-size:3rem;
      color:yellow;
      text-shadow:2px 2px 0 #000, 0 0 10px #ff00aa;
      margin-top:2rem;
    }
    p{
      color:rgb(56, 36, 122);
      font-size:1.2rem;
      margin:1rem 0;
    }
    img{
      margin-top:1rem;
      border:5px ridge #ff00aa;
      max-width:300px;
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
</head>
<body>
  <h1 id="title">★ Sleeping ★</h1>
  <p id="message">ZZzzZZzzZZZZzzzzzzzzzzz</p>
  <img id="image" src="https://www.helloanimaux.fr/wp-content/uploads/2022/01/chien-qui-dort.jpg" alt="Dog sleeping">

<?php

// Envoi immédiat du contenu initial
echo str_repeat(' ',1024); // force l’envoi sur certains serveurs
flush();

// Attente de 5 secondes
sleep(1000);

echo <<<HTML
<script>
  document.getElementById('title').textContent = "★ Done sleeping ★";
  document.getElementById('message').textContent = "The script is done sleeping, finally !";
  document.getElementById('image').src = "https://woufipedia.com/wp-content/uploads/2024/04/bulldog-9.webp";
  document.getElementById('image').alt = "Dog awake";
</script>
HTML;
?>
<p> </p>
<a href="/cgi.html" class="back-link">← Go back to the CGI page</a>
</body>
</html>
