#pragma once

/************ INCLUDES ************/

#include <iostream>
#include <map>
#include <string.h>
#include <sstream>
#include <vector>
#include <set>
#include <limits>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include "server.hpp"

using namespace std;

#define MAX_BODY_SIZE 1048576

class	c_server;
class   c_client;

/************ CLASS ************/

class c_request
{
	public:
		c_request(c_server& server, c_client &client);
		~c_request();

		// c_request const& operator=(const c_request& rhs);
	
		void	read_request();
		void	append_read_buffer(const char* buffer, ssize_t bytes);
		int		parse_request(const string& str);
		int		parse_start_line(string& str);
		int		parse_headers(string& str);
		void	determine_body_reading_strategy(int socket_fd);
		bool	has_end_of_headers(const std::vector<char> &buf);
		void 	read_body_with_length(int socket_fd);
		void	read_body_with_chunks(int socket_fd);
		int		fill_body_with_bytes(const char *buffer, size_t len);
		void	fill_body_with_chunks(string &accumulator);
		size_t	find_end_of_headers_pos(vector<char>& request);
		void	consume_read_buffer(size_t n);
		void	clear();

		void	check_required_headers();
		void	check_port();
		void	extract_body_part();
		bool	is_valid_header_value(string& key, const string& value);
		bool	is_limited() const { return _client_max_body_size > 0; };
		bool	is_uri_valid();
		
		void	print_full_request() const;
		void	init_request();

		const string		&get_method() const { return _method; }
		const string		&get_query() const { return _query; }
		const string		&get_target() const { return _target; }
		const string		&get_path() const { return _path; }
		const string		&get_version() const { return _version; }
		const int			&get_socket_fd() const { return _socket_fd; }
		const int			&get_status_code() const { return _status_code; }
		const int			&get_port() const { return _port; }
		const string		&get_ip_client() const { return _ip_client; }
		bool				get_has_body() {return _has_body; }
		bool				get_headers_parsed() { return _headers_parsed; };
		bool				get_error() {return _error; }
		bool				get_error() const {return _error; }
		bool				is_client_disconnected() { return _disconnected; };
		bool				is_request_fully_parsed() const { return _request_fully_parsed; };
		const size_t		&get_content_length() const { return _content_length; }
		const size_t		&get_total_bytes() const { return _total_bytes; };
		const string		&get_header_value(const string& key) const;
		const vector<char>	&get_body() const { return _body; }
		const size_t		&get_client_max_body_size() const { return _client_max_body_size; };
		void				set_status_code(int code);
		void				set_target(string target) { _target = target; };
		void				set_client_max_body_size(size_t bytes) { _client_max_body_size = bytes; };
		void				set_headers_parsed(bool state) { _headers_parsed = state; };
		void				set_request_fully_parsed(bool state) { _request_fully_parsed = state; };
		void				set_total_bytes(size_t bytes) { _total_bytes = bytes; };

		const map<string, string>	&get_headers() const { return _headers; }

	private:
		c_server&			_server;
		int					_socket_fd;
		string				_ip_client;

		vector<char>		_read_buffer;
		string				_method;
		string				_target;
		string				_version;
		string				_query;
		string				_path;
		vector<char>		_body;
		// string              _buffered_data;
		map<string, string>	_headers;
		c_client&			_client;

		int					_status_code;
		int					_port;
	
		bool				_headers_parsed;
		bool				_request_fully_parsed;
		bool				_error;
		bool				_has_body;
		bool				_disconnected;

		int					_chunk_line_count;
		string				_chunk_accumulator;
		size_t				_content_length;
		size_t				_total_bytes;
		size_t				_expected_chunk_size;
		size_t				_client_max_body_size;
};