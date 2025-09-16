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
	size_t			_header_size;

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
	const size_t &get_header_size() const { return _header_size; };
	const client_state &get_state() const { return (_state); }
	
	
	/******* SETTERS ******* */
	void append_to_read_buffer(const string &data) { _read_buffer += data; }
	void set_state(client_state new_state) { (_state = new_state); }
	void set_bytes_written(size_t bytes) { (_bytes_written = bytes); }
	void set_header_size(size_t bytes) { _header_size = bytes; };
};
