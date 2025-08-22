#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request(const string& str) : _method(""), _version("")
{
    for (map<string, string>::iterator it = _headers.begin(); it != _headers.end(); it++)
        it->second = "";

    try 
    {
        this->parse_request(str);
    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
    }
}

c_request::~c_request()
{
}


/************ PARSE FUNCTIONS ************/

int c_request::parse_request(const string& raw_request)
{
    istringstream stream(raw_request);
    string line;

    //---- ETAPE 1: start-line -----
    if (!getline(stream, line, '\n'))
        throw runtime_error("Request is empty");

    if (line.empty() || line[line.size() - 1] != '\r')
        throw runtime_error("Invalid header format: missing '\r' at the end of header element");

    line.erase(line.size() - 1);

    this->parse_start_line(line);

    //---- ETAPE 2: headers -----
    while (getline(stream, line, '\n'))
    {
        if (line[line.size() - 1] != '\r')
            throw runtime_error("Invalid header format: missing '\r' at the end of header element");

        line.erase(line.size() - 1);

        if (line.empty())
            break ;
        
        parse_headers(line);
    }

    cout << "*********** Headers map ***********" << endl;
    for (map<string, string>::iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
        cout << it->first << " : " << it->second << endl;
    
    //---- ETAPE 3: body -----

    return (0);
}

int c_request::parse_start_line(string& start_line)
{
    size_t start = 0;
    size_t pos = start_line.find(' ', start);
    
    // METHOD
    if (pos == string::npos)
        throw runtime_error("Invalid start-line: missing ' ' separator");

    this->_method = start_line.substr(start, pos - start);


    // TARGET
    start = pos + 1;
    pos = start_line.find(' ', start);

    if (pos == string::npos)
        throw runtime_error("Invalid start-line: missing ' ' separator");

    this->_target = start_line.substr(start, pos - start);
    

    // VERSION
    start = pos + 1;
    this->_version = start_line.substr(start);
    if (this->_version.empty())
        throw runtime_error("Invalid start-line: missing one parameter");

    cout << "*********** Start-line ************" << endl;
    cout << "method: " << this->_method << endl;
    cout << "target: " << this->_target << endl;
    cout << "version: " << this->_version << endl << endl;

    return (0);
}

bool    is_valid_header_name(const string& key_name)
{
    const string allowed_special_chars = "!#$%&'*+-.^_`|~";

    if (key_name.empty())
        return (false);
    for (size_t i = 0; i < key_name.length(); i++)
    {
        if (!isalnum(key_name[i]) && allowed_special_chars.find(key_name[i]) == string::npos)
            return (false);
    }
    return (true);
}

bool    is_valid_header_value(const string& value)
{
    for (size_t i = 0; i < value.length(); i++)
    {
        if ((value[i] < 33 && value[i] != '\t') || value[i] == 127)
            return (false);
    }
    return (true);
}

int c_request::parse_headers(string& headers)
{
    size_t pos = headers.find(':', 0);

    string key;
    string value;

    key = headers.substr(0, pos);
    if (!is_valid_header_name(key))
        throw runtime_error("Invalid header name: '" + key + "'");

    pos++;
    if (headers[pos] != 32)
        throw runtime_error("Invalid header format: missing space after ':'");

    pos++;
    while (pos < headers.length() && headers[pos] == 32)
        pos++;
    
    size_t end_of_string = headers.find(' ', pos);
    value = headers.substr(pos, end_of_string - pos);
    if (!is_valid_header_value(value))
        throw runtime_error("Invalid header value: '" + value + "'");

    this->_headers[key] = value;

    return (0);
}

/************ UTILS ************/

const string& c_request::get_header_value(const string& key) const
{
    for (map<string, string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); it++)
    {
        if (it->first == key)
            return (it->second);
    }
    throw std::out_of_range("Header not found: " + key);
}
