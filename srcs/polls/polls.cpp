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
			case IDLE:
				client_pollfd.events = POLLIN;
			default:
				continue ;
		}

		_poll_fds.push_back(client_pollfd); // on push dans poll fds
	}

	for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); ++it) 
	{
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
	// D'abord vérifier les processus CGI terminés (avant poll)
    check_terminated_cgi_processes();

	int num_events = poll(_poll_fds.data(), _poll_fds.size(), 30000);
	if (num_events < 0)
	{
		close_all_sockets_and_fd();
		return ;
	}
	if (num_events == 0)
		return ;

	size_t n = _poll_fds.size();
	for (size_t i = 0; i < n; i++)
	{
		/*struct*/ pollfd pfd = _poll_fds[i];
		if (pfd.revents == 0)
			continue ;

		if (is_listening_socket(pfd.fd))
		{
			if (pfd.revents & POLLIN)
			{
				int port = get_port_from_socket(pfd.fd);
				cout << "Nouvelle connection sur le port " << port << endl;
				handle_new_connection(pfd.fd);
			}
			if (pfd.revents & (POLL_ERR | POLLHUP | POLLNVAL))
			{
				cerr << "Error: socket server on port " << get_port_from_socket(pfd.fd) << endl;
				close_all_sockets_and_fd();
			}
		}
		else
		{
			int	fd = pfd.fd;
			if (_active_cgi.count(fd))
			{
				c_cgi* cgi = _active_cgi[fd];
				c_client *client = find_client(cgi->get_client_fd());
				if (!client)
					continue ;

				// Ecriture vers CGI
				if ((pfd.revents & POLLOUT) && (fd == cgi->get_pipe_in()))
					handle_cgi_write(cgi);

				// Lecture vers CGI
				if ((pfd.revents & POLLIN) && (fd == cgi->get_pipe_out()))
					handle_cgi_read(cgi);

				// Gestion POLLHUP pour les pipes CGI
				if (pfd.revents & POLLHUP)
				{
					if (fd == cgi->get_pipe_out() && !client->get_response_complete())
					{
						handle_cgi_final_read(cgi->get_pipe_out(), cgi);
						return ;
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
			{
				handle_client_write(fd);
				if (client->get_write_buffer().empty())
    			{
    			    c_cgi* cgi = find_cgi_by_client(fd);
    			    if (cgi && cgi->is_finished())
    			    {
    			        cleanup_cgi(cgi);
    			        client->set_state(READING);
    			    }
    			}
			}
			else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				c_cgi* cgi = find_cgi_by_client(client->get_fd());
				if (cgi)
				{
					kill(cgi->get_pid(), SIGTERM);
					cleanup_cgi(cgi);
					close_all_sockets_and_fd();
				}
				remove_client(fd);
			}			
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
void	c_server::handle_new_connection(int listening_socket)
{
	vector<int> new_fds;
	while (true)
	{
		struct sockaddr_in client_address;
		socklen_t client_len = sizeof(client_address);

		int client_fd = accept(listening_socket, (struct sockaddr *)&client_address, &client_len);
		if (client_fd < 0)
			break ;
		set_non_blocking(client_fd);
		int port = get_port_from_socket(listening_socket);
		cout << GREEN << "\n✅ NEW CONNECTION FOR CLIENT : " << client_fd << " ON PORT : " << port << RESET << endl;
		c_client new_client(client_fd);
		new_client.set_state(READING);
		_clients[client_fd] = new_client;
	}
	for (size_t i = 0; i < new_fds.size(); i++)
		add_client(new_fds[i]);
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
	{
		std::cout << "Client not found ! " << __FILE__ << "/" << __LINE__ << endl;
		return ;
	}

	c_request request(*this);
	request.read_request(client_fd);
	
	if (request.is_client_disconnected())
	{
		close(client_fd);
		remove_client(client_fd);
		return ;
	}
	/* */
	request.print_full_request();
	c_response response(*this, client_fd);
	response.define_response_content(request);
	if (response.get_is_cgi())
	{
		client->set_state(PROCESSING);
		std::cout << PINK << "*Client " << client->get_fd() << " processing request*" << RESET << endl;
	}
	else
	{
		client->get_write_buffer() = response.get_response();
		client->set_state(SENDING);
		cout << PINK << "*Client " << client->get_fd() << " is ready to receive data : POLLOUT*" << RESET << endl;
	}
	client->set_bytes_written(0);
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
	{
		return ;
	}

	const string	&write_buffer = client->get_write_buffer();
	size_t bytes_written = client->get_bytes_written();

	size_t remaining = write_buffer.length() - bytes_written;
	if (remaining == 0)
		return ;

	const char *data_to_send = write_buffer.c_str() + bytes_written;
	size_t bytes_sent = send(client_fd, data_to_send, remaining, MSG_NOSIGNAL);
	if (bytes_sent <= 0)
	{
		cout << "⚠️ WARNING: problem while sending response to client" << client_fd << endl;
		close(client_fd);
		remove_client(client_fd);
		return ;
	}

	client->set_bytes_written(bytes_written + bytes_sent);

    if (client->get_bytes_written() >= write_buffer.length())
    {
        c_cgi *cgi = find_cgi_by_client(client_fd);
		if (cgi && !cgi->is_finished() && !client->get_response_complete())
		{ 
        	cout << "CGI " << cgi->get_pipe_out() << " linked to client " << client_fd << " is not finished..." << endl;
			return ;
		}

		// keep-alive
		if (!cgi || (client->get_response_complete() && cgi->is_finished()))
		{	
			cout << GREEN << "\n✅ RESPONSE FULLY SENT TO CLIENT " << client->get_fd() << RESET << endl << endl;
			cout << PINK << "*Client " << client->get_fd() << " can send a new request : POLLIN*" << RESET << endl;
			client->set_bytes_written(0);
			client->clear_read_buffer();
			client->clear_write_buffer();
			//client->set_response_complete(false);
			client->set_state(READING);
			return ;
		}
    }
}

// Il y a des choses a lire dans le pipe_out : POLLIN
void	c_server::handle_cgi_read(c_cgi *cgi)
{
	char	buffer[BUFFER_SIZE];
	ssize_t	bytes_read = read(cgi->get_pipe_out(), buffer, sizeof(buffer) - 1);

	buffer[bytes_read] = '\0';
	cgi->append_read_buffer(buffer, bytes_read);

	c_client *client = find_client(cgi->get_client_fd());
	if (!client)
		return ;

	if (bytes_read > 0)
	{
		if (!cgi->headers_parsed())
		{
			size_t pos = cgi->get_read_buffer().find("\r\n\r\n");
			if (pos != string ::npos)
			{
				string headers = cgi->get_read_buffer().substr(0, pos);
				fill_cgi_response_headers(headers, cgi);
				pos += 4;

				string initial_body = cgi->get_read_buffer().substr(pos);
				if (!initial_body.empty())
					fill_cgi_response_body(initial_body, cgi);
				client->set_state(SENDING);
				cgi->consume_read_buffer(cgi->get_read_buffer().size());
			}
			else
				return ;
		}
		else
		{
			fill_cgi_response_body(buffer, cgi);
			client->set_state(SENDING);
			cgi->consume_read_buffer(cgi->get_read_buffer().size());
		}
	}
}	

// Le CGI a ferme son pipe_out, on envoie tout ce qui a ete lu au client
void	c_server::handle_cgi_final_read(int fd, c_cgi* cgi)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	c_client *client = find_client(cgi->get_client_fd());
	if (client && !cgi->headers_parsed() && cgi->get_read_buffer().size() > 0)
	{
		cout << __FILE__ << "/" << __LINE__ << endl;
		fill_cgi_response_headers("", cgi);
		fill_cgi_response_body(cgi->get_read_buffer(), cgi);
	}

	while (client &&  (bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0)
	{
		if (cgi->get_content_length() == 0)
			transfer_with_chunks(cgi, buffer);
		else
		{
			buffer[bytes_read] = '\0';
			transfer_by_bytes(cgi, buffer);
		}
	}

	if (cgi->get_content_length() == 0 && !client->get_response_complete())
	{
		std::string chunk = int_to_hex(0) + "\r\n\r\n";
		client->get_write_buffer().append(chunk);
	}

	client->set_state(SENDING);
	cout << PINK << "*Client " << client->get_fd() << " is ready to receive the end of the response's body : POLLOUT*" << RESET << endl;

	cgi->consume_read_buffer(cgi->get_read_buffer().size());

	cout << YELLOW << "\n==FINAL RESPONSE TO BE SEND TO CLIENT " << client->get_fd() << "==" << RESET << endl;
	cout << client->get_write_buffer() << endl << endl;

	client->set_response_complete(true);
}

void	c_server::handle_cgi_write(c_cgi* cgi)
{
	ssize_t bytes = write(cgi->get_pipe_in(), 
						cgi->get_write_buffer().c_str() + cgi->get_bytes_written(),
						cgi->get_write_buffer().size() - cgi->get_bytes_written());
	
	if (bytes > 0)
		cgi->add_bytes_written(bytes);
	

	if (cgi->get_write_buffer().size() == cgi->get_bytes_written())
	{
		cgi->mark_request_fully_sent();
		_active_cgi.erase(cgi->get_pipe_in());
		remove_client(cgi->get_pipe_in());
		cout << "fd " << cgi->get_pipe_in() << " erased from _active_cgi" << endl;
		cgi->mark_stdin_closed();
	}	
}

void c_server::check_terminated_cgi_processes()
{
    pid_t pid;
    int status;
    
    // Vérifier tous les processus enfants terminés
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Trouver le CGI correspondant
        c_cgi* terminated_cgi = find_cgi_by_pid(pid);
		if (this->_active_cgi.find(terminated_cgi->get_pipe_out()) != _active_cgi.end())
		{
        	if (!terminated_cgi)
        	{
        	    cerr << "Warning: Unknown CGI process " << pid << " terminated\n";
        	    continue;
        	}
		
        	cout << "CGI process " << pid << " terminated";
		
        	// Analyser le statut de sortie
        	if (WIFEXITED(status))
        	{
        	    int exit_code = WEXITSTATUS(status);
        	    cout << " exited with code: " << exit_code << "\n";
        	    terminated_cgi->set_exit_status(exit_code);
				terminated_cgi->set_finished(true);
        	}
		}
		else
			return ;
    }
}

void c_server::cleanup_cgi(c_cgi* cgi) 
{
    if (!cgi) return;

    // 1. Fermer les pipes si encore ouverts
    if (cgi->get_pipe_in() > 0) 
	{
		_active_cgi.erase(cgi->get_pipe_in());
        remove_client(cgi->get_pipe_in());
		cout << "fd " << cgi->get_pipe_in() << " erased from _active_cgi" << endl;
		cgi->mark_stdin_closed();
		
    }
    if (cgi->get_pipe_out() > 0) 
	{
		 _active_cgi.erase(cgi->get_pipe_out());
        remove_client(cgi->get_pipe_out());
		cout << "fd " << cgi->get_pipe_out() << " erased from _active_cgi" << endl;
		cgi->mark_stdout_closed();
		
    }

    // 2. Attendre la fin du process enfant (si pas déjà récupéré)
    int status;
    waitpid(cgi->get_pid(), &status, WNOHANG);

	cout << "CGI with PID " << cgi->get_pid() << " cleaned !" << endl;

	// 4. Libérer la mémoire de l’objet
    delete cgi;

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