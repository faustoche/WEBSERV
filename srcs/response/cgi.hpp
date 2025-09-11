#pragma once

#include "server.hpp"
#include <sys/wait.h>

class   c_response;
class   c_request;
class   c_location;

class c_cgi
{
    public:
        c_cgi();
        ~c_cgi();


        int             init_cgi(const c_request &request, const c_location &loc);
        string          launch_cgi(const string &body);
        int             resolve_cgi_paths(const c_location &loc, const string& filename);
        void            set_environment(const c_request &request);
        void            set_script_filename(const string& filename) { this->_script_filename = filename; };
        const string&   get_script_filename() const { return this->_script_filename; };
        void            get_header_from_cgi(c_response &response, const string& content_cgi);
        const string&   get_interpreter() const { return _interpreter; };
        int             parse_headers(c_response &response, string& headers);
        bool            is_valid_header_value(string& key, const string& value);
        void            vectorize_env();
        size_t          identify_script_type(const string& path);

    private:
        map<string, string> _map_env_vars;
        vector<string>      _vec_env_vars;
        int                 _socket_fd; // pas necessaire une fois connecte a la classe reponse
        int                 _status_code;
        const c_location*   _loc;
        string              _script_name;
        string              _path_info;
        string              _translated_path;
        string              _script_filename;
        string              _interpreter;
};