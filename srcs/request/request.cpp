#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request()
{
	this->init_request();
}

c_request::~c_request()
{
}


/************ REQUEST ************/

int	c_request::read_request(int socket_fd)
{
	char	buffer[BUFFER_SIZE];
	int		receivedBytes;
	string	request;
	
	this->init_request();

	/* ----- Lire jusqu'a la fin des headers ----- */
	while (request.find("\r\n\r\n") == string::npos)
	{
		fill(buffer, buffer + sizeof(buffer), '\0');
		receivedBytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (receivedBytes == 0)
		{
			cout << "Client closed connection" << endl;
			this->_error = true;
			return (400);
		}
		if (receivedBytes < 0) 
		{
			this->_error = true;
			return (408);
		}
		buffer[receivedBytes] = '\0';
        request.append(buffer);
	}
	this->parse_request(request);

	/* -----Lire le body -----*/
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);

	/* ----- 1. Cas Content-Length ----- */
	if (this->_has_body && this->get_content_lentgh())
	{
		size_t 	max_body_size = this->_content_length;
		size_t	total_bytes = 0;

		this->fill_body_with_bytes(body_part.data(), body_part.size());
		total_bytes += body_part.size();
		while (total_bytes < max_body_size)
		{	
			fill(buffer, buffer + sizeof(buffer), '\0');
			receivedBytes = recv(socket_fd, buffer, sizeof(buffer) -1, 0);
			
    		if (receivedBytes <= 0)
			{
				// Connexion fermee avant d'avoir tout recu
				cerr << "(Request) Error: Incomplete body" << endl;
				this->_error = true;
    		    return (receivedBytes == 0) ? 400 : 408;
			}
			buffer[receivedBytes] = '\0';
			total_bytes += receivedBytes;
			if (total_bytes > max_body_size)
			{
				cerr << "(Request) Error: actual body size (" << total_bytes 
					<< ") excess announced size (" << max_body_size << ")" << endl;
				this->_error = true;
				return (413);
			}
    		this->fill_body_with_bytes(buffer, receivedBytes);
			if (total_bytes == max_body_size)
				break;
		}
		fill(buffer, buffer + sizeof(buffer), '\0');
	}

	/* ----- 2. Cas chunked ----- */
	if (this->_has_body && this->get_header_value("Transfer-Encoding") == "chunked")
	{
		if (!body_part.empty())
			this->fill_body_with_chunks(body_part);

		while (!this->_request_fully_parsed && !this->_error)
		{
			fill(buffer, buffer + sizeof(buffer), '\0');
			receivedBytes = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
			if (string(buffer) == "\0\r\n\r\n")
				break;
        	if (receivedBytes <= 0) 
			{
				this->_error = true;
				return (receivedBytes == 0) ? 400 : 408;
			}
			buffer[receivedBytes] = '\0';
			this->_chunk_accumulator.append(buffer, receivedBytes);
			this->fill_body_with_chunks(this->_chunk_accumulator);
		}
	}
	else
		this->_request_fully_parsed = true;
	return (this->_status_code);
}

int c_request::parse_request(const string& raw_request)
{
	istringstream stream(raw_request);
	string line;

	/*---- ETAPE 1: start-line -----*/
	if (!getline(stream, line, '\n'))
	{
		this->_error = true;
		this->_status_code = 400;
	}   

	if (line.empty() || line[line.size() - 1] != '\r')
	{
		this->_error = true;
		this->_status_code = 400;
	} 
	line.erase(line.size() - 1);

	this->parse_start_line(line);

	/*---- ETAPE 2: headers -----*/
	while (getline(stream, line, '\n'))
	{
		if (line[line.size() - 1] != '\r')
		{
			this->_error = true;
			this->_status_code = 400;
		} 

		line.erase(line.size() - 1);

		if (line.empty())
			break ;
		
		this->parse_headers(line);
	}
	this->check_required_headers();

	return (0);
}

/************ START-LINE ************/

int c_request::parse_start_line(string& start_line)
{
	size_t start = 0;
	size_t pos = start_line.find(' ', start);
	
	/*- ---- Method ----- */
	if (pos == string::npos)
	{
		this->_error = true;
		this->_status_code = 400;
		return (0);
	}
	this->_method = start_line.substr(start, pos - start);;
	if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE")
	{
		this->_error = true;
		this->_status_code = 405;
		return (0);
	}

	/*- ---- Target ----- */
	start = pos + 1;
	pos = start_line.find(' ', start);

	if (pos == string::npos)
	{
		this->_error = true;
		this->_status_code = 400;
		return (0);
	}
	this->_target = start_line.substr(start, pos - start);
	
	/*- ---- Version ----- */
	start = pos + 1;
	this->_version = start_line.substr(start);
	if (this->_version.empty())
	{
		this->_error = true;
		this->_status_code = 400;
		return (0);
	}
	if (this->_version != "HTTP/1.1")
	{
		this->_error = true;
		this->_status_code = 505;
		return (0);
	}
	return (1);
}


/************ HEADERS ************/

int c_request::parse_headers(string& headers)
{
	size_t pos = headers.find(':', 0);
	string key;
	string value;

	key = headers.substr(0, pos);
	if (!is_valid_header_name(key))
	{
		cerr << "(Request) Error: invalid header_name: " << key << endl;
		this->_error = true;
		this->_status_code = 400;
	}

	pos++;
	if (headers[pos] != 32)
	{
		this->_error = true;
		this->_status_code = 400;
	}

	value = ft_trim(headers.substr(pos + 1));
	if (!is_valid_header_value(key, value))
	{
		cerr << "(Request) Error: invalid header_value: " << key << endl;
		this->_error = true;
		this->_status_code = 400;
	}

	this->_headers[key] = value;
	return (0);
}


/************ BODY ************/

void    c_request::fill_body_with_bytes(const char *buffer, size_t len)
{
    this->_body.append(buffer, len);
}

void c_request::fill_body_with_chunks(string &accumulator)
{
	string			tmp;
	size_t			pos;

	while ((pos = accumulator.find("\r\n")) != string::npos)
    {
        tmp = accumulator.substr(0, pos);
		accumulator.erase(0, pos + 2); // supprimer \r\n

		if (tmp.empty())
			continue ;
		this->_chunk_line_count++;

        if (this->_chunk_line_count % 2 == 1)
        {
            // On lit la taille du chunk
            cout << "Lecture taille chunk: " << tmp << endl;
            this->_expected_chunk_size = strtoul(tmp.c_str(), NULL, 16);
            cout << "Taille en bytes: " << this->_expected_chunk_size << endl;
            
            // Si taille = 0, c'est le chunk final
            if (this->_expected_chunk_size == 0)
            {
                cout << "*** Chunk final détecté - Body complet ***" << endl;
                this->_request_fully_parsed = true;
				accumulator.clear();
                return;
            }
        }
        else
        {
            // On lit les données du chunk
            cout << "Lecture données chunk (attendu: " << this->_expected_chunk_size << " bytes): ";
            
            if (tmp.size() < this->_expected_chunk_size)
            {
				accumulator.insert(0, tmp + "\r\n");
				this->_chunk_line_count--;
                return;
            }
			if (tmp.size() > this->_expected_chunk_size)
			{
				cerr << "(Request) Error: Invalid chunk size: " 
                     << "reçu: " << tmp.size() << " attendu: " << _expected_chunk_size << endl;
                this->_error = true;
                this->_status_code = 400;
                return;				
			}
            
			cout << "✓ Chunk valide, ajout au body !\n" << endl;
            this->fill_body_with_bytes(tmp.c_str(), this->_expected_chunk_size);
        }
    }
}

/************ SETTERS & GETTERS ************/

const string& c_request::get_header_value(const string& key) const
{
	static const string empty_string;

	for (map<string, string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
	{
		if (it->first == key)
			return (it->second);
	}
	return (empty_string);
}

void c_request::set_status_code(int code)
{
	this->_status_code = code;
}

