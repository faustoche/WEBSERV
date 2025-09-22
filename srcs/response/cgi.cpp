#include "cgi.hpp"

c_cgi::c_cgi(c_server& server, int client_fd) : 
_server(server), _client_fd(client_fd)
{
    this->_loc = NULL;
    this->_script_name = "";
    this->_path_info = "";
    this->_translated_path ="";
    this->_interpreter = "";
    this->_finished = false;
    this->_content_length = 0;
    this->_headers_parsed = false;
    this->_pipe_in = 0;
    this->_pipe_out = 0;
    this->_write_buffer.clear();
    this->_read_buffer.clear();
    this->_bytes_written = 0;
    this->_map_env_vars.clear();
    this->_vec_env_vars.clear();
}

c_cgi::c_cgi(const c_cgi& other): _server(other._server)
{
    this->_client_fd = other._client_fd;
    this->_pipe_in = other._pipe_in;
    this->_pipe_out = other._pipe_out;
    this->_pid = other._pid;
    this->_write_buffer = other._write_buffer;
    this->_read_buffer = other._read_buffer;
    this->_bytes_written = other._bytes_written;
    this->_finished = other._finished;
    this->_map_env_vars = other._map_env_vars;
    this->_vec_env_vars = other._vec_env_vars;
    // this->_socket_fd = other._socket_fd;
    this->_status_code = other._status_code;
    this->_loc = other._loc;
    this->_script_name = other._script_name;
    this->_path_info = other._path_info;
    this->_translated_path = other._translated_path;
    this->_interpreter = other._interpreter;
}


c_cgi const& c_cgi::operator=(const c_cgi& rhs)
{
    if (this != &rhs)
    {
        this->_server = rhs._server;
        this->_client_fd = rhs._client_fd;
        this->_pipe_in = rhs._pipe_in;
        this->_pipe_out = rhs._pipe_out;
        this->_pid = rhs._pid;
        this->_write_buffer = rhs._write_buffer;
        this->_read_buffer = rhs._read_buffer;
        this->_bytes_written = rhs._bytes_written;
        this->_finished = rhs._finished;
        this->_content_length = rhs._content_length;
        this->_headers_parsed = rhs._headers_parsed;
        this->_map_env_vars = rhs._map_env_vars;
        this->_vec_env_vars = rhs._vec_env_vars;
        // this->_socket_fd = rhs._socket_fd;
        this->_status_code = rhs._status_code;
        this->_loc = rhs._loc;
        this->_script_name = rhs._script_name;
        this->_path_info = rhs._path_info;
        this->_translated_path = rhs._translated_path;
        this->_interpreter = rhs._interpreter;
    }
    return (*this);
}

c_cgi::~c_cgi()
{
}

void    c_cgi::append_read_buffer( const char* buffer, ssize_t bytes)
{
    if (!buffer || bytes == 0)
        return ;

    this->_read_buffer.append(buffer, bytes);
}

void    c_cgi::consume_read_buffer(size_t n) 
{
    if (n >= _read_buffer.size())
        _read_buffer.clear();
    else
        _read_buffer.erase(0, n);
}

map<string, c_location>::const_iterator   find_location(const string &path, map<string, c_location>& map_location)
{
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
    string empty_string = "";
    size_t dot_position = real_path.find_last_of('.');

    if (dot_position != string::npos && dot_position != 0)
    {
        string extension = real_path.substr(dot_position);
        return (extension);
    }
    return (empty_string);
}

size_t c_cgi::identify_script_type(const string& path)
{
    size_t pos_script = string::npos;
    string  script_type;

    pos_script = path.find(".py");
    if (pos_script == string::npos)
    {
        pos_script= path.find(".php");
        if (pos_script == string::npos)
        {
            cerr << "Error: unknown script type" << endl;
            return (string::npos);
        }
         this->_script_name = path.substr(0, pos_script + 4);
        return (pos_script + 4);
    }
    this->_script_name = path.substr(0, pos_script + 3);
    return (pos_script + 3);
}

int    c_cgi::resolve_cgi_paths(const c_location &loc, string const& script_filename)
{
    size_t ext_pos = identify_script_type(script_filename);

    if (ext_pos == string::npos)
        return(1);
    if (this->_script_name.size() < script_filename.size())
    {
        this->_path_info = script_filename.substr(ext_pos);
        this->_translated_path = loc.get_alias() + this->_path_info;
    }

    string  root = loc.get_alias();
    string  url_key = loc.get_url_key();
    string  relative_path = this->_script_name.substr(url_key.size());
    this->_script_filename = root + relative_path;
    
    return(0);
}

int    c_cgi::init_cgi(const c_request &request, const c_location &loc)
{
    this->_status_code = request.get_status_code();
    this->_loc = &loc;
    
    /* Recherche de l'interpreteur de fichier selon le langage identifie */
    string extension = find_extension(this->_script_filename);

    if (extension.size() > 0)
        this->_interpreter = loc.extract_interpreter(extension);
    if (this->_interpreter.empty())
        return (1);

    /* Construction de l'environnement pour l'execution du script */
    set_environment(request);

    return (0);
}

void  c_cgi::vectorize_env()
{
    map<string, string> map_copy = this->_map_env_vars;

    for (map<string, string>::const_iterator it = map_copy.begin(); it != map_copy.end(); it++)
        this->_vec_env_vars.push_back(it->first + "=" + it->second);
}

void    c_cgi::set_environment(const c_request &request)
{
    this->_socket_fd = request.get_socket_fd();
    this->_map_env_vars["REQUEST_METHOD"] = request.get_method();
    this->_map_env_vars["SCRIPT_NAME"] = this->_script_name;
    this->_map_env_vars["PATH_INFO"] = this->_path_info;
    this->_map_env_vars["TRANSLATED_PATH"] = this->_translated_path;
    this->_map_env_vars["SCRIPT_FILENAME"] = this->_script_filename;
    this->_map_env_vars["CONTENT-LENGTH"] = int_to_string(request.get_content_length());
    this->_map_env_vars["CONTENT_TYPE"] = request.get_header_value("Content-Type");
    this->_map_env_vars["QUERY_STRING"] = request.get_query();
    this->_map_env_vars["SERVER_PROTOCOL"] = request.get_version();
    this->_map_env_vars["GATEWAY_INTERFACE"] = "CGI/1.1";
    this->_map_env_vars["REMOTE_ADDR"] = request.get_ip_client();
    this->_map_env_vars["REDIRECT_STATUS"] = "200";

    this->_map_env_vars["HTTP_ACCEPT"] = request.get_header_value("Accept");
    this->_map_env_vars["HTTP_USER_AGENT"] = request.get_header_value("User-Agent");
    this->_map_env_vars["HTTP_ACCEPT_LANGUAGE"] = request.get_header_value("Accept-Language");
    this->_map_env_vars["HTTP_COOKIE"] = request.get_header_value("Cookie");
    this->_map_env_vars["HTTP_REFERER"] = request.get_header_value("Referer");

    this->vectorize_env();
}

string make_absolute(const string &path)
{
    char resolved[1000];

    if (realpath(path.c_str(), resolved))
        return (string(resolved));

    return (path);
}

string  c_cgi::launch_cgi(const string &body)
{
    int server_to_cgi[2];
    int cgi_to_server[2];

    if (pipe(server_to_cgi) < 0 || pipe(cgi_to_server) < 0)
    {
        cout << "(CGI): Error de pipe" << endl;
        return ("500 Internal server error");
    }

    this->_pipe_in = server_to_cgi[1];
    this->_pipe_out = cgi_to_server[0];
    this->_write_buffer = body;
    this->_read_buffer.clear();
    this->_bytes_written = 0;

    this->_pid = fork();
    if (this->_pid < 0)
    {
        cout << "(CGI): Error de fork";
        return ("500 Internal server error");        
    }

    /**** Processus enfant ****/
    if (this->_pid == 0)
    {
        /* Redirection stdin depuis le pipe d'entree: permet au parent server le body au cgi */
        dup2(server_to_cgi[0], STDIN_FILENO);
        close(server_to_cgi[1]);

        /* Redirection stdout vers le pipe de sortie: permet au cgi de renvoyer la reponse au server */
        dup2(cgi_to_server[1], STDOUT_FILENO);
        close(cgi_to_server[0]);
     
        /**** Convertir en tableau de char* pour execve ****/
        vector<char*> envp;
        for (size_t i = 0; i < this->_vec_env_vars.size(); i++)
            envp.push_back(const_cast<char *>(this->_vec_env_vars[i].c_str()));
        envp.push_back(NULL);

        string  abs_path = make_absolute(this->_map_env_vars["SCRIPT_FILENAME"]);
        this->_map_env_vars["SCRIPT_FILENAME"] = abs_path;

        char *argv[3];
        argv[0] = const_cast<char*>(this->_interpreter.c_str());
        argv[1] = const_cast<char*>(this->_map_env_vars["SCRIPT_FILENAME"].c_str());
        argv[2] = NULL;

        execve(this->_interpreter.c_str(), argv, &envp[0]);

        cout << "Status: 500 Internal Server Error" << endl;
        exit(1);
    }

    /**** Processus parent ****/

    close(server_to_cgi[0]);
    close(cgi_to_server[1]);

    fcntl(this->_pipe_in, F_SETFL, O_NONBLOCK);
    fcntl(this->_pipe_out, F_SETFL, O_NONBLOCK);

    _server.add_fd(cgi_to_server[0], POLLIN);
    _server.add_fd(server_to_cgi[1], POLLOUT);

    return ("");
}

