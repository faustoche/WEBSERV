#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request(c_server& server, c_client &client) : _server(server), _client(client)
{
	this->init_request();
}

c_request::~c_request(){}

/************ REQUEST ************/

void	c_request::read_request()
{

	char	buffer[BUFFER_SIZE];
	ssize_t	receivedBytes = recv(_socket_fd, buffer, BUFFER_SIZE, MSG_NOSIGNAL);

	// tester le == 0
	if (receivedBytes <= 0)
		return ;

	this->append_read_buffer(buffer, receivedBytes);

	if (!this->get_headers_parsed())
	{
		if (has_end_of_headers(_read_buffer))
		{
			size_t i = find_end_of_headers_pos(_read_buffer);
			string	header_str(_read_buffer.begin(), _read_buffer.end());
			this->parse_request(header_str);
			if (this->_error)
			{
				this->set_request_fully_parsed(true);
				return ;
			}
			if (this->_has_body && this->_read_buffer.size() > i)
			{
				this->_read_buffer.erase(this->_read_buffer.begin(), this->_read_buffer.begin() + i);
				this->determine_body_reading_strategy(_socket_fd);
			}
			else
			{
				this->set_request_fully_parsed(true);
				this->_read_buffer.clear();
			}
			return ;
		}
		else
			return ;
	}
	else
	{
		this->determine_body_reading_strategy(_socket_fd);
		this->consume_read_buffer(this->_read_buffer.size());
	}
}



int c_request::parse_request(const string& raw_request)
{
	istringstream stream(raw_request);
	string line;

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
	this->set_headers_parsed(true);
	return (0);
}

/************ START-LINE ************/

int c_request::parse_start_line(string& start_line)
{
	size_t	start = 0;
	size_t	space_pos = start_line.find(' ', start);
	string	tmp = "";

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
		if (this->_status_code == 414)
		{
			this->_error = true;
			return (1);
		}
		else
		{
			this->_status_code = 400;
			this->_error = true;
			return (1);
		}
	}

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
	this->_body.insert(this->_body.end(), buffer, buffer + len);
	this->set_total_bytes(this->_body.size());
	return (0);
}

void c_request::fill_body_with_chunks(string &accumulator)
{
	string			tmp;
	size_t			pos;

	while ((pos = accumulator.find("\r\n")) != string::npos)
	{
		tmp = accumulator.substr(0, pos);
		accumulator.erase(0, pos + 2);

		if (tmp.empty())
			continue ;
		this->_chunk_line_count++;

		if (this->_chunk_line_count % 2 == 1)
		{
			this->_expected_chunk_size = strtoul(tmp.c_str(), NULL, 16);
			if (this->_expected_chunk_size == 0)
			{
				this->set_request_fully_parsed(true);
				accumulator.clear();
				return;
			}
		}
		else
		{          
			if (tmp.size() < this->_expected_chunk_size)
			{
				accumulator.insert(0, tmp + "\r\n");
				this->_chunk_line_count--;
				return;
			}
			if (tmp.size() > this->_expected_chunk_size)
			{
				_server.log_message("[ERROR] Invalid chunk size. Received: " + int_to_string(tmp.size()) + " Expected: " + int_to_string(_expected_chunk_size));
				this->_status_code = 400;
				this->_error = true;
				return;				
			}
			if (this->fill_body_with_bytes(tmp.c_str(), this->_expected_chunk_size))
				return ;
		}
	}
}

void	c_request::read_body_with_chunks(int socket_fd)
{
	if (!_read_buffer.empty())
	{

		string tmp_chunk(_read_buffer.begin(), _read_buffer.end());
		this->fill_body_with_chunks(tmp_chunk);

		if (this->_request_fully_parsed)
		{
			_server.log_message("[DEBUG] Complete chunked body received in first packet");
			return ;
		}
		if (this->_error)
			return ;
	}

	int 	receivedBytes;
	char	buffer[BUFFER_SIZE];
	if (!this->_request_fully_parsed && !this->_error)
	{
		receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
		this->append_read_buffer(buffer, receivedBytes);

		// Attention == 0
		if (receivedBytes <= 0) 
				return;

		_chunk_accumulator.insert(_chunk_accumulator.end(), buffer, buffer + receivedBytes);

		string	chunk_data(_chunk_accumulator.begin(), _chunk_accumulator.end());
		this->fill_body_with_chunks(chunk_data);
	}
}

size_t	c_request::find_end_of_headers_pos(vector<char>& request)
{
	for (size_t i = 0; i + 3 < request.size(); i++)
	{
		if (request[i] == '\r' && request[i + 1] == '\n' &&
			request[i + 2] == '\r' && request[i + 3] == '\n')
		{
			return (i + 4);
		}
	}

	_server.log_message("[ERROR] could not find end of headers (\\r\\n\\r\\n))");
	this->_error = true;
	return (-1);
}

void	c_request::read_body_with_length(int socket_fd)
{
	size_t 	expected_length = this->_content_length;
	int		receivedBytes;

	if (expected_length > this->_client_max_body_size  && is_limited())
	{
		_server.log_message("[ERROR] expected length (" 
							+ int_to_string(expected_length) 
							+ ") excesses announced client max body size (" + int_to_string(_client_max_body_size) + ")");
		this->_status_code = 413;
		this->_error = true;
		this->_request_fully_parsed = true;
		this->consume_read_buffer(_read_buffer.size());
		_server.log_message("[DEBUG] Socket drained. Total bytes discarded: " + int_to_string(_total_bytes));
		return ;
	}

	if (this->_read_buffer.size() > 0)
	{
		const char* ptr = _read_buffer.data();
		size_t len = _read_buffer.size();
		if (fill_body_with_bytes(ptr, len))
			return ;
		this->consume_read_buffer(this->_read_buffer.size());

		if (_total_bytes == _content_length)
		{
			this->_request_fully_parsed = true;
			return ;
		}
	}

	if (_total_bytes == _content_length)
	{
		this->_request_fully_parsed = true;
		return ;
	}

	if (_total_bytes < _content_length)
	{
		char	buffer[BUFFER_SIZE];
		receivedBytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
		if (receivedBytes < 0)
			return;
		if (receivedBytes == 0)
		{
			this->_request_fully_parsed = true;
			return ;
		}
		this->append_read_buffer(buffer, receivedBytes);
		
		if (_total_bytes > _content_length && is_limited())
		{
			_server.log_message("[ERROR] actual body size (" 
								+ int_to_string(_total_bytes) 
								+ ") excesses announced size (" + int_to_string(_content_length) + ")");
			this->_status_code = 413;
			this->_error = true;
			this->_request_fully_parsed = true;
			return ;
		}

		if (this->fill_body_with_bytes(buffer, receivedBytes))
			return ;
		this->consume_read_buffer(receivedBytes);

		if (_total_bytes == _content_length)
			this->_request_fully_parsed = true;
	}
}


void	c_request::determine_body_reading_strategy(int socket_fd)
{
	c_location *matching_location = _server.find_matching_location(this->get_target());
	if (matching_location != NULL && matching_location->get_body_size() > 0)
		this->set_client_max_body_size(matching_location->get_body_size());

	if (this->_client_max_body_size == 0)
		_client_max_body_size = 100 * 1024 * 1024; // 100 MB

	if (matching_location == NULL)
		this->set_client_max_body_size(_server.get_body_size());

	if (this->_has_body)
	{
		if (this->get_content_length())
			this->read_body_with_length(socket_fd);
		else
			this->read_body_with_chunks(socket_fd);

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



