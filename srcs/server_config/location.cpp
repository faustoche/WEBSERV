
#include "location.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_location::c_location()
{
    _location_body_size = 0;
    _location_root = "";
    // _location_path = "";
    _auto_index = false;
    _upload_path = "";
    _cgi_extension.clear();


}

/******* AC ******/
// c_location::c_location(c_location const& other)
// {
//     this->_url_key = other._url_key;
//     this->_location_root = other._location_root;
//     this->_location_index_files = other._location_index_files;
//     this->_location_methods = other._location_methods;
//     this->_location_body_size = other._location_body_size;
//     this->_auto_index = other._auto_index;
//     this->_redirect = other._redirect;
//     this->_cgi_extension = other._cgi_extension;
//     this->_upload_path = other._upload_path;
// }

c_location  const& c_location::operator=(c_location const& rhs)
{
    if (this != & rhs)
    {
        this->_url_key = rhs._url_key;
        this->_location_root = rhs._location_root;
        this->_location_index_files = rhs._location_index_files;
        this->_location_methods = rhs._location_methods;
        this->_location_body_size = rhs._location_body_size;
        this->_auto_index = rhs._auto_index;
        this->_redirect = rhs._redirect;
        this->_cgi_extension = rhs._cgi_extension;
        this->_upload_path = rhs._upload_path;
    }
    return (*this);
}

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

    for (map<string, string>::const_iterator it = this->_cgi_extension.begin(); it != this->_cgi_extension.end(); it++)
    {
        cout << "_cgi_extention[" << it->first << "] = " << it->second << endl; 
    } 
    cout << "upload_path = " << this->_upload_path << endl;
}

void    c_location::clear_cgi()
{
    if (!_cgi_extension.empty())
        _cgi_extension.clear();
}


/*-----------------  DESTRUCTOR -------------------*/

c_location::~c_location()
{

}

/*----------------- MEMBERS METHODS  --------------*/

