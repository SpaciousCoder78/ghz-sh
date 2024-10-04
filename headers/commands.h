/****************************************************
 * 
 * @headerfile       commands.h
 * 
 * @author           Aryan Karamtoth (SpaciousCoder78)
 * 
 * @date             Thursday, August 15 2024
 * 
 * @brief            Gigahertz Shell's Command Functions
 * 
 ****************************************************/



#ifndef COMMANDS_H
#define COMMANDS_H

#include<stdio.h>
#include<unistd.h>
/*
  Function Declarations for builtin shell commands:
 */
int ghzsh_chdir(char **args); //change directory
int ghzsh_sos(char **args); //help command
int ghzsh_leave(char **args); //exit command
int ghzsh_tell(char **args); //tell command

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "chdir", //change directory
  "sos", //help command
  "leave" //exit command
  "tell" //tell command
};

int (*builtin_func[]) (char **) = {
  &ghzsh_chdir, //change directory
  &ghzsh_sos, //help command
  &ghzsh_leave //exit command
  &ghzsh_tell //tell command
};

int ghzsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
***********************************Builtin function implementations*************************************
*/

//***************************************chdir*********************************************** */
int ghzsh_chdir(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "ghz-sh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("ghz-sh");
    }
  }
  return 1;
}

int ghzsh_tell(char **args){
  args = strtok(NULL, " \t");

    while(args != NULL)
    {
        int n = strlen(args);
        int doubleQuotes = 0;
        int singleQuotes = 0;

        for(int i=0; i<n; i++)
        {
            if(args[i] == '"')
            {
                doubleQuotes++;
            }
            
            else if(args[i] == '\'')
            {
                singleQuotes++;
            }
        }

        for(int i = 0; i < n; i++)
        {
            if(singleQuotes%2 == 0 && doubleQuotes%2 == 0)
            {
                if(args[i] == '"' || args[i] == '\'')
                {
                    continue;
                }

                else
                {
                    printf("%c", args[i]);
                }
            }

            else if(singleQuotes%2 == 0)
            {
                if(args[i] == '\''){
                    continue;
                }

                else{
                    printf("%c", args[i]);
                }
            }

            else if(doubleQuotes%2 == 0){
                if(args[i] == '"'){
                    continue;
                }

                else{
                    printf("%c", args[i]);
                }
            }

            else{
                printf("%c", args[i]);
            }
        }

        printf(" ");
        args = strtok(NULL, " \t");
    }
    printf("\n");
}

//*****************************************sos*********************************************** */
int ghzsh_sos(char **args)
{
  int i;
  printf("Aryan Karamtoth's ghz-sh\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < ghzsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}


//*****************************************leave*********************************************** */
int ghzsh_leave(char **args)
{
  return 0;
}

#endif