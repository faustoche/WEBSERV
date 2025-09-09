#include "response.hpp"

/************ FILE CONTENT MANAGEMENT ************/

void	c_response::define_response_content(const c_request &request, c_server &server)
{
	_response.clear();
	_file_content.clear();
	
	/***** RÉCUPÉRATIONS *****/
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

	/***** CHERCHE LA LOCATION LA PLUS ADAPTÉE AU CHEMIN DEMANDÉE *****/
	// 1. Vérifier si on trouve une location qui matche -> fonction find_location()
	// on va trouver la location qui correspond a la requete
	c_location *matching_location = server.find_matching_location(target);
	
	// 2. Est-ce que la méthode est autorisée pour cette location spécifique?
	if (!server.is_method_allowed(matching_location, method))
	{
		build_error_response(405, version, request);
		return ;
	}

	// 3. Est-ce qu'on a redéfini une redirection? Si oui -> gérer cette redirection
	if (matching_location != NULL)
	{
		pair<int, string> redirect = matching_location->get_redirect();
		if (redirect.first != 0 && !redirect.second.empty())
		{
			build_redirect_response(redirect.first, redirect.second, version, request);
			return ;
		}
	}

	/***** CONSTRUCTION DU CHEMIN DU FICHIER - À CHANGER POUR AJOUTER LA LOCATIONS *****/
	
	string file_path = server.resolve_file_path(matching_location, target, "www");

	/***** CHARGER LE CONTENU DU FICHIER *****/
	_file_content = load_file_content(file_path);
	if (_file_content.empty())
	{
		if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index())
		{
			build_directory_listing_response(file_path, version, request);
			return ;
		}
		build_error_response(404, version, request);
	}
	else
		build_success_response(file_path, version, request);
}

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

/************ RESPONSES ************/

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

/* Vérifier les messages d'erreur s'ils sont cohérents avec un autre site */

void c_response::build_error_response(int error_code, const string version, const c_request &request)
{
	string status;
	string error_content;
	(void)request;

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

	string content = "<html><body><h1>" + int_to_string(code) + " - " + status + "</h1>"
					"<p>The document has moved <a href=\"" + location + "\">here</a>.</p>"
                    "</body></html>";
	
	ostringstream oss;
	oss << content.length();
	_response = version + " " + int_to_string(code) + " " + status + "\r\n";
	_response += "Location: " + location + "\r\n";
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

void	c_response::build_directory_listing_response(const string &dir_path, const string &version, const c_request &request)
{
	//generation du listing html simple dur epapetoire
	string content = "<html><head><title>Index of " + dir_path + "</title></head>";
	content += "<body><h1>Index of " + dir_path + "</h1><hr><ul>";
	
	ostringstream oss;
	oss << content.length();
	_response = version + "200 OK\r\n";
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


// trouver la location

c_location	*c_server::find_matching_location(const string &request_path)
{
	c_location *best_match = NULL;
	size_t best_match_length = 0;

	// parcourir toutes les locations de la map
	for (map<string, c_location>::iterator it = _location_map.begin(); it != _location_map.end(); it++)
	{
		//je recupere la cle
		const string &location_path = it->first;
		// est-ce que le chemin de la requete comment par le chemin de la location?
		if (request_path.find(location_path) == 0)
		{
			// check loction type repertoire qui terminent par /
			// est-ce que le caracetere suivant est / ou bien on est a la fin ?
			if (location_path[location_path.length() - 1] == '/')
			{
				if (request_path.length() == location_path.length() || request_path[location_path.length()] == '/')
				{
					// on choisi la correspondance la plus long 
					if (location_path.length() > best_match_length)
					{
						best_match = &(it->second);
						best_match_length = location_path.length();
					}
				}
			}
			// sinon location exact
			else
			{
				if (request_path == location_path)
					return (&(it->second));
			}
		}
	}
	return (best_match);
}

bool c_server::is_method_allowed(const c_location *location, const string &method)
{
	if (location == NULL)
		return (true); // pas de restriction si pas de location!

	vector<string> allowed_methods = location->get_methods();
	if (allowed_methods.empty())
		return (true); //pas de restrictions si le vector est vide
	
	for (size_t i = 0; i < allowed_methods.size(); i++)
	{
		if (allowed_methods[i] == method)
			return (true);
	}
	return (false);
}

string c_server::resolve_file_path(const c_location *location, const string &request_path, const string &default_root)
{
	if (location == NULL)
	{
		// si pas de location, alors on fait le request path par default
		if (request_path == "/")
			return (default_root + "/" + _index);
		return (default_root + request_path);
	}
	string location_root = location->get_root();
	string location_key = location->get_url_key();

	// calculer le chemin relatif en enlevant la partie location du chemin de requete
	string relative_path;
	if (request_path.find(location_key) == 0)
	{
		relative_path = request_path.substr(location_key.length());
		// on tej les //
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path = relative_path.substr(1);
	}
	// est-ce que c'est un repertoire? aucun fichier specifique n'est demande?
	if (location->get_bool_is_directory() && (relative_path.empty() || relative_path[relative_path.length() - 1] == '/'))
	{
		// chercher un fichier index
		vector<string> index_files = location->get_indexes();
		if (index_files.empty())
			return (location_root + "/" + relative_path + _index);
		else
		{
			// essayer chaque fichier index dans l'rodre 
			for (size_t i = 0; i < index_files.size(); i++)
			{
				string index_path = location_root + "/" + relative_path + index_files[i];
				ifstream test_file(index_path.c_str());
				if (test_file.good())
				{
					test_file.close();
					return (index_path);
				}
				test_file.close();
			}
			// aucun fichier inde xtrouve alors on retourne le premier de la liste
			return (location_root + "/" + relative_path + index_files[0]);
		}
	}
	return (location_root + "/" + relative_path);
}