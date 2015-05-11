// Bich Diep
// Creating Kernel
//

#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
using namespace std;


// constant characters used for tokenizing inputs
const char SINGLE_QUOTE = '\'';
const char DOUBLE_QUOTE = '\"';
const char PIPE = '|';

// values of descriptors
const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
const int STD_IN = 0;
const int STD_OUT = 1;

// an array store a maximum number of 50 pipes
int pipes[50][2];


// a struct that holds a single command
struct Single_Command
{
    vector<char*>  indv_cmd;

    Single_Command(vector<char*> a)
    {
        indv_cmd = a;
    }
};


// a struct that holds 1 built-in command
struct BuiltinCmd {

    string name; //you may use char name[100] instead
    int (*fun)(char**); //function placeholder

    BuiltinCmd(string n, int (*f)(char**))
    {
        name = n;
        fun = f;
    }
};


//wrapper for command cd
int builtinCD(char** args) {

    // get the direction from args[1]
    char* dir = args[1];

    if (args[1] == NULL) // if there's no dir, dir = HOME
        dir = getenv("HOME");

    return chdir(dir); //call system call chdir()
}

//wrapper for command exit
int builtinExit(char** args) {

    if(args[1]!= NULL)
        exit(atoi(args[1])); // exit returns with the error code provided by value
    else
        exit(0); // otherwise, exit with code 0

    return 0;
}


// an array hold built-in commands. This project only works with cd and exit commands
BuiltinCmd sysCmd[] = { BuiltinCmd("cd",&builtinCD),
                        BuiltinCmd("exit",&builtinExit)};


void close_Single_Pipes(int (&fd)[2])
{
    close(fd[0]);
    close(fd[1]);
}

// closing pipes in range startPosition -> endPosition inclusively
void closeMultiPipes(int startPosition, int endPosition)
{
    for( int i =startPosition; i <= endPosition; i++)
    {
        close_Single_Pipes(pipes[i]);
    }
}


// waiting for children terminate
// Print the process ID of that command and the error code
void pick_DeadChildren()
{
    int pid, status;
    while((pid = wait(&status)) != -1)
    {
        fprintf(stderr, "process %d exited with %d\n", pid, WEXITSTATUS(status));
    }
}


// Checks if a built-in command and executes
// Returns false if not a built-in command
bool Single_BuiltIn(char** args)
{
    char* cmd = args[0];

    // check each command
    for(unsigned i=0; i<2; i++)
    {
        if (!strcmp(cmd, sysCmd[i].name.c_str()))
        {
            // execute if can, or return error message
            if (sysCmd[i].fun(args)<0)
                perror(cmd);

            return true;
        }
    }

    return false;
}


// Executes a single non-built in command
void Single_NonBuiltIn(char** args)
{
    char* cmd = args[0];

    int pid = fork();


    switch (pid) {
        case 0: // belongs to child
            execvp(cmd, args);
            perror(cmd);
            exit (1);
            return;
        case -1:
            perror("fork");
            exit(1);
            return;
        default: // parent
            pick_DeadChildren();
            return;
    }
}


// Processes a single pipe for the multi-processing part
void processPipe(int pid, char** cmd, int position, int no_pipes)
{

    // default: start at 2nd cmd, end at 2nd last cmd
    // used for marking the start and end of array to close pipes
    int startPosition = 1;
    int endPosition = position -1;


    switch (pid) {
        case 0: // belongs to child

            // if it's not the first cmd
            if(position != 0)
            {
                // replace standard input of this cmd with the read part
                // of the pipe connecting this cmd and prev cmd
                dup2(pipes[position-1][PIPE_READ],STD_IN);
                // close unused other end of that pipe
                close(pipes[position-1][PIPE_WRITE]);
                startPosition = 0;
            }

            // if it's not the last cmd
            if(position != no_pipes)
            {
                // replace standard output of this cmd with the write part
                // of the pipe connecting this cmd and next cmd
                dup2(pipes[position][PIPE_WRITE], STD_OUT);
                // close unused other end of that pipe
                close(pipes[position][PIPE_READ]);
                endPosition = position;
            }

            // close other unused pipes
            closeMultiPipes(startPosition, endPosition);

            // execute
            // check for built in, if true, process
            if(Single_BuiltIn(cmd))
                // exit if successfully execute
                exit(0);

            // if not built in commands, process non builtin
            execvp(cmd[0], cmd);
            perror(cmd[0]);
            exit(1);

        case -1: // fail
            perror("fork");
            exit(1);

        default: // parent
            break;
    }
    
}


void execute_MultipleCommand(const vector<Single_Command> &cmdContainer, int no_pipes )
{

    for(int i =0; i <= no_pipes; i++) // loop through every single command, stop when no command left
    {
        Single_Command cmd = cmdContainer[i];
        char** c_cmd1 = &cmd.indv_cmd[0]; // convert into char*[] from vector

        // create a pipe if there's still a pipe left
        if (i<no_pipes)
            pipe(pipes[i]);

        // fork a child and then process
        processPipe(fork(), c_cmd1, i, no_pipes);

    }

    // close all the pipes
    closeMultiPipes(0, no_pipes+1);

    // wait for children to terminate
    pick_DeadChildren();

}


void execute_SingleCommand(char** args)
{
    // check for built in, if true, process
    // if not built in commands, process non builtin
    if(!Single_BuiltIn(args))
        Single_NonBuiltIn(args);
}

// executes single command or multi-process dependent on
// input line
void execute(const vector<Single_Command> &cmdContainer)
{
    // Single process
    if (cmdContainer.size() <2)
    {
        Single_Command cmd = cmdContainer[0];
        execute_SingleCommand(&cmd.indv_cmd[0]);
        return;
    }


    // multi process (using pipes)
    execute_MultipleCommand(cmdContainer, (int)cmdContainer.size()-1);

}

// Groups tokens into commands and inserts into a vector
// Return the number of commands
unsigned parseCommand(vector < Single_Command > &cmdContainer, const vector<string> &container)
{
    // holds the pointer to each respective token
    vector<char* > indv_command;

    // converts from string to char*
    char *c_token;
    string s_token;
    unsigned cmd_count = 0;

    // iterates through the token container
    for (unsigned i = 0; i < container.size(); i++)
    {
        // if the token is not a pipe, push the token into the individual vector command
        // otherwise push the individual command into cmdContainer
        if(container[i] != "|") {
            s_token = container[i];
            // Convert that std::string into a C string.
            c_token = new char[s_token.size()+1];
            strcpy(c_token, s_token.c_str());
            indv_command.push_back(c_token);

            // for case of last command of container
            // push command into cmdContainer
            if (i== container.size()-1) {
                indv_command.push_back(NULL);
                cmdContainer.push_back(Single_Command(indv_command));
                indv_command.clear();
                cmd_count++;
            }
        } else {
            // after pushing individual command clear the temporary vector, indv_command
            indv_command.push_back(NULL);
            cmdContainer.push_back(Single_Command(indv_command));
            indv_command.clear();
            cmd_count++;
        }
    }

    indv_command.clear();

    return cmd_count;
}


// Tokenizes the input line and puts it into the vector
// Returns -1 if incorrect input, otherwise returns the number of pipes found
int tokenize_Line(vector<string> &container, string s)
{
    // keeps track of index of string
    unsigned  current = 0;

    // token number
    unsigned count = 0;

    // current token
    string token;

    unsigned no_Pipes = 0;

    // while string still has unread characters
    while (current<s.length())
    {
        // check delimiter type, single quote or double quote, pipe, or space
        // splits the string into tokens by the delimiters found
        if(s[current] == SINGLE_QUOTE || s[current] == DOUBLE_QUOTE)
        {
            // saves quote character to end token
            char  deleteChar = s[current];
            string temp = s.substr(current+1);

            size_t found = s.find_first_of(deleteChar,current+1);

            // if input error, returns -1
            if(found < current || found >s.length()-1)
                return -1;

            // gets token
            token = s.substr(current+1,found-current-1);
            count++;

            // increments index of s
            current = (int) found;

            container.push_back(token);

        }
        else if ( s[current] == PIPE)
        {
            // gets pipe token
            token = s.substr(current,1);

            count++;
            no_Pipes++;
            container.push_back(token);


        } else
        {
            // tokenizes for remainding string minus spaces
            size_t found = s.find_first_of(" \"\'|",current+1);

            // error handling
            if(found < current)
                return -1;

            // for last token of the string
            if (found> s.length()-1)
                found = s.length();

            // if spaces, get substring of token minus spaces
            if(found>current)
            {
                token = s.substr(current,found-current);

                current = (int) found - 1;

                // checking if token is empty space
                bool allSpace = true;
                for(unsigned i =0; i< token.length();i++)
                {
                    if(token[i] != ' ')
                    {
                        allSpace = false;
                        break;
                    }
                }

                // if correctly inputted token, put in container
                if(!allSpace)
                {
                    count++;
                    if(token[0] == ' ')
                        token = token.substr(1);

                    container.push_back(token);
                }

            }
        }

        current++;

    }

    return no_Pipes;

}



int main(int argc, const char * argv[]) {

    vector<string> container;
    vector< Single_Command > cmd_Container;

    string s = "";

    cout << "\nWelcome to our awesome shell ^.^ \n\n";

    // takes a single line of input and executes it in our shell

    cout << "\nAwesomeTeam > $ ";
    getline(cin, s);

    // input check, tokenize, and put in container
    if(tokenize_Line(container, s) < 0)
    {
        cout << "invalid input" << endl;
        return 1;
    }

    parseCommand(cmd_Container, container);

    container.clear();

    execute(cmd_Container);

    cmd_Container.clear();
    cout << endl;

    return 0;
}



