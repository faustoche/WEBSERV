#include "server.hpp"
#include "clients.hpp"

void c_server::setup_pollfd()
{
	_poll_fds.clear(); // on vide le vecteur pour commencer à 0

	// initialisation des données de struct pollfd
	struct pollfd server_pollfd;
	server_pollfd.fd = _socket_fd;
	server_pollfd.events = POLLIN; // evenements attendus - POLLIN = données en attente de lecture
	server_pollfd.revents = 0; // evenement detectes et produits. 0 pour commencer

	// on ajoute notre server de base en premier dans notre container
	_poll_fds.push_back(server_pollfd);


	// creation des clients dans une boucle for
		// add client


	// switch case pour gerer les differents cas des clients
		// READING -> POLLIN attendre des données / break
		// PROCESSING -> 0 pas d'event en cours  / break
		// SENDING -> POLLOUT / break
}


void c_server::handle_poll_events()
{

}