#include "server.hpp"


/********************    POST    ********************/

/* Content-Type
--> formulaire classique = application/x-www-form-urlencoded
--> formulaire avec fichiers ou champs multiples = multipart/form-data
	en-tete du content-type contient un boundary, c'est une chaine unique choisit par le client qui sert de
	separateur entre les differentes parties du body
 */

/* Dispatch to the matching function based on content-type or target path */

void	c_response::handle_post_request(const c_request &request, c_location *location, const string &version)
{
	string	body = request.get_body();
	string	content_type = request.get_header_value("Content-Type");
	string	target = request.get_target();

	if (request.get_error() || request.get_status_code() != 200)
	{
		build_error_response(request.get_status_code(), version, request);
		return ;
	}

	if (target == "/test_post")
		handle_test_form(request, version);
	else if (content_type.find("application/x-www-form-urlencoded") != string::npos)
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

/* Process file uploads, validate size and content, saves files and redirect */

void	c_response::handle_upload_form_file(const c_request &request, const string &version, c_location *location)
{
	// pour tester la size_max
	size_t max_size = 1 * 1024 * 1024; // 1MB
    if (request.get_content_length() > max_size)
    {
        build_error_response(413, version, request);
        return;
    }
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
		build_error_response(get_status() >= 400 ? get_status() : 400, version, request);
		return ;
	}

	vector<s_multipart> parts = parse_multipart_data(request.get_body(), boundary);
	if (get_status() >= 400)
	{
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

/* Generate a unique filename if a file has the same name as a targfet directory*/

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

/* Save uploaded file to the directory */

string	c_response::save_uploaded_file(const s_multipart &part, c_location *location)
{
	string	uploaded_dir = location->get_upload_path();
	
	if (uploaded_dir.empty())
		uploaded_dir = "./www/upload/";
	if (!directory_exists(uploaded_dir))
	{
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
		boundary = content_type.substr(pos + 9);
	else
	{
		set_status(400);
		return "";
	}
	return boundary;
}

/* parse multipart body in single parts */

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

/* parse one multipart section, extract headers and content */

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

/* parse header lines inside a multipart section, extract filename and content-type */

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

/* extracts a single header line fr om a given position */

string	c_response::extract_line(const string &header_section, const size_t &pos)
{
	size_t	end_pos = header_section.find("\r\n", pos);

	if (end_pos == string::npos)
		end_pos = header_section.length();
	
	string line = header_section.substr(pos, end_pos - pos);
	return line;
}

/* extract a "quoted" value */

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
/* */

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
			bool	is_hexa = true;
			string hex_str = body.substr(i + 1, 2);
			for (size_t j = 0; j < hex_str.size(); ++j)
			{
				if (!isxdigit(hex_str[j]))
					is_hexa = false;
			}
			if (is_hexa)
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

/***** TODO ******/


void c_response::handle_todo_form(const c_request &request, const string &version)
{
	map<string, string>form = parse_form_data(request.get_body());
	string task;

	map<string, string>::iterator it = form.find("task");
	if (it != form.end())
		task = it->second;
	else
		task = "";
	if (task.empty())
	{
		build_error_response(400, version, request);
		return ;
	}

	string filename = "./www/data/todo.txt";
	ofstream file(filename.c_str(), ios::app);
	if (!file.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}
	file << task << endl;
	file.close();
	load_todo_page(version, request);
}
