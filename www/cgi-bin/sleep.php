#!/usr/bin/php-cgi

<?php
header("Content-type: text/html; charset=UTF-8");
header("Connection: close");

ob_implicit_flush(true);
ob_end_flush(); 

echo "Sleeping...\n";
flush();

// Attente de 5 secondes
sleep(5);

echo "Done sleeping!\n";
flush();
?>