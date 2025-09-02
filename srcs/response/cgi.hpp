#pragma once

#include "server.hpp"
#include <sys/wait.h>

class c_cgi
{
    public:
        c_cgi();
        c_cgi(c_request &request);
        ~c_cgi();

        string  launch_cgi(const string &script_path, const string &interpreter, const string &body);

    private:
        vector<string> _env_vars;
        int            _socket_fd; // pas necessaire une fois connecte a la classe reponse
};