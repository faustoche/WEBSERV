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

// FAIRE CONSTRUCTEUR ET OPERATEUR= // ! //

class c_server
{
private:
	int						_socket_fd;
	struct sockaddr_in		_socket_address;
	//map<int, c_client>		_clients;
	vector<struct pollfd>	_poll_fds;

	// CONFIGURATION FILE (completer)
    string							_ip; // reflechir si pas de directive listen -> valeur par defaut ?
    uint16_t                        _port; // dans RFC entier non signe de 16 bits + pas de negatif
    string                          _root; // root propre au serveur a definir en dur
    // string							_host;
	vector <string>                 _indexes; // plusieurs fichiers index possibles, il faut verifier la validite de l'index au moment de la requete & prendre le premier valide 
    vector <string>					_names;
	size_t							_body_size;
	map<string, c_location>   		_location_map;
	map<string, vector<int> >		_err_pages;
	// map<string, string>				_cgi_extension; // revoir la particularite des cgi ac directive dans le serveur 

	
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
	c_server();
	~c_server();
	c_server const &	operator=(c_server const & rhs);
    // Setters
	void								set_name(vector<string> const & names);
    void                				set_indexes(vector<string> const & index);
	void								set_port(uint16_t const & port);
	void								set_ip(string const & ip);
	void								set_root(string const & root); // A FAIRE
	void								set_err_pages(map<string, vector<int> > err_pages); // A FAIRE
	// void								set_cgi(map<string, string> const & cgi) {this->_cgi_extension = cgi;};
    void                				add_location(string const & path, c_location const & loc);
    // Getters
    vector<string> const &      		get_indexes() const { return (_indexes); };
	string const &						get_ip_adress() const {return (_ip); };
	uint16_t const &					get_port() const {return (_port); };
	string								get_root() const {return (_root);} ;
	vector<string> const &				get_name() const {return (_names); };
	size_t								get_body_size() const {return (_body_size); };
	map<string, vector<int> > const &	get_err_pages() const {return (_err_pages); };
	map<string, c_location> const &		get_location() const {return (_location_map); };
	// map<string, string>					get_cgi() const {return _cgi_extension; }; // pas de cgi dans le bloc server

    // Debug
    void    				print_config() const;
	void					print_indexes() const;
	void					print_names() const;
};

/************ FUNCTIONS ************/

string int_to_string(int n);
