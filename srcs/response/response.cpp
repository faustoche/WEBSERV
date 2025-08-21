#include "response.hpp"

string	c_response::define_response_content()
{
	ifstream	file;
	string		content;
	string		line;
	string		response;
	
	file.open("www/index.html");
	if (!file.is_open())
	cout << "Error: can't open the file\n";
	
	while (getline(file, line))
		content += line + "\n";
	
	int	content_size = content.length();
	file.close();

	ostringstream oss;
	oss << content_size;
	response = "HTTP/1.1 200 OK\r\n"; // dynamic
	response += "Content-Type: text/html\r\n"; // dynamic
	response += "Content-Length: " + oss.str() + "\r\n";
	response += "Server: webserv/1.0\r\n";
	response += "Connection: keep-alive\r\n"; // dynamic
	response += "\r\n";
	response += content;

	return (response);
}