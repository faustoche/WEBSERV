#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

// c_request::c_request() : _server(NULL)
// {
// 	this->init_request();
// }

c_request::c_request(c_server& server) : _server(server)
{
	this->init_request();
	// (void)ip_str;
	// this->_ip_client = static_cast<string>(ip_str);
}

c_request::~c_request()
{
}


/************ REQUEST ************/

void	c_request::read_request(int socket_fd)
{
	char	buffer[BUFFER_SIZE];
	int		receivedBytes;
	string	request;
	
	this->init_request();
	this->_socket_fd = socket_fd;


	/* ----- Lire jusqu'a la fin des headers ----- */
	while (request.find("\r\n\r\n") == string::npos)
	{
		fill(buffer, buffer + sizeof(buffer), '\0');
		// condition pour l'appel de recv ?
		receivedBytes = recv(socket_fd, buffer, sizeof(buffer) - 1, MSG_NOSIGNAL);
        if (receivedBytes <= 0)
		{
			if (receivedBytes == 0) // break ou vrai erreur ?
			{
				cout << "(Request) client closed connection: " << __FILE__ << "/" << __LINE__ << endl;;
				this->_error = true;
				// close(this->_socket_fd);
				return ;
			} 
			else
			{
				cout << "(Request) Error: client disconnected unexepectedly: " << __FILE__ << "/" << __LINE__ << endl;;
				this->_error = true;
				return ;
			}
		}
		buffer[receivedBytes] = '\0';
		request.append(buffer);
	}
	this->parse_request(request);
	c_location *matching_location = _server.find_matching_location(this->get_target());
	if (matching_location != NULL && matching_location->get_body_size() > 0)
	{
		this->set_client_max_body_size(matching_location->get_body_size());
	}
	
	/* -----Lire le body -----*/
	this->determine_body_reading_strategy(socket_fd, buffer, request);

	if (!this->_error)
		this->_request_fully_parsed = true;
	else
		return ;
}

int c_request::parse_request(const string& raw_request)
{
	istringstream stream(raw_request);
	string line;

	/*---- ETAPE 1: start-line -----*/
	if (!getline(stream, line, '\n'))
	{
		this->_status_code = 400;
		this->_error = true;
	} 

	if (line.empty() || line[line.size() - 1] != '\r')
	{
		this->_status_code = 400;
		this->_error = true;
		return (1);
	}
	line.erase(line.size() - 1);

	
	this->parse_start_line(line);
	
	/*---- ETAPE 2: headers -----*/
	while (getline(stream, line, '\n'))
	{
		if (line[line.size() - 1] != '\r')
		{
			this->_status_code = 400;
			this->_error = true;
			return (1);
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
	size_t	start = 0;
	size_t	space_pos = start_line.find(' ', start);
	string	tmp = "";
	
	/*- ---- Method ----- */
	if (space_pos == string::npos)
	{
		this->_status_code = 400;
		this->_error = true;
		return (0);
	}
	this->_method = start_line.substr(start, space_pos - start);;
	if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE")
	{
		this->_status_code = 405;
		this->_error = true;
		return (0);
	}

	/*- ---- Target ----- */
	start = space_pos + 1;
	space_pos = start_line.find(' ', start);

	if (space_pos == string::npos)
	{
		this->_status_code = 400;
		this->_error = true;
		return (0);
	}
	tmp = start_line.substr(start, space_pos - start);
	this->_target = tmp;

	
	size_t	question_pos;
	if ((question_pos = tmp.find('?')) != string::npos)
	{
		this->_path = tmp.substr(0, question_pos);
		this->_query = tmp.substr(question_pos + 1);
		cout << "this->_path: " << this->_path << endl;
		cout << "this->_query: " << this->_query << endl;
	}	
	else
	{
		this->_path = start_line.substr(start, space_pos - start);
		this->_query = "";
	}
	

	/*- ---- Version ----- */
	start = space_pos + 1;
	this->_version = start_line.substr(start);
	if (this->_version.empty())
	{
		
		this->_status_code = 400;
		return (0);
	}
	if (this->_version != "HTTP/1.1")
	{
		cout << "this->_version: " << this->_version << endl; 
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


/************ BODY ************/

int    c_request::fill_body_with_bytes(const char *buffer, size_t len)
{
    this->_body.append(buffer, len);
	if (this->_client_max_body_size > 0 && this->_body.size() > this->_client_max_body_size)
	{
		this->_status_code = 413;
		this->_error = true;
		return (1);
	}
	return (0);
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
            this->_expected_chunk_size = strtoul(tmp.c_str(), NULL, 16);
            cout << "Taille en bytes: " << this->_expected_chunk_size << endl;
            
            // Si taille = 0, c'est le chunk final
            if (this->_expected_chunk_size == 0)
            {
                cout << "*** Chunk final détecté - Body complet ***\n" << endl;
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
                this->_status_code = 400;
				this->_error = true;
                return;				
			}
            
			cout << "✓ Chunk valide, ajout au body !\n" << endl;
            if (this->fill_body_with_bytes(tmp.c_str(), this->_expected_chunk_size))
				return ;
        }
    }
}


void	c_request::read_body_with_chunks(int socket_fd, char* buffer, string request)
{
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);
	int		receivedBytes;

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
			if (receivedBytes == 0) 
			{
				cout << "Client " << socket_fd << " closed connection, cleaning up socket " << endl;;
				this->_disconnected = true;
			} 
			else
			{
				cout << "Client " << socket_fd << " client disconnected unexepectedly, closing socket " << endl;;
				this->_error = true;
				return ;
			}
		}
		// buffer[receivedBytes] = '\0';
		this->_chunk_accumulator.append(buffer, receivedBytes);
		this->fill_body_with_chunks(this->_chunk_accumulator);
	}
}

void	c_request::read_body_with_length(int socket_fd, char* buffer, string request, size_t buffer_size)
{
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);

	size_t 	max_body_size = this->_content_length;
	int		receivedBytes;
	size_t	total_bytes = 0;

	if (!body_part.empty())
	{
		if (this->fill_body_with_bytes(body_part.data(), body_part.size()))
			return ;
		total_bytes += body_part.size();

		if (total_bytes >= max_body_size)
		{
			cout << "(Request) Complete body already received in the initial request" << endl;
			return;
		}
	}

	while (total_bytes < max_body_size)
	{	
		fill(buffer, buffer + buffer_size, '\0');
		receivedBytes = recv(socket_fd, buffer, buffer_size -1, 0); // faut-il conditionner l'appel a recv
		if (receivedBytes <= 0)
		{
    		if (receivedBytes == 0) // faut-il vraiment return et mettre _error a true ?
			{
				// verifier si on a recu tout ce qu'on attendait
				if (total_bytes == max_body_size)
				{
					cout << "(Request) Full body received, connection closed" << endl;
					return ;
				}
				// Connexion fermee avant d'avoir tout recu
				cerr << "(Request) Error: Incomplete body. Expected " << max_body_size 
					<< " and received " << total_bytes << endl;
				this->_error = true;
				return;
			}
			else // faut-il vraiment return et mettre _error a true ?
			{
				// erreur reseau
				cout << "(Request) Error: client disconnected unexepectedly: " << __FILE__ << "/" << __LINE__ << endl;
				this->_error = true;
				return;
			}
		}
		buffer[receivedBytes] = '\0';
		total_bytes += receivedBytes;
		
		if (total_bytes > max_body_size)
		{
			cerr << "(Request) Error: actual body size (" << total_bytes 
				<< ") excess announced size (" << max_body_size << ")" << endl;
			this->_status_code = 413;
			this->_error = true;
			return ;
		}

    	if (this->fill_body_with_bytes(buffer, receivedBytes))
			return ;

		if (total_bytes == max_body_size)
			break;
	}
	fill(buffer, buffer + sizeof(buffer), '\0');
}


void	c_request::determine_body_reading_strategy(int socket_fd, char* buffer, string request)
{
	if (this->_has_body)
	{
		if (this->get_content_length())
		{
			this->read_body_with_length(socket_fd, buffer, request, sizeof(buffer)); // sizeof(buffer) correspond a la taille du pointeur, changer pour passer BUFFER_SIZE
		}
		else
			this->read_body_with_chunks(socket_fd, buffer, request);
		if (this->_error)
			return ;
	}
	else
		this->_request_fully_parsed = true;
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

