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
        void    set_environment(const c_request &request);
        string  prepare_http_env_var(const c_request &request, string header_key);
        void    vectorize_env();

        // const c_location*   find_location(const string &path, map<string, c_location>& map_location);
        

    private:
        map<string, string> _map_env_vars;
        vector<string>      _vec_env_vars;
        int                 _socket_fd; // pas necessaire une fois connecte a la classe reponse
        const c_location*   _loc;
        string              _real_path;
        string              _interpreter;
};