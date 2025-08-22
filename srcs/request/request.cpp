#include "request.hpp"

/************ CONSTRUCTORS & DESTRUCTORS ************/

c_request::c_request(const string& str) : _method(""), _version(""), _status_code(200)
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

void c_request::check_required_headers()
{
    bool has_body = false;
    bool has_content_length = this->_headers.count("Content-Length");
    bool has_transfer_encoding = this->_headers.count("Transfer-Encoding");

    if (this->_method == "POST" || this->_method == "DELETE")
        has_body = true;

    if (!this->_headers.count("Host"))
        this->_status_code = 400;
    
    if (has_body)
    {
        if (!has_content_length && !has_transfer_encoding)
            this->_status_code = 400;
        if (has_content_length && has_transfer_encoding)
            this->_status_code = 400;
    }     
}

int c_request::parse_request(const string& raw_request)
{
    istringstream stream(raw_request);
    string line;

    //---- ETAPE 1: start-line -----
    if (!getline(stream, line, '\n'))
        this->_status_code = 400;        

    if (line.empty() || line[line.size() - 1] != '\r')
        this->_status_code = 400;
    line.erase(line.size() - 1);

    this->parse_start_line(line);

    //---- ETAPE 2: headers -----
    while (getline(stream, line, '\n'))
    {
        if (line[line.size() - 1] != '\r')
            this->_status_code = 400;

        line.erase(line.size() - 1);

        if (line.empty())
            break ;
        
        this->parse_headers(line);
    }
    this->check_required_headers();

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
    {
        this->_status_code = 400;
        return (0);
    }
    this->_method = start_line.substr(start, pos - start);


    // TARGET
    start = pos + 1;
    pos = start_line.find(' ', start);

    if (pos == string::npos)
    {
        this->_status_code = 400;
        return (0);
    }
    this->_target = start_line.substr(start, pos - start);
    

    // VERSION
    start = pos + 1;
    this->_version = start_line.substr(start);
    if (this->_version.empty())
    {
        this->_status_code = 400;
        return (0);
    }

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
        this->_status_code = 400;

    pos++;
    if (headers[pos] != 32)
        this->_status_code = 400;

    pos++;
    while (pos < headers.length() && headers[pos] == 32)
        pos++;
    
    size_t end_of_string = headers.find(' ', pos);
    value = headers.substr(pos, end_of_string - pos);
    if (!is_valid_header_value(value))
        this->_status_code = 400;

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
