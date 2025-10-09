#include "clients.hpp"
#include "server.hpp"
#include <cstring>   // memset
#include <unistd.h>  // close

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_client::c_client() : _fd(-1), _client_ip(""), _state(READING)
{
    memset(_buffer, 0, sizeof(_buffer));
    gettimeofday(&_timestamp, 0);
    _response_body_size = 0;
    _response_complete = false;
    _read_buffer.clear();
	_write_buffer.clear();
	_bytes_written = 0;
    _creation_time = time(NULL);
    _last_modified = time(NULL);
    _last_request.clear();
    _status_code = 200; // modifie avant a 0
}

c_client::c_client(int client_fd, string client_ip) : _fd(client_fd), _client_ip(client_ip), _state(READING)
{
    memset(_buffer, 0, sizeof(_buffer));
    gettimeofday(&_timestamp, 0);
    _response_body_size = 0;
    _response_complete = false;
    _read_buffer.clear();
	_write_buffer.clear();
	_bytes_written = 0;
    _creation_time = time(NULL);
    _last_modified = time(NULL);
    _last_request.clear();
    _status_code = 0;
}

c_client::~c_client() {}

/************ CLIENT CREATION ************/

void    c_server::add_fd(int fd, short events)
{
    struct pollfd tmp_poll_fd;
    tmp_poll_fd.fd = fd;
    tmp_poll_fd.events = events;
    tmp_poll_fd.revents = 0;

    _poll_fds.push_back(tmp_poll_fd);
    log_message("[DEBUG] fd " + int_to_string(fd) + " is now monitored by _poll_fds");
}

void c_server::add_client(int client_fd, string client_ip)
{
    _clients[client_fd] = c_client(client_fd, client_ip);
    set_non_blocking(client_fd);
    add_fd(client_fd, POLLIN);
    log_message("[DEBUG] Client " + int_to_string(client_fd) + " can send a request : POLLIN");
}

void c_server::remove_client(int client_fd)
{
    if (client_fd < 0)
        return ;
   
    for (vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); it++)
    {
        if (it->fd == client_fd)
        {
            _poll_fds.erase(it);
            log_message("[DEBUG] fd " + int_to_string(client_fd) + " is no longer monitored by _poll_fds");
            break;
        }
    }
    
    map<int, c_client>::iterator it = _clients.find(client_fd);
    if (it != _clients.end())
    {
        _clients.erase(it);
        log_message("[DEBUG] fd " + int_to_string(client_fd) + " erased from _clients");
    }
     close(client_fd);
    
}

c_client *c_server::find_client(int client_fd)
{
    map<int, c_client>::iterator it = _clients.find(client_fd);
    if (it != _clients.end())
        return &(it->second);
    return (NULL);
}

c_cgi   *c_server::find_cgi_by_client(int client_fd)
{
    for (map<int, c_cgi*>::iterator it = _active_cgi.begin(); it != _active_cgi.end(); it++)
    {
        if (it->second->get_client_fd() == client_fd)
            return (it->second);
    }
    return (NULL);
}

int c_server::find_client_fd_by_cgi(c_cgi* cgi)
{
    for (map<int, c_client>::iterator it = _clients.begin(); it != _clients.end(); it++)
    {
        if (it->second.get_fd() == cgi->get_client_fd())
            return (it->second.get_fd());
    }
    return (-1);
}

c_cgi   *c_server::find_cgi_by_pid(pid_t pid)
{
    for (std::map<int, c_cgi*>::iterator it = _active_cgi.begin(); 
         it != _active_cgi.end(); ++it)
    {
        if (it->second && it->second->get_pid() == pid)
        {
            return it->second;
        }
    }
    return (NULL);
}

void    c_server::clear_write_buffer()
{
    
}
