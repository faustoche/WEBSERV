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
#include "colors.hpp"

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
    // int                                 _ip;
    // int                                 _port;
    // int                                 _root; // root propre au serveur a definir en dur
    string                              _index; // recuperation du premier fichier valide dans le parsing (possible plusieurs fichiers) 
    std::map<std::string, c_location>   _location_map;

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
    // Setters
    void                set_index_file(string const & index);
    void                set_location(string const & path, c_location const & loc);
    // Getters
    string const &      get_index_file() const;

    // Debug
    void    print_config() const;
};

/************ FUNCTIONS ************/

string int_to_string(int n);
