#include "response.hpp"

/************ GETTERS ************/

const string &c_response::get_response() const {
	return (_response);
}

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

	c_location *matching_location = server.find_matching_location(target);
	
	// 1. Vérifier si on trouve une location qui matche -> fonction find_location()
	// 2. Est-ce que la méthode est autorisée pour cette location spécifique?
	// 3. Est-ce qu'on a redéfini une redirection? Si oui -> gérer cette redirection
	
	/***** CONSTRUCTION DU CHEMIN DU FICHIER - À CHANGER POUR AJOUTER LA LOCATIONS *****/
	
	string file_path;
	if (target == "/")
	file_path = "www/index.html";
	else if (target.substr(0, 1) == "/")
	file_path = "www" + target;
	else
	file_path = "www/" + target;
	
	
	/***** CHARGER LE CONTENU DU FICHIER *****/
	_file_content = load_file_content(file_path);

	if (_file_content.empty())
		build_error_response(404, version, request);
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
			if (location_path[location_path.length()] - 1 == '/')
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
}

