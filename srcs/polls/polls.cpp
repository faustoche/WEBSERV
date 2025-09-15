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
			case IDLE:
				client_pollfd.events = POLLIN;
			default:
				continue ;
		}


		_poll_fds.push_back(client_pollfd); // on push dans poll fds
	}

	for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); ++it) {
    int fd = it->first;
    c_cgi *cgi = it->second;

    struct pollfd cgi_pollfd;
    cgi_pollfd.fd = fd;
    cgi_pollfd.revents = 0;

    if (fd == cgi->get_pipe_in())
        cgi_pollfd.events = POLLOUT; // on écrit vers le CGI
    else if (fd == cgi->get_pipe_out())
        cgi_pollfd.events = POLLIN;  // on lit la sortie du CGI

    _poll_fds.push_back(cgi_pollfd);
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
	int num_events = poll(_poll_fds.data(), _poll_fds.size(), 1000);
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
		
		/**** GESTION DEPUIS LA SOCKET SERVEUR *****/
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
			int	fd = pfd.fd;
			
			/* Si le fd fait partie d'un process cgi */
			if (_active_cgi.count(fd))
			{
				c_cgi* cgi = _active_cgi[fd];

				// Ecriture vers CGI
				if ((pfd.revents & POLLOUT) && (fd == cgi->get_pipe_in()))
				{
					ssize_t bytes = write(cgi->get_pipe_in(), 
										cgi->get_write_buffer().c_str() + cgi->get_bytes_written(),
										cgi->get_write_buffer().size() - cgi->get_bytes_written());
					if (bytes > 0)
						cgi->add_bytes_written(bytes);
					
					if (cgi->get_bytes_written() >= cgi->get_write_buffer().size())
					{
						close(cgi->get_pipe_in());
						remove_client(cgi->get_pipe_in());
					}
					
				}
				// Lecture vers CGI
				if ((pfd.revents & POLLIN) && (fd == cgi->get_pipe_out()))
				{
					char buffer[100];
    				std::string content_cgi;
    				ssize_t bytes_read = read(cgi->get_pipe_out(), buffer, sizeof(buffer));
					if (bytes_read > 0)
					{
					    cgi->append_read_buffer(buffer, bytes_read);
					
					    // Vérifier si on a déjà extrait les headers
					    if (!cgi->headers_parsed())
					    {
					        size_t pos = cgi->get_read_buffer().find("\r\n\r\n");
					        if (pos != string::npos)
					        {
					            // Séparer headers et premier morceau de body
					            string headers = cgi->get_read_buffer().substr(0, pos);
					            string initial_body = cgi->get_read_buffer().substr(pos + 4);
							
					            c_client *client = find_client(cgi->get_client_fd());
					            if (client)
					            {
					                // Construire la réponse HTTP avec headers
					                client->get_write_buffer().append("HTTP/1.1 200 OK\r\n");
					                client->get_write_buffer().append(headers);
					                client->get_write_buffer().append("\r\n\r\n");
								
					                // Ajouter le premier body s'il existe
					                if (!initial_body.empty())
					                    client->get_write_buffer().append(initial_body);
								
					                client->set_state(SENDING);
					            }
							
					            // Marquer headers comme déjà envoyés
					            cgi->set_headers_parsed(true);
							
					            // ⚠️ Nettoyer le buffer jusqu’à la fin de initial_body
					            cgi->consume_read_buffer(pos + 4 + initial_body.size());
					        }
					    }
					    else
					    {
					        // On est déjà en phase "body only"
					        c_client *client = find_client(cgi->get_client_fd());
					        if (client)
					        {
					            client->get_write_buffer().append(cgi->get_read_buffer());
					            client->set_state(SENDING);
					        }
						
       						// on a copié tout le contenu actuel -> on le supprime
       						 cgi->consume_read_buffer(cgi->get_read_buffer().size());
					    }
					}

					else if (bytes_read == 0)
					{
						cout << "on close la lecture du CGI" << endl;
						close(cgi->get_pipe_out());
						remove_client(cgi->get_pipe_out());
						cgi->set_finished(true);
						delete _active_cgi[fd];
						_active_cgi.erase(fd);
					}

				}
				continue;
			}
			c_client *client = find_client(fd);
			if (!client)
				continue;
			if (pfd.revents & POLLIN)
				handle_client_read(fd);
			else if (pfd.revents & POLLOUT)
				handle_client_write(fd);
			else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
				remove_client(fd);
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

	c_request request(*this);
	request.read_request(client_fd);

	c_response response(*this, client_fd);
	response.define_response_content(request);
	// client->get_write_buffer() = response.get_response();
	client->set_bytes_written(0);
	// SENDING OU PROCESSING ?
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

	string response_headers = "HTTP/1.1 200 OK\r\nServer: webserv/1.0\r\n";
	string write_buffer = response_headers + client->get_write_buffer();
	size_t bytes_written = client->get_bytes_written();

	size_t remaining = write_buffer.length() - bytes_written;
	// if (remaining == 0)
	// {
	// 	// remove_client(client_fd);
	// 	return ;
	// }

	const char *data_to_send = write_buffer.c_str() + bytes_written;
	int bytes_sent = send(client_fd, data_to_send, remaining, 0);
	if (bytes_sent <= 0)
		return ;

	client->set_bytes_written(bytes_written + bytes_sent);
    if (client->get_bytes_written() >= write_buffer.length())
    {
        // Vérifier si ce client est lié à un CGI terminé
        c_cgi *cgi = find_cgi_by_client(client_fd);
		if (cgi)
		{ 
			if (cgi->is_finished())
        	{
        	    cout << "Suppression du CGI lié au client " << client_fd << endl;
        	    _active_cgi.erase(cgi->get_pipe_out());
        	    delete cgi;
        	}
			else if (cgi && !cgi->is_finished())
        	{
        	    cout << "CGI lié au client " << client_fd << " n'est pas termine" << endl;
        	}
		}
		else
		{
			cout << "Réponse envoyée au client " << client_fd << endl;
        	// Ici, deux choix :
        	// - soit tu fermes la connexion (HTTP/1.0 ou sans keep-alive)
        	// - soit tu remets le client en attente pour une nouvelle requête
        	// remove_client(client_fd);
			client->set_state(IDLE);

		}
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

// void c_server::process_client_request(int client_fd)
// {
// 	c_client *client = find_client(client_fd);
// 	if (client == NULL)
// 		return ;

// 	string raw_request = client->get_read_buffer();
// 	c_request request;
// 	request.parse_request(raw_request);
// 	c_response response;
// 	response.define_response_content(request, *this);

// 	client->get_write_buffer() = response.get_response();
// 	client->set_bytes_written(0);
// 	client->set_state(SENDING);
// 	cout << "Requête traitée" << endl;
// }