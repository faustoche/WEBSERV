#pragma once

#include "server.hpp"
#include <sys/wait.h>

class   c_response;
class   c_request;

class c_cgi
{
    public:
        c_cgi();
        c_cgi(const c_request &request, c_response &response);
        ~c_cgi();


        void    init_cgi(const c_request &request);
        string  launch_cgi(const string &script_path, const string &interpreter, const string &body);
        void    set_environment(const c_request &request);
        string  prepare_env_variable(const c_request &request, string header_key);
        

    private:
        vector<string> _env_vars;
        int            _socket_fd; // pas necessaire une fois connecte a la classe reponse
};