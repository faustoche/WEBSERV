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
    
        int parse_request(const string& str);
        int parse_start_line(string& str);
        int parse_headers(string& str);

    private:
        string              _method;
        string              _target;
        string              _version;
        map<string, string> _headers;
};