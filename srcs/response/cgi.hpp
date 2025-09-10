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
        c_cgi(const c_request &request, c_response &response, map<string, c_location>& map_location);
        ~c_cgi();


        void    init_cgi(const c_request &request, map<string, c_location>& map_location);
        string  launch_cgi(const string &body);
        void    resolve_cgi_paths(const c_request &request, const c_location* loc);
        void    set_environment(const c_request &request);
        void    get_header_from_cgi(c_response &response, const string& content_cgi);
        const string&   get_interpreter() const { return _interpreter; };
        int     parse_headers(c_response &response, string& headers);
        bool    is_valid_header_value(string& key, const string& value);
        void    vectorize_env();
        size_t  identify_script_type(const c_request &request);

    private:
        map<string, string> _map_env_vars;
        vector<string>      _vec_env_vars;
        int                 _socket_fd; // pas necessaire une fois connecte a la classe reponse
        const c_location*   _loc;
        string              _script_name;
        string              _path_info;
        string              _translated_path;
        string              _script_filename;
        string              _interpreter;
};