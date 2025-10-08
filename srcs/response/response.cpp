#include "response.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_response::c_response(c_server& server, c_client &client) : _server(server), _client(client)
{
	this->_is_cgi = false;
	this->_error = false;
	this->_client_fd = _client.get_fd();
	this->_status = 200;
	// this->_client = _server.find_client(_client_fd);
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
		return (false);
	return (S_ISDIR(path_stat.st_mode));
}

bool	is_regular_file(const string& path)
{
	struct stat path_stat;

	if (stat(path.c_str(), &path_stat) != 0)
		return (false);
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
	if (status_code != 200)
	{
		build_error_response(status_code, version, request);
		return ;
	}
	if (method == "GET" && target == "/todo.html")
	{
		load_todo_page(version, request);
		return ;
	}
	if (method == "DELETE" && (target == "/delete_todo" || target.find("/delete_todo?") == 0))
	{
		handle_delete_todo(request, version);
		return;
	}
	if (method == "POST" && target == "/post_todo")
	{
		handle_todo_form(request, version);
		return;
	}
	if (method == "GET" && target == "/page_upload.html")
    {
        load_upload_page(version, request);
        return ;
    }
    if (method == "DELETE" && (target.find("/delete_upload?") || target == "/delete_todo") == 0)
    {
        handle_delete_upload(request, version);
        return;
    }
	if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT")
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
	
	c_location *matching_location = _server.find_matching_location(target);

	if (matching_location != NULL && matching_location->get_cgi().size() > 0)
		this->_is_cgi = true;

	if (matching_location == NULL) 
	{
		cout << __LINE__ << " / " << __FILE__ << endl;
		string full_path = _server.get_root() + target;
		if (is_directory(full_path))
		{
			if (_server.get_indexes().empty())
			{
				// cout << "Error: no location found for target: " << target  << endl;
				_server.log_message("[ERROR] No location found for target " + target);
				build_error_response(404, version, request);
				return ;
			}
		}
	}

	if (!_server.is_method_allowed(matching_location, method))
	{
		build_error_response(405, version, request);
		return ;	
	}

	if (matching_location != NULL)
	{
		pair<int, string> redirect = matching_location->get_redirect();
		if (redirect.first != 0 && !redirect.second.empty())
		{
			build_redirect_response(redirect.first, redirect.second, version, request);
			return ;
		}
	}

	string file_path = _server.convert_url_to_file_path(matching_location, target, "./www");

	if (is_regular_file(file_path))
	{
		cout << __LINE__ << " / " << __FILE__ << endl;
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

		_server.log_message("[DEBUG] PROCESS CGI IDENTIFIED FOR FD " + int_to_string(_client_fd));
		c_cgi* cgi = new c_cgi(this->_server, this->_client_fd);
		
		if (cgi->init_cgi(request, *matching_location, request.get_target()))
		{
			_server.cleanup_cgi(cgi);
			this->_is_cgi = false;
			set_error();
			build_error_response(cgi->get_status_code(), version, request);
			return ;
		}
		build_cgi_response(*cgi, request);
		
		this->_server.set_active_cgi(cgi->get_pipe_out(), cgi);
		this->_server.set_active_cgi(cgi->get_pipe_in(), cgi);
		return ;
	}
	else if (method == "POST")
	{
		handle_post_request(request, matching_location, version);
		return;
	}
	else if (method == "DELETE")
	{
		if (target == "/delete_todo")
		{
			handle_delete_todo(request, version);
			return;
		}
		handle_delete_request(request, version, file_path);
		build_success_response(file_path, version, request);
	}
	else
	{
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
	string	body = request.get_body();
	string	content_type = request.get_header_value("Content-Type");
	string	target = request.get_target();

	// cout << "=== DEBUG POST ===" << endl
	// 		<< "Body recu: [" << body << "]" << endl
	// 		<< "Content-Type: [" << content_type << "]" << endl
	// 		<< "Target: [" << request.get_target() << "]" << endl;
	if (request.get_error() || request.get_status_code() != 200)
	{
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		build_error_response(request.get_status_code(), version, request);
		return ;
	}

	if (target == "/test_post")
		handle_test_form(request, version);
	else if (content_type.find("application/x-www-form-urlencoded") != string::npos)// sauvegarde des donnees (upload)
		handle_contact_form(request, version);
	else if (content_type.find("multipart/form-data") != string::npos)
		handle_upload_form_file(request, version, location);
	else if (target == "/post_todo")
		handle_todo_form(request, version);
	else
		build_error_response(404, version, request);
}

/********************   upload file   ********************/
/*
Objectif : Partage d'images, de fichier txt, pdf sur les chiens

AUTORISER :
- Images : jpg, png, gif (2 MB max)
- Documents : pdf, txt (5 MB max)

REJETER : Vidéos, exécutables, archives

Exemple config :
allowed_extensions = ["jpg", "jpeg", "png", "gif", "pdf", "txt"]
max_file_size = 2 * 1024 * 1024  // 2 MB
*/

void	c_response::handle_upload_form_file(const c_request &request, const string &version, c_location *location)
{
	string body = request.get_body();
	if (body.empty())
	{
		_server.log_message("[ERROR] Empty body for upload request");
		build_error_response(400, version, request);
		return ;
	}
	string content_type = request.get_header_value("Content-Type");
	
	// PARSING
	string boundary = extract_boundary(content_type);
	if (boundary.empty() || get_status() >= 400)
	{
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		build_error_response(get_status() >= 400 ? get_status() : 400, version, request);
		return ;
	}

	vector<s_multipart> parts = parse_multipart_data(request.get_body(), boundary);
	if (get_status() >= 400)
	{
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		build_error_response(get_status(), version, request);
		return ;
	}

	// if (parts.empty()) // pour fichiers lourds parfois rentre parfois non
	// {
		// cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		// build_error_response(400, version, request);
		// return ;
	// }


	// TRAITEMENT de chaque partie
	// string 			description;
	vector<string>	uploaded_files;
	for(size_t i = 0; i < parts.size(); i++)
	{
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		s_multipart &part = parts[i];
		if (part.is_file)
		{
			string saved_path = save_uploaded_file(part, location);
			if (get_status() >= 400)
			{
				cout << PINK << __LINE__ << " / " << __FILE__ << endl;
				build_error_response(get_status(), version, request);
				return;
			}
			if (saved_path.empty())
			{
				build_error_response(500, version, request);
				return ;
			}
			uploaded_files.push_back(saved_path);
			_file_content = load_file_content(saved_path);
			_server.log_message("[INFO] ✅ UPLOADED FILE : " + part.filename 
									+ " (" + int_to_string(part.content.size()) + " bytes)");
		}
	}
	if (uploaded_files.size() > 0)
	{
		_response = version + " 303 See Other\r\n";
		_response += "Location: /page_upload.html\r\n";
		_response += "Content-Length: 0\r\n";
		_response += "Connection: keep-alive\r\n";
		_response += "Server: webserv/1.0\r\n\r\n";

		_server.log_message("[INFO] ✅ Upload done. Redirection to /page_upload.html");
	}
	else
	{
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		build_error_response(400, version, request);
	}
}

bool	file_exists(const std::string &path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

bool	directory_exists(const string &path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool	create_directory(const string &path)
{
	return (mkdir(path.c_str(), 0755) == 0);
}

string	get_unique_filename(const string &directory, string &filename)
{
	string full_path = directory + filename;
	if (!file_exists(full_path))
		return filename;
	
	size_t dot_pos = filename.find_last_of('.');
	string name = filename.substr(0, dot_pos);
	string extension = filename.substr(dot_pos);

	int counter = 1;
	string new_filename;

	while (true)
	{
		ostringstream oss;
		oss << counter;
		new_filename = name + "_" + oss.str() + extension;
		full_path = directory + new_filename;

		if (!file_exists(full_path))
			return new_filename;
		counter++;
		if (counter > 1000)
		{
			ostringstream oss_t;
			oss_t << time(0);
			return name + "_" + oss_t.str() + extension;
		}
	}
}


string	c_response::save_uploaded_file(const s_multipart &part, c_location *location)
{
	string	uploaded_dir = location->get_upload_path();
	
	// si pas d'upload path donne dans le .conf on en donne un par defaut
	if (uploaded_dir.empty())
		uploaded_dir = "./www/upload/";
	if (!directory_exists(uploaded_dir))
	{
		// si le dossier d'upload n'existe pas on cree un dossier
		if (!create_directory("./www/upload/"))
			return "";
	}
	string safe_filename = sanitize_filename(part.filename, location);
	if (safe_filename.empty())
		return "";

	
	string final_path = uploaded_dir + safe_filename;
	if (file_exists(final_path))
	{
		safe_filename = get_unique_filename(uploaded_dir, safe_filename);
		final_path = uploaded_dir + safe_filename;
	}

	ofstream file(final_path.c_str(), ios::binary);
	if (!file.is_open())
	{
		_server.log_message("[ERROR] the server cannot upload file " + final_path);
		// cerr << "Error: the server can't upload the file " << final_path <<endl;
		return "";
	}
	file.write(part.content.data(), part.content.size());
	file.close();
	return final_path;
}

string	c_response::extract_boundary(const string &content_type)
{
	string boundary;

	size_t pos = content_type.find("boundary=");
	if (pos != string::npos)
		boundary = content_type.substr(pos + 9);// si PB trim espace ou / et guillemet
	else
	{
		set_status(400); // parsing multipart echoue
		return "";
	}
	return boundary;
}


vector<s_multipart> const	c_response::parse_multipart_data(const string &body, const string &boundary)
{
	string			delimiter = "--" + boundary;
	string			closing_delimier = delimiter + "--";
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
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
		if (get_status() >= 400)
			break;

		size_t	begin = boundary_pos[i] + delimiter.length();
		
		// sauter le \r\n ou \n apres le boundary
		if (begin < body.size() && body[begin] == '\r')
			begin++;
		if (begin < body.size() && body[begin] == '\n')
			begin++;

		// end doit pointer juste avant le prochain boundary
		// on ne doit pas inclure le \r\n qui precede le boundary
		size_t	end = boundary_pos[i + 1];
		if (end >= 2 && body[end - 2] == '\r' && body[end - 1] == '\n')
			end -= 2;
		else if (end >= 1 && body[end - 1] == '\n')
			end -= 1;
		
		if (begin >= end)
			continue;

		string	raw_part = body.substr(begin, end - begin);

		if (raw_part.find(closing_delimier) != string::npos)
			continue;
		if (raw_part.find_first_not_of(" \r\n") == string::npos)
			continue;

		s_multipart single_part = parse_single_part(raw_part);

		if (get_status() >= 400)
			break;
		
		parts.push_back(single_part);
		if (single_part.content.empty())
			cout << PINK << __LINE__ << " / EMPTY / " << __FILE__ << endl;
	}
	if (parts.empty())
		cout << PINK << __LINE__ << " / " << __FILE__ << endl;
	return parts;
}

s_multipart const	c_response::parse_single_part(const string &raw_part)
{
	s_multipart	part;

	if (get_status() >= 400)
		return part;

	size_t		separator_pos = raw_part.find("\r\n\r\n");
	if (separator_pos == string::npos)
	{
		set_status(400); // en-tete manquant, parsing multipart echoue
		return part;
	}

	string header_section = raw_part.substr(0, separator_pos);
	string content_section = raw_part.substr(separator_pos + 4);

	while (!content_section.empty() && (content_section[content_section.size() -1] == '\r'))
		content_section.erase(content_section.size() - 1, 1);
	while (!content_section.empty() && (content_section[content_section.size() -1] == '\n'))
		content_section.erase(content_section.size() - 1, 1);

	size_t pos = content_section.rfind("\r\n--");
	if ( pos != string::npos)
	{
		content_section.erase(pos);
	}

	// parser les header
	parse_header_section(header_section, part);

	if (get_status() >= 400)
		return part;

	part.content = content_section;
	part.is_file = !part.filename.empty();

	return part;
}

void	c_response::parse_header_section(const string &header_section, s_multipart &part)
{
	if (get_status() >= 400)
		return;

	// Headers possibles :
	// - Content-Disposition: form-data; name="xxx"; filename="yyy"
	// - Content-Type: image/jpeg

	// Parsing Content-Disposition
	size_t	pos_disposition = header_section.find("Content-Disposition");
	if (pos_disposition != string::npos)
	{
		string line = extract_line(header_section, pos_disposition);
		if (line.empty())
		{
			set_status(400); // en-tete manquant, parsing multipart echoue
			return;
		}
		part.name = extract_quotes(line, "name=");
		if (part.name.empty())
		{
			set_status(400); // en-tete manquant, parsing multipart echoue
			return;
		}
		part.filename = extract_quotes(line, "filename=");
	}
	else
	{
		set_status(400);
		return;
	}

	size_t	pos_type = header_section.find("Content-Type");
	if (pos_type != string::npos)
	{
		string line = extract_line(header_section, pos_type);
		part.content_type = extract_after_points(line);
		if (part.content_type.empty())
			set_status(400);
	}
}


string	c_response::extract_line(const string &header_section, const size_t &pos)
{
	size_t	end_pos = header_section.find("\r\n", pos);

	if (end_pos == string::npos)
		end_pos = header_section.length();
	
	string line = header_section.substr(pos, end_pos - pos);
	return line;
}

string	c_response::extract_quotes(const string &line, const string &type)
{
	size_t key_pos = line.find(type);
	if (key_pos == string::npos)
		return "";
	
	size_t first_quote = line.find('"', key_pos);
	if (first_quote == string::npos)
		return "";
	size_t second_quote = line.find('"', first_quote + 1);

	if (second_quote == string::npos)
		return "";
	
	string value = line.substr(first_quote + 1, second_quote - first_quote - 1);
	return trim(value);
}

string	c_response::extract_after_points(const string &line)
{
	size_t pos = line.find(':');
	if (pos == string::npos)
		return "";
	
	string value = line.substr(pos + 1);
	return trim(value);
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

	if (save_contact_data(form_data))
	{
		_response = version + " 303 See Other\r\n";
		_response += "Location: /contact_success.html\r\n";
		_response += "Content-Length: 0\r\n";
		_response += "\r\n";
		return;
	}
	else
		build_error_response(500, version, request);
}

bool	c_response::save_contact_data(const map<string, string> &data)
{
	string filename = "./www/data/contact.txt"; 
	ofstream file(filename.c_str(), ios::binary | ios::app);

	if (!file.is_open())
	{
		_server.log_message("[ERROR] error with the creation of file " + filename);
		// cerr << "Error with the creation of the file " << filename << endl;
		return false;
	}

	time_t now = time(0);
	char *str_time = ctime(&now);
	str_time[strlen(str_time) - 1] = '\0';

	// file << "[ " << str_time << " ] ";
	for (map<string, string>::const_iterator it = data.begin(); it != data.end(); it++)
	{
		file << it->first << "=" << it->second << "; ";
	}
	file << endl;
	file.close();
	return true;
}

string  c_response::extract_extension(const string &filename, string &name, c_location *location)
{
	size_t point_pos = filename.find_last_of(".");
	if (point_pos == string::npos)
		return "";
	if (point_pos == 0)
		return "";
	string extension = filename.substr(point_pos);
	name = filename.substr(0, point_pos);

	if (location->get_allowed_extensions().empty() || 
		find(location->get_allowed_extensions().begin(), location->get_allowed_extensions().end(), extension) != location->get_allowed_extensions().end())
	{
		return extension;
	}
	else 
	{
		cout << "Error: extension not allowded (" << extension << ")" << endl;
		return "";
	}
	return extension;
}


string  c_response::sanitize_filename(const string &filename, c_location *location)
{
	string name;
	string extension = extract_extension(filename, name, location);
	if (extension.empty())
	{
		set_status(415);
		return "";
	}
	if (name.empty())
		name = "file";

	string clean_name;
	for(size_t i = 0; i < name.size(); i++)
	{
		char c = name[i];
		if (isalnum(static_cast<unsigned char>(c)))
			clean_name += c;
		else if (c == '_' || c == '-' || c == ' ')
			clean_name += '_';
		else
			clean_name += '_';
	}
	while (clean_name.find("__") != string::npos)
		clean_name.replace(clean_name.find("__"), 2, "_");
	while (!clean_name.empty() && clean_name[clean_name.size() - 1] == '_')
		clean_name.erase(clean_name.size() - 1);
	if (clean_name.size() > 200)
		clean_name = clean_name.substr(0, 200);
	clean_name = trim_underscore(clean_name);
	if (!extension.empty())
		return clean_name += extension;
	else
		return clean_name;
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

	// cout << "request_body: " << request_body << endl; 
	if (cgi.get_interpreter().empty())
		return ;

	this->set_status(cgi.launch_cgi(request_body));

}

/* COde 303 avec redirection */

void	c_response::buid_upload_success_response(const string &file_path, const string version, const c_request &request)
{
	(void)file_path;
	_response = version + " 303 See other\r\n";
	_response += "Location: /page_upload.html\r\n";
	
	string connection;
	connection = request.get_header_value("Connection");
	if (connection.empty())
		connection = "keep-alive";

	_response += "Connection: " + connection + "\r\n";
	_response += "Server: webserv/1.0\r\n";
	_response += "Content-Length: 0\r\n";
	_response += "\r\n";

	_file_content.clear();
}

/* Code 201 - créée */

// void	c_response::buid_upload_success_response(const string &file_path, const string version, const c_request &request)
// {
// 	(void)file_path;
// 	_response = version + " 201 Created\r\n";
// 	_response += "Content-Type: text/html\r\n";
	

// 	string connection;
// 	try {
// 		connection = request.get_header_value("Connection");
// 	} catch (...) {
// 		connection = "keep-alive";
// 	}

// 	_response += "Connection: " + connection + "\r\n";
// 	_response += "Server: webserv/1.0\r\n";

// 	string body = "Location: /upload_success.html\r\n";

// 	ostringstream oss;
// 	oss << body.size();
// 	_response += "Content-Length: " + oss.str() + "\r\n";
// 	_response += "\r\n";
// 	_response += body;

// 	_file_content.clear();
// }

void c_response::build_success_response(const string &file_path, const string version, const c_request &request)
{
	if (_file_content.empty())
	{
		build_error_response(404, version, request);
		return ;
	}
	
	_client.set_status_code(200);

	size_t content_size = _file_content.size();
	ostringstream oss;
	oss << content_size;

	_response = version + " 200 OK\r\n";
	_response += "Content-Type: " + get_content_type(file_path) + "\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection;
	connection = request.get_header_value("Connection");
	if (connection.empty())
		connection = "keep-alive";

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
			break;
		case 401:
			status = "Unauthorized";
			break;
		case 403:
			status = "Forbidden";
			break;
		case 404:
			status = "Not Found";
			break;
		case 405:
			status = "Method Not Allowed";
			break;
		case 408:
			status = "Request Timeout";
			break;
		case 413:
			status = "Payload Too Large";
			break;
		case 414:
			status = "URI Too Long";
			break;
		case 415:
			status = "Unsupported Media Type";
			break;
		case 429:
			status = "Too Many Request";
			break;
		case 500:
			status = "Internal Server Error";
			break;
		case 501:
			status = "Not Implemented";
			break;
		case 502:
			status = "Bad Gateway";
			break;
		case 503:
			status = "Service Unavailable";
			break;
		case 504:
			status = "Gateway Timeout";
			break;
		case 505:
			status = "HTTP Version Not Supported";
			break;
		default:
			status = "Internal Server Error";
			break;
	}

	_client.set_status_code(error_code);

	map<int, string> const &err_pages = _server.get_err_pages();
	map<int, string>::const_iterator it = err_pages.find(error_code);
	if (it != err_pages.end())
	{
		string error_path = it->second;
		
		if (!error_path.empty() && error_path[0] == '/')
		{
			string root = _server.get_root();
			if (root.empty() || root == "." || root == "./")
				root = "./www";
			error_path = root + error_path;
		}
		
		error_content = load_file_content(error_path);
		if (error_content.empty())
		{
			_server.log_message("[ERROR] could not load error page : " + error_path);
			ostringstream fallback;
			fallback << "<html><body><h1>" << error_code << " - " << status << "</h1></body></html>";
			error_content = fallback.str();
		}
		else
		{
			_server.log_message("[INFO] ✅ Successfully loaded error page: " + error_path);
		}
	}
	else
	{
		_server.log_message("[WARN] No error page configured for code " + int_to_string(error_code));
		ostringstream fallback;
		fallback << "<html><body><h1>" << error_code << " - " << status << "</h1></body></html>";
		error_content = fallback.str();
	}
	
	ostringstream oss;
	oss << error_content.length();
	
	_response = version + " " + int_to_string(error_code) + " " + status + "\r\n";
	_response += "Content-Type: text/html\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection;
	connection = request.get_header_value("Connection");
	if (connection.empty())
		connection = "keep-alive";

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

	string connection;
	connection = request.get_header_value("Connection");
	if (connection.empty())
		connection = "keep-alive";

	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += content;

	_client.set_status_code(code);
}

/* Build the response according to the auto-index. If auto-index is on, list all of the files in the directory concerned. */

void c_response::build_directory_listing_response(const string &dir_path, const string &version, const c_request &request)
{
	string content = "<html><head><title>Index of " + dir_path + "</title></head>";
	content += "<body><h1>Index of " + dir_path + "</h1><hr><ul>";
	
	DIR *dir = opendir(dir_path.c_str());
	if (dir != NULL) 
	{
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) 
		{
			string name = entry->d_name;
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

 	(void)version;
	_response = "HTTP/1.1 200 OK\r\n";
	_response += "Content-Type: text/html\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection;
	connection = request.get_header_value("Connection");
	if (connection.empty())
		connection = "keep-alive";

	_response += "Connection: " + connection + "\r\n";
	_response += "\r\n";
	_response += content;
	_file_content = content;

	_client.set_status_code(200);
}

/************ HANDLING LOCATIONS ************/

/* Finding the locations the matches the most with what the request is asking for */

c_location	*c_server::find_matching_location(const string &request_path)
{
	c_location *best_match = NULL;
	size_t best_match_length = 0;

	for (map<string, c_location>::iterator it = _location_map.begin(); it != _location_map.end(); it++)
	{
		const string &location_path = it->first;
		if (request_path.find(location_path) == 0)
		{
			if (location_path.length() > 0 && location_path[location_path.length() - 1] == '/')
			{
				if (request_path.length() == location_path.length() || 
					(request_path.length() > location_path.length() && 
					 request_path[location_path.length()] == '/') ||
					request_path.length() > location_path.length())
				{
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
		cout << __LINE__ << " / " << __FILE__ << endl;
		string index = get_valid_index(_root, this->get_indexes());
		if (request_path == "/")
			return (default_root + "/" + index);
		return (default_root + request_path);
	}

	string location_root = location->get_alias();
	string location_key = location->get_url_key();

	string relative_path;
	if (request_path.find(location_key) == 0)
	{
		relative_path = request_path.substr(location_key.length());
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path = relative_path.substr(1);
	}

	if (is_directory(location_root + relative_path) && (relative_path.empty() || relative_path[relative_path.length() - 1] == '/'))
	{
		location->set_is_directory(true);
		vector<string> index_files = location->get_indexes();
		if (index_files.empty())
			return (location_root + relative_path);
		else
		{
			for (size_t i = 0; i < index_files.size(); i++)
			{
				string index_path = location_root + relative_path + index_files[i];
				ifstream file_checker(index_path.c_str());
				if (file_checker.is_open())
				{
					file_checker.close();
					return (index_path);
				}
				file_checker.close();
			}
			if (location->get_auto_index())
				return (location_root + relative_path);
			return (location_root + relative_path + index_files[0]);
		}
	}
	return (location_root + relative_path);
}
