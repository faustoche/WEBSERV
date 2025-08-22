#include "response.hpp"

const string &c_response::get_response() const {
	return (_response);
}

void	c_response::define_response_content(const c_request &request)
{
	_response.clear();

	string method = request.get_method();
	string target = request.get_target();
	string version = request.get_version();
	std::map<string, string> headers = request.get_headers();

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
	if (target == "/" || target == "/index.html")
		build_success_response("www/index.html", version, request);
	if (headers.find("Host") != headers.end())
		build_success_response("www/index.html", version, request);
	else if (headers.find("Host") == headers.end())
		build_error_response(400, version, request);
	else
	{
		string file_path = "www" + target;
		string content = load_file_content(file_path);
		if (content.empty())
			build_error_response(404, version, request);
		else
			build_success_response(file_path, version, request);
	}
}

string c_response::load_file_content(const string &file_path)
{
	ifstream	file(file_path.c_str());
	string		content;
	string		line;
	
	if (!file.is_open()) {
		cout << "Error: can't open the file\n";
	}
	
	while (getline(file, line))
	content += line + "\n";
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
	else if (extension == ".gif")
		return ("image/gif");
	else if (extension == ".pdf")
		return ("application/pdf");
	else if (extension == ".zip")
		return ("application/zip");
	else
		return ("text/plain"); 
}

void c_response::build_success_response(const string &file_path, const string version, const c_request &request)
{
	string content = load_file_content(file_path);
	if (content.empty())
	{
		build_error_response(404, version, request);
		return ;
	}

	int content_size = content.length();
	ostringstream oss;
	oss << content_size;

	_response = version + " 200 OK\r\n";
	_response += "Content-Type: " + get_content_type(file_path) + "\r\n";
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";

	string connection = request.get_header_value("Connection");

	if (connection == "close")
		_response += "Connection: close\r\n";
	else if (connection == "keep-alive")
		_response += "Connection: keep-alive\r\n";
	else
		_response += "Connection: close\r\n";

	_response += "\r\n\r\n";
	_response += content;
}

/* Vérifier les messages d'erreur s'ils sont cohérents avec un autre site */

void c_response::build_error_response(int error_code, const string version, const c_request &request)
{
	string status;
	string error_content;

	switch (error_code)
	{
		case 400:
			status = "Bad Request";
			error_content = "<html><body><h1>400 - Bad Request</h1></body></html>";
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

	string connection = request.get_header_value("Connection");
	
	if (connection == "keep-alive" && (error_code == 404 || error_code == 405))
		_response += "Connection: keep-alive\r\n";
	else
		_response += "Connection: close\r\n";
	
	_response += "\r\n\r\n";
	_response += error_content;
}
