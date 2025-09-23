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

/************ CLASS ************/

class c_request
{
    public:
        // c_request();
        c_request(c_server& server);
        // c_request(char* ip_str);

        ~c_request();
    
        void    read_request(int socket_fd);
        int     parse_request(const string& str);
        int     parse_start_line(string& str);
        int     parse_headers(string& str);
        void    determine_body_reading_strategy(int socket_fd, char* buffer, string request);
        // void    read_body_with_length(int socket_fd, char* buffer, string request);
        void    read_body_with_length(int socket_fd, char* buffer, string request, size_t buffer_size);
        void    read_body_with_chunks(int socket_fd, char* buffer, string request);
        void    fill_body_with_bytes(const char *buffer, size_t len);
        void    fill_body_with_chunks(string &accumulator);

        void    check_required_headers();
        void    check_port();
        bool    is_valid_header_value(string& key, const string& value);
        

        // string  ft_trim(const string& str);
        void    print_full_request() const;
        void    init_request();

        const string    &get_method() const { return _method; }
        const string    &get_query() const { return _query; }
        const string    &get_target() const { return _target; }
        const string    &get_path() const { return _path; }
        const string    &get_version() const { return _version; }
        const int       &get_socket_fd() const { return _socket_fd; }
        const int       &get_status_code() const { return _status_code; }
        const int       &get_port() const { return _port; }
        const string    &get_ip_client() const { return _ip_client; }
        bool            get_has_body() {return _has_body; }
        bool            get_error() {return _error; }
        const size_t    &get_content_length() const { return _content_length; }
        const string    &get_header_value(const string& key) const;
        const string    &get_body() const { return _body; }
        void            set_status_code(int code);

		const map<string, string> &get_headers() const { return _headers; }

    private:
        c_server&           _server;
        int                 _socket_fd;
        string              _ip_client;
        string              _method;
        string              _target;
        string              _version;
        string              _query;
        string              _path;
        string              _body;
        string              _buffered_data;
        map<string, string> _headers;

		int                 _status_code;
		int                 _port;
	
		bool                _request_fully_parsed;
		bool                _error;
		bool                _has_body;

		int                 _chunk_line_count;
		string              _chunk_accumulator;
		size_t              _content_length;
		size_t              _expected_chunk_size;
};