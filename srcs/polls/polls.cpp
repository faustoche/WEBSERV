#include "server.hpp"
#include "clients.hpp"

/*
* Vide la liste des pollfds
* Ajoute le socket serveur au début de pollfds pour surveiller les nouvelles connexions (événements pollin)
* Poarcours tous les clients actifs : ignorer ceux en disconnected, crée un pollfd pour chaque cloent, configure l'evenemtn attendu selon l'etat
* Ajoute les pollfd clients dans le vecteur
*/


void c_server::setup_pollfd()
{
	_poll_fds.clear();

	/**** INITIALISATION DES DONNÉES *****/
	/**** 1 SEUL PORT *****/
	struct pollfd server_pollfd;
	server_pollfd.fd = _socket_fd;
	server_pollfd.events = POLLIN; // evenements attendus et surveillés - POLLIN = données en attente de lecture
	server_pollfd.revents = 0; // evenement detectes et produits. 0 pour commencer
	_poll_fds.push_back(server_pollfd);

	/**** MULTIPLES PORTS *****/
	for (std::map<int, int>::iterator it = _multiple_ports.begin(); it != _multiple_ports.end(); it++)
	{
		struct pollfd server_pollfd;
		server_pollfd.fd = it->first; // ici socket_fd où on itere 
		server_pollfd.events = POLLIN;
		server_pollfd.revents = 0;
		_poll_fds.push_back(server_pollfd);
	}

	/**** AJOUT DES CLIENTS *****/
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

/*
* Appelle poll() avec un timeout de 1s et recupere le nombre d'evements prets
* SI erreur alors j'ffiche un message et je quitte
* Si aucun evenement, on quitte
* Pourcours tous les pollfd: ignore ceux sans evenements, 
	* si c'est le serveur: si polling alkors handle nouvelle connexions sinon logs d'erreur
	* si c'est un client: pollin = handle client read ou sinon pollout = handle client write
	* sinon erreur alors je remove client
*/

void c_server::handle_poll_events()
{
	int num_events = poll(&_poll_fds[0], _poll_fds.size(), 1000);
	if (num_events < 0)
	{
		cerr << "Error: Poll() failed\n"; 
		return ;
	}
	if (num_events == 0)
		return ;

	for (size_t i = 0; i < _poll_fds.size(); i++)
	{
		struct pollfd &pfd = _poll_fds[i];
		if (pfd.revents == 0)
			continue ;

		// ici, verifier si c'est un socket serveur ou pas 
		// creer une nouvelle fonction  pour verifier si la socket est en ecoute
			// nouvelle variable int port qui va prendre la donnee du socket concerne
			// print nouvelle connecion sur le port xxxx
			// handle new connections() -> on passe le socket fd a cette fonction


		/**** GESTION DEPUIS LA SOCKET SERVEUR POUR 1 PORT *****/
		if (i == 0)
		{
			if (pfd.revents & POLLIN)
				handle_new_connection();
			if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				cerr << "Error: Socket server\n";
				// gestion d'erreur du serveur, fermeture etc
			}
		}
		else
		{
			int client_fd = pfd.fd;
			if (pfd.revents & POLLIN)
				handle_client_read(client_fd);
			else if (pfd.revents & POLLOUT)
				handle_client_write(client_fd);
			else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
				remove_client(client_fd);
		}
	}
}

/*
* Boucle pour accepter toutes les connexions en attente (accept())
* Si plus de clients (EAGAIN | EWOULDBLOCK) -> on quitte la boucle
* Si autre erreur -> affichage des logs et quitte
* Pour chaque client accepté:
	* Passe le socket en mode non bloquant
	* Ajoute le client à la map
	* Log d'arrivée d'un nouveau client
*/

void	c_server::handle_new_connection()
{
	while (true)
	{
		socklen_t addrlen = sizeof(_socket_address);
		int client_fd = accept(_socket_fd, (struct sockaddr*)&_socket_address, &addrlen);
		if (client_fd < 0)
			break ;

		set_non_blocking(client_fd);
		add_client(client_fd);
		cout << "Nouvelle connexion acceptée : " << client_fd << endl;
	}
}

void	c_server::handle_new_connection_multiple(int listening_socket)
{
	struct sockaddr_in client_address;
	socklen_t client_len = sizeof(client_address);

	int client_fd = accept(listening_socket, (struct sockaddr *)&client_address, &client_len);
	if (client_fd < 0)
		return ;

	int port = jerecupereleportdelasocket();
	cout << "Nouveau client connecté sur le port " << port << endl;
	set_non_blocking(client_fd);

	c_client new_client;
	new_client.set_state(READING);
	_clients[client_fd] = new_client;
}

/*
* Vérifie le client
* Lit la requête via read_request
* Crée la réponse avec define_response_content
* Remplit le buffer d'écriture et passe l'état à SENDING
*/

void c_server::handle_client_read(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (!client)
		return ;

	c_request request;
	request.read_request(client_fd);

	c_response response;
	response.define_response_content(request, *this);
	client->get_write_buffer() = response.get_response();
	client->set_bytes_written(0);
	client->set_state(SENDING);
	cout << "Requête traitée pour le client " << client_fd << endl;
}

/*
* Recupère le client correspondant
* Vérifie le buffer de réponse et combien d'octet ont déjà été envoyés
* Si tout est déjà envoyé -> supprime le client
* Sinon:
	* envoie toutes les données restantes avec send()
	* si erreur non eagain/ewouyldblock -> log et supprime le client
	* sinin -> net a jour bytes_written
	* si tout le buffer est envoyé -> log et supprime le client
*/

void	c_server::handle_client_write(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
		return ;

	string write_buffer = client->get_write_buffer();
	size_t bytes_written = client->get_bytes_written();

	size_t remaining = write_buffer.length() - bytes_written;
	if (remaining == 0)
	{
		remove_client(client_fd);
		return ;
	}

	const char *data_to_send = write_buffer.c_str() + bytes_written;
	int bytes_sent = send(client_fd, data_to_send, remaining, 0);
	if (bytes_sent <= 0)
		return ;

	client->set_bytes_written(bytes_written + bytes_sent);
	if (client->get_bytes_written() >= write_buffer.length())
	{
		cout << "Réponse envoyé au client " << client_fd << endl;
		remove_client(client_fd);
	}
}

/*
* Récupère le client correspondant
* Récupère la requête brute du buffer du client
* Crée un objet crequest et le parse
* Crée un objet cresponse et construit une réponse (define response content)
* Stocke la réponse dans le buffer d'écriture du client
* Réinitialise bytes_written et met l'état du client à sending
* Log que la requête est traitée
*/

void c_server::process_client_request(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
		return ;

	string raw_request = client->get_read_buffer();
	c_request request;
	request.parse_request(raw_request);
	c_response response;
	response.define_response_content(request, *this);

	client->get_write_buffer() = response.get_response();
	client->set_bytes_written(0);
	client->set_state(SENDING);
	cout << "Requête traitée" << endl;
}