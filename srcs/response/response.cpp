#include "response.hpp"

const string &c_response::get_response() const {
	return (_response);
}

void	c_response::define_response_content()
{
	ifstream	file;
	string		content;
	string		line;
	
	file.open("www/index.html");
	if (!file.is_open()) {
		cout << "Error: can't open the file\n";
		return ;
	}
	
	while (getline(file, line))
		content += line + "\n";
	
	int	content_size = content.length();
	file.close();

	ostringstream oss;
	oss << content_size;

	_response = "HTTP/1.1 200 OK\r\n"; // dynamic
	_response += "Content-Type: text/html\r\n"; // dynamic
	_response += "Content-Length: " + oss.str() + "\r\n";
	_response += "Server: webserv/1.0\r\n";
	_response += "Connection: keep-alive\r\n"; // dynamic
	_response += "\r\n";
	_response += content;
}

// void c_response::build_success_response(string &file_path, string version)
// {
// 	string content = load_file_content(file_path);
// 	if (content.empty())
// 	{
// 		build_error_response(404, version);
// 		return ;
// 	}

// 	int content_size = content.length();
// 	ostringstream oss;
// 	oss << content_size;

// 	_response = version + " 200 OK\r\n"; // dynamic
// 	_response += "Content-Type: " + get_content_type(file_path) + "\r\n"; // dynamic
// 	_response += "Content-Length: " + oss.str() + "\r\n";
// 	_response += "Server: webserv/1.0\r\n";
// 	_response += "Connection: keep-alive\r\n"; // dynamic
// 	_response += "\r\n";
// 	_response += content;
// }

// string c_response::load_file_content(string &file_path)
// {
// 	ifstream	file(file_path);
// 	string		content;
// 	string		line;
	
// 	file.open("www/index.html");
// 	if (!file.is_open()) {
// 		cout << "Error: can't open the file\n";
// 		return ;
// 	}
	
// 	while (getline(file, line))
// 		content += line + "\n";
	
// 	int	content_size = content.length();
// 	file.close();
// }

// string c_response::get_content_type(string &file_path)
// {
// 	size_t dot_position = file_path.find_last_of('.');
// 	if (dot_position == string::npos)
// 		return ("text/plain");
	
// 		string extensio

// }
