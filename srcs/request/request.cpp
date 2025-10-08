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

void	c_request::read_request()
{
	char	buffer[BUFFER_SIZE];
	int		receivedBytes;
	string	request = "";
	
	this->init_request();
	this->_socket_fd = _client.get_fd();

	/* ----- Lire jusqu'a la fin des headers ----- */
	while (request.find("\r\n\r\n") == string::npos)
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
    			_server.log_message("[WARNING] recv() returned <0 for client " 
    			                    + int_to_string(_socket_fd) 
    			                    + ". Will retry on next POLLIN.");
				this->_request_fully_parsed = false;
    			return;
			}
		}
		// buffer[receivedBytes] = '\0';
		request.append(buffer, receivedBytes);
	}
	this->parse_request(request);
	c_location *matching_location = _server.find_matching_location(this->get_target());
	if (matching_location != NULL && matching_location->get_body_size() > 0)
	{
		this->set_client_max_body_size(matching_location->get_body_size());
	}
	if (this->_client_max_body_size == 0)
		_client_max_body_size = 100 * 1024 * 1024; // 100 MB
	
	/* -----Lire le body -----*/
	this->determine_body_reading_strategy(_socket_fd, buffer, request);

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
	this->_method = start_line.substr(start, space_pos - start);
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
		this->_status_code = 400;

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
    this->_body.append(buffer, len);
	if (this->_client_max_body_size > 0 && this->_body.size() > this->_client_max_body_size && is_limited())
	{
		this->_status_code = 413;
		this->_error = true;
		return (1);
	}
	return (0);
}


/* PB de taille des chunck dans le cas dun file avec taille illimite --> PB des fois ulpoad reussi dautre fois non */
// void c_request::fill_body_with_chunks(string &accumulator)
// {
// 	string			tmp;
// 	size_t			pos;

// 	while (true)
//     {
//         if (this->_chunk_line_count % 2 == 0)
//         {
//            // on attend la ligne de taille
// 			pos = accumulator.find("\r\n");
// 			if (pos == string::npos)
// 				return ; // pas assez de donnees on attend le prochain recv
			
// 			tmp = accumulator.substr(0, pos);
// 			accumulator.erase(0, pos + 2); // supprimer \r\n
			
// 			if (tmp.empty())
// 				continue ;

// 			this->_expected_chunk_size = strtoul(tmp.c_str(), NULL, 16);
// 			this->_chunk_line_count++;

// 			if (this->_expected_chunk_size == 0)
// 			{
// 				this->_request_fully_parsed = true;
// 				accumulator.clear();
// 				return ;
// 			}
//         }
//         else
//         {
//             // On attend les données du chunk + \r\n       
// 			if (accumulator.size() < this->_expected_chunk_size + 2)
// 				return ; // pas assez de donnees, on attend
			
// 			tmp = accumulator.substr(0, this->_expected_chunk_size);
// 			accumulator.erase(0, this->_expected_chunk_size);

// 			// verifier et consommer le \r\n
// 			if (accumulator.size() >= 2 && accumulator.substr(0, 2) == "\r\n")
// 				accumulator.erase(0, 2);
// 			else
// 			{
// 				_server.log_message("[ERROR] Missing \\r\\n after chunk data");
// 				this->_status_code = 400;
// 				this->_error = true;
// 				return;
// 			}

// 			if (this->fill_body_with_bytes(tmp.c_str(), this->_expected_chunk_size))
// 				return;
			
// 			this->_chunk_line_count++;
//         }
//     }
// }

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


void	c_request::read_body_with_chunks(int socket_fd, char* buffer, string request)
{
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);
	int		receivedBytes;

	c_client *client = _server.find_client(socket_fd);
	if (!client)
	{
		this->_error = true;
		return ;
	}
	if (!body_part.empty())
	{
		this->fill_body_with_chunks(body_part);
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
				client->set_state(IDLE);
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
		this->_chunk_accumulator.append(buffer, receivedBytes);
		this->fill_body_with_chunks(this->_chunk_accumulator);
	}
}

void	c_request::read_body_with_length(int socket_fd, char* buffer, string request)
{
	size_t 	body_start = request.find("\r\n\r\n") + 4;
	string 	body_part = request.substr(body_start);

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
		return ;
	}

	if (!body_part.empty())
	{
		if (this->fill_body_with_bytes(body_part.data(), body_part.size()))
			return ;
		total_bytes += body_part.size();

		if (total_bytes >= expected_length)
			return;
	}

	while (total_bytes < expected_length)
	{
		receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0); // faut-il conditionner l'appel a recv
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
			return ;
		}

    	if (this->fill_body_with_bytes(buffer, receivedBytes))
			return ;

		if (total_bytes == expected_length)
			break;
	}
}


void	c_request::determine_body_reading_strategy(int socket_fd, char* buffer, string request)
{
	if (this->_has_body)
	{
		if (this->get_content_length())
		{
			this->read_body_with_length(socket_fd, buffer, request);
		}
		else
			this->read_body_with_chunks(socket_fd, buffer, request);
		if (this->_error)
			return ;
		if (this->_error)
			return ;
	}
	// else
	// 	this->_request_fully_parsed = true;
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

