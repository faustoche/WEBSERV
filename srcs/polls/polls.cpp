#include "server.hpp"
#include "clients.hpp"

/* Check any new connections, go through active clients and create a new fd for each client. Add them to the vector.*/

void c_server::setup_pollfd()
{
	_poll_fds.clear();
	std::vector<int> to_remove;

	for (std::map<int, int>::iterator it = _multiple_ports.begin(); it != _multiple_ports.end(); it++)
	{
		struct pollfd server_pollfd;
		server_pollfd.fd = it->first;
		server_pollfd.events = POLLIN;
		server_pollfd.revents = 0;
		_poll_fds.push_back(server_pollfd);
	}

	for (map<int, c_client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		int client_fd = it->first;
		c_client &client = it->second;

		time_t now = time(NULL);
		int timeout_value = TIMEOUT;
		if ((client.get_state() == PROCESSING || client.get_state() == SENDING))
			timeout_value = TIMEOUT * 3;
		if (now - client.get_last_modified() > timeout_value)
		{
			log_message("[DEBUG] Client " + int_to_string(client.get_fd()) + " has timed out");
			to_remove.push_back(client_fd);
			continue ;
		}

		struct pollfd client_pollfd;
		client_pollfd.fd = client_fd;
		client_pollfd.revents = 0;

		switch (client.get_state())
		{
			case READING:
				client_pollfd.events = POLLIN;
				break;
			case PROCESSING:
				client_pollfd.events = 0;
				break;
			case SENDING:
				client_pollfd.events = POLLOUT;
				break;
			case IDLE:
				client_pollfd.events = 0;
			default:
				continue ;
		}

		_poll_fds.push_back(client_pollfd);
	}

	for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); ++it) 
	{
		int fd = it->first;
		c_cgi *cgi = it->second;

		struct pollfd cgi_pollfd;
		cgi_pollfd.fd = fd;
		cgi_pollfd.revents = 0;

		if (fd == cgi->get_pipe_in())
			cgi_pollfd.events = POLLOUT;
		else if (fd == cgi->get_pipe_out())
			cgi_pollfd.events = POLLIN;
		_poll_fds.push_back(cgi_pollfd);
	}

	for (size_t i = 0; i < to_remove.size(); i++)
	{
		c_cgi *cgi = find_cgi_by_client(to_remove[i]);
		if (cgi)
		{
			log_message("[INFO] Killing CGI pid " + int_to_string(cgi->get_pid()) 
							+ "linked to client" + int_to_string(to_remove[i]));
			kill(cgi->get_pid(), SIGTERM);
			cleanup_cgi(cgi);
		}
		log_message("[INFO] Closing client " + int_to_string(to_remove[i]) + " (timeout inactivity)");
		remove_client(to_remove[i]);
	}
}

/* Get the number of events ready back. Go through fds and handle client/server connections */

void c_server::handle_poll_events()
{
    check_terminated_cgi_processes();
	int num_events = poll(_poll_fds.data(), _poll_fds.size(), 100);
	if (num_events < 0)
	{
		if (errno == EINTR)
		{
			log_message("[INFO] Poll interrupted by signal");
			return ;
		}
		cout << RED_BOLD << endl << "Error: Poll failed. Stopping all servers." << RESET << endl;
		log_message("[ERROR] Poll failed. Stopping all server.");
		this->_fatal_error = 1;
		close(this->_socket_fd);
		return ;
	}
	if (num_events == 0)
		return ;

	size_t n = _poll_fds.size();
	for (size_t i = 0; i < n; i++)
	{
		pollfd pfd = _poll_fds[i];
		if (pfd.revents == 0)
			continue ;
		if (is_listening_socket(pfd.fd))
		{
			if (pfd.revents & POLLIN)
				handle_new_connection(pfd.fd);
			if (pfd.revents & (POLL_ERR | POLLHUP | POLLNVAL))
			{
				int port = get_port_from_socket(pfd.fd);
				log_message("[ERROR] error on socket server on port " + int_to_string(port));
				close_all_sockets_and_fd();
			}
		}
		else
		{
			int	fd = pfd.fd;
			if (_active_cgi.count(fd))
			{
				c_cgi* cgi = _active_cgi[fd];
				if (!cgi)
				{
					log_message("[DEBUG] CGI already finished, skipping poll event for fd " + int_to_string(fd));
					continue ;
				}
				c_client *client = find_client(cgi->get_client_fd());
				if (!client)
					continue ;

				if ((pfd.revents & POLLOUT) && (fd == cgi->get_pipe_in()))
					handle_cgi_write(cgi);

				if ((pfd.revents & POLLIN) && (fd == cgi->get_pipe_out()))
					handle_cgi_read(cgi);

				if (pfd.revents & POLLHUP)
				{
					if (fd == cgi->get_pipe_out() && !client->get_response_complete())
					{
						handle_cgi_final_read(cgi->get_pipe_out(), cgi);
						return ;
					}
					else
					{
						if (!cgi->is_finished())
							cgi->set_finished(true);
						client->set_state(SENDING);
						return ;
					}
				}
				continue ;
			}
			c_client *client = find_client(fd);
			if (!client)
				continue ;
			if (pfd.revents & 0 && time(NULL) - client->get_last_modified() > TIMEOUT)
			{
				log_message("[WARNING] Client " + int_to_string(fd) + " has timed out");
				remove_client(fd);
			}

			if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
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
			else if (pfd.revents & POLLIN)
				handle_client_read(fd);
			else if (pfd.revents & POLLOUT)
			{
				handle_client_write(fd);
				if (client->get_write_buffer().empty() || client->get_response_complete())
				{
					c_cgi* cgi = find_cgi_by_client(fd);
					if (cgi && cgi->is_finished())
					{
						cleanup_cgi(cgi);
						client->set_state(READING);
					}
				}
			}		
		}
	}
}

/* Accept all connections waiting. For each client, pass as non-blocking, add client to the map */

void	c_server::handle_new_connection(int listening_socket)
{
	vector<pair<int, string> > new_fds;
	while (true)
	{
		struct sockaddr_in client_address;
		socklen_t client_len = sizeof(client_address);
		
		int client_fd = accept(listening_socket, (struct sockaddr *)&client_address, &client_len);
		if (client_fd < 0)
			break ;

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);

		if (_clients.size() > MAX_CONNECTIONS)
		{
			c_client client(client_fd, client_ip);
			c_request  too_many_request(*this, client);
			too_many_request.init_request();

			c_response resp(*this, client);
			resp.build_error_response(503, too_many_request);

			const string &raw = resp.get_response();
			// interdiction de faire un send sans passer par poll() avant 
			send(client_fd, raw.c_str(), raw.size(), 0);
			log_message("[ERROR] Rejected client " + int_to_string(client_fd) + " with 503 (server is full)");
			close(client_fd);
			continue ;
		}

		set_non_blocking(client_fd);
		int port = get_port_from_socket(listening_socket);
		log_message("[INFO] ✅ NEW CONNECTION FOR CLIENT : " + int_to_string(client_fd) + " ON PORT : " + int_to_string(port));
		new_fds.push_back(make_pair(client_fd, string(client_ip)));
	}
	for (size_t i = 0; i < new_fds.size(); i++)
		add_client(new_fds[i].first, new_fds[i].second);
}

/* Check if the client exist, read the request and create the appropriate response. Fill the writing buffer */

void c_server::handle_client_read(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (!client)
	{
		log_message("[ERROR] Client not found : " + int_to_string(client_fd));
		return ;
	}

	c_request request(*this, *client);
	request.read_request();

	if (!request.is_request_fully_parsed())
	{
		log_message("[DEBUG] Request not fully parsed yet for client " + int_to_string(client_fd));
		return ;
	}
	if (client->get_state() == DISCONNECTED)
	{
		log_message("[INFO] Client " + int_to_string(client_fd) + " disconnected");
		remove_client(client_fd);
		return ;
	}
	if (client->get_state() != IDLE)
	{
		client->set_creation_time();
		client->set_last_request(request.get_method() + " " + request.get_target() + " " + request.get_version());
		c_response response(*this, *client);
		response.define_response_content(request);
		client->clear_read_buffer();
		if (response.get_is_cgi())
		{
			client->set_state(PROCESSING);
			log_message("[DEBUG] Client " + int_to_string(client->get_fd()) + " is processing request");
		}
		else
		{
			client->get_write_buffer() = response.get_response();
			client->set_state(SENDING);
			log_message("[DEBUG] Client " + int_to_string(client->get_fd()) + " is ready to receive the end of the response's body : POLLOUT");
		}
		client->set_bytes_written(0);
	}
}

// void c_server::handle_client_read(int client_fd)
// {
// 	c_client *client = find_client(client_fd);
// 	if (!client)
// 	{
// 		log_message("[ERROR] Client not found : " + int_to_string(client_fd));
// 		return ;
// 	}

// 	c_request request(*this, *client);
// 	request.read_request();

// 	if (!request.is_request_fully_parsed())
// 	{
// 		log_message("[DEBUG] Request not fully parsed yet for client " + int_to_string(client_fd));
// 		return ;
// 	}
// 	if (request.is_client_disconnected())
// 	{
// 		close(client_fd);
// 		remove_client(client_fd);
// 		return ;
// 	}
// 	if (client->get_state() != IDLE)
// 	{
// 		client->set_creation_time();
// 		client->set_last_request(request.get_method() + " " + request.get_target() + " " + request.get_version());
// 		c_response response(*this, *client);
// 		response.define_response_content(request);
// 		if (response.get_is_cgi())
// 		{
// 			client->set_state(PROCESSING);
// 			log_message("[DEBUG] Client " + int_to_string(client->get_fd()) + " is processing request");
// 		}
// 		else
// 		{
// 			client->get_write_buffer() = response.get_response();
// 			client->set_state(SENDING);
// 			log_message("[DEBUG] Client " + int_to_string(client->get_fd()) + " is ready to receive the end of the response's body : POLLOUT");
// 		}
// 		client->set_bytes_written(0);
// 	}
// }

/* Check response's buffer and number of bytes sent. If everything has been sent, delete client. If not, send what's left. */

void	c_server::handle_client_write(int client_fd)
{
	c_client *client = find_client(client_fd);
	if (client == NULL)
		return ;

	const string	&write_buffer = client->get_write_buffer();
	size_t bytes_written = client->get_bytes_written();

	c_cgi *cgi = find_cgi_by_client(client_fd);
	size_t remaining = write_buffer.length() - bytes_written;
	if (remaining == 0)
	{
		if (client->get_response_complete() && cgi->is_finished())
		{
			handle_fully_sent_response(client);
		}
		return;
	}

	const char *data_to_send = write_buffer.c_str() + bytes_written;
	size_t bytes_sent = send(client_fd, data_to_send, remaining, MSG_NOSIGNAL);
	if (bytes_sent <= 0)
	{
		log_message("[ERROR] problem while sending response to client " + int_to_string(client_fd));
		close(client_fd);
		remove_client(client_fd);
		return ;
	}
	else
		client->set_last_modified();

	client->set_bytes_written(bytes_written + bytes_sent);

	if (client->get_bytes_written() >= write_buffer.length())
	{	
		if (cgi && !cgi->is_finished() && !client->get_response_complete())
		{ 
			log_message("[DEBUG] CGI " + int_to_string(cgi->get_pipe_out()) + " linked to client " + int_to_string(client_fd) + " is not finished...");
			return ;
		}

		if (!cgi || (client->get_response_complete() && cgi->is_finished()))
		{
			handle_fully_sent_response(client);
		}
	}
}

/* Read CGI's out, analyze headers and add datas to the client for sending */

void	c_server::handle_cgi_read(c_cgi *cgi)
{
	char	buffer[BUFFER_SIZE];
	ssize_t	bytes_read = read(cgi->get_pipe_out(), buffer, BUFFER_SIZE);

	if (bytes_read <= 0)
		return ;

	cgi->append_read_buffer(buffer, bytes_read);

	c_client *client = find_client(cgi->get_client_fd());
	if (!client)
		return ;

	if (!cgi->headers_parsed())
	{
		size_t pos = cgi->get_read_buffer().find("\r\n\r\n");
		if (pos != string ::npos)
		{
			string read_buffer = cgi->get_read_buffer();
			string headers = read_buffer.substr(0, pos);
			fill_cgi_response_headers(headers, cgi);
			pos += 4;

			if (read_buffer.size() > pos)
				fill_cgi_response_body(read_buffer.data() + pos, read_buffer.size() - pos, cgi);
			client->set_state(SENDING);
			cgi->consume_read_buffer(cgi->get_read_buffer().size());
		}
		else
			return ;
	}
	else
	{
		fill_cgi_response_body(buffer, bytes_read, cgi);
		client->set_state(SENDING);
		cgi->consume_read_buffer(cgi->get_read_buffer().size());
	}
	
}	

/* CGI close its pipeline out. We send what's have been read to the client */

void	c_server::handle_cgi_final_read(int fd, c_cgi* cgi)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	c_client *client = find_client(cgi->get_client_fd());
	if (!client)
		return ;

	if (!cgi->headers_parsed() && cgi->get_read_buffer().size() > 0)
	{
		fill_cgi_response_headers("", cgi);
		fill_cgi_response_body(cgi->get_read_buffer().data(), cgi->get_read_buffer().size(), cgi);
	}

	while (true)
	{
		bytes_read = read(fd, buffer, BUFFER_SIZE);
		if (bytes_read < 0)
			return ;
		if (bytes_read == 0)
			break ;
		else
		{
			if (cgi->get_content_length() == 0)
				transfer_with_chunks(buffer, bytes_read, cgi);
			else
				transfer_by_bytes(cgi, buffer, bytes_read);
		}
	}

	if (cgi->get_content_length() == 0 && !client->get_response_complete())
	{
		std::string chunk = int_to_hex(0) + "\r\n\r\n";
		client->get_write_buffer().append(chunk);
	}
	client->set_state(SENDING);

	log_message("[DEBUG] Client " + int_to_string(client->get_fd()) + " is ready to receive the end of the response's body : POLLOUT");

	cgi->consume_read_buffer(cgi->get_read_buffer().size());
	
	client->set_response_complete(true);
	client->set_status_code(200);
}

/* Send request's body to CGI and close/clean the connections */

void	c_server::handle_cgi_write(c_cgi* cgi)
{
	if (cgi->is_body_fully_sent_to_cgi())
		return ;

	size_t	remaining = cgi->get_body_to_send().length() - cgi->get_body_sent();
	int 	fd = cgi->get_pipe_in();

	if (remaining == 0)
	{
		if (cgi->get_body_sent() > 0)
			log_message("[DEBUG] request body fully sent to CGI " + int_to_string(fd) + ". Closing pipe_in.");
		cgi->set_body_fully_sent_to_cgi();
		_active_cgi.erase(fd);
		remove_client(fd);
		log_message("[DEBUG] fd " + int_to_string(fd) + " erased from active_cgi");
		close(fd);
		cgi->mark_stdin_closed();
		return ;
	}

	ssize_t bytes = write(fd, cgi->get_body_to_send().c_str() + cgi->get_body_sent(), remaining);

	if (bytes < 0)
	{
		log_message("[WARNING] recv() returned < 0 for CGI " + int_to_string(fd) + ". Will retry on next POLLIN.");
		return;
	}

	cgi->add_body_sent(bytes);

}

/* Check if CGI are done and update status */

void c_server::check_terminated_cgi_processes()
{
	pid_t pid;
	int status;
	
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		c_cgi* terminated_cgi = find_cgi_by_pid(pid);
		if (this->_active_cgi.find(terminated_cgi->get_pipe_out()) != _active_cgi.end())
		{
			if (!terminated_cgi)
			{
				log_message("[WARN] Unkown CGI pid " + int_to_string(pid));
				continue;
			}
		
			if (WIFEXITED(status))
			{
				int exit_code = WEXITSTATUS(status);
				log_message("[DEBUG] CGI process " + int_to_string(pid) + " terminated exited with code: " + int_to_string(exit_code));
				terminated_cgi->set_exit_status(exit_code);
				terminated_cgi->set_finished(true);
			}
		}
		else
			return ;
	}
}

/* Free all the ressources linked to CGI */

void c_server::cleanup_cgi(c_cgi* cgi) 
{
	if (!cgi) return;

	if (cgi->get_pipe_in() > 0) 
	{
		if (_active_cgi.find(cgi->get_pipe_in()) != _active_cgi.end())
			_active_cgi.erase(cgi->get_pipe_in());
		remove_client(cgi->get_pipe_in());
		log_message("[DEBUG] fd " + int_to_string(cgi->get_pipe_in()) + " erased from active_cgi");
		cgi->mark_stdin_closed();
	}

	if (cgi->get_pipe_out() > 0) 
	{
		if (_active_cgi.find(cgi->get_pipe_out()) != _active_cgi.end())
			_active_cgi.erase(cgi->get_pipe_out());		
		remove_client(cgi->get_pipe_out());
		log_message("[DEBUG] fd " + int_to_string(cgi->get_pipe_out()) + " erased from active_cgi");
		cgi->mark_stdout_closed();	
	}

	int status;
	waitpid(cgi->get_pid(), &status, WNOHANG); 
	log_message("[DEBUG] CGI with PID " + int_to_string(cgi->get_pid()) + " cleaned !");

	delete cgi;
}