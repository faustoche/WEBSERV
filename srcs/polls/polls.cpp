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
	// timeout de 1000 millisecondes = 1 seconde pour le moment, à tester
	// va retourner le nombre d'evenements (POLLIN POLLOUT etc) dans un poll()
	/**** VÉRIFICATION DES ÉVÉNEMENTS *****/
	int num_events = poll(_poll_fds.data(), _poll_fds.size(), 1000);
	if (num_events < 0) // nb d'evenemtn inferieur = erreur
	{
		cerr << "Error: Poll() failed\n"; 
		return ;
	}
	if (num_events == 0) // aucun evenement donc rien ne se passe
	{
		return ;
		// gestion des timeouts client
	}

	/**** BOUCLE À TRAVERS LES FDS *****/
	for (size_t i = 0; i < _poll_fds.size(); i++) // on boucle a travers tous les fd pour le reste
	{
		struct pollfd &pfd = _poll_fds[i];
		if (pfd.revents == 0)
			continue ;
		/**** GESTION DEPUIS LA SOCKET SERVEUR *****/
		if (i == 0)
		{
			if (pfd.revents && POLLIN)
			{
				handle_new_connection();
				return ;
			}
			if (pfd.revents && (POLLERR || POLLHUP || POLLNVAL))
			{
				cerr << "Error: Socket server\n";
				// gestion d'erreur du serveur, fermeture etc
			}
		}
		else
		{
			/**** SINON GESTION CÔTÉ SERVEURS CLIENTS *****/
			int client_fd = pfd.fd;
			if (pfd.revents && POLLIN)
			{
				return ;
				// gestion de la lecture client
			}
			else if (pfd.revents && POLLOUT)
			{
				return ;
				// gestion écriture client
			}
			else if (pfd.revents && (POLLERR || POLLHUP || POLLNVAL))
			{
				return ;
				// gestion de la déconnexion du (client_fd)
			}

		}
	}
}

void	c_server::handle_new_connection()
{
	// accepter toutes les connexions en attente
	while (true)
	{
		socklen_t addrlen = sizeof(_socket_address);
		int client_fd = accept(_socket_fd, (struct sockaddr*)&_socket_address, &addrlen);
		if (client_fd < 0)
		{
			// ressource temporairement unavailable
			if (errno == EAGAIN || errno == EWOULDBLOCK) // aucune connexion en attente 
				break ;
			else
			{
				cerr << "Error: Accept()\n";
				return ;
			}
		}
		// configuration des nouveaux clients
		set_non_blocking(client_fd);
		add_client(client_fd);
		cout << "Nouvelle connexion acceptée : " << client_fd << endl;
	}
}

void	c_server::handle_client_read(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
	{
		return ; // on ne retrouve pas le client
	}
	char buffer[BUFFER_SIZE];
	int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
	if (bytes_received <= 0)
	{
		cout << "Client fd déconnect e"
	}
}

void	c_server::handle_client_write()
{

}