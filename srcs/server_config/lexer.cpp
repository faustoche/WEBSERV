#include "lexer.hpp"


/* 
creer tokens pour
mots cles
valeurs
delimiteurs {} ;
 */

 // mettre tout dans une string
 // tokenizer
 // creer un parser qui lit et construit la classe
 // 

/*-----------------  CONSTRUCTOR -------------------*/

c_lexer::c_lexer(string const filename) : _content(""), _pos(0), _current_line(1), _current_column(1)
{
    if (!this->open_file(filename))
        throw runtime_error("Error: could not open the configuration file for the server");
    init_list_of_tokens();
    
}

/*-----------------  DESTRUCTOR --------------------*/

c_lexer::~c_lexer()
{

}

/*-----------  PRIVATE MEMBER FUNCTION ------------*/

bool   c_lexer::open_file(string const & filename)
{
    ifstream file(filename.c_str());
    if (!file.is_open())
        return false;

    for(string line; getline(file, line);)
    {
        this->_content += line;
        this->_content += '\n';
    }
    file.close();

    if (this->_content.empty())
        throw invalid_argument("Error: the configuration file is empty");

    return true;
}

/*--------------------  _content ------------------*/

string    c_lexer::read_word()
{
    string result;
    while (!is_at_end())
    {
        char c = current_char();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ';' || c == '{' || c == '}')
            break ;
        result += c;
        advance();
    }
    return result;
}

char    c_lexer::current_char() const
{
    if (is_at_end())
        return '\0';
    return this->_content[_pos];
}

bool    c_lexer::is_at_end() const
{
    return this->_pos >= _content.size();
}

void    c_lexer::advance()
{
    if(!is_at_end())
    {
        // cout << _content[_pos] << " line = " << _current_line << " col = " << _current_column << endl;
        char c = this->_content[_pos];
        this->_pos++;
        if (c == '\n')
        {
            this->_current_line++;
            this->_current_column = 1;
        }
        else
            this->_current_column++;
    }
}

void    c_lexer::skip_whitespace()
{
    while (!is_at_end())
    {
        char c = current_char();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else
            break;
    }
}

void    c_lexer::skip_line()
{
    int temp_current_line = this->_current_line;
    while (!is_at_end())
    {
        if (temp_current_line == this->_current_line)
            advance();
        else
            break;
    }
}

/*--------------------  tokens -------------------*/

bool    c_lexer::is_bloc_keyword(string const & word) const
{
    if (word == "server" || word == "location")
        return true ;
    return false ;
}

bool    c_lexer::is_directive_keyword(string const & word) const
{
    /* list avec des find ? */
    if (word == "listen" || word == "index" || word == "cgi" || word == "autoindex"
        || word == "redirect" || word == "alias" || word == "methods" || word == "upload_enable"
        || word == "error_page" || word == "server_name" || word == "client_max_body_size")
        return true ;
    return false ;
}

// bool    c_lexer::is_value()
// {

// }

void    c_lexer::get_next_token()
{
    skip_whitespace();

    if (is_at_end())
    {
        this->_tokens.push_back(s_token(TOKEN_EOF, "\0", this->_current_line, this->_current_column));
        return ;
    }
        // return s_token(TOKEN_EOF, "\0", this->_current_line, this->_current_column);
    
    int line = this->_current_line;
    int column = this->_current_column;

    char c = current_char();

    // cout << line << column << c << endl;

    if (c == '{')
    {
        this->advance();
        this->_tokens.push_back(s_token(TOKEN_LBRACE, "{", line, column));
        return ;
        // return s_token(TOKEN_LBRACE, "{", line, column);
    }
    if (c == '}')
    {
        this->advance();
        this->_tokens.push_back(s_token(TOKEN_RBRACE, "}", line, column));
        return ;
    }
    if (c == ';')
    {
        this->advance();
        this->_tokens.push_back(s_token(TOKEN_SEMICOLON, ";", line, column));
        return ;
    }
    if (c == '#')
    {
        skip_line();
        return ;
    }

    string word = read_word(); // avance sur le _content
    if (is_bloc_keyword(word))
    {
        this->_tokens.push_back(s_token(TOKEN_BLOC_KEYWORD, word, line, column));
        return ;
    }
    if (is_directive_keyword(word))
    {
        this->_tokens.push_back(s_token(TOKEN_DIRECTIVE_KEYWORD, word, line, column));
        return ;
    }
    if (!word.empty())
    {
       this->_tokens.push_back(s_token(TOKEN_VALUE, word, line, column));
       return ;
    }

    this->advance();
    this->_tokens.push_back(s_token(TOKEN_ERROR, "\0", this->_current_line, this->_current_column));
    return ;
}

bool    c_lexer::has_more_tokens() const
{
    return !is_at_end();
}

void    c_lexer::init_list_of_tokens()
{
    while (has_more_tokens())
    {
        get_next_token();
    }
}



/*--------------------  getters ------------------*/

int     c_lexer::get_current_column() const
{
    return this->_current_column;
}

int     c_lexer::get_current_line() const
{
    return this->_current_line;
}

vector<s_token> & c_lexer::get_list_of_token()
{
    return this->_tokens;
}

