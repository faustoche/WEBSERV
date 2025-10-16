#include "clients.hpp"
#include "server.hpp"
#include <cstring>
#include <unistd.h>

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_client::c_client(c_server &server, int client_fd, string client_ip) : _server(server), _fd(client_fd), _client_ip(client_ip), _state(READING)
{
	memset(_buffer, 0, sizeof(_buffer));
	gettimeofday(&_timestamp, 0);
	_response_body_size = 0;
	_response_complete = false;
	_read_buffer.clear();
	_write_buffer.clear();
	_bytes_written = 0;
	_creation_time = time(NULL);
	_last_modified = time(NULL);
	_last_request.clear();
	_status_code = 0;

	_request = new c_request(_server, *this);
    _response = new c_response(_server, *this);
}

c_client::~c_client() 
{
	delete	_request;
	delete	_response;
}

/************ FUNCTIONS ************/

/* Add a file descriptor to the list handled by poll */

void    c_server::add_fd(int fd, short events)
{
	struct pollfd tmp_poll_fd;
	tmp_poll_fd.fd = fd;
	tmp_poll_fd.events = events;
	tmp_poll_fd.revents = 0;

	_poll_fds.push_back(tmp_poll_fd);
	log_message("[DEBUG] fd " + int_to_string(fd) + " is now monitored by _poll_fds");
}

/* Creates new client. Set its socket to non-blocking */

void c_server::add_client(int client_fd, string client_ip)
{
	// Vérifier que le client n'existe pas déjà
    if (_clients.find(client_fd) != _clients.end())
    {
        log_message("[WARN] Client " + int_to_string(client_fd) + " already exists!");
        return;
    }

	_clients[client_fd] = new c_client(*this, client_fd, client_ip);

	set_non_blocking(client_fd);
	add_fd(client_fd, POLLIN);
	log_message("[DEBUG] ✅ Added client " + int_to_string(client_fd) + " (total clients: " + int_to_string(_clients.size()) + ")");
	log_message("[DEBUG] Client " + int_to_string(client_fd) + " can send a request : POLLIN");
}

/* Removes a client's fd from poll list, erase the fd from clients map and close the connection */

void c_server::remove_client(int client_fd)
{
	if (client_fd < 0)
		return ;
   
	for (vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); it++)
	{
		if (it->fd == client_fd)
		{
			_poll_fds.erase(it);
			log_message("[DEBUG] fd " + int_to_string(client_fd) + " is no longer monitored by _poll_fds");
			break;
		}
	}
	
	map<int, c_client*>::iterator it = _clients.find(client_fd);
	if (it != _clients.end())
	{
		_clients.erase(it);
		log_message("[DEBUG] fd " + int_to_string(client_fd) + " erased from _clients");
	}
	 close(client_fd);
	
}

c_client *c_server::find_client(int client_fd)
{
	map<int, c_client*>::iterator it = _clients.find(client_fd);
	if (it != _clients.end())
		return (it->second);
	return (NULL);
}

/* Find the specific CHI process linked to the specific client */

c_cgi   *c_server::find_cgi_by_client(int client_fd)
{
	for (map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); it++)
	{
		if (it->second->get_client_fd() == client_fd)
			return (it->second);
	}
	return (NULL);
}

/* Return client's fd linked to the given CGI */

int c_server::find_client_fd_by_cgi(c_cgi* cgi)
{
	for (map<int, c_client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->get_fd() == cgi->get_client_fd())
			return (it->second->get_fd());
	}
	return (-1);
}

c_cgi   *c_server::find_cgi_by_pid(pid_t pid)
{
	for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); 
		 it != _active_cgi.end(); ++it)
	{
		if (it->second && it->second->get_pid() == pid)
		{
			return it->second;
		}
	}
	return (NULL);
}
