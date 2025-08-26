#pragma once

/************ INCLUDES ************/

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <map>

#include "response.hpp"
#include "clients.hpp"
#include "request.hpp"
#include "colors.hpp"

/************ DEFINE ************/

#define	BUFFER_SIZE 4096
using	namespace std;

/************ CLASS ************/

class c_server
{
private:
	int					_socket_fd;
	struct sockaddr_in	_socket_address;
	map<int, c_client>	_clients;
	
public:
	const int &get_socket_fd() const { return (_socket_fd); }
	const struct sockaddr_in &get_socket_addr() const { return (_socket_address); }
	
	void create_socket();
	void bind_and_listen();
	void set_non_blocking(int fd); // gestion des fds non bloquants

	void		add_client(int client_fd);
	void		remove_client(int client_fd);
	c_client	*find_client(int client_fd);
};

/************ FUNCTIONS ************/

string int_to_string(int n);