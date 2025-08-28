#include "server.hpp"
#include "clients.hpp"

void c_server::setup_pollfd()
{
	_poll_fds.clear(); // on vide le vecteur pour commencer à 0

	/**** INITIALISATION DES DONNÉES *****/
	/**** LE SERVEUR DE BASE DOIT ÊTRE EN PREMIER DANS NOTRE CONTAINER *****/
	struct pollfd server_pollfd;
	server_pollfd.fd = _socket_fd;
	server_pollfd.events = POLLIN; // evenements attendus - POLLIN = données en attente de lecture
	server_pollfd.revents = 0; // evenement detectes et produits. 0 pour commencer
	_poll_fds.push_back(server_pollfd);

	for (map<int, c_client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		/**** AJOUT DES CLIENTS ACTIFS *****/
		int client_fd = it->first; // recoit le descripteur du client concerne
		c_client &client = it->second; // ref vers l'objet client de la map
		if (client.get_state() == DISCONNECTED)
			continue;

		/***** POLLFD LOCAL POUR LE CLIENT *****/
		struct pollfd client_pollfd;
		client_pollfd.fd = client_fd; // fd devient le descripteur client
		client_pollfd.revents = 0;

		/***** SWITCH POUR AJUSTER SELON L'ÉTAT *****/
		// est-ce que je dois égqlment actualiser revent ou ça se fait automatiquement?
		switch (client.get_state())
		{
			case READING:
				client_pollfd.events = POLLIN;
				break;
			case PROCESSING:
				client_pollfd.events = 0;
				break;
			case SENDING:
				client_pollfd.events = POLLOUT; // ou 0? a tester 
				break;
			default:
				continue ;
		}
		_poll_fds.push_back(client_pollfd); // on push dans poll fds
	}
}

// je check chaque evenement pour voir l'avancer du poll
// si pas d'erreur et que notre fds est dispo alors on gere l'entree des nouvelles connexions
// sinon, on agit sur les sockets clients et donc on lit et on ecrit
	// ensuite on gere la partie deconnexion


void c_server::handle_poll_events()
{
	// 1. on appelle poll() et on lui met un timeout genre 1 secondes?
		// numero d'event = poll()

	// si le numero d'event est ibferieur a 0
		// erreur avec poll

	// si le numero d'event est 0
		// il se passe rien
		// peut-etre verifier les timeout de chaque client?
	
	// boucle for : on parcours les fd
		// on fait un pollfds[i]

		// si revents = 0
			// on continue
		
		// si i =0 ca veut dire qu'on est sur le socket serveur de base
			// si revennts n'est pas null (il y a des evenements) et que POLLIN n'esdt pas null
				// on fait une nouvelle connexion
			// si revents n'est pas null et que on recoit une erreur (pollerr poll hup)
				// gestion d'erreir
		// else
			// 

}