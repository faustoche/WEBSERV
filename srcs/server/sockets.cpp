#include "server.hpp"

void c_server::create_socket_for_each_port(const std::vector<int>&ports)
{
	// je parcours la liste de port avec mon iterateur
		// 1. creation de la socket
		// je cree une variable socket fd = create socket
		// si la socket est invalide
			// j'affiche une erreur
			// j'affiche errno
			// je continue 
		
		// 2. configuration de l'option
		// j'applique mon option pour reutiliser l'adresse
		// si ca echoue
			// j'affiche une erreur + code d'erreur
			// je ferme le socket fd
			// je continue

		// 3. structure d'adresse
		// creer l'object socket adress type sockaddr in
		// tout initialise a 0
		// finet / hton(port), inadress any

		// 4. associer le soket a l'adresse et au port
		// resultat de la conf socket = bind
			// si echec alors errur et code d'erreur
			// fermer socker fd
			// continue
		
		// 5. mettre la socket en ecoute
		// resultat  = listen
		// si echec alors erreur et code d'erreur
		// fermer socker fd
		// continue

		// 6. non bloquand
		// set non blocking

		// 7. enregistrer le tout
	// fin de boucle
}