#include "server.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_client::c_client() : _fd(-1), _state(DISCONNECTED)
{
	memset(_buffer, 0, sizeof(_buffer));
	gettimeofday(&_timestamp, 0);
}

c_client::c_client(int client_fd) : _fd(client_fd), _state(DISCONNECTED)
{
	memset(_buffer, 0, sizeof(_buffer));
	gettimeofday(&_timestamp, 0);
}

c_client::~c_client(){}

/************ CLIENT CREATION ************/

void	c_server::add_client(int client_fd)
{
	_clients[client_fd] = c_client(client_fd);
	set_non_blocking(client_fd);
}

void	c_server::remove_client(int client_fd)
{
	close(client_fd);
	_clients.erase(client_fd);
}

c_client	*c_server::find_client(int client_fd)
{
	map<int, c_client>::iterator it = _clients.find(client_fd);
	if (it != _clients.end())
		return &(it->second);
	return (NULL);
}
