#include "response.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_response::c_response(c_server& server, int client_fd) : _server(server), _client_fd(client_fd)
{
	this->_is_cgi = false;
}

c_response::~c_response()
{}


/************ GETTERS ************/

const string& c_response::get_header_value(const string& key) const
{
	static const string empty_string = "";

	map<string, string>::const_iterator it = this->_headers_response.find(key);
	if (it != this->_headers_response.end())
		return (it->second);
	return (empty_string);
}

/************ FILE CONTENT MANAGEMENT ************/

/* Check to see what kind of response must be sent 
* First, we clear everything and define variables
* Second, we check that the method, versions and host are corrects
* Third, we verify that the method is allowed according to the locations
* We also check if the locations gave us redirections or not
* We construct the correct path according to locations again
*/


bool	is_directory(const string& path)
{
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
	{
		// Erreur chemin n'existe pas ou n'est pas accessible
		return (false);
	}
	return (S_ISDIR(path_stat.st_mode));
}

bool	is_regular_file(const string& path)
{
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
	{
		// Erreur chemin n'existe pas ou n'est pas accessible
		return (false);
	}
	return (S_ISREG(path_stat.st_mode));
}

void	c_response::define_response_content(const c_request &request)
{
	_response.clear();
	_file_content.clear();
	int status_code = request.get_status_code();
	string method = request.get_method();
	string target = request.get_target();
	string version = request.get_version();
	std::map<string, string> headers = request.get_headers();

	/***** VÉRIFICATIONS *****/
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		build_error_response(405, version, request);
		return ;
	}
	if (version != "HTTP/1.1")
	{
		build_error_response(505, version, request);
		return ;
	}
	if (headers.find("Host") == headers.end())
	{
		build_error_response(status_code, version, request);
		return ;
	}

	/* A SUPPRIMER */
	c_location loc;
	map<string, string> cgi_extension;
	cgi_extension[".php"] = "/usr/bin/php-cgi";
	cgi_extension[".py"] = "/usr/bin/python3";
	loc.set_url_key("/cgi-bin");
	loc.set_alias("./www/cgi-bin");
	loc.set_cgi(cgi_extension);
	loc.set_auto_index(true);
	// vector<string> index_file;
	// index_file.push_back("index.py");
	// loc.set_index_files(index_file);

	/***** TROUVER LA CONFIGURATION DE LOCATION LE PLUS APPROPRIÉE POUR L'URL DEMANDÉE *****/
	c_location *matching_location = _server.find_matching_location(target);

	if (matching_location != NULL && matching_location->get_cgi().size() > 0)
		this->_is_cgi = true;

	/* A SUPPRIMER */
	if (target.find("cgi") != string::npos)
	{
		this->_is_cgi = true;
		request.print_full_request();
		matching_location = &loc;
	}
	/***************/

	if (matching_location == NULL)
	{
		cout << "Error: no location found for target: " << target  << endl;
		build_error_response(404, version, request);
		return ;
	}

	/***************/

	if (!_server.is_method_allowed(matching_location, method))
	{
		build_error_response(405, version, request);
		return ;	
	}

	/***** VÉRIFICATION DE LA REDIRECTION CONFIGURÉE OU NON *****/
	if (matching_location != NULL)
	{
		pair<int, string> redirect = matching_location->get_redirect(); // pair avec code de redirection et URL de destination
		if (redirect.first != 0 && !redirect.second.empty()) // first = redirection (301, 302), second = URL
		{
			build_redirect_response(redirect.first, redirect.second, version, request);
			return ;
		}
	}

	/***** CONSTRUCTION DU CHEMIN DU FICHIER *****/
	string file_path = _server.convert_url_to_file_path(matching_location, target, "www");

	/***** CHARGER LE CONTENU DU FICHIER *****/
	if (is_regular_file(file_path))
		_file_content = load_file_content(file_path);

	/* SI AUCUN FICHIER, GENERER UNE PAGE QUI LISTE LES FICHIERS DU DOSSIER */
	if (_file_content.empty())
	{
		if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index()) // si la llocation est un repertoire ET que l'auto index est activé alors je genere un listing de repertoire
		{
			
			build_directory_listing_response(file_path, version, request);
			return ;
		}
		build_error_response(404, version, request);
	}
	if (this->_is_cgi)
	{
		cout << "Process cgi identified" << endl;
		c_cgi* cgi = new c_cgi(this->_server, this->_client_fd);
		cgi->set_script_filename(file_path);
		cgi->init_cgi(request, *matching_location);
		cgi->resolve_cgi_paths(*matching_location, cgi->get_script_filename());
		build_cgi_response(*cgi, request);
		// cout 
		// 	<< "Taille de pollfd dans define_response_content: "
		// 	<< this->_server.get_size_pollfd() << endl;
		this->_server.set_active_cgi(cgi->get_pipe_out(), cgi);
		// std::cout << "Insertion active_cgi[" << cgi->get_pipe_out() << "]" << std::endl;
		return ;
	}
	else
		build_success_response(file_path, version, request);
}

/* Proceed to load the file content. Nothing else to say. */

string c_response::load_file_content(const string &file_path)
{
	ifstream	file(file_path.c_str(), ios::binary);
	
	if (!file.is_open()) {
		return ("");
	}
	string	content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();

	return (content);
}

/* Check extension to see what type of content where are dealing with to use it in our response. */

string c_response::get_content_type(const string &file_path)
{
	size_t dot_position = file_path.find_last_of('.');
	if (dot_position == string::npos)
		return ("text/plain");
	
	string extension = file_path.substr(dot_position);
	if (extension == ".html")
		return ("text/html");
	else if (extension == ".css")
		return ("text/css");
	else if (extension == ".jpeg" || extension == ".jpg")
		return ("image/jpeg");
	else if (extension == ".png")
		return ("image/png");
	else if (extension == ".ico")
		return ("image/x-icon");
	else if (extension == ".gif")
		return ("image/gif");
	else if (extension == ".pdf")
		return ("application/pdf");
	else if (extension == ".zip")
		return ("application/zip");
	else
		return ("text/plain"); 
}

/************ BUILDING RESPONSES ************/

/* Build the successfull request response */

void	c_response::build_cgi_response(c_cgi & cgi, const c_request &request)
{

	this->_status = request.get_status_code();
	const string request_body = request.get_body();

	if (cgi.get_interpreter().empty())
		return ;
	string content_cgi = cgi.launch_cgi(request_body);
	// cgi.get_header_from_cgi(*this, content_cgi);
	// cgi.set_headers_parsed(true);
	// this->_response += request.get_version() + " " + int_to_string(this->_status) + "\r\n";
	// this->_response += "Server: webserv/1.0\r\n";
	// if (!get_header_value("Content-Type").empty())
	// 	this->_response += "Content-Type: " + this->_headers_response["Content-Type"] + "\r\n";
	// else
	// 	this->_response += "Content-Type: text/plain\r\n";
	// this->_response += "Content-Length: " + this->_headers_response["Content-Length"] + "\r\n";

	// string connection;
	// connection = request.get_header_value("Connection");
	// if (connection.empty())
	// 	connection = "keep-alive";
	// this->_response += "Connection: " + connection + "\r\n";

	// this->_response += "\r\n";
	// this->_response += get_body() + "\r\n";
}

void c_response::build_success_response(const string &file_path, const string version, const c_request &request)
{
	if (_file_content.empty())
	{
		build_error_response(request.get_status_code(), version, request);
		return ;
	}

	size_t content_size = _file_content.size();
	ostringstream oss;
	oss << content_size;

	_response = version + " 200 OK\r\n";
	_response += "Content-Type: " + get_content_type(file_path) + "\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection;
	try {
		connection = request.get_header_value("Connection");
	} catch (...) {
		connection = "keep-alive";
	}
	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += _file_content;
}

/* Build basic error response for some cases. */

void c_response::build_error_response(int error_code, const string version, const c_request &request)
{
	string status;
	string error_content;

	switch (error_code)
	{
		case 400:
			status = "Bad Request";
			error_content = "<html><body><h1>400 - Bad Request</h1></body></html>";
			break;
		case 404:
			status = "Not Found";
			error_content = "<html><body><h1>404 - Page Not Found</h1></body></html>";
			break;
		case 405:
			status = "Method Not Allowed";
			error_content = "<html><body><h1>405 - Method Not Allowed</h1></body></html>";
			break;
		case 505:
			status = "HTTP Version Not Supported";
			error_content = "<html><body><h1>505 - HTTP Version Not Supported</h1></body></html>";
			break;
		default:
			status = "Internal Server Error";
			error_content = "<html><body><h1>500 - Internal Server Error</h1></body></html>";
			break;
	}
	
	ostringstream oss;
	oss << error_content.length();
	
	_response = version + " " + int_to_string(error_code) + " " + status + "\r\n";
	_response += "Content-Type: text/html\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection;
	try {
		connection = request.get_header_value("Connection");
	} catch (...) {
		cerr << "Error: No Header: Connection is kept alive by default" << endl;
		connection = "keep-alive";
	}
	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += error_content;
	_file_content.clear();
}

/* Build the response in case of redirection's error with the locations. */

void	c_response::build_redirect_response(int code, const string &location, const string &version, const c_request &request)
{
	string status;
	switch (code)
	{
		case 301:
			status = "Moved Permanently";
			break ;
		case 302:
			status = "Found";
			break ;
		case 307:
			status = "Temporary Redirect";
			break ;
		case 308:
			status = "Permanent Redirect";
			break ;
		default:
			status = "Found";
			code = 302;
			break ;
	}

	// contenu en html histoire d'avoir un message 
	string content = "<html><body><h1>" + int_to_string(code) + " - " + status + "</h1>"
					"<p>The document has moved <a href=\"" + location + "\">here</a>.</p>"
					"</body></html>";
	
	ostringstream oss;
	oss << content.length();
	_response = version + " " + int_to_string(code) + " " + status + "\r\n";
	_response += "Location: " + location + "\r\n"; // indique ou se trouve la nouvelle ressource
	_response += "Content-Type: text/html\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection = "keep-alive";
	try {
		connection = request.get_header_value("Connection");
	} catch (...) {
		cerr << "Error: No Header: Connection is kept alive by default" << endl;
		connection = "keep-alive";
	}
	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += content;
}

/* Build the response according to the auto-index. If auto-index is on, list all of the files in the directory concerned. */

void c_response::build_directory_listing_response(const string &dir_path, const string &version, const c_request &request)
{
	// genere du début de la page HTML
	string content = "<html><head><title>Index of " + dir_path + "</title></head>";
	content += "<body><h1>Index of " + dir_path + "</h1><hr><ul>";
	
	// ouvrir le répertoire
	DIR *dir = opendir(dir_path.c_str());
	if (dir != NULL) 
	{
		struct dirent *entry;
		// lire chaque fichier/dossier
		while ((entry = readdir(dir)) != NULL) 
		{
			string name = entry->d_name;
			// ignore le répertoire courant "."
			if (name == ".") 
				continue ;
			content += "<li><a href=\"" + name + "\">" + name + "</a></li>";
		}
		closedir(dir);
	}
	else 
		content += "<li>Cannot read directory</li>";
	content += "</ul><hr></body></html>";
	ostringstream oss;
	oss << content.length();

	_response = version + " 200 OK\r\n";
	_response += "Content-Type: text/html\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection = "keep-alive";
	try {
		connection = request.get_header_value("Connection");
	} catch (...) {
		cerr << "Error: No Header: Connection is kept alive by default" << endl;
		connection = "keep-alive";
	}
	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += content;
	_file_content = content;
}

/************ HANDLING LOCATIONS ************/

/* Finding the locations the matches the most with what the request is asking for */

c_location	*c_server::find_matching_location(const string &request_path)
{
	c_location *best_match = NULL;
	size_t best_match_length = 0;

	// parcourir toutes les locations de la map
	for (map<string, c_location>::iterator it = _location_map.begin(); it != _location_map.end(); it++)
	{
		const string &location_path = it->first;
		// est-ce que le chemin de la requete comment par le chemin de la location?
		if (request_path.find(location_path) == 0)
		{
			// check loction type repertoire qui terminent par /
			// est-ce que le caracetere suivant est / ou bien on est a la fin ?
			if (location_path.length() > 0 && location_path[location_path.length() - 1] == '/')
			{
				if (request_path.length() == location_path.length() || 
					(request_path.length() > location_path.length() && 
					 request_path[location_path.length()] == '/') ||
					request_path.length() > location_path.length())
				{
					// on choisi la correspondance la plus long 
					if (location_path.length() > best_match_length)
					{
						best_match = &(it->second);
						best_match_length = location_path.length();
					}
				}
			}
			else
			{
				if (request_path == location_path)
				{
					best_match = &(it->second);
					best_match_length = location_path.length();
					break ;
				}
			}
		}
	}
	return (best_match);
}

/* Check if the method is allowed by the locations. */

bool c_server::is_method_allowed(const c_location *location, const string &method)
{
	if (location == NULL)
		return (true);

	vector<string> allowed_methods = location->get_methods();
	if (allowed_methods.empty())
		return (true);
	
	for (size_t i = 0; i < allowed_methods.size(); i++)
	{
		if (allowed_methods[i] == method)
			return (true);
	}
	return (false);
}

/* Convert the url given into a real file path to access all of the informations */

string c_server::convert_url_to_file_path(c_location *location, const string &request_path, const string &default_root)
{
	
	if (location == NULL)
	{
		string index = get_valid_index(this->get_indexes());
		// si pas de location, alors on fait le request path par default. par exemple default root = repertoire racine par default cad www et l'index = index.html
		if (request_path == "/")
			return (default_root + "/" + index);
		return (default_root + request_path);
	}
	string location_root = location->get_alias();
	string location_key = location->get_url_key();

	// calculer le chemin relatif en enlevant la partie location du chemin de requete
	string relative_path;
	if (request_path.find(location_key) == 0) // on verifie si l'url commence par le pattern de la location
	{
		relative_path = request_path.substr(location_key.length());
		// on tej les // qui sont en plus pour evciter les doublons
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path = relative_path.substr(1);
	}
	// Si l'utilisateur demande un dossier et pas un fichier precis -> on cherche un fichier index a l'interieur du dossier
	if (is_directory(location_root + "/" + relative_path) && (relative_path.empty() || relative_path[relative_path.length() - 1] == '/'))
	{
		location->set_is_directory(true);
		// on va recuperer tous les fichiers index.xxx existants
		vector<string> index_files = location->get_indexes();
		if (index_files.empty()) // si c'est vide, alors on lui donne le fichier index par default (index.html par exemple)
		{
			return (location_root + "/" + relative_path);
		}
		else
		{
			// Processus: On teste tous les autres fichiers dans l'ordre
				// 1. essaye index.html 
			for (size_t i = 0; i < index_files.size(); i++)
			{
				string index_path = location_root + "/" + relative_path + index_files[i]; // on itere dans les index
				ifstream file_checker(index_path.c_str());
				if (file_checker.is_open()) // est-ce que le fichier existe? est-ce que j'ai reussi a l'ouvrir
				{
					file_checker.close();
					return (index_path); // ca fonctione donc je le return
				}
				file_checker.close();
			}
			// aucun fichier inde xtrouve alors on retourne le premier de la liste car il renverra une erreur 404
			return (location_root + "/" + relative_path + index_files[0]);
		}
	}
	return (location_root + "/" + relative_path);
}