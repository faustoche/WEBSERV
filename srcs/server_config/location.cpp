
#include "location.hpp"

/*-----------------  CONSTRUCTOR -------------------*/

c_location::c_location()
{

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
        _location_indexes = rhs._location_indexes;
        _auto_index = rhs._auto_index;
        _upload_path = rhs._upload_path;
        _redirect = rhs._redirect;
        _cgi_extension = rhs._cgi_extension;
        _is_directory = rhs._is_directory;
    }
    return *this;
}


/*-------------------- setters  -------------------*/


/*--------------------- utils  --------------------*/

void    c_location::add_index_file(string const & file)
{
    _location_indexes.push_back(file);
}

void	c_location::add_error_page(vector<int> const & codes, string path)
{

	for (size_t i = 0; i < codes.size(); i++)
		_location_err_pages.insert(make_pair(codes[i], path));
}


void	c_location::print_indexes() const
{
	vector<string>::const_iterator it;
	for (it = get_indexes().begin(); it != get_indexes().end(); it++)
	{
		cout << *it << " " << flush;
	}
}

void	c_location::print_methods() const
{
	vector<string>::const_iterator it;
	for (it = get_methods().begin(); it != get_methods().end(); it++)
	{
		cout << *it << " " << flush;
	}
}

void	c_location::print_error_page() const
{
	map<int, string>::const_iterator it;
    
    if (_location_err_pages.empty())
        cout << "EMPTY" << endl;
    else
    {
	    for (it = _location_err_pages.begin(); it != _location_err_pages.end(); it++)
	    {

	    	cout << "		" << it->first << " " << it->second << " " << endl;
	    }
    }
}


void    c_location::print_location() const
{
    cout 
            << "            max body size: " << get_body_size() << " octets" << endl
            << "            alias: " << get_alias() << endl
            << "            authorised methods = ";
            print_methods();
    cout << endl
            << "            index files = ";
            print_indexes();
    cout << endl
            << "            auto index: ";
            if (get_auto_index())
                cout << "ON";
            else
                cout << "OFF";
    cout << endl
            << "            upload path: " << get_upload_path() << endl
            // << "Redirect = " << get_redirect() << endl
            << "            CGI extension (.py): " << get_cgi()[".py"] << endl
            << "            CGI extension (.sh): " << get_cgi()[".sh"] << endl
            << "            is directory: ";
            if (get_bool_is_directory())
                cout << "yes";
            else
                cout << "no";
    cout << endl
            << "            Error pages = " << endl;
            print_error_page();
}

void    c_location::clear_cgi()
{
    if (!_cgi_extension.empty())
        _cgi_extension.clear();
}

void    c_location::clear_indexes()
{
    if (!_location_indexes.empty())
        _location_indexes.clear();
}

void    c_location::clear_err_pages()
{
    if (!_location_err_pages.empty())
        _location_err_pages.clear();
}

