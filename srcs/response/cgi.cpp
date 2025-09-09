#include "cgi.hpp"

c_cgi::c_cgi() : _loc(NULL), _script_name(""), _path_info(""), _translated_path(""), _interpreter("")
{
    this->_map_env_vars.clear();
    this->_vec_env_vars.clear();
}

c_cgi::c_cgi(const c_request &request, c_response &response, map<string, c_location>& map_location)
: _loc(NULL), _script_name(""), _path_info(""), _translated_path(""), _interpreter("")
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

size_t c_cgi::identify_script_type(const c_request &request)
{
    size_t pos_script = string::npos;
    string  script_type;

    pos_script = request.get_path().find(".py");
    if (pos_script == string::npos)
    {
        pos_script= request.get_path().find(".php");
        if (pos_script == string::npos)
        {
            cerr << "Error: unknown script type" << endl;
            return (string::npos);
        }
         this->_script_name = request.get_path().substr(0, pos_script + 4);
        return (pos_script + 4);
    }
    this->_script_name = request.get_path().substr(0, pos_script + 3);
    return (pos_script + 3);
}

void    c_cgi::resolve_cgi_paths(const c_request &request, const c_location* loc)
{
    size_t ext_pos = identify_script_type(request);
    if (ext_pos == string::npos)
        return ;
    if (this->_script_name.size() < request.get_path().size())
    {
        this->_path_info = request.get_path().substr(ext_pos);
        this->_translated_path = loc->get_root() + this->_path_info;
    }

    string  root = loc->get_root();
    string  url_key = loc->get_url_key();
    string  relative_path = this->_script_name.substr(url_key.size());
    this->_script_filename = root + relative_path;
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

    resolve_cgi_paths(request, this->_loc);
    
    /* Recherche de l'interpreteur de fichier selon le langage identifie */
    string extension = find_extension(this->_script_name);
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
    /* BIEN SE CALER POUR COMPRENDRE LES VAR SCRIPT_NAME / PATH_INFO ET TRANSLATED_PATH */
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

bool    c_cgi::is_valid_header_value(string& key, const string& value)
{
	for (size_t i = 0; i < value.length(); i++)
	{
		if ((value[i] < 32 && value[i] != '\t') || value[i] == 127)
		{
			cerr << "(Request) Error: Invalid char in header value: " << value << endl;
			return (false);
		}
	}

	if (key == "Content-Length")
	{
		for (size_t i = 0; i < value.length(); i++)
		{
			if (!isdigit(value[i]))
			{
				cerr << "(Request) Error: Invalid content length: " << value << endl;
				return (false);
			}
		}
	}

	if (value.size() > 4096)
	{
		cerr << "(Request) Error: Header field too large: " << value << endl;
		return (false);
	}
	return (true);
}

int c_cgi::parse_headers(c_response &response, string& headers)
{
	size_t pos = headers.find(':', 0);
	string key;
	string value;

	key = headers.substr(0, pos);
	if (!is_valid_header_name(key))
	{
		cerr << "(Request) Error: invalid header_name: " << key << endl;
		// response.set_status(400);
	}

	pos++;
	// if (headers[pos] != 32)
	// 	this->_status = 400;

	value = ft_trim(headers.substr(pos + 1));
	if (!is_valid_header_value(key, value))
	{
		cerr << "(Request) Error: invalid header_value: " << key << endl;
		// this->_status = 400;
	}

	response.set_header_value(key, value);
    std::cerr << "Header enregistré : [" << key << "] = [" << value << "]" << std::endl;
    cout << "******************allo:" << response.get_header_value("Content-Type") << endl;
	return (0);
}

void	c_cgi::get_header_from_cgi(c_response &response, const string& content_cgi)
{
	size_t end_of_headers;

	if ((end_of_headers = content_cgi.find("\r\n\r\n")) == string::npos)
		return ;
	string headers = content_cgi.substr(0, end_of_headers);
    cout << "headers: " << headers << endl;

	istringstream stream(headers);
	string	line;
	while (getline(stream, line, '\n'))
	{
		if (line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		parse_headers(response, line);
        cout << "get header dans getline: " << line << endl;
	}

	response.set_body(content_cgi.substr(end_of_headers + 4));

    string body = response.get_body();
    if (response.get_header_value("Content-Length").empty() && !body.empty())
        response.set_header_value("Content-Length", int_to_string(body.size()));

    cout << "get header dans cgi: " << response.get_header_value("Content-Type") << endl;
}

string make_absolute(const string &path)
{
    char resolved[1000];
    if (realpath(path.c_str(), resolved))
        return (string(resolved));
    return (path);
}

/* interpreter = loc->_cgi_extension["extension"]*/
string  c_cgi::launch_cgi(const string &body)
{
    int server_to_cgi[2];
    int cgi_to_server[2];

    cout << __FILE__ << "/" << __LINE__ << endl;
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
    else
    {
        cout << __FILE__ << "/" << __LINE__ << endl;
        /**** Processus parent ****/
        close(server_to_cgi[0]);
        close(cgi_to_server[1]);

        /* Envoyer le body au CGI */
        if (!body.empty())
            write(server_to_cgi[1], body.c_str(), body.size());
        close(server_to_cgi[1]); // signale EOF au script

       
        /* Lire la sortie du CGI */
        char buffer[BUFFER_SIZE];
        std::string content_cgi;
        ssize_t bytes_read;
        while ((bytes_read = read(cgi_to_server[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';
            content_cgi += buffer;
        }
        close(cgi_to_server[0]);
        
        // Attendre la fin du process enfant
        int status;
        waitpid(pid, &status, WNOHANG);
        
        cout << "status: " << status << endl;
        return content_cgi;
    }
}

