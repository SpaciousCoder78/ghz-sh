/***********************************************************
 * 
 * @headerfile       shellops.h
 * 
 * @author           Aryan Karamtoth (SpaciousCoder78)
 * 
 * @date             Thursday, August 15 2024
 * 
 * @brief            Gigahertz Shell's Operation Functions
 * 
 * 
 * 
 * 
 **********************************************************/



//header file for handling shell operations
#include<stdio.h>
#include<stdlib.h>
//avoiding double execution
#ifndef SHELLOPS_H
#define SHELLOPS_H
#define GHZ_RL_BUFFERSIZE 1024 //buffer size
#define EXIT_FAILURE 1 //exit error code
#define GHZ_TOK_BUFFERSIZE 64
#define 
//loop for shell
void ghzsh_loop(void){
    char *line;
    char **args;
    int status;

    do{
        printf("> ");
        //reading and executing input
        line= ghzsh_readLine();
        args = ghzsh_splitLine();
        status = ghzsh_execute(args);

        //freeing allocated memory
        free(line);
        free(args);
    }while(status);
}

// function for reading input

char *ghzsh_readLine(void){
    //setting buffer size
    int bufsize = GHZ_RL_BUFFERSIZE;
    //setting buffer position
    int position = 0;
    //allocating memory for buffer
    char *buffer = (char*)malloc(sizeof(char)*bufsize);
    int c;

    //handling buffer memory allocation error
    if(!buffer){
        fprintf(stderr,"ghz-sh: Buffer Allocation Failed");
        exit(EXIT_FAILURE);
    }

    while(1){
        //reading input
        c=getchar();

        //handling null or EOF event
        if(c==EOF || c=='\n'){
            buffer[position]='\0';
            return buffer;
        }else{
            buffer[position]=c;
        }
        position++;

        //handling event where buffer limit is exceeded
        if(position>=bufsize){
            bufsize+=GHZ_RL_BUFFERSIZE;
            //realloc the buffer
            buffer = (char*)realloc(buffer,bufsize);
            if(!buffer){
                fprintf(stderr,"ghz-sh: Buffer Allocation Error");
                exit(EXIT_FAILURE);
            }
        }


    }

}

//split line



#endif