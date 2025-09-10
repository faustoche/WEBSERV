
#include "location.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_location::c_location() : _location_root("./"), _auto_index(false), _is_directory(false) 
{
    _url_key = "";
    _upload_path = "";
    _location_body_size = 0;
    _location_root = "";
    // _location_path = "";
    _auto_index = false;
    _upload_path = "";
    _cgi_extension.clear();
}

c_location  const& c_location::operator=(c_location const& rhs)
{
    if (this != & rhs)
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
    return (*this);
}

/*-----------------  DESTRUCTOR -------------------*/

c_location::~c_location()
{

}

/*----------------- MEMBERS METHODS  --------------*/


/*--------------------- utils  --------------------*/

void    c_location::print_location() const
{
    cout << "   Location : " << endl
        << "    URL key = " << get_url_key() << endl
        << "    Body size = " << get_body_size() << endl
        << "    Root = " << get_root() << endl
        // << "Authorised methods = " << get_methods() << endl //revoir
        // << "Index file = " << get_indexes() << endl
        << "    Auto index = " << get_auto_index() << endl
        << "    Upload path = " << get_upload_path() << endl
        // << "Redirect = " << get_redirect() << endl
        // << "CGI extension = " << get_cgi() << endl
        << "    Is directory = " << get_bool_is_directory() << endl;
}

void    c_location::clear_cgi()
{
    if (!_cgi_extension.empty())
        _cgi_extension.clear();
}
