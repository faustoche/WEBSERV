#pragma once

/*-----------  INCLUDES -----------*/

#include <iostream>
#include <map>
#include <string.h>
#include <sstream>

using namespace std;

/*-----------  CLASS -----------*/

class c_request
{
    public:
        c_request(const string& str);
        ~c_request();
    
        int     parse_request(const string& str);
        int     parse_start_line(string& str);
        int     parse_headers(string& str);
        void    check_required_headers();

        const string &get_method() const { return _method; }
        const string &get_target() const { return _target; }
        const string &get_version() const { return _version; }
        int &get_status_code() {return _status_code; }
        const map<string, string> &get_headers() const { return _headers; }
        const string &get_header_value(const string& key) const;

    private:
        string              _method;
        string              _target;
        string              _version;
        map<string, string> _headers;
        int                 _status_code;
};