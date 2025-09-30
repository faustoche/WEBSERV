#include "response.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_response::c_response(c_server& server, int client_fd) : _server(server), _client_fd(client_fd)
{
	this->_is_cgi = false;
	this->_error = false;
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
	

	/***** TROUVER LA CONFIGURATION DE LOCATION LE PLUS APPROPRIÉE POUR L'URL DEMANDÉE *****/
	c_location *matching_location = _server.find_matching_location(target);

	if (matching_location != NULL && matching_location->get_cgi().size() > 0)
		this->_is_cgi = true;

	if (matching_location == NULL) 
	{// si on a une requete vers un dossier qui ne matche avec aucune location, sinon build la reponse avec le fichier index s'il existe ou renvoyer une erreur
		string full_path = _server.get_root() + target;
		if (is_directory(full_path))
		{
			if (_server.get_indexes().empty())
			{
				cout << "Error: no location found for target: " << target  << endl;
				build_error_response(404, version, request);
				return ;
			}
		}
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

	string file_path = _server.convert_url_to_file_path(matching_location, target, "./www");
	
	/***** CHARGER LE CONTENU DU FICHIER *****/
	if (is_regular_file(file_path))
	{
		_file_content = load_file_content(file_path);
	}
	if (_file_content.empty() && !this->_is_cgi)
	{
		if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index()) // si la llocation est un repertoire ET que l'auto index est activé alors je genere un listing de repertoire
		{
			this->_is_cgi = false;
			build_directory_listing_response(file_path, version, request);
			return ;
		}
		if (_server.get_indexes().empty())
		{
			build_error_response(404, version, request);
		}
	}

	if (this->_is_cgi)
	{
		if (request.get_path().find(".") == string::npos)
		{
			if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index()) // si la llocation est un repertoire ET que l'auto index est activé alors je genere un listing de repertoire
			{
				this->_is_cgi = false;	
				build_directory_listing_response(file_path, version, request);
				return ;
			}
			build_error_response(404, version, request);
		}

		cout << YELLOW << "==PROCESS CGI IDENTIFIED FOR FD " << this->_client_fd << "=="  << RESET << endl << endl;
		c_cgi* cgi = new c_cgi(this->_server, this->_client_fd);
		
		if (cgi->init_cgi(request, *matching_location, request.get_target()))
		{
			set_error();
			build_error_response(404, version, request);
			return ;
		}
		
		build_cgi_response(*cgi, request);
		this->_server.set_active_cgi(cgi->get_pipe_out(), cgi);
		this->_server.set_active_cgi(cgi->get_pipe_in(), cgi);
		return ;
	}
	else if (method == "POST") //peut etre a bouger a la fin
	{
		handle_post_request(request, matching_location, version);
		return;
	}
	else
	{
		cout << CYAN << __FILE__ << "/" << __LINE__ << RESET << endl;
		build_success_response(file_path, version, request);
	}
}

/********************    POST    ********************/
/* Content-Type
--> formulaire classique = application/x-www-form-urlencoded
--> formulaire avec fichiers ou champs multiples = multipart/form-data
	en-tete du content-type contient un boundary, c'est une chaine unique choisit par le client qui sert de
	separateur entre les differentes parties du body
 */

void	c_response::handle_post_request(const c_request &request, c_location *location, const string &version)
{
	(void)location;
	string	body = request.get_body();
	string	content_type = request.get_header_value("Content-Type");
	string	target = request.get_target();

	// cout << "=== DEBUG POST ===" << endl
	// 		<< "Body recu: [" << body << "]" << endl
	// 		<< "Content-Type: [" << content_type << "]" << endl
	// 		<< "Target: [" << request.get_target() << "]" << endl;

	if (target == "/test_post")
		handle_test_form(request, version);
	if (content_type.find("application/x-www-form-urlencoded") != string::npos)// sauvegarde des donnees (upload)
		handle_contact_form(request, version);
	if (content_type.find("multipart/form-data") != string::npos)
		handle_upload_form_file(request, version);

	 // if (content_type.find("application/x-www-form-urlencoded") != string::npos)
	// upload file
	// else
	// 	create_generic_response()

}

/********************   upload form   ********************/

void	c_response::handle_upload_form_file(const c_request &request, const string &version)
{
	(void)version;
	string content_type = request.get_header_value("Content-Type");
	string boundary;
	size_t pos = content_type.find("boundary=");
	if (pos != string::npos)
		boundary = content_type.substr(pos + 9);// si PB trim espace ou / et guillemet
	else
		throw invalid_argument("Can't find the boundary in the Content-Type value for upload a file");
	
	cout << PINK << boundary << RESET << endl;
	vector<s_multipart> parsed_body = parse_multipart_data(request.get_body(), boundary);
	// faut-il forcement avoir un upload_path de definit pour upload un fichier ?

}

vector<s_multipart> const	c_response::parse_multipart_data(const string &body, const string &boundary)
{
	string			delimiter = "--" + boundary;
	size_t			pos = 0;
	vector<size_t>	boundary_pos;

	while((pos = body.find(delimiter, pos)) != string::npos)
	{
		boundary_pos.push_back(pos);
		pos += delimiter.size();
	}

	vector<s_multipart>	parts;
	for (size_t i = 0; i < boundary_pos.size() - 1; i++)
	{
		size_t	begin = boundary_pos[i] + delimiter.length();
		size_t	end = boundary_pos[i + 1];
		string	raw_part = body.substr(begin, end);
		s_multipart single_part = parse_single_part(raw_part);
		parts.push_back(single_part);
	}
	
	// for(vector<size_t>::iterator it = boundary_pos.begin(); it != boundary_pos.end(); it++ )
	// {
	// 	cout << GREEN << *it << endl;
	// }
	return parts;
}

s_multipart const	c_response::parse_single_part(const string &raw_part)
{
	s_multipart	part;
	size_t		separator_pos = raw_part.find("\r\n\r\n");

	if (separator_pos == string::npos)
		return part;

	string header_section = raw_part.substr(0, separator_pos);
	string content_section = raw_part.substr(separator_pos + 4, raw_part.size());
	cout << ORANGE << header_section << endl;
	cout << FUCHSIA << content_section << endl;
	
	// nettoyer le contenu -> "\r\n" ou boundary de fin 

	// parser les header
	parse_header_section(header_section, part);

	// determiner le type

	// verifier le content-type ?

	return part;


}

void	c_response::parse_header_section(const string &header_section, s_multipart &part)
{
	(void)header_section;
	// Headers possibles :
    // - Content-Disposition: form-data; name="xxx"; filename="yyy"
    // - Content-Type: image/jpeg

	// Parser Content-Disposition
	size_t	pos_disposition = header_section.find("Content-Disposition");
	if (pos_disposition != string::npos)
	{
		string line = extract_line(header_section, pos_disposition);
		cout << GREEN << line << endl;
		part.name = extract_quotes(line, "name=");
		// cout << GREEN << part.name << endl;
		part.filename = extract_quotes(line, "filename=");
		// cout << GREEN << part.filename << RESET << endl;
		//creer fonction pour extraire la valeur entre guillemets
		//peut etre vide
		// y a t-il dautres info dans content-disposition ?
	}

	// Parser Content-Type
	size_t	pos_type = header_section.find("Content-Type");
	if (pos_type != string::npos)
	{
		string line = extract_line(header_section, pos_type);
		cout << GREEN << line << endl;
		part.content_type = extract_after_points(line);
		cout << GREEN << part.content_type << RESET << endl;
	}
		// extraire toute la ligne
		// extraite la valeur apres Content-Type
		// trim les espaces
}


string	c_response::extract_line(const string &header_section, const size_t &pos)
{
	size_t	end_pos = header_section.find("\r\n", pos);

	if (end_pos == string::npos)
		end_pos = header_section.length(); //verifier si lenght ou size
	
	string line = header_section.substr(pos, end_pos);
	return line;
}

string	c_response::extract_quotes(const string &line, const string &type)
{
	size_t key_pos = line.find(type);
	if (key_pos == string::npos)
		return "";
	
	size_t first_quote = line.find('"', key_pos);
	if (first_quote == string::npos)
		return ""; // fautil throw une exception pour format invalide ?
	size_t second_quote = line.find('"', first_quote + 1);

	if (second_quote == string::npos)
		return ""; // fautil throw une exception pour format invalide ?
	
	string value = line.substr(first_quote + 1, second_quote - first_quote - 1);
	return value;
}

string	c_response::extract_after_points(const string &line)
{
	size_t pos = line.find(':');
	if (pos == string::npos)
		return "";
	
	string value = line.substr(pos + 1);
	// value.erase(line.find_last_not_of(" "));
	// trim les espaces
	return value;
}

/*******************   contact form    *******************/

void	c_response::handle_contact_form(const c_request &request, const string &version)
{
	(void)version;
	string body = request.get_body();
	map<string, string> form_data = parse_form_data(request.get_body());

	if (form_data["nom"].empty() || form_data["email"].empty())
	{
		build_error_response(400, version, request);
		return;
	}
	// creer fonciton is_valid email ?
	if (save_contact_data(form_data))
	{
		string success_html = "<html><body><h1>Message enregistre !</h1>"
                             "<p>Merci " + form_data["nom"] + "</p>"
                             "<a href=\"/contact.html\">Nouveau message</a></body></html>";
		_file_content = success_html;
		build_success_response("response.html", version, request);
		return;
	}
	else
		build_error_response(500, version, request);
}



bool	c_response::save_contact_data(const map<string, string> &data)
{
	string filename = "./www/data/contact.txt"; //location.get_upload_path();
	ofstream file(filename.c_str(), ios::app);

	if (!file.is_open())
	{
		cerr << "Error with the creation of the file " << filename << endl;
		return false;
	}

	time_t now = time(0);
	char *str_time = ctime(&now);
	str_time[strlen(str_time) - 1] = '\0';

	file << "[ " << str_time << " ] ";
	for (map<string, string>::const_iterator it = data.begin(); it != data.end(); it++)
	{
		file << it->first << "=" << it->second << "; ";
	}
	file << endl;
	file.close();
	return true;
}

/********************    test form    ********************/

void	c_response::handle_test_form(const c_request &request, const string &version)
{
	map<string, string> form_data = parse_form_data(request.get_body());
	// cout << GREEN << "=== DONNEES PARSEES ===" << endl;
	// for (map<string, string>::iterator it = form_data.begin(); it != form_data.end(); it++)
	// 	cout << it->first << " = [ " << it->second << " ]" << endl;
	create_form_response(form_data, request, version);
}

void	c_response::create_form_response(const map<string, string> &form, const c_request &request, const string &version)
{
	string html = "<!DOCTYPE html>\n<html><head><title>Formulaire recu</title></head>\n<body>\n";
	html += "<h1>Donnees recues :</h1>\n<ul>\n";

	for (map<string, string>::const_iterator it = form.begin(); it != form.end(); it++)
	{
		html += "<li><strong>" + it->first + "</strong>" + ": " + it->second + "</li>\n";
	}
	html += "</ul>\n<a href=\"/post_test.html\">Nouveau formulaire</a>\n</body></html>";
	_file_content = html;
	build_success_response("response.html", version, request);
}

/****************   utils for test form   ****************/

string const	c_response::url_decode(const string &body)
{
	string	res;
	size_t	i = 0;
	
	while (i < body.size())
	{
		if (body[i] == '+')
		{
			res += ' ';
			i++;
		}
		else if (body[i] == '%' && i + 2 < body.size())
		{
			bool	ishexa = true;
			string hex_str = body.substr(i + 1, 2);
			for (size_t j = 0; j < hex_str.size(); ++j)
			{
				if (!isxdigit(hex_str[j]))
					ishexa = false;
			}
			if (ishexa)
			{
				int value = 0;
				istringstream iss(hex_str);
				iss >> std::hex >> value;
				res += static_cast<char>(value);
				i +=3 ;
			}
			else
			{
				res += body[i];
				i++;
			}
		}
		else
		{
			res += body[i];
			i++;
		}
	}
	return res;
}

map<string, string> const	c_response::parse_form_data(const string &body)
{
	map<string, string>	form_data;

	size_t	pos_and = 0;
	string	key;
	string	value;
	size_t start = 0;

	while (start < body.size())
	{
		size_t pos_equal = 0;

		pos_and = body.find("&", start);
		if (pos_and == string::npos)
			pos_and = body.size();
		
		string pairs = body.substr(start, pos_and - start);
		pos_equal = pairs.find("=", 0);
		key = pairs.substr(0, pos_equal);
		value = pairs.substr(pos_equal + 1);

		key = url_decode(key);
		value = url_decode(value);

		form_data[key] = value;

		start += pairs.size();
		start++;
	}
	return form_data;
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

/* READING PAGES */

/* Reading the error pages*/

string c_response::read_error_pages(int error_code)
{
	ostringstream path;
	path << "www/error_pages/" << error_code << ".html";
	ifstream file(path.str().c_str());
	if (file.is_open())
	{
		ostringstream content;
		content << file.rdbuf();
		file.close();
		return (content.str());
	}
	ostringstream fallback;
	fallback << "<html><body><h1>" << error_code << " - Error</h1></body></html>";
	return (fallback.str());
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

}

void c_response::build_success_response(const string &file_path, const string version, const c_request &request)
{
	if (_file_content.empty())
	{
		cout << CYAN << __FILE__ << "/" << __LINE__ << RESET << endl;
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
			//error_content = "<html><body><h1>400 - Bad Request</h1></body></html>";
			break;
		case 404:
			status = "Not Found";
			//error_content = "<html><body><h1>404 - Page Not Found</h1></body></html>";
			break;
		case 405:
			status = "Method Not Allowed";
			//error_content = "<html><body><h1>405 - Method Not Allowed</h1></body></html>";
			break;
		case 505:
			status = "HTTP Version Not Supported";
			//error_content = "<html><body><h1>505 - HTTP Version Not Supported</h1></body></html>";
			break;
		default:
			status = "Internal Server Error";
			//error_content = "<html><body><h1>500 - Internal Server Error</h1></body></html>";
			break;
	}
	error_content = read_error_pages(error_code);
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
		string index = get_valid_index(_root, this->get_indexes());
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
		// if (!relative_path.empty())
		// 		relative_path = '/' + relative_path;
	}
	// Si l'utilisateur demande un dossier et pas un fichier precis -> on cherche un fichier index a l'interieur du dossier
	if (is_directory(location_root + relative_path) && (relative_path.empty() || relative_path[relative_path.length() - 1] == '/'))
	{
		location->set_is_directory(true);
		// on va recuperer tous les fichiers index.xxx existants
		vector<string> index_files = location->get_indexes();
		cout << CYAN << __LINE__ << " / " << __FILE__ << RESET << endl;
		if (index_files.empty()) // si c'est vide, alors on lui donne le fichier index par default (index.html par exemple)
			return (location_root + relative_path);
		else
		{
			// Processus: On teste tous les autres fichiers dans l'ordre
				// 1. essaye index.html 
			for (size_t i = 0; i < index_files.size(); i++)
			{
				string index_path = location_root + relative_path + index_files[i]; // on itere dans les index
				ifstream file_checker(index_path.c_str());
				if (file_checker.is_open()) // est-ce que le fichier existe? est-ce que j'ai reussi a l'ouvrir
				{
					file_checker.close();
			
					return (index_path); // ca fonctione donc je le return
				}
				file_checker.close();
			}
			// aucun fichier index trouve alors on retourne le premier de la liste car il renverra une erreur 404
			// CM = si aucun fichier index n'est trouve dans le dossier de la location et que l'autoindex est on il faut afficher le listing du dossier
			if (location->get_auto_index())
				return (location_root + relative_path);
			return (location_root + relative_path + index_files[0]);
		}
	}
	return (location_root + relative_path);
}
