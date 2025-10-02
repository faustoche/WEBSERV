#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html; charset=UTF-8");
header("Connection: close");

ob_implicit_flush(true);
ob_end_flush(); 
?>
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8" />
  <title>Sleeping</title>
  <style>
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
      animation: flash 1s infinite alternate;
    }
    @keyframes flash{
      from{color:yellow}
      to{color:#ff66ff}
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
  </style>
</head>
<body>
  <h1>★ Sleeping ★</h1>

<?php
// Contenu avant le sleep
echo '<p>ZZzzZZzzZZZZzzzzzzzzzzz</p>';
echo '<img src="../images/sleeping_script.jpg" alt="Dog sleeping">';
flush();

// Pause de 30 secondes
sleep(5);
// Attente de 5 secondes
sleep(50);

// Contenu après le sleep (on remplace texte et image)
echo '<p>✔ Done sleeping !</p>';
echo '<img src="../images/awake_script.jpg" alt="Dog awake">';
?>
</body>
</html>
