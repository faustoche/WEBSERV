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
#include <vector>
#include <poll.h>
#include "response.hpp"
// #include "clients.hpp"
#include "request.hpp"
#include "location.hpp"
// #include "colors.hpp"

/************ DEFINE ************/

#define	BUFFER_SIZE 4096
using	namespace std;

/************ CLASS ************/

class c_location;

class c_server
{
private:
	// int						_socket_fd;
	// struct sockaddr_in		_socket_address;
	// map<int, c_client>		_clients;
	// vector<struct pollfd>	_poll_fds;

    // CONFIGURATION FILE
    // int                     _IP;
    // int                     _port;
    string                              _index;
    std::map<std::string, c_location>   location_map;
    // Defaults values ?
    static const int        DEFAULT_PORT = 80;
    static const string DEFAULT_ROOT;
    static const string DEFAULT_INDEX;

    // // Setters
    void                set_index_file(string & index);
    // // Validation
    // bool            is_valid() const;
    // string          get_validation_error() const;

public:
	// const int &get_socket_fd() const { return (_socket_fd); }
	// const struct sockaddr_in &get_socket_addr() const { return (_socket_address); }

	// void create_socket();
	// void bind_and_listen();
	// void set_non_blocking(int fd);

	// void		add_client(int client_fd);
	// void		remove_client(int client_fd);
	// // c_client	*find_client(int client_fd);
	// void		setup_pollfd(); // mise en place de poll()
	// void		handle_poll_events(); // gestion de chaque event lié à poll(), conserver le suivi

    // CONFIGURATION FILE
    // // Getters
    string const &      get_index_file() const;
};

/************ FUNCTIONS ************/

string int_to_string(int n);
