#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request(c_server& server, c_client &client) : _server(server), _client(client)
{
	this->init_request();
}

c_request::~c_request()
{
}


/************ REQUEST ************/
static bool has_end_of_headers(const std::vector<char> &buf)
{
	if (buf.size() < 4)
		return false;
	for (size_t i = 0; i + 3 < buf.size(); ++i)
	{
		if (buf[i] == '\r' && buf[i + 1] == '\n' &&
			buf[i + 2] == '\r' && buf[i + 3] == '\n')
			return true;
	}
	return false;
}

void	c_request::read_request()
{
	char			buffer[BUFFER_SIZE];
	int				receivedBytes;
	vector<char>	request;

	this->init_request();
	this->_socket_fd = _client.get_fd();

	/* ----- Lire jusqu'à la fin des headers ----- */
	while (!has_end_of_headers(request))
	{
		receivedBytes = recv(_socket_fd, buffer, BUFFER_SIZE, MSG_NOSIGNAL);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == 0)
			{
				_client.set_state(IDLE);
				return ;
			}
			else if (receivedBytes < 0)
			{
				_server.log_message("[WARN] recv() returned <0 for client " 
						+ int_to_string(this->_socket_fd) 
						+ ". Will retry on next POLLIN.");
				this->_request_fully_parsed = false;
				return ;	
			}
		}

		request.insert(request.end(), buffer, buffer + receivedBytes);
	}

	string	header_str(request.begin(), request.end());
	this->parse_request(header_str);

	if (this->_error)
	{
		this->_request_fully_parsed = true;
		return ;
	}

	c_location *matching_location = _server.find_matching_location(this->get_target());
	if (matching_location != NULL && matching_location->get_body_size() > 0)
	{
		this->set_client_max_body_size(matching_location->get_body_size());
	}

	if (this->_client_max_body_size == 0)
		_client_max_body_size = 100 * 1024 * 1024; // 100 MB

	/* -----Lire le body -----*/
	this->determine_body_reading_strategy(_socket_fd, buffer, request);
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
	if (this->_error == true)
		return (1);
		
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
	this->_method = start_line.substr(start, space_pos - start);
	if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE")
	{
		this->_status_code = 501;
		this->_error = true;
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
	}	
	else
	{
		this->_path = start_line.substr(start, space_pos - start);
		this->_query = "";
	}
	if (!is_uri_valid())
	{
		this->_status_code = 400;
		this->_error = true;
		return (1);
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
		cout << "version : " << this->_version << " !" << endl;
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
		_server.log_message("[ERROR] invalid header name: " + key);
		this->_status_code = 400;
	}

	pos++;
	if (headers[pos] != 32)
	{
		this->_status_code = 400;
	}

	value = ft_trim(headers.substr(pos + 1));
	if (!is_valid_header_value(key, value))
	{
		_server.log_message("[ERROR] invalid header value: " + value);
		this->_status_code = 400;
	}

	this->_headers[key] = value;
	return (0);
}


/************ BODY ************/

int    c_request::fill_body_with_bytes(const char *buffer, size_t len)
{
	if (this->_client_max_body_size > 0 && this->_body.size() > this->_client_max_body_size && is_limited())
	{
		this->_status_code = 413;
		this->_error = true;
		this->_request_fully_parsed = true;
		return (1);
	}
	// this->_body.append(buffer, len);
	this->_body.insert(this->_body.end(), buffer, buffer + len);
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
			
			// Si taille = 0, c'est le chunk final
			if (this->_expected_chunk_size == 0)
			{
				this->_request_fully_parsed = true;
				accumulator.clear();
				return;
			}
		}
		else
		{
			// On lit les données du chunk            
			if (tmp.size() < this->_expected_chunk_size)
			{
				accumulator.insert(0, tmp + "\r\n");
				this->_chunk_line_count--;
				return;
			}
			if (tmp.size() > this->_expected_chunk_size)
			{
				_server.log_message("[ERROR] Invalid chunk size. Received: " 
										+ int_to_string(tmp.size()) + " Expected: " 
										+ int_to_string(_expected_chunk_size));
				this->_status_code = 400;
				this->_error = true;
				return;				
			}
			if (this->fill_body_with_bytes(tmp.c_str(), this->_expected_chunk_size))
				return ;
		}
	}
}

void	c_request::read_body_with_chunks(int socket_fd, char* buffer, vector<char>& request)
{
	size_t 	body_start = 0;
	bool	found = false;

	while (!found)
	{
		body_start = find_end_of_headers_pos(request);
		if (body_start < 0)
			return ;
		else
			found = true;
	}

	// --- Extraire le corps déjà reçu dans le premier paquet ---
	std::vector<char> body_part;
	if (body_start < request.size())
		body_part.insert(body_part.end(), request.begin() + body_start, request.end());

	int receivedBytes;

	if (!body_part.empty())
	{
		string tmp_chunk(body_part.begin(), body_part.end());
		this->fill_body_with_chunks(tmp_chunk);

		if (this->_request_fully_parsed)
		{
			_server.log_message("[DEBUG] Complete chunked body received in first packet");
			return ;
		}
		if (this->_error)
			return ;
	}
	while (!this->_request_fully_parsed && !this->_error)
	{
		receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
		if (string(buffer) == "\0\r\n\r\n")
			break;
		if (receivedBytes <= 0) 
		{
			if (receivedBytes == 0) 
			{
				_client.set_state(IDLE);
				return ;
			} 
			else if (receivedBytes < 0) 
			{
				_server.log_message("[WARNING] recv() returned <0 for client " 
									+ int_to_string(socket_fd) 
									+ ". Will retry on next POLLIN.");
				this->_request_fully_parsed = false;
				return;
			}
		}

		_chunk_accumulator.insert(_chunk_accumulator.end(), buffer, buffer + receivedBytes);

		string	chunk_data(_chunk_accumulator.begin(), _chunk_accumulator.end());
		this->fill_body_with_chunks(chunk_data);
	}
}

size_t	c_request::find_end_of_headers_pos(vector<char>& request)
{
	size_t 	body_start = 0;
	for (size_t i = 0; i + 3 < request.size(); i++)
	{
		if (request[i] == '\r' && request[i + 1] == '\n' &&
			request[i + 2] == '\r' && request[i + 3] == '\n')
		{
			body_start = i + 4;
			return (body_start);
		}
	}

	_server.log_message("[ERROR] could not find end of headers (\\r\\n\\r\\n))");
	this->_error = true;
	return (-1);
}

void	c_request::read_body_with_length(int socket_fd, char* buffer, vector<char>& request)
{
	size_t 	body_start = 0;
	bool	found = false;

	while (!found)
	{
		body_start = find_end_of_headers_pos(request);
		if (body_start < 0)
			return ;
		else
			found = true;
	}

	// Extraire la partie du body déjà reçue dans 'request'
	vector<char>	body_part;
	if (body_start < request.size())
		body_part.insert(body_part.end(), request.begin() + body_start, request.end());

	size_t 	expected_length = this->_content_length;
	int		receivedBytes;
	size_t	total_bytes = 0;

	if (expected_length > this->_client_max_body_size  && is_limited())
	{
		_server.log_message("[ERROR] expected length (" 
							+ int_to_string(expected_length) 
							+ ") excesses announced client max body size (" + int_to_string(_client_max_body_size) + ")");
		this->_status_code = 413;
		this->_error = true;
		this->_request_fully_parsed = true;

		// On vide le socket pour eviter de bloquer la connexion
		_server.log_message("[DEBUG] Draining socket to avoid connection issues...");

		// compter ce qui est deja dans le premier paquet
		total_bytes = body_part.size();

		// lire et jeter le reste des donnees
		while (total_bytes < expected_length)
		{
			receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
			if (receivedBytes <= 0)
				break;
			total_bytes += receivedBytes;
		}
		_server.log_message("[DEBUG] Socket drained. Total bytes discarded: " + int_to_string(total_bytes));
		return ;
	}

	if (!body_part.empty())
	{
		if (this->fill_body_with_bytes(body_part.data(), body_part.size()))
			return ;
		total_bytes += body_part.size();

		if (total_bytes >= expected_length)
		{
			this->_request_fully_parsed = true;
			return;
		}
	}

	while (total_bytes < expected_length)
	{
		receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == 0)
			{
				if (total_bytes == expected_length)
					return ;

				_server.log_message("[ERROR] Incomplete body. Expected: " 
									+ int_to_string(expected_length) + " Received: " + int_to_string(total_bytes));
				this->_error = true;
				return;
			}

			else if (receivedBytes < 0) 
			{
				_server.log_message("[WARNING] recv() returned <0 for client " + int_to_string(socket_fd) + ". Will retry on next POLLIN.");
				this->_request_fully_parsed = false;
				return;
			}
		}
	
		total_bytes += receivedBytes;
		
		if (total_bytes > expected_length && is_limited())
		{
			_server.log_message("[ERROR] actual body size (" 
								+ int_to_string(total_bytes) 
								+ ") excesses announced size (" + int_to_string(expected_length) + ")");
			this->_status_code = 413;
			this->_error = true;
			this->_request_fully_parsed = true; // rajout car pb quand gros fichier voir si ca pause pas de pb (jen ai pas limpression)
			return ;
		}

		if (this->fill_body_with_bytes(buffer, receivedBytes))
			return ;

		if (total_bytes == expected_length)
		{
			this->_request_fully_parsed = true;
			break;
		}
	}
}


void	c_request::determine_body_reading_strategy(int socket_fd, char* buffer, vector<char>& request)
{
	if (this->_has_body)
	{
		if (this->get_content_length())
			this->read_body_with_length(socket_fd, buffer, request);

		else
			this->read_body_with_chunks(socket_fd, buffer, request);

		if (this->_error)
			return ;
			
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

