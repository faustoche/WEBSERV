#!/usr/bin/php-cgi
<?php
// Définir le type de contenu (ne pas ajouter \r\n\r\n ici)
header("Content-Type: text/html; charset=UTF-8");
// Exemple de variables PHP utiles
$server = $_SERVER['SERVER_SOFTWARE'] ?? 'Serveur inconnu';
$time   = date("Y-m-d H:i:s");
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <title>Test CGI avec PHP</title>
</head>
<body>
    <h1>Page générée par PHP CGI</h1>
    <p><strong>Heure du serveur :</strong> <?= $time ?></p>
    <p>Ceci est un test simple pour vérifier que votre serveur HTTP gère bien le CGI avec PHP.</p>
</body>
</html>