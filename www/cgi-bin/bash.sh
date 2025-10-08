#!/bin/bash

# Lire exactement le nombre d’octets spécifié par CONTENT_LENGTH
read -n "$CONTENT_LENGTH" POST_DATA

# En-têtes CGI
echo "Content-Type: text/plain"
echo ""

# Corps de la réponse
echo "Données reçues : $POST_DATA"

