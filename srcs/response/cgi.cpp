#include "cgi.hpp"


c_cgi::c_cgi(c_request &request)
{
    this->_socket_fd = request.get_socket_fd();
    this->_env_vars.push_back("REQUEST_METHOD=" + request.get_method());
    // this->_env_vars.push_back("SCRIPT_FILENAME=" + script_path);
    this->_env_vars.push_back("CONTENT_LENGTH=16");
    this->_env_vars.push_back("CONTENT_TYPE=" + request.get_header_value("Content-Type"));
    this->_env_vars.push_back("QUERY_STRING=" + request.get_query());
    this->_env_vars.push_back("SERVER_PROTOCOL=" + request.get_version());
    this->_env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    this->_env_vars.push_back("REMOTE_ADDR=" + request.get_ip_client());

    this->launch_cgi("www/cgi-bin/test.py", "/usr/bin/python3", request.get_body());
}

c_cgi::~c_cgi()
{
}

string  c_cgi::launch_cgi(const string &script_path, const string &interpreter, const string &body)
{
    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0)
    {
        cout << "(CGI): Error de pipe";
        return ("500 Internal server error");
    }

    pid_t   pid = fork();
    if (pid < 0)
    {
        cout << "(CGI): Error de fork";
        return ("500 Internal server error");        
    }

    /**** Processus enfant ****/
    if (pid == 0)
    {
        /* Redirection stdin depuis le pipe d'entree: permet au parent d'envoyer le body a l'enfant */
        dup2(in_pipe[0], STDIN_FILENO);
        close(in_pipe[1]);

        /* Redirection stdout vers le pipe de sortie: permet a l'enfant d'envoyer la sortie du CGI a l'enfant */
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[0]);
     
        /**** Convertir en tableau de char* pour execve ****/
        vector<char*> envp;
        for (size_t i = 0; i < this->_env_vars.size(); i++)
            envp.push_back(const_cast<char *>(this->_env_vars[i].c_str()));
        envp.push_back(NULL);

        char *argv[3];
        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(script_path.c_str());
        argv[2] = NULL;

        execve(interpreter.c_str(), argv, &envp[0]);
        cout << "(CGI) Error execve" << endl;
        exit(1);
    }
    else
    {
        /**** Processus parent ****/
        close(in_pipe[0]);
        close(out_pipe[1]);

        /* Envoyer le body au CGI */
        if (!body.empty())
            write(in_pipe[1], body.c_str(), body.size());
        close(in_pipe[1]); // signale EOF au script

        /* Lire la sortie du CGI */
        char buffer[BUFFER_SIZE];
        std::string response;
        ssize_t bytes_read;
        while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            send(this->_socket_fd, buffer, bytes_read, 0);
            // buffer[bytes_read] = '\0';
            // response += buffer;
        }
        close(out_pipe[0]);

        // Attendre la fin du process enfant
        int status;
        waitpid(pid, &status, 0);

        return response;
    }
}

