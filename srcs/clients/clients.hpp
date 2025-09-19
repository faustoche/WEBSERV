#pragma once

/************ INCLUDES ************/

#include <poll.h>
#include "server.hpp"
#include <sys/time.h>

/************ DEFINE ************/

using namespace std;

enum client_state {
	READING,
	PROCESSING,
	SENDING,
	IDLE,
	DISCONNECTED
};

/************ CLASS ************/

class c_client
{
private:
	int				_fd;
	client_state	_state;
	char			_buffer[4096];
	struct timeval	_timestamp;
	string			_read_buffer;
	string			_write_buffer;
	size_t			_bytes_written;
	size_t			_response_body_size;
	bool			_response_complete;

public:
	c_client();
	c_client(int client_fd);
	~c_client();

	/******* GETTERS ******* */
	const int &get_fd() const { return (_fd); }
	const struct timeval &get_timestamp() const { return (_timestamp); }
	const string &get_read_buffer() const { return (_read_buffer); }
	const string &get_write_buffer() const { return (_write_buffer); }
	string &get_write_buffer() { return (_write_buffer); }
	const size_t &get_bytes_written() const { return (_bytes_written); }
	const size_t &get_response_body_size() const { return _response_body_size; };
	const client_state &get_state() const { return (_state); }
	bool get_response_complete() const { return _response_complete; };
	
	
	/******* SETTERS ******* */
	void append_to_read_buffer(const string &data) { _read_buffer += data; }
	void set_state(client_state new_state) { (_state = new_state); }
	void set_bytes_written(size_t bytes) { (_bytes_written = bytes); }
	void set_response_complete(bool state) { _response_complete = state; };
	void append_response_body_size(size_t bytes) { _response_body_size += bytes; };
	void clear_write_buffer() { _write_buffer.clear(); };
	void clear_read_buffer() { _read_buffer.clear(); };
	void clear_response_body_size() { _response_body_size = 0; };
};
