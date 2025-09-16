#include "werbserv_config.hpp"

/* il s'agit dun main de test exclusivement pour le parsing du .conf */

int main(int argc, char **argv)
{
    try
    {
        if (argc == 2)
        {
            c_webserv_config webserv(argv[1]);

            if (!webserv.load_configuration())
                throw invalid_argument("Invalid server configuration");
            // vector<c_server> & servers = webserv.get_servers();
            webserv.print_configurations();
            // vector<s_token> my_tok = myparser.get_list_of_token();
            // vector<s_token>::iterator it;
            // for (it = my_tok.begin(); it != my_tok.end(); it++)
            // {
            //     cout << "Value = " << (*it).value;
            //     cout << " ; Type = " << (*it).type << endl;
            // }
        }
    }
    catch (exception & e)
    {
        cerr << RED << e.what() << RESET << endl; // a revoir 
    }
    return 0;
}