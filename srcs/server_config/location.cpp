
#include "location.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_location::c_location() : _location_root("./"), _auto_index(false), _is_directory(false) 
{
    _url_key = "";
    _upload_path = "";
}

/*-----------------  DESTRUCTOR -------------------*/

c_location::~c_location()
{

}

/*----------------- MEMBERS METHODS  --------------*/

c_location &    c_location::operator=(c_location const & rhs)
{
    if (this != &rhs)
    {
        _url_key = rhs._url_key;
        _location_body_size = rhs._location_body_size;
        _location_root = rhs._location_root;
        _location_methods = rhs._location_methods;
        _location_index_files = rhs._location_index_files;
        _auto_index = rhs._auto_index;
        _upload_path = rhs._upload_path;
        _redirect = rhs._redirect;
        _cgi_extension = rhs._cgi_extension;
        _is_directory = rhs._is_directory;
    }
    return *this;
}


/*-------------------- setters  -------------------*/

