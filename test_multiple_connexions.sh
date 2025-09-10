#!/bin/bash

echo "🔥 STRESS TEST - CLIENTS MULTIPLES"
echo "=================================="

PORT=8080
HOST=localhost

# Test 1: Beaucoup de clients simultanés
echo "🚀 Test 1: 20 clients simultanés"
time (
    for i in {1..20}; do
        curl -s -o /dev/null -w "Client $i: %{http_code} en %{time_total}s\n" http://$HOST:$PORT/ &
    done
    wait
)

echo ""
echo "🚀 Test 2: Connexions rapides successives"
time (
    for i in {1..50}; do
        curl -s -o /dev/null http://$HOST:$PORT/
        echo -n "."
    done
)
echo " ✅ 50 requêtes séquentielles terminées"

echo ""
echo "🚀 Test 3: Mélange POST/GET simultanés"
(
    # Clients GET
    for i in {1..5}; do
        curl -s -X GET http://$HOST:$PORT/ > /dev/null &
    done
    
    # Clients POST
    for i in {1..3}; do
        curl -s -X POST -d "test=data" http://$HOST:$PORT/ > /dev/null &
    done
    
    wait
    echo "✅ Mélange GET/POST terminé"
) &

wait
echo ""
echo "🏁 STRESS TEST TERMINÉ - Vérifiez les logs du serveur !"