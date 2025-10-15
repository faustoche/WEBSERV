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
#include <iomanip>
#include <sstream>
#include <map>
#include <vector>
#include <poll.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "response.hpp"
#include "clients.hpp"
#include "request.hpp"
#include "location.hpp"
#include "cgi.hpp"
#include "colors.hpp"

/************ DEFINE ************/

#define	BUFFER_SIZE 4096
#define TIMEOUT 10
using	namespace std;

/************ CLASS ************/

class c_location;
class c_client;

class c_server
{
private:
	int						_socket_fd;
	struct sockaddr_in		_socket_address;
	map<int, c_client>		_clients;
	vector<struct pollfd>	_poll_fds;
	map<int, c_cgi*>		_active_cgi;
	map<int, int>			_multiple_ports;
	bool					_fatal_error;

	string							_ip;
	vector<int>						_ports;
	string                          _root;
	vector <string>                 _indexes;
	vector <string>					_names;
	size_t							_body_size;
	map<string, c_location>   		_location_map;
	map<int, string>				_err_pages;

public:
	c_server();
	~c_server();
	c_server const &	operator=(c_server const & rhs);
		
	int								get_size_pollfd() const { return _poll_fds.size(); };
	void 							set_active_cgi(int key_fd, c_cgi* cgi);
	void 							add_fd(int fd, short events);
	void 							remove_client_from_pollout(int client_fd);
	const int						&get_socket_fd() const { return (_socket_fd); };
	const struct sockaddr_in		&get_socket_addr() const { return (_socket_address); };
	const map<string, c_location>	&get_location_map() const { return _location_map; };
	const bool						&get_fatal_error() const { return _fatal_error; };

	void		create_socket_for_each_port(const std::vector<int>& ports);
	int			get_port_from_socket(int socket_fd);
	bool		is_listening_socket(int fd);

	void 		set_non_blocking(int fd);
	void		add_client(int client_fd, string client_ip);
	void		remove_client(int client_fd);
	c_client	*find_client(int client_fd);
	void		setup_pollfd();
	void		handle_poll_events();
	void		handle_new_connection(int listening_socket);
	void		handle_client_read(int client_fd);
	void		handle_client_write(int client_fd);
	void		handle_cgi_write(c_cgi* cgi);
	void		handle_cgi_read(c_cgi* cgi);
	void		transfer_by_bytes(c_cgi *cgi, const string& buffer, size_t bytes);
	void 		transfer_with_chunks(const char *buffer, size_t bytes, c_cgi *cgi);
	void		process_client_request(int client_fd);
	void		fill_cgi_response_headers(string headers, c_cgi *cgi);
	void		fill_cgi_response_body(const char* buffer, size_t bytes, c_cgi *cgi);
	void		cleanup_cgi(c_cgi* cgi);
	void 		clear_read_buffer();
	void		log_message(const string& message);
	void		log_access(c_client *client);

	c_location	*find_matching_location(const string &request_path);
	bool		is_method_allowed(const c_location *location, const string &method);
	string		convert_url_to_file_path(c_location *location, const string &request_path, const string &default_root);
	c_cgi		*find_cgi_by_client(int client_fd);
	c_cgi 		*find_cgi_by_pid(pid_t pid);
	int			find_client_fd_by_cgi(c_cgi* cgi);
	size_t		extract_content_length(string headers);
	void		check_terminated_cgi_processes();
	void 		handle_cgi_final_read(int fd, c_cgi* cgi);
	void 		handle_fully_sent_response(c_client *client);
	void		close_all_sockets_and_fd(void);

	void								add_port(int const & port);
	void								set_ip(string const & ip);
	void                				add_location(string const & path, c_location const & loc);
	void								set_name(vector<string> const & names);
	void                				set_indexes(vector<string> const & index);
	void								set_body_size(size_t const & size){_body_size = size; };
	void								add_error_page(vector<int> const & codes, string path);
	void								set_root(string const & root);
	
	vector<string> const &      		get_indexes() const {return (_indexes); };
	string const &						get_ip_adress() const {return (_ip); };
	vector<int> const &					get_ports() const {return (_ports); };
	string								get_root() const {return (_root);} ;
	vector<string> const &				get_name() const {return (_names); };
	size_t								get_body_size() const {return (_body_size); };
	map<int, string> const &			get_err_pages() const {return (_err_pages); };
	map<string, c_location> const &		get_location() const {return (_location_map); };

	void    				print_config() const;
	void					print_indexes() const;
	void					print_names() const;
	void					print_error_page() const;
	void					print_ports() const;
};

/************ FUNCTIONS ************/

string 		int_to_string(int n);
string 		int_to_hex(size_t value);
bool   		is_valid_header_name(const string& key_name);
string 		ft_trim(const string& str);
string		get_valid_index(const string & root, vector<string> const & indexes);
bool		is_executable_file(const string & path);
bool        is_existing_file(const string & path);
bool		is_readable_file(const string & path);
string 		trim(const string &str);
string		trim_underscore(const string &str);
bool		create_directory(const string &path);
bool		directory_exists(const string &path);
bool		file_exists(const std::string &path);
string		url_decode(const string &str);
string		escape_html(const string &text);
string		escape_html_attr(const string &text);