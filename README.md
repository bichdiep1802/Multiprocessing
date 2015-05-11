A simple shell that will run one command or a pipeline of commands. When each process terminates, the shell will print the exit status for the process. The shell can simply exit after the command or pipeline of command is executed.

Design Limitations:
  Not as efficient as possible.  Performance loss with the use of a vector
  Only handles two built-in commands
  User can input commands that are not actual commands and the shell will not tell them it is an incorrect command, the system will default return the message “No such file or directory”
  To hold each individual command we use a struct to hold a vector instead of just a vector, which uses extra memory (but is easier to read)
  The program runs for only one line of input
  No separation of .cpp file and .h file, harder to maintain code

Design Highlights:
  Passes the tests
  Multi-processing
  Easier to read with use of structs to wrap vectors
  No separation of .cpp file and .h file, easier to read, less files to keep track of


