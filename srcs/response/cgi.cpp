#include "cgi.hpp"

c_cgi::c_cgi() : _loc(NULL), _real_path(""), _interpreter("")
{
    this->_map_env_vars.clear();
    this->_vec_env_vars.clear();
}

c_cgi::c_cgi(const c_request &request, c_response &response, map<string, c_location>& map_location)
: _loc(NULL), _real_path(""), _interpreter("")
{
    (void)response;

    this->_map_env_vars.clear();
    this->_vec_env_vars.clear();

    init_cgi(request, map_location);
}

c_cgi::~c_cgi()
{
}

map<string, c_location>::const_iterator   find_location(const string &path, map<string, c_location>& map_location)
{
    // const c_location* best_match = NULL;
    map<string, c_location>::const_iterator best_match = map_location.end();
    size_t  max_len = 0;

    for (map<string, c_location>::const_iterator it = map_location.begin(); it != map_location.end(); it++)
    {
        const string& loc_path = it->first;
        if (path.compare(0, loc_path.size(), loc_path) == 0 && loc_path.size() > max_len)
        {
            best_match = it;
            max_len = loc_path.size();
        }
    }
    return (best_match);
}

string  find_extension(const string& real_path)
{
    size_t dot_position = real_path.find_last_of('.');

    if (dot_position != string::npos)
    {
        string extension = real_path.substr(dot_position);
        return (extension);
    }
    return (NULL);
}

void    c_cgi::init_cgi(const c_request &request, map<string, c_location>& map_location)
{
    /* Recherche de la location correspondante au path de la requete */
    string  path = request.get_path();
    map<string, c_location>::const_iterator it = find_location(path, map_location);
    if (it == map_location.end()) 
    {
        std::cerr << "Error: no location matched path " << path << std::endl;
        return;
    }
    this->_loc = &it->second;
    this->_loc->print_location();

    /* Construction du real_path du script */
    size_t pos = path.rfind(this->_loc->get_url_key());
    this->_real_path = this->_loc->get_root() + path.substr(pos + this->_loc->get_url_key().size());

    /* Recherche de l'interpreteur de fichier selon le langage identifie */
    string extension = find_extension(this->_real_path);
    this->_interpreter = this->_loc->get_cgi_extension().at(extension);

    /* Construction de l'environnement pour l'execution du script */
    this->set_environment(request);
}

void  c_cgi::vectorize_env()
{
    map<string, string> map_copy = this->_map_env_vars;

    for (map<string, string>::const_iterator it = map_copy.begin(); it != map_copy.end(); it++)
        this->_vec_env_vars.push_back(it->first + "=" + it->second);
}

void    c_cgi::set_environment(const c_request &request)
{
    /* BIEN SE CALER POUR COMPRENDRE LES VAR SCRIPT_NAME / PATH_INFO ET TRANSLATED_PATH*/
    this->_socket_fd = request.get_socket_fd();
    this->_map_env_vars["REQUEST_METHOD"] = request.get_method();
    // alias de la location + script_name - nom du fchier + path_info
    this->_map_env_vars["TRANSLATED_PATH"] = this->_real_path;
    // this->_map_env_vars.push_back("SCRIPT_NAME=WARNING A AJOUTER");
    this->_map_env_vars["CONTENT-LENGTH"] = int_to_string(request.get_content_length());
    this->_map_env_vars["CONTENT_TYPE"] = request.get_header_value("Content-Type");
    this->_map_env_vars["QUERY_STRING"] = request.get_query();
    this->_map_env_vars["SERVER_PROTOCOL"] = request.get_version();
    this->_map_env_vars["GATEWAY_INTERFACE"] = "CGI/1.1";
    this->_map_env_vars["REMOTE_ADDR"] = request.get_ip_client();

    this->_map_env_vars["HTTP_ACCEPT"] = request.get_header_value("Accept");
    this->_map_env_vars["HTTP_USER_AGENT"] = request.get_header_value("User-Agent");
    this->_map_env_vars["HTTP_ACCEPT_LANGUAGE"] = request.get_header_value("Accept-Language");
    this->_map_env_vars["HTTP_COOKIE"] = request.get_header_value("Cookie");
    this->_map_env_vars["HTTP_REFERER"] = request.get_header_value("Referer");

    this->vectorize_env();

}

string    c_cgi::prepare_http_env_var(const c_request &request, string header_key)
{
    string tmp;

    for (size_t i = 0; i < header_key.size(); i++)
    {
        if (isalpha(header_key[i]))
            tmp += toupper(header_key[i]);
        else if (header_key[i] == '-')
            tmp += '_';
    }

    tmp += "=";
    tmp += request.get_header_value(header_key);

    return (tmp);  
}


/* interpreter = loc->_cgi_extension["extension"]*/
string  c_cgi::launch_cgi(const string &body)
{
    int server_to_cgi[2];
    int cgi_to_server[2];

    if (pipe(server_to_cgi) < 0 || pipe(cgi_to_server) < 0)
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
        /* Redirection stdin depuis le pipe d'entree: permet au parent server le body au cgi */
        // fcntl(server_to_cgi[0], F_SETFL, O_NONBLOCK);
        dup2(server_to_cgi[0], STDIN_FILENO);
        close(server_to_cgi[1]);

        /* Redirection stdout vers le pipe de sortie: permet au cgi de renvoyer la reponse au server */
        // fcntl(cgi_to_server[1], F_SETFL, O_NONBLOCK);
        dup2(cgi_to_server[1], STDOUT_FILENO);
        close(cgi_to_server[0]);
     
        /**** Convertir en tableau de char* pour execve ****/
        vector<char*> envp;
        for (size_t i = 0; i < this->_vec_env_vars.size(); i++)
            envp.push_back(const_cast<char *>(this->_vec_env_vars[i].c_str()));
        envp.push_back(NULL);

        char *argv[3];
        argv[0] = const_cast<char*>(this->_interpreter.c_str());
        argv[1] = const_cast<char*>(this->_map_env_vars["TRANSLATED_PATH"].c_str());
        argv[2] = NULL;

        execve(this->_interpreter.c_str(), argv, &envp[0]);
        cout << "(CGI) Error execve" << endl;
        exit(1);
    }
    else
    {
        /**** Processus parent ****/
        close(server_to_cgi[0]);
        close(cgi_to_server[1]);

        /* Envoyer le body au CGI */
        if (!body.empty())
            write(server_to_cgi[1], body.c_str(), body.size());
        close(server_to_cgi[1]); // signale EOF au script

        /* Lire la sortie du CGI */
        char buffer[BUFFER_SIZE];
        std::string response;
        ssize_t bytes_read;
        while ((bytes_read = read(cgi_to_server[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';
            response += buffer;
        }
        close(cgi_to_server[0]);

        // Attendre la fin du process enfant
        int status;
        waitpid(pid, &status, WNOHANG);
        
        return response;
    }
}

