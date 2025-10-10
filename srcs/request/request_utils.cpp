#include "request.hpp"

/************ CHECK FUNCTIONS ************/

bool    c_request::is_valid_header_value(string& key, const string& value)
{
	for (size_t i = 0; i < value.length(); i++)
	{
		if ((value[i] < 32 && value[i] != '\t') || value[i] == 127)
		{
			_server.log_message("[ERROR] Invalid char in header_value: " + key);
			return (false);
		}
	}

	if (key == "Content-Length")
	{
		for (size_t i = 0; i < value.length(); i++)
		{
			if (!isdigit(value[i]))
			{
				_server.log_message("[ERROR] Invalid Content_Length: " + value);
				return (false);
			}
		}
		char   *end;
		this->_content_length = strtoul(value.c_str(), &end, 10);
	}

	if (value.size() > 4096)
	{
		_server.log_message("[ERROR] Header field too large: " + value);
		return (false);
	}
	return (true);
}

bool	c_request::is_uri_valid()
{
	if (this->_target.empty())
		return (false);

	if (this->_target.find("//") != string::npos || this->_target.find("..") != string::npos)
		return (false);

	for (size_t i = 0; i < this->_path.size(); i++)
	{
		char c = this->_path[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '/' || c == '-' || c == '_' || c == '.'  || c == '~'))
			return (false);
	}

	for (size_t i = 0; i < this->_query.size(); i++)
	{
		char c = this->_query[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '&' || c == '=' || c == '$' ||
              c == '?' || c == '-' || c == '_' || c == '.' || c == '~' || c == '%'))
            return false;
	}
	return (true);
}

void	c_request::check_required_headers()
{
	bool has_content_length = this->_headers.count("Content-Length");
	bool has_transfer_encoding = this->_headers.count("Transfer-Encoding");

	if (this->_method == "POST" && (has_content_length || has_transfer_encoding))
	{
		_server.log_message("[DEBUG] Request has a body");
		this->_has_body = true;
	}

	if (this->_headers.count("Host") != 1)
	{
		_server.log_message("[ERROR] invalid header Host");
		this->_error = true;
		this->_status_code = 400;
	}

	if (this->_has_body)
	{
		if (!has_content_length && !has_transfer_encoding)
		{
			this->_error = true;
			this->_status_code = 400;
			_server.log_message("[ERROR] Missing header indicating body size");
		}
		if (has_content_length && has_transfer_encoding)
		{
			this->_error = true;
			this->_status_code = 400;
			_server.log_message("[ERROR] Misleading header body size");
		}
	}     
}

void    c_request::check_port()
{
	char *end;
	size_t pos = this->get_header_value("Host").find_first_of(':');

	if (pos == string::npos)
		this->_port = 80;
	else
	{
		string port = this->_headers["Host"].substr(pos + 1);
		this->_port = strtod(port.c_str(), &end);
	}
}

/************ UTILS ************/

void	c_request::print_full_request() const
{
	cout << "print full request" << endl;
	if (this->_request_fully_parsed)
	{
		cout << "*********** START-LINE ************" << endl;
		cout << "method: " << this->_method << endl;
		if (!this->_query.empty())
			cout << "query: " << this->_query << endl;
		cout << "target: " << this->_target << endl;
		cout << "version: " << this->_version << endl << endl;

		cout << "************ HEADERS **************" << endl;
		
		for (map<string, string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
			cout << it->first << " : " << it->second << endl;
		cout << endl;
		
		if (this->_has_body)
		{
			cout << "************* BODY ***************" << endl;
			cout << this->_body << endl;
		}

		cout << endl;
	}
}

void	c_request::init_request()
{
	this->_method.clear();
	this->_query.clear();
	this->_target.clear();
	this->_version.clear();
	this->_path.clear();
	this->_body.clear();
	this->_chunk_accumulator.clear();
	this->_status_code = 200;
	this->_port = 8080;
	this->_has_body = false;
	this->_chunk_line_count = 0;
	this->_expected_chunk_size = -1;
	this->_request_fully_parsed = false;
	this->_error = false;
	this->_disconnected = false;
	this->_content_length = 0;
	this->_chunk_line_count = 0;
	this->_client_max_body_size = 0; // limite systeme par defaut
	this->_socket_fd = _client.get_fd();
	this->_ip_client = _client.get_ip();

	for (map<string, string>::iterator it = _headers.begin(); it != _headers.end(); it++)
		it->second = "";
}


