#include "parser.hpp"
#include "lexer.hpp"
#include "parser.hpp"

/* il s'agit dun main de test exclusivement pour le parsing du .conf */

// int main()
// {
//     c_lexer mylexer("server_config_file/default.conf"); // adapter la maniere de passer le chemin
    
//     vector<s_token> my_tok = mylexer.get_list_of_token();
//     vector<s_token>::iterator it;
//     for (it = my_tok.begin(); it != my_tok.end(); it++)
//     {
//         cout << "Value = " << (*it).value;
//         cout << " ; Type = " << (*it).type << endl;
//     }
//     return 0;
// }

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        c_parser myparser(argv[1]);
        
        vector<s_token> my_tok = myparser.get_list_of_token();
        vector<s_token>::iterator it;
        for (it = my_tok.begin(); it != my_tok.end(); it++)
        {
            cout << "Value = " << (*it).value;
            cout << " ; Type = " << (*it).type << endl;
        }
    }
    return 0;
}