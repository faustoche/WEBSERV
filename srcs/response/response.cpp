#include "response.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_response::c_response(c_server& server, c_client &client) : _server(server), _client(client)
{
	this->_is_cgi = false;
	this->_error = false;
	this->_client_fd = _client.get_fd();
	this->_status = 200;
}

c_response::~c_response(){}

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

void	c_response::define_response_content(const c_request &request)
{
	_response.clear();
	_file_content.clear();
	int status_code = request.get_status_code();
	string method = request.get_method();
	string target = request.get_target();
	string version = request.get_version();
	std::map<string, string> headers = request.get_headers();

	if (status_code != 200)
	{
		build_error_response(status_code, request);
		return ;
	}
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		build_error_response(405, request);
		return ;
	}
	if (method == "GET" && target == "/todo.html")
	{
		load_todo_page(request);
		return ;
	}
	if (method == "DELETE" && (target == "/delete_todo" || target.find("/delete_todo?") == 0))
	{
		handle_delete_todo(request);
		return;
	}
	if (method == "POST" && target == "/post_todo")
	{
		handle_todo_form(request);
		return;
	}
	if (method == "GET" && target == "/page_upload.html")
	{
		load_upload_page(request);
		return ;
	}
	if (method == "DELETE" && (target.find("/delete_upload?") || target == "/delete_todo") == 0)
	{
		handle_delete_upload(request);
		return;
	}
	if (version != "HTTP/1.1")
	{
		build_error_response(505, request);
		return ;
	}
	if (headers.find("Host") == headers.end())
	{
		build_error_response(status_code, request);
		return ;
	}

	c_location *matching_location = _server.find_matching_location(target);

	if (matching_location != NULL && matching_location->get_cgi().size() > 0)
		this->_is_cgi = true;

	if (matching_location == NULL) 
	{
		string full_path = _server.get_root() + target;
		if (is_directory(full_path))
		{
			if (_server.get_indexes().empty())
			{
				_server.log_message("[ERROR] No location found for target " + target);
				build_error_response(404, request);
				return ;
			}
		}
	}

	if (!_server.is_method_allowed(matching_location, method))
	{
		build_error_response(405, request);
		return ;	
	}

	if (matching_location != NULL)
	{
		pair<int, string> redirect = matching_location->get_redirect();
		if (redirect.first != 0 && !redirect.second.empty())
		{
			build_redirect_response(redirect.first, redirect.second, request);
			return ;
		}
	}

	string file_path = _server.convert_url_to_file_path(matching_location, target, "./www");
	char resolved_path[PATH_MAX];
	if (!file_path.empty() && !realpath(file_path.c_str(), resolved_path))
	{
		build_error_response(403, request);
		return ;
	}

	if (is_regular_file(file_path))
		_file_content = load_file_content(file_path);

	if (_file_content.empty() && !this->_is_cgi)
	{
		if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index()) // si la llocation est un repertoire ET que l'auto index est activé alors je genere un listing de repertoire
		{
			this->_is_cgi = false;
			build_directory_listing_response(file_path, request);
			return ;
		}
		if (_server.get_indexes().empty())
		{
			build_error_response(404, request);
		}
	}
	if (this->_is_cgi)
	{
		if (request.get_path().find(".") == string::npos)
		{
			this->_is_cgi = false;
			if (request.get_target() != "/cgi-bin/" && request.get_target() != "/cgi-bin")
			{
				build_error_response(404, request);
				return ;
			}
			if (matching_location != NULL && matching_location->get_bool_is_directory() && matching_location->get_auto_index()) // si la llocation est un repertoire ET que l'auto index est activé alors je genere un listing de repertoire
			{
				build_directory_listing_response(file_path, request);
				return ;
			}
		}

		_server.log_message("[DEBUG] PROCESS CGI IDENTIFIED FOR FD " + int_to_string(_client_fd));
		c_cgi* cgi = new c_cgi(this->_server, this->_client_fd);
		
		if (cgi->init_cgi(request, *matching_location, request.get_target()))
		{
			_server.cleanup_cgi(cgi);
			this->_is_cgi = false;
			set_error();
			cgi->set_exit_status(this->_client.get_status_code());
			build_error_response(cgi->get_status_code(), request);
			return ;
		}
		build_cgi_response(*cgi, request);
		
		this->_server.set_active_cgi(cgi->get_pipe_out(), cgi);
		this->_server.set_active_cgi(cgi->get_pipe_in(), cgi);
		return ;
	}
	else if (method == "POST")
	{
		handle_post_request(request, matching_location);
		return;
	}
	else if (method == "DELETE")
	{
		if (target == "/delete_todo")
		{
			handle_delete_todo(request);
			return;
		}
		cout << "file_path: " << file_path << endl;
		handle_delete_request(request, file_path);
		build_success_response(file_path, request);
	}
	else
	{
		build_success_response(file_path, request);
	}
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

	this->set_status(cgi.launch_cgi(request_body));
}

void	c_response::buid_upload_success_response(const c_request &request)
{
	_response = "HTTP/1.1 303 See other\r\n";
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

void c_response::build_success_response(const string &file_path, const c_request &request)
{
	if (_file_content.empty())
	{
		build_error_response(404, request);
		return ;
	}
	
	_client.set_status_code(200);

	size_t content_size = _file_content.size();
	ostringstream oss;
	oss << content_size;

	_response = "HTTP/1.1 200 OK\r\n";
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

void c_response::build_error_response(int error_code, const c_request &request)
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
	
	_response = "HTTP/1.1 " + int_to_string(error_code) + " " + status + "\r\n";
	_response += "Content-Type: text/html; charset=UTF-8\r\n";
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

void	c_response::build_redirect_response(int code, const string &location, const c_request &request)
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
	_response = "HTTP/1.1 " + int_to_string(code) + " " + status + "\r\n";
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

void c_response::build_directory_listing_response(const string &dir_path, const c_request &request)
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
