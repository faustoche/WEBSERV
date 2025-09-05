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
#include "clients.hpp"
#include "request.hpp"
#include "location.hpp"
#include "colors.hpp"

/************ DEFINE ************/

#define	BUFFER_SIZE 10
using	namespace std;

/************ CLASS ************/

class c_location;

class c_server
{
private:
	int						_socket_fd;
	struct sockaddr_in		_socket_address;
	//map<int, c_client>		_clients;
	vector<struct pollfd>	_poll_fds;

	// CONFIGURATION FILE (completer)
    string								_ip; // reflechir si pas de directive listen -> valeur par defaut ?
    uint16_t                            _port; // dans RFC entier non signe de 16 bits + pas de negatif
    // int                                 _root; // root propre au serveur a definir en dur
    string                              _index; // recuperation du premier fichier valide dans le parsing (possible plusieurs fichiers) 
    map<string, c_location>   			_location_map;
	
public:
	const int &get_socket_fd() const { return (_socket_fd); }
	const struct sockaddr_in &get_socket_addr() const { return (_socket_address); }
	
	void create_socket();
	void bind_and_listen();
	void set_non_blocking(int fd);

	void		add_client(int client_fd);
	void		remove_client(int client_fd);
	//c_client	*find_client(int client_fd);
	void		setup_pollfd();
	void		handle_poll_events();
	void		handle_new_connection();
	void		handle_client_read();
	void		handle_client_write();

	// CONFIGURATION FILE
    // Setters
    void                	set_index_file(string const & index);
	void					set_port(uint16_t const & port);
	void					set_ip(string const & ip);
    void                	add_location(string const & path, c_location const & loc);
    // Getters
    string const &      			get_index_file() const { return (_index); };
	string const &					get_ip_adress() const {return (_ip); };
	uint16_t const &				get_port() const {return (_port); };
	map<string, c_location> const &	get_location() const {return (_location_map); };

    // Debug
    void    				print_config() const;
};

/************ FUNCTIONS ************/

string int_to_string(int n);
