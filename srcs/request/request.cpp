#include "request.hpp"

c_request::c_request(const string& str) : _method(""), _version("")
{
    for (map<string, string>::iterator it = _headers.begin(); it != _headers.end(); it++)
    {
        it->second = "";
    }
    if (this->parse_request(str))
    {
        std::cerr << "Error: invalid syntax in request" << std::endl;
    }
}

c_request::~c_request()
{
}

int c_request::parse_request(const string& raw_request)
{
    istringstream stream(raw_request);
    string line;

    //---- ETAPE 1: start-line -----
    if (!getline(stream, line, '\n'))
    {
        cerr << "Error: request is empty" << endl;
        return (1);
    }
    if (line.empty() || line[line.size() - 1] != '\r')
    {
        cerr << "Error: missing '\r' at the end of start-line" << endl;
        return (1);
    }
    line.erase(line.size() - 1);

    if (this->parse_start_line(line))
    {
        cerr << "Error: invalid start-line" << endl;
        return (1);
    }

    //---- ETAPE 2: headers -----
    while (getline(stream, line, '\n'))
    {
        if (line[line.size() - 1] != '\r')
        {
            cerr << "Error: missing '\r' at the end of header element" << endl;
            return (1);
        }
        line.erase(line.size() - 1);

        if (line.empty())
            break ;
        
        if (parse_headers(line))
        {
            cerr << "Error: invalid header line: " << line << endl;
            return (1);
        }
    }
    
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
        cerr << "Error: missing space after method (start-line)" << endl;
        return (1);
    }
    this->_method = start_line.substr(start, pos - start);


    // TARGET
    start = pos + 1;
    pos = start_line.find(' ', start);

    if (pos == string::npos)
    {
        cerr << "Error: missing space after target (start-line)" << endl;
        return (1);
    }
    this->_target = start_line.substr(start, pos - start);
    

    // VERSION
    start = pos + 1;
    this->_version = start_line.substr(start);
    if (this->_version.empty())
    {
        cerr << "Error: missing HTTP version (start-line)" << endl;
        return (1);
    }

    cout << "method: " << this->_method << endl;
    cout << "target: " << this->_target << endl;
    cout << "version: " << this->_version << endl;

    return (0);
    // A quel moment verifier que method, target et version sont ok ?;
}

int c_request::parse_headers(string& headers)
{
    size_t pos = headers.find(':', 0);

    string key;
    string value;

    key = headers.substr(0, pos);
    cout << "key: " << key << endl;

    pos++;
    if (headers[pos] != 32)
    {
        cerr << "Error: missing space after key (headers)" << endl;
        return (1);
    }
    pos++;
    while (pos < headers.length() && headers[pos] == 32)
        pos++;
    
    value = headers.substr(pos);
    cout << "value: " << value << endl;

    // this->_headers[key] = value; // creer une map avec des headers pre-definis

    return (0);
}