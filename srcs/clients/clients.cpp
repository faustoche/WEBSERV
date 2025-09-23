#include "clients.hpp"
#include "server.hpp"
#include <cstring>   // memset
#include <unistd.h>  // close

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_client::c_client() : _fd(-1), _state(READING)
{
    memset(_buffer, 0, sizeof(_buffer));
    gettimeofday(&_timestamp, 0);
    _response_body_size = 0;
    _response_complete = false;
    
}

c_client::c_client(int client_fd) : _fd(client_fd), _state(READING)
{
    memset(_buffer, 0, sizeof(_buffer));
    gettimeofday(&_timestamp, 0);
    _response_body_size = 0;
    _response_complete = false;
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
    // _clients[fd] = c_client(fd);
}

void c_server::add_client(int client_fd)
{
    _clients[client_fd] = c_client(client_fd);
    set_non_blocking(client_fd);
    add_fd(client_fd, POLLIN);
    cout << PINK << "\n*Client " << client_fd << " set to READING mode*" << RESET << endl;
}

void c_server::remove_client(int client_fd)
{
    close(client_fd);
    if (_clients.find(client_fd) != _clients.end())
    {
        _clients.erase(client_fd);
        cout << PINK << "*Client " << client_fd << " erased from _poll_fds*" << RESET << endl;
    }
}

// void c_server::remove_client_from_pollout(int client_fd)
// {
//     for (size_t i = 0; i < _poll_fds.size(); i++)
//     {
//         if (_poll_fds[i].fd == client_fd)
//         {
//             _poll_fds[i].events &= ~POLLOUT;
//             cout << "Removed POLLOUT for client fd " << client_fd << "\n";
//             return;
//         }
//     }
// }

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
