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

<<<<<<< HEAD
enum HostType 
{
    INVALID,
    HOSTNAME,
    IPV4,
    IPV6
};

/*-----------  CLASS -----------*/
=======
/************ CLASS ************/
>>>>>>> faustoche

class c_request
{
    public:
        c_request();
        ~c_request();
    
        int     read_request(int socket_fd);
        int     parse_request(const string& str);
        int     parse_start_line(string& str);
        int     parse_headers(string& str);
        void    fill_body_with_bytes(const char *buffer, size_t len);
        void    fill_body_with_chunks(string &accumulator);

        void    check_required_headers();
        void     check_port();
        bool    is_valid_header_value(string& key, const string& value);
        bool    is_valid_header_name(const string& key_name);

        string  ft_trim(const string& str);
        void    print_full_request();
        void    init_request();

        const string    &get_method() const { return _method; }
        const string    &get_target() const { return _target; }
        const string    &get_version() const { return _version; }
        const int       &get_status_code() const { return _status_code; }
        const int       &get_port() const { return _port; }
        bool            get_has_body() {return _has_body; }
        const size_t    &get_content_lentgh() const { return _content_length; }
        const string    &get_header_value(const string& key) const;
        const string    &get_body() const { return _body; }
        void            set_status_code(int code);

        const map<string, string> &get_headers() const { return _headers; }

    private:
        string              _method;
        string              _target;
        string              _version;
        string              _body;
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