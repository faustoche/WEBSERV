#include "server.hpp"
#include "clients.hpp"

void c_server::setup_pollfd()
{
	_poll_fds.clear(); // on vide le vecteur pour commencer à 0

	/**** INITIALISATION DES DONNÉES *****/
	/**** LE SERVEUR DE BASE DOIT ÊTRE EN PREMIER DANS NOTRE CONTAINER *****/
	struct pollfd server_pollfd;
	server_pollfd.fd = _socket_fd;
	server_pollfd.events = POLLIN; // evenements attendus et surveillés - POLLIN = données en attente de lecture
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

	/**** BOUCLE À TRAVERS LES POLLS QUI ONT DÉCLENCHÉ DES ÉVÉNEMENTS *****/
	for (size_t i = 0; i < _poll_fds.size(); i++) // on boucle a travers tous les fd pour le reste
	{
		struct pollfd &pfd = _poll_fds[i];
		if (pfd.revents == 0)
			continue ;
		/**** GESTION DEPUIS LA SOCKET SERVEUR *****/
		// SI C'est le serveur et qu'il y a une nouvelle connexion
		if (i == 0)
		{
			if (pfd.revents && POLLIN)
			{
				// on appelle cette fonction pour accéder les clients en attente
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
			// pour chaque client, on lit les données ou bien on les envois
			int client_fd = pfd.fd;
			if (pfd.revents && POLLIN)
				handle_client_read(client_fd);
			else if (pfd.revents && POLLOUT)
				handle_client_write(client_fd);
			else if (pfd.revents && (POLLERR || POLLHUP || POLLNVAL))
				remove_client(client_fd);
		}
	}
}

void	c_server::handle_new_connection()
{
	// boulce pour accepter toutes les connexions en attente
	while (true)
	{
		socklen_t addrlen = sizeof(_socket_address);
		int client_fd = accept(_socket_fd, (struct sockaddr*)&_socket_address, &addrlen);
		if (client_fd < 0)
		{
			// pas d;autres connexions a accepter-> on sort de la boucle
			if (errno == EAGAIN || errno == EWOULDBLOCK) // aucune connexion en attente 
				break ;
			else
			{
				cerr << "Error: Accept()\n";
				return ;
			}
		}
		// configuration des nouveaux clients
		set_non_blocking(client_fd); // OPTION socket non bloquante
		add_client(client_fd); // ajout a la map des clients
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
		if (bytes_received == 0) // le client a fermé la connexion
			cout << "Client " << client_fd << " déconnecté" << endl;
		else if (errno != EAGAIN && errno != EWOULDBLOCK)
			cerr << "Error: recv() on client " << client_fd << endl;
		remove_client(client_fd);
		return ;
	}
	// j'ajoute les données reçues au buffer du client
	buffer[bytes_received] = '\0';
	client->append_to_read_buffer(buffer);
	if (client->get_read_buffer().find("\r\n\r\n") != string::npos)
	{
		client->set_state(PROCESSING);
		process_client_request(client_fd); // on gere la requete client
	}
}

void	c_server::handle_client_write(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
		return ;
	// JE RECUPERE le buffer a envoyer et combien d;citet ont deja ete ecrits
	string write_buffer = client->get_write_buffer();
	size_t bytes_written = client->get_bytes_written();

	// on envoit les données restantes
	size_t remaining = write_buffer.length() - bytes_written;
	if (remaining == 0)
	{
		remove_client(client_fd);
		return ;
	}
	// mise a jour de bytes written,
	const char *data_to_send = write_buffer.c_str() + bytes_written;
	int bytes_sent = send(client_fd, data_to_send, remaining, 0);
	// une fois qu'il n'y a plus rien a envoyé, on peut supprimer le client
	if (bytes_sent <= 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			cerr << "Error: send() on client " << client_fd << endl;
			remove_client(client_fd);
		}
		return ;
	}
	// je verifie le nb d'cotet deja envoye, et l'addition me donne la position dans le buffer
	client->set_bytes_written(bytes_written + bytes_sent);
	if (client->get_bytes_written() >= write_buffer.length())
	{
		cout << "Réponse envoyé au client " << client_fd << endl;
		remove_client(client_fd);
	}
}

void c_server::process_client_request(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
		return ;
	// creation d'un obket request a partir du buffer du client
	string raw_request = client->get_read_buffer();
	c_request request(raw_request);
	// je cree une reponse base sur la requete
	c_response response;
	response.define_response_content(request);

	// on prépare la réponse à envoyer
	client->get_write_buffer() = response.get_response();
	client->set_bytes_written(0);
	client->set_state(SENDING);
	cout << "Requête traitée" << endl;
}