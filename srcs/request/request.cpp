#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request() : 
_method(""), _version(""), _status_code(200), _port(8080), _has_body(false), _content_length(0)
{
	for (map<string, string>::iterator it = _headers.begin(); it != _headers.end(); it++)
		it->second = "";
}

c_request::~c_request()
{
}

/************ PARSE FUNCTIONS ************/

void c_request::check_required_headers()
{
	bool has_content_length = this->_headers.count("Content-Length");
	bool has_transfer_encoding = this->_headers.count("Transfer-Encoding");

    if (this->_method == "POST" && (has_content_length || has_transfer_encoding))
	{
		cout << "Warning: Request has body!\n" << endl;
        this->_has_body = true;
	}

	if (this->_headers.count("Host") != 1)
	{
		cerr << "(Request) Error: Header host" << endl;
		this->_status_code = 400;
	}

	if (this->_has_body)
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

int c_request::parse_headers(string& headers)
{
	size_t pos = headers.find(':', 0);

	string key;
	string value;

	key = headers.substr(0, pos);
	if (!is_valid_header_name(key))
	{
		cerr << "(Request) Error: invalid header_name: " << key << endl;
		this->_status_code = 400;
	}

	pos++;
	if (headers[pos] != 32)
		this->_status_code = 400;

	value = ft_trim(headers.substr(pos + 1));
	if (!is_valid_header_value(key, value))
	{
		cerr << "(Request) Error: invalid header_value: " << key << endl;
		this->_status_code = 400;
	}

	this->_headers[key] = value;

	return (0);
}

void debugLine(const std::string &s)
{
    std::cout << "DEBUG: [";
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (isprint(c))
            std::cout << s[i];
        else
            std::cout << "\\x" << std::hex << (int)c << std::dec;
    }
    std::cout << "] (len=" << s.size() << ")" << std::endl;
}

int c_request::fill_body_with_chunks(const char *buffer)
{
	string			tmp;
	istringstream 	stream(buffer);
	bool			hex_expected = true;
	bool			string_expected = false;
	size_t 			size_in_bytes = 0;
	bool			body_is_complete = false;

	cout << "Body_part dans fill_body_with_chunks: " << buffer << endl;

	while (getline(stream, tmp, '\n') && tmp != "0\r\n\r\n")
	{
		// debugLine(tmp);
		// Retirer '\r'
		tmp.erase(tmp.size() - 1);
		if (string_expected)
		{
			if (tmp.size() != size_in_bytes)
			{
				cerr << "(Request) Error: Invalid chunk size: " << "tmp.size(): " << tmp.size() << " size_in_bytes: " << size_in_bytes << endl;
				return (413);
			}
			this->fill_body_with_bytes(tmp.c_str(), size_in_bytes);
			hex_expected = true;
			string_expected = false;
		}
		else
		{
			// debugLine(tmp);
			std::cout << std::endl;
			size_in_bytes = strtoul(tmp.c_str(), NULL, 16);
			hex_expected = false;
			string_expected = true;
		}
	}
	cout << "*** Body complet ***" << endl;
	return (200);
}


int	c_request::read_request(int socket_fd)
{
	char	buffer[BUFFER_SIZE];
	int		receivedBytes;
	string	request;

	while (request.find("\r\n\r\n") == string::npos)
	{
		fill(buffer, buffer + sizeof(buffer), '\0');
		receivedBytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (receivedBytes <= 0) 
		{
			if (receivedBytes == 0) 
			{
				cout << "Client closed connection" << endl;
				return (400);
			} 
			else if (errno == EAGAIN || errno == EWOULDBLOCK) 
			{
				cout << "Error: Timeout - no data received" << endl;
				return (408);
			} 
			else 
			{
				cerr << "Error: message not received - " << errno << endl;
				return (500);
			}
		}
		buffer[receivedBytes] = '\0';
        request.append(buffer);
	}
	this->parse_request(request);
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);

	if (this->_has_body && this->get_content_lentgh())
	{
		size_t 	max_body_size = this->_content_length;
		size_t	total_bytes = 0;

		this->fill_body_with_bytes(body_part.data(), body_part.size());
		total_bytes += body_part.size();
		while (total_bytes < max_body_size)
		{	
			fill(buffer, buffer + sizeof(buffer), '\0');
			receivedBytes = recv(socket_fd, buffer, sizeof(buffer), 0);
			
    		if (receivedBytes == 0)
			{
				// Connexion fermee avant d'avoir tout recu
				cerr << "(Request) Error: Incomplete body" << endl;
    		    return (400);
			}
			if (receivedBytes < 0)
			{
				// Reessayer en cas d'erreur reseau
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					continue ;
				return (500);
			}
			total_bytes += receivedBytes;
    		this->fill_body_with_bytes(buffer, receivedBytes);
		}

		if (total_bytes > max_body_size)
		{
			cerr << "(Request) Error: actual body size excess announced size" << endl;
			return (413);
		}
		fill(buffer, buffer + sizeof(buffer), '\0');

	}

	cout << "Body_part avant dechunk: " << body_part << endl;
	if (this->_has_body && this->get_header_value("Transfer-Encoding") == "chunked")
	{
		int status_code = this->fill_body_with_chunks(body_part.c_str());
		if (status_code != 200)
			return (status_code);
		while (true)
		{
			fill(buffer, buffer + sizeof(buffer), '\0');
			receivedBytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
			if (string(buffer) == "\0\r\n\r\n")
				break;
        	if (receivedBytes <= 0) 
			{
				if (receivedBytes == 0) 
				{
					cout << "Client closed connection" << endl;
					return (400);
				} 
				else if (errno == EAGAIN || errno == EWOULDBLOCK) 
				{
					cout << "Error: Timeout - no data received" << endl;
					return (408);
				} 
				else 
				{
					cerr << "Error: message not received - " << errno << endl;
					return (500);
				}
			}
        	body_part.append(buffer, receivedBytes);
			cout << "Body part: " << body_part << endl;
			this->fill_body_with_chunks(body_part.c_str());
		}

		/*
		idee chatgpt:
		while (!finished)
		{
		    // 1. Lire un chunk size si nécessaire
		    if (!has_current_chunk_size)
		    {
		        size_t pos = body_part.find("\r\n");
		        if (pos == string::npos)
		        {
		            // besoin de plus de données
		            receivedBytes = recv(socket_fd, buffer, sizeof(buffer), 0);
		            if (receivedBytes <= 0) handle_error();
		            body_part.append(buffer, receivedBytes);
		            continue;
		        }
		        string hex = body_part.substr(0, pos);
		        current_chunk_size = strtoul(hex.c_str(), NULL, 16);
		        body_part.erase(0, pos + 2); // retirer la ligne taille + \r\n
		        if (current_chunk_size == 0) break; // dernier chunk
		    }

		    // 2. Lire le chunk content
		    if (body_part.size() < current_chunk_size + 2) // +2 pour le \r\n
		    {
		        receivedBytes = recv(socket_fd, buffer, sizeof(buffer), 0);
		        if (receivedBytes <= 0) handle_error();
		        body_part.append(buffer, receivedBytes);
		        continue;
		    }

		    // 3. Extraire le chunk et remplir le body
		    this->fill_body(body_part.c_str(), current_chunk_size);
		    body_part.erase(0, current_chunk_size + 2); // retirer chunk + \r\n
		    has_current_chunk_size = false;
		}
		*/
	}
	return (200);
}

/************ SETTERS & GETTERS ************/

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

void    c_request::fill_body_with_bytes(const char *buffer, size_t len)
{
    this->_body.append(buffer, len);
}
