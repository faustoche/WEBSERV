#!/usr/bin/php-cgi

<?php
header("Content-Type: text/html; charset=UTF-8");
ob_implicit_flush(true);
ob_end_flush(); 

echo "Sleeping...\n";
flush();

// Attente de 10 secondes
sleep(10);

echo "Done sleeping!\n";

?>