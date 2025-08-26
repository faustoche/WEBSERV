#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request(const string& str) : 
_method(""), _version(""), _status_code(200), _port(8080), _content_length(0)
{
	for (map<string, string>::iterator it = _headers.begin(); it != _headers.end(); it++)
		it->second = "";

	try 
	{
		this->parse_request(str);
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}

c_request::~c_request()
{
}


/************ PARSE FUNCTIONS ************/

void c_request::check_required_headers()
{
	bool has_body = false;
	bool has_content_length = this->_headers.count("Content-Length");
	bool has_transfer_encoding = this->_headers.count("Transfer-Encoding");

    if (this->_method == "POST")
        has_body = true;

	else if (this->_headers.count("Host") != 1 || !this->check_host_value())
	{
		cerr << "(Request) Error: Header host" << endl;
		this->_status_code = 400;
	}
	else if (has_body)
	{
		if (!has_content_length && !has_transfer_encoding)
			this->_status_code = 400;
		if (has_content_length && has_transfer_encoding)
			this->_status_code = 400;
	}     
}

int c_request::parse_request(const string& raw_request)
{
	istringstream stream(raw_request);
	string line;

	//---- ETAPE 1: start-line -----
	if (!getline(stream, line, '\n'))
		this->_status_code = 400;        

	if (line.empty() || line[line.size() - 1] != '\r')
		this->_status_code = 400;
	line.erase(line.size() - 1);

	this->parse_start_line(line);

	//---- ETAPE 2: headers -----
	while (getline(stream, line, '\n'))
	{
		if (line[line.size() - 1] != '\r')
			this->_status_code = 400;

		line.erase(line.size() - 1);

		if (line.empty())
			break ;
		
		this->parse_headers(line);
	}
	this->check_required_headers();

	cout << "*********** Headers map ***********" << endl;
	for (map<string, string>::iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
		cout << it->first << " : " << it->second << endl;
	cout << endl;
	//---- ETAPE 3: body -----

	return (0);
}

int c_request::parse_start_line(string& start_line)
{
	size_t start = 0;
	size_t pos = start_line.find(' ', start);
	
	// METHOD
	if (pos == string::npos)
	{
		this->_status_code = 400;
		return (0);
	}
	this->_method = start_line.substr(start, pos - start);;
	if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE")
	{
		this->_status_code = 405;
		return (0);
	}

	// TARGET
	start = pos + 1;
	pos = start_line.find(' ', start);

	if (pos == string::npos)
	{
		this->_status_code = 400;
		return (0);
	}
	this->_target = start_line.substr(start, pos - start);
	
	// VERSION
	start = pos + 1;
	this->_version = start_line.substr(start);
	if (this->_version.empty())
	{
		this->_status_code = 400;
		return (0);
	}
	if (this->_version != "HTTP/1.1")
	{
		this->_status_code = 505;
		return (0);
	}

	cout << "*********** Start-line ************" << endl;
	cout << "method: " << this->_method << endl;
	cout << "target: " << this->_target << endl;
	cout << "version: " << this->_version << endl << endl;

    // VERSION
    start = pos + 1;
    this->_version = start_line.substr(start);
    if (this->_version.empty())
    {
        this->_status_code = 400;
        return (0);
    }
    if (this->_version != "HTTP/1.1")
    {
        this->_status_code = 505;
        return (0);
    }
	return (1);
}

string  ft_trim(const string& str)
{
    size_t start = 0;
    size_t end = str.length();

    while (start < end && (str[start] == ' ' || str[start] == '\t'))
        start++;

    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
        end--;
    
    return (str.substr(start, end - start));
}

bool    is_valid_header_name(const string& key_name)
{
	const string allowed_special_chars = "!#$%&'*+-.^_`|~";

	if (key_name.empty())
		return (false);
	for (size_t i = 0; i < key_name.length(); i++)
	{
		if (!isalnum(key_name[i]) && allowed_special_chars.find(key_name[i]) == string::npos)
			return (false);
	}
	return (true);
}

bool	is_valid_label(string& label)
{

	if (label.empty() || label.length() > 63)
	{
		return (false);
	}

	if (!isalnum(label[0]) || !isalnum(label[label.size() - 1]))
		return (false);
	
	for (size_t i = 1; i < label.length() - 1; i++)
	{
		if (!isalnum(label[i]) && label[i] != '-')
			return (false);
	}
	return (true);
}

bool	is_valid_hex_group(string& group)
{
	if (group.empty() || group.size() > 4)
		return (false);
	
	for (size_t i = 0; i < group.size(); i++)
	{
		if (!isxdigit(group[i]))
			return (false);
	}
	return (true);
}

bool	is_valid_ipv6(string& ipv6)
{
	size_t	colons = 0;
	size_t	double_colons = string::npos;
	size_t	start = 0;

	if (ipv6[0] != '[' || ipv6[ipv6.size() - 1] != ']' || ipv6.size() <= 2)
		return (false);
	
	ipv6 = ipv6.substr(1, ipv6.size() - 2);
	
	for (size_t i = 0; i < ipv6.size(); i++)
	{
		if (i == ipv6.size() || ipv6[i] == ':')
		{
			string	group = ipv6.substr(start, i - start);

			if (group.empty())
			{
				if (i > 0 && ipv6[i - 1] == ':')
				{
					if (double_colons != string::npos)
						return (false);
					double_colons = i - 1;
				}
			}
			else
			{
				if (!is_valid_hex_group(group))
					return (false);
			}

			start = i + 1;
			colons++;
		}
	}

	int groups = colons - 1;

	if (double_colons == string::npos && groups != 8)
		return (false);
	if (double_colons != string::npos && groups > 8)
		return (false);

	return (true);
}

bool	is_valid_ipv4(string& ipv4)
{
	int		num;
	char	dot;
	istringstream stream(ipv4);

	for (int i = 0; i < 4; i++)
	{
		if (!(stream >> num)) // si ce n'est pas stockable dans un int
			return (false);
		
		if (num < 0 || num > 255)
			return (false);

		if (i < 3)
		{
			if (!(stream >> dot) || dot != '.')
				return (false);
		}
	}

	if (stream >> dot)
		return (false);

	return (true);
}

bool		c_request::is_host_value_valid(int host_type)
{
	string	uri_host = this->_host_value.first;
	int		port = this->_host_value.second;

	if (uri_host.empty())
	{
		return (false);
	}

	cout << "HostType avant validation de la forme: " << host_type << endl;
	switch (host_type)
	{
		
		case INVALID:
			return (false);
		
		case HOSTNAME:
		{
			if (uri_host.size() > 253)
				return (false);
			
			stringstream 	ss(uri_host);
			string			label;			
			while (getline(ss, label, '.'))
			{
				if (!is_valid_label(label))
				{
					cerr << "(Request) Error: invalid label for hostname." << endl; 
					return (false);
				}
			}
			break;
		}

		case IPV4:
		{
			if (uri_host.size() > 15 || !is_valid_ipv4(uri_host))
				return (false);
			break;
		}

		case IPV6:
		{
			if (uri_host.size() > 39 || !is_valid_ipv6(uri_host))
				return (false);
			break;
		}			
	}

	if (port > 65535)
	{
		return (false);
	}

	return (true);
}

HostType    c_request::detect_host_type(string& host)
{
	string	uri_host;
	size_t	pos;
	string	port;
	int		colon_count;
	char	*end;

	cout << "Host_value: " << host << " / host size: " << host.size() << endl;
    if ((!isalnum(host[0]) && host[0] != '[') 
		|| (!isalnum(host[host.size() - 1]) && host[host.size() - 1] != ']'))
	{
		return (INVALID);
	}
	
	else
	{
	
		/* Presence d'un port */
		colon_count = count(host.begin(), host.end(), ':');
		if (colon_count == 1)
		{
			pos = host.find(':', 0);
			uri_host = host.substr(0, pos);
			port = host.substr(pos + 1, pos - uri_host.size());
			if (port.find_first_not_of("0123456789") != string::npos)
				return (INVALID);
			this->_host_value.second = strtod(port.c_str(), &end);

			this->_host_value.first = uri_host;
			if (uri_host.find_first_not_of("0123456789.") == string::npos)
				return (IPV4);
			else
				return (HOSTNAME);
		}

		else if (colon_count > 1 && host[host.size() - 1] != ']')
		{
			pos = host.rfind(':', 0);
			this->_host_value.first = host.substr(0, pos);
			port = host.substr(pos + 1, pos - this->_host_value.first.size());
			if (port.find_first_not_of("0123456789") != string::npos)
				return (INVALID);
			this->_host_value.second = strtod(port.c_str(), &end);
		}

		/* Port absent */
		else
		{
			this->_host_value.second = 8080;
			if (host[0] == '[' && host[host.size() - 1] == ']')
			{
				this->_host_value.first = host;
				return (IPV6);			
			}

			else if (host.find_first_not_of("0123456789.") == string::npos)
			{
				this->_host_value.first = host;
				return (IPV4);
			}		
			else
			{
				this->_host_value.first = host;
				return (HOSTNAME);
			}
		}
	}
	return (INVALID);
}

bool    c_request::check_host_value()
{
    string  header_value = this->get_header_value("Host");

    if (this->_target.substr(0, 6) == "file://" || this->_target.substr(0, 4) == "data:")
    {
        if (!header_value.empty())
		{
        	this->_status_code = 400;
        	return (false);
		}
    }

	else if (!is_host_value_valid(detect_host_type(header_value)))
	{
        this->_status_code = 400;
        return (false);
    }
	return (true);
}

bool    c_request::is_valid_header_value(string& key, const string& value)
{
	for (size_t i = 0; i < value.length(); i++)
	{
		if ((value[i] < 32 && value[i] != '\t') || value[i] == 127)
		{
			this->_status_code = 400;
			return (false);
		}
	}

	if (key == "Content-Length")
	{
		for (size_t i = 0; i < value.length(); i++)
		{
			if (!isdigit(value[i]))
			{
				this->_status_code = 400;
				return (false);
			}
		}
		char   *end;
		long limit_tester = strtol(value.c_str(), &end, 10);
		if (limit_tester > MAX_BODY_SIZE)
		{
			this->_status_code = 413;
			return (false);
		}
		else
			this->_content_length = limit_tester;
	}

	return (true);
}

int c_request::parse_headers(string& headers)
{
	size_t pos = headers.find(':', 0);

	string key;
	string value;

	key = headers.substr(0, pos);
	if (!is_valid_header_name(key))
		this->_status_code = 400;

	pos++;
	if (headers[pos] != 32)
		this->_status_code = 400;

	value = ft_trim(headers.substr(pos + 1));
	if (!is_valid_header_value(key, value))
		this->_status_code = 400;

	this->_headers[key] = value;

	return (0);
}

void    c_request::fill_body(const char *buffer, size_t len)
{
    this->_body.append(buffer, len);
}


/************ UTILS ************/

const string& c_request::get_header_value(const string& key) const
{
	for (map<string, string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
	{
		if (it->first == key)
			return (it->second);
	}
	throw std::out_of_range("Header not found: " + key);
}

void c_request::set_status_code(int code)
{
	this->_status_code = code;
}
