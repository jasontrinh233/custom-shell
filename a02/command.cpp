#include "tokenizer.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <list>
#include <iterator>
#include <string>
#include <cstring>
#include <vector>
#include <readline/readline.h> // work with readline()
#include <unistd.h> // work with getcwd(), chdir()
#include <dirent.h> // work with opendir(), readdir()
#include <term.h> // work with clears creen
#include <sys/wait.h>
#include <sys/types.h>
#include <typeinfo>


using namespace std;


// Convert vector<string> to char**
char** argument(vector<string> child_cmd)
{
    char** result = new char*[child_cmd.size()];
    for(int i=0; i < child_cmd.size(); ++i)
    {
        result[i] = new char[child_cmd[i].size() + 1];
        strncpy( result[i], child_cmd[i].c_str(), child_cmd[i].size() );
    }
    
    return result;
}


// Clear screen in
// current shell terminal.
void ClearScreen()
{
    if (!cur_term)
    {
        int erret;
        setupterm( NULL, STDOUT_FILENO, &erret );
        
        if (erret <= 0) // invalid
            return;
    }
    
    string str = "clear"; // terminal clear command
    char *clr = &str[0u]; // points to addr.
                          // of first char
                          // in 'str'
    
    putp( tigetstr(clr) );
}



// Parse tokens list and
// execute valid commands.
void execute_commands( list<string>& tokens_list )
{

	vector<string> tokens_array;

	list<string>::iterator it;
	
	// Store token list in vector to gain
    // ability to access individual elements.
	for(it = tokens_list.begin(); it != tokens_list.end(); ++it)
    {
		tokens_array.push_back( *it );
    }
    

    /* Split tokens_array by ';'
     * or '|' into valid commands
     */
    int idx = 0;
    bool done = false;
    bool piped = false;
    string str;
    vector<string> cmd;
    vector< vector<string> > cmd_array;

    while(idx < tokens_array.size())
    {

        str = tokens_array.at(idx);
        bool add_status = true;

        if(str == ";" || str == "|" )
        {
            done = true; // mark the end of command
            add_status = false; // skip this str

            if(str == "|")
                piped = true;
        }

        // add str to command
        if(add_status == true)
        {
            cmd.push_back(str);
        }

        // add command to cmd_array
        if(done == true && !cmd.empty())
        {
            cmd_array.push_back(cmd);

            // clear cmd contents
            cmd.clear();

            // reset for next cmd
            done = false;
        }

        idx++;

    }

    // add last cmd
    if(!cmd.empty())
    {
        cmd_array.push_back(cmd);
    }
    /* END splitting cmd */


    //
    // EXECUTE EACH VALID COMMAND
    //
    for(int j=0; j<cmd_array.size(); ++j)
    {

        if(piped)
        {
            cout << "WARNING: Pipe not implemented." << endl;
            piped = false;
        }

    	/*
         * Strip <,> and &
         */
    	int strip_from = 0;

    	for(int i=0; i<cmd_array.at(j).size(); ++i)
    	{
    		if(cmd_array.at(j).at(i) == ">" || cmd_array.at(j).at(i) == "&" ||  cmd_array.at(j).at(i) == "<")
    		{
    			// strip all tokens
    			// from this point
    			strip_from = i;
    			break;

    		}
    	}

    	if(strip_from != 0)
    	{
    		for(int i=cmd_array.at(j).size(); i>strip_from; --i)
    		{
                // Delete tokens until
                // i = strip_from.
    			cmd_array.at(j).pop_back(); 
    		}
            
    		cout << "WARNING: IO redirection and background not implemented." << endl;
            return; // exit this function
    	}
    	/* END  <,>,& */


    	/*
    	 * Change directory cd
    	 */
    	if( cmd_array.at(j).at(0) == "cd" )
    	{
    		if(cmd_array.at(j).size() != 2)
    		{
    			cout << "ERROR: Accepts exactly one argument." << endl;
    		}
    		else
    		{
    			if( chdir(cmd_array.at(j).at(1).c_str()) != 0 )
    				cout << "ERROR: Directory does not exit or is not accessible." << endl;
    		}
    	}
    	/* END  cd */


    	/*
    	 * Current directory pwd
    	 */
        else if(cmd_array.at(j).size() == 1 && cmd_array.at(j).at(0) == "pwd")
    	{
    		char path[2048];

    		if(getcwd(path, sizeof(path)) != NULL)
            {
    			cout << path << endl;
            }
    		else
            {
    			cout << "ERROR: Unable to obtain current directory." << endl;
            }
    	}
    	/* END pwd */

        
        /*
         * List files - 'ls'
         * List all directories and files
         * in current dirrectory
         */
        else if(cmd_array.at(j).at(0) == "ls")
        {
            if(cmd_array.at(j).size() > 2)
            {
                cout << "ERROR: Unable to open the directory." << endl;
            }
            else
            {
                DIR *directory;
                struct dirent *dp;
                
                if( cmd_array.at(j).size() == 1 )  // current directory
                {
                    // opendir() returns a pointer to
                    // object of type 'DIR'
                    if( (directory = opendir(".")) == NULL)
                    {
                        cout << "ERROR: Unable to open current directory." << endl;
                    }
                    else
                    {
                        // readdir() returns a pointer to
                        // object of type 'struct dirent'
                        while( (dp = readdir(directory)) )
                            cout << dp->d_name << endl;
                        
                        // close directory
                        closedir (directory);
                    }
                    
                }
                else // custom directory
                {
                    if( (directory = opendir(cmd_array.at(j).at(1).c_str())) == NULL)
                    {
                        cout << "ERROR: No such file or directory." << endl;
                    }
                    else
                    {
                        while ( (dp = readdir(directory)) )
                            cout << dp->d_name << endl;
                        
                        closedir (directory);
                    }
                }
            }
        }
        /* END ls */
        
        /*
         * Clear Screen command,
         * Link -lcurses when compile,
         * ie, g++ -o file -g file.cpp -lcurses
         */
        else if(cmd_array.at(j).size() == 1 && cmd_array.at(j).at(0) == "clear")
        {
            ClearScreen();
        }
        /* END clear */
        
        /*
         * Other system call using fork()
         * and execvp()
         */
        else if(cmd_array.at(j).at(0) != "cd" || cmd_array.at(j).at(0) != "pwd" || cmd_array.at(j).at(0) != "clear")
        {
            pid_t pid;
            int status;
            char **argv = argument(cmd_array.at(j));
            
            if( (pid = fork()) < 0 )  // fork fail
            {
                cout << "ERROR: Unable to spawn program." << endl;
            }
            else if( pid == 0 ) // fork a child process
            {
                const char *cmd = cmd_array.at(j).at(0).c_str();
                
                
                if( execvp( cmd , argument(cmd_array.at(j)) ) < 0 )
                {
                    cout << "ERROR: Unable to execute '" << cmd_array.at(j).at(0) << "'" << endl;
                    exit(1);
                }
            }
            else
            {
                // parent waits for
                // child to complete
                while( wait(&status) != pid )
                    ;
            }
            free(argv);
        }
        /* END fork() */
    }
}



int main ()
{
    // Clear shell screen
    ClearScreen();
    
    // Welcome statement
    cout << "#############################################################################################" << endl;
    cout << "##                                 WELCOME TO MUSH v.1.0                                   ##" << endl;
    cout << "#############################################################################################" << endl;
    
    
	// Session starting...
	while(true)
	{
		char* str;
		list<string> tokens_list;

        
		// get user input
		str = readline("> ");
        
        // exit shell
		if(str == NULL)
			break;

        
        // cast str to type string to
        // work with other funcs.
		string prompt = string(str);


		// tokenize user prompt
        // into tokens_list.
		tokenize(prompt, tokens_list);


        // // DEBUG: display list
        // // of tokens.
		// display(tokens_list);


		// validate and execute
		// user commands.
		execute_commands(tokens_list);


	}
    // End session.

	cout << endl;

}
