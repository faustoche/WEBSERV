#pragma once

/*-----------  INCLUDES -----------*/

#include <iostream>
#include <map>
#include <string.h>
#include <sstream>
#include <set>
#include <limits>
#include <stdlib.h>

using namespace std;

#define MAX_BODY_SIZE 1048576

/*-----------  CLASS -----------*/

class c_request
{
    public:
        /* CONSTRUCTORS & DESTRUCTORS */
        c_request(const string& str);
        ~c_request();
    
        /* PARSING */
        int     parse_request(const string& str);
        int     parse_start_line(string& str);
        int     parse_headers(string& str);

        /* UTILS */
        void    check_required_headers();
        bool    is_valid_header_value(const string& key, const string& value);

        /* GETTERS & SETTERS */
        const string                &get_method() const { return _method; }
        const string                &get_target() const { return _target; }
        const string                &get_version() const { return _version; }
        const map<string, string>   &get_headers() const { return _headers; }
        const string                &get_header_value(const string& key) const;
        const string                &get_body() const { return _body; }
        const int                   &get_status_code() const { return _status_code; }
        const size_t                &get_content_lentgh() const { return _content_length; }
        void                        set_status_code(int code);
        void                        fill_body(const char *buffer, size_t len);

    private:
        string              _method;
        string              _target;
        string              _version;
        string              _body;
        map<string, string> _headers;
        int                 _status_code;
        size_t              _content_length;
};