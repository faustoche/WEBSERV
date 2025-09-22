#!/usr/bin/php-cgi
<?php
// En-tête HTTP obligatoire pour CGI
header("Content-Type: text/html; charset=UTF-8");


// Contenu HTML
echo "<!DOCTYPE html>";
echo "<html lang='fr'>";
echo "<head>";
echo "<meta charset='UTF-8'>";
echo "<title>Test PHP CGI</title>";
echo "</head>";
echo "<body>";
echo "<h1>Bonjour bonjour depuis PHP CGI !</h1>";
echo "<p>Ceci est une page HTML générée par un script PHP.</p>";
echo "</body>";
echo "</html>";
?>
