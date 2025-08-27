#include "request.hpp"

/************ HEADERS FUNCTIONS ************/

string  c_request::ft_trim(const string& str)
{
    size_t start = 0;
    size_t end = str.length();

    while (start < end && (str[start] == ' ' || str[start] == '\t'))
        start++;

    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
        end--;
    
    return (str.substr(start, end - start));
}

bool    c_request::is_valid_header_name(const string& key_name)
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
		this->_content_length = strtoul(value.c_str(), &end, 10);
	}

	return (true);
}


/************ HOST ************/

// bool	is_valid_label(string& label)
// {

// 	if (label.empty() || label.length() > 63)
// 	{
// 		return (false);
// 	}

// 	if (!isalnum(label[0]) || !isalnum(label[label.size() - 1]))
// 		return (false);
	
// 	for (size_t i = 1; i < label.length() - 1; i++)
// 	{
// 		if (!isalnum(label[i]) && label[i] != '-')
// 			return (false);
// 	}
// 	return (true);
// }

// bool	is_valid_hex_group(string& group)
// {
// 	if (group.empty() || group.size() > 4)
// 		return (false);
	
// 	for (size_t i = 0; i < group.size(); i++)
// 	{
// 		if (!isxdigit(group[i]))
// 			return (false);
// 	}
// 	return (true);
// }

// bool	is_valid_octet(const string &part)
// {
// 	if (part.empty() || part.size() > 3)
// 		return (false);

// 	for (size_t i = 0; i < part.size(); i++)
// 	{
// 		if (!isdigit(part[i]))
// 			return (false);
// 	}

// 	if (part.size() > 1 && part[0] == '0')
// 		return (false);

// 	int num = atoi(part.c_str());
// 	if (num < 0 || num > 255)
// 		return (false);

// 	return (true);
// }

// bool	is_valid_ipv6(string& ipv6)
// {
// 	size_t	colons = 0;
// 	size_t	double_colons = string::npos;
// 	size_t	start = 0;

// 	if (ipv6[0] != '[' || ipv6[ipv6.size() - 1] != ']' || ipv6.size() <= 2)
// 		return (false);

// 	/* Retirer les crochets */
// 	ipv6 = ipv6.substr(1, ipv6.size() - 2);
	
// 	for (size_t i = 0; i < ipv6.size(); i++)
// 	{
// 		if (i == ipv6.size() || ipv6[i] == ':')
// 		{
// 			string	group = ipv6.substr(start, i - start);

// 			if (group.empty())
// 			{
// 				if (i > 0 && ipv6[i - 1] == ':')
// 				{
// 					if (double_colons != string::npos)
// 						return (false);
// 					double_colons = i - 1;
// 				}
// 			}
// 			else
// 			{
// 				if (!is_valid_hex_group(group))
// 					return (false);
// 			}
// 			start = i + 1;
// 			colons++;
// 		}
// 	}

// 	int groups = colons - 1;
// 	if (double_colons == string::npos && groups != 8)
// 		return (false);
// 	if (double_colons != string::npos && groups > 8)
// 		return (false);

// 	return (true);
// }

// bool	is_valid_ipv4(string& ipv4)
// {
// 	string	part;
// 	istringstream stream(ipv4);

// 	for (int i = 0; i < 3; i++)
// 	{
// 		if (!getline(stream, part, '.') || !is_valid_octet(part))
// 			return (false);
// 	}

// 	if (!getline(stream, part) || !is_valid_octet(part))
// 		return (false);

// 	return (true);
// }

// bool		c_request::is_host_value_valid(int host_type)
// {
// 	string	uri_host = this->_host_value.first;
// 	int		port = this->_host_value.second;

// 	if (uri_host.empty())
// 		return (false);

// 	switch (host_type)
// 	{
// 		case INVALID:
// 			return (false);
// 		case HOSTNAME:
// 		{
// 			if (uri_host.size() > 253)
// 				return (false);
			
// 			stringstream 	ss(uri_host);
// 			string			label;			
// 			while (getline(ss, label, '.'))
// 			{
// 				if (!is_valid_label(label))
// 				{
// 					cerr << "(Request) Error: invalid label for hostname\n" << endl; 
// 					return (false);
// 				}
// 			}
// 			break;
// 		}
// 		case IPV4:
// 		{
// 			if (uri_host.size() > 15 || !is_valid_ipv4(uri_host))
// 				return (false);
// 			break;
// 		}
// 		case IPV6:
// 		{
// 			if (uri_host.size() > 39 || !is_valid_ipv6(uri_host))
// 				return (false);
// 			break;
// 		}			
// 	}
// 	if (port > 65535 || port <= 0)
// 		return (false);

// 	return (true);
// }

// HostType    c_request::detect_host_type(string& host)
// {
// 	string	uri_host;
// 	size_t	pos;
// 	string	port;
// 	int		colon_count;
// 	char	*end;

// 	this->_host_value.second = -1;

//     if ((!isalnum(host[0]) && host[0] != '[') 
// 		|| (!isalnum(host[host.size() - 1]) && host[host.size() - 1] != ']'))
// 		return (INVALID);
	
// 	else
// 	{
// 		/* Presence d'un port */
// 		colon_count = count(host.begin(), host.end(), ':');
// 		if (colon_count == 1)
// 		{
// 			pos = host.find(':', 0);
// 			uri_host = host.substr(0, pos);
// 			port = host.substr(pos + 1, pos - uri_host.size());
// 			if (port.find_first_not_of("0123456789") != string::npos)
// 				return (INVALID);
// 			this->_host_value.second = strtol(port.c_str(), &end, 10);

// 			this->_host_value.first = uri_host;
// 			if (uri_host.find_first_not_of("0123456789.") == string::npos)
// 				return (IPV4);
// 			else
// 				return (HOSTNAME);
// 		}

// 		else if (colon_count > 1 && host[host.size() - 1] != ']')
// 		{
// 			pos = host.rfind(':');
// 			this->_host_value.first = host.substr(0, pos);
// 			port = host.substr(pos + 1);
// 			cout << __FILE__ << "/" << __LINE__ << endl;
// 			if (port.find_first_not_of("0123456789") != string::npos || port.empty())
// 				return (INVALID);
// 			this->_host_value.second = strtol(port.c_str(), &end, 10);
// 			return (IPV6);
// 		}

// 		/* Pas de port */
// 		if (this->_host_value.second == -1)
// 			this->_host_value.second = 8080;

// 		if (host[0] == '[' && host[host.size() - 1] == ']')
// 		{
// 			this->_host_value.first = host;
// 			return (IPV6);			
// 		}

// 		else if (host.find_first_not_of("0123456789.") == string::npos)
// 		{
// 			this->_host_value.first = host;
// 			return (IPV4);
// 		}

// 		else
// 		{
// 			this->_host_value.first = host;
// 			return (HOSTNAME);
// 		}
// 	}
// }

int    c_request::check_port()
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
    return (this->_port);
}
