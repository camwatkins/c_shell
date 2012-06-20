#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* macros as illustrated by matt on piazza.com */

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define SEPARATORS " \t\n"

extern char **environ;
char *home;

/* struct used as wrapper for passing bulk group input */

typedef struct{
     const char *command;
     char *arguments[MAX_ARGS];
     int count;
     
/* flags! */

     int flag_q;
     
     int flag_w;
     int flag_a;
     int flag_r;
     
     int flag_f;
} args;

/* i would have dismanteled this block of prototypes 
 * but my extra credit submissions kept blowing up
 * now i'm afraid to introduce anything dramatic */  

void init_env();
void comm_clear();
void comm_quit();
void comm_environ();
void comm_line();
void comm_pause();
void comm_sleep();
void in_shell();
void sig_trap();

void comm_dir(args a);
void comm_system(args a);	
void comm_cd(args a);
void comm_echo(args a);
void comm_help(args a);
void out_shell(char f[]);
void all_com(args prime);



/***************************
 *        [main]           *
 ***************************/

/* main signal catches and checks for command line arguments 
 * calls for the command line go one way, while calls to 
 * read from file go the other */

int main(int argc, char **argv){

     signal(SIGINT, sig_trap);      

     init_env();
     
     if(argc <= 1){     
          
          in_shell();
     }
    
     else if(argc > 2){

          fprintf(stderr, "ERROR-- no more than one command line argument accepted (did not launch)\n");
          exit(-1);
     }

     else{     
  
          char f[MAX_BUFFER];              
          strcpy(f, argv[1]);  

          out_shell(f);
     }

return 0;
}



/***************************
 * [tokenizing from argv]  *
 ***************************/

void out_shell(char f[]){

     FILE *incoming = NULL;

     char fname[MAX_BUFFER];
     char input_buffer[MAX_BUFFER];
     char *tokens;   
     
     strcpy(fname, f);

     if(!(incoming = fopen(fname, "r"))){
     
          fprintf(stderr, "ERROR-- could not open file %s in function out_shell\n", fname);
          exit(EXIT_FAILURE);
     }
      
     while(!feof(incoming)){

          int i = 0;
          args user_in = { 0 };
   
          user_in.flag_w = -1;
          user_in.flag_a = -1;
          user_in.flag_r = -1;
     
          char hly_str[MAX_BUFFER]; 

/* standard tokenizer */        

          if (fgets(input_buffer, MAX_BUFFER, incoming)) {
        
               strcpy(hly_str, input_buffer);          
          
               if((tokens = strtok(input_buffer, SEPARATORS))){
                               
                    user_in.command = tokens;
                
                    while((tokens = strtok(NULL, SEPARATORS))){     

/* checking to make sure quotes are closed around redirects */
               
                        if(strchr(tokens, '\"')){
  
                             if(user_in.flag_q == 0){
 
                                  user_in.flag_q++;
                             }
         
                             else{
                     
                                  user_in.flag_q = 0;
                             }                         
                         
                         }

/* looking for redirects */

                         if(user_in.flag_q != 1){                   
    
                              if(!strcmp(tokens, "<")){
                     
                                   user_in.flag_r = i;                              
                              }
                
                              if(!strcmp(tokens, ">")){
                     
                                   user_in.flag_w = i;                              
                              }

                              if(!strcmp(tokens, ">>")){
                      
                                   user_in.flag_a = i;                              
                              }
                         }
                    
                         if(!strcmp(tokens, "&")){
                         
                              user_in.flag_f++;
                              continue;
                         }                  

                         user_in.arguments[i] = tokens;
                         user_in.count++;
                         i++;
                    }
               }
           
               else{
                    continue;
          }
     
          all_com(user_in);
     }    

}

}



/****************************
 * [tokenizing from input]  *
 ***************************/

/* virtually identical to the out_shell function, changed
   instead to read from stdin */

void in_shell(){

     char input_buffer[MAX_BUFFER];
     char *tokens;   

     while(!feof(stdin)){
    
/* process variable */

          pid_t pid;
          waitpid(-1, &pid, WNOHANG);

          int i = 0;
          args user_in = {0};
   
          user_in.flag_w = -1;
          user_in.flag_a = -1;
          user_in.flag_r = -1;
     
/* pure, untouched user input */

          char hly_str[MAX_BUFFER];
    
          comm_line();

/* tokenizing */

          if (fgets(input_buffer, MAX_BUFFER, stdin)) {
        
               strcpy(hly_str, input_buffer);          
          
               if((tokens = strtok(input_buffer, SEPARATORS))){
                               
                    user_in.command = tokens;
                
                    while((tokens = strtok(NULL, SEPARATORS))){     
               
                        if(strchr(tokens, '\"')){
  
                             if(user_in.flag_q == 0){
 
                                  user_in.flag_q++;
                             }
         
                             else{
                     
                                  user_in.flag_q = 0;
                             }                         
                         
                         }

                         if(user_in.flag_q != 1){                   
    
                              if(!strcmp(tokens, "<")){
                     
                                   user_in.flag_r = i;                              
                              }
                
                              if(!strcmp(tokens, ">")){
                     
                                   user_in.flag_w = i;                              
                              }

                              if(!strcmp(tokens, ">>")){
                      
                                   user_in.flag_a = i;                              
                              }
                         }
                    
                         if(!strcmp(tokens, "&")){
                         
                              user_in.flag_f++;
                              continue;
                         }                  

                         user_in.arguments[i] = tokens;
                         user_in.count++;
                         i++;
                    }
               }
           
               else{
                    continue;
               }
          
     
          all_com(user_in);
     }    
     
}     
}


/********************************
 * [calls to external commands] *
 *******************************/

void all_com(args prime){ 

/* zombie reaping updates every loop and is handled here */

      pid_t pid;
      waitpid(-1, &pid, WNOHANG);

/*****************[dir]*/

      if(!strcmp(prime.command, "dir")){
                   
/* forking and sorting by process id-- inspired by assignment example */

          pid = fork();

          switch(pid){

/* fork failed */
                
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 

/* child */
       
               case 0:  comm_dir(prime);
                        exit(EXIT_SUCCESS);
   
/* parent process waits for child (or moves along if redirection was detected */
           
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
          } 
     }
     
/*****************[clr]*/
   
     else if(!strcmp(prime.command, "clr")){

/* new fork */

          pid = fork();

          switch(pid){
           
/* fork botched */
     
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 
       
/* child */

               case 0:  comm_clear();
                        exit(EXIT_SUCCESS);
/* parent */
               
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                  
          }        
     }

/*****************[quit]*/

     else if(!strcmp(prime.command, "quit")){

          comm_quit();
     }

/**************[environ]*/     

     else if(!strcmp(prime.command, "environ")){

          pid = fork();

          switch(pid){
            
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 

               case 0:  comm_environ();
                        exit(EXIT_SUCCESS);
               
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                            
          }
     }

/******************[cd]*/

     else if(!strcmp(prime.command, "cd")){
     
          comm_cd(prime);
     }

/***************[pause]*/

     else if(!strcmp(prime.command, "pause")){

          pid = fork();

          switch(pid){
            
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 

               case 0:  comm_pause(prime);
                        exit(EXIT_SUCCESS);
               
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                            
          }
      }

/******************[echo]*/   

      else if(!strcmp(prime.command, "echo")){

          pid = fork();

          switch(pid){
            
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 

               case 0:  comm_echo(prime);
                        exit(EXIT_SUCCESS);
               
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                            
          }
      }

/******************[help]*/

     else if(!strcmp(prime.command, "help")){

          pid = fork();

          switch(pid){
            
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 

               case 0:  comm_help(prime);
                        exit(EXIT_SUCCESS);
               
               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                            
          }
           
     }


/****************[everything else]*/ 

     else{
  
          pid = fork();

          switch(pid){
                
               case -1: fprintf(stderr, "ERROR-- call to fork failed at request from dir\n");
                        exit(EXIT_FAILURE); 
       
               case 0:  comm_system(prime);
                        exit(EXIT_SUCCESS);

               default: if(prime.flag_f == 0){
                             waitpid(pid, NULL, 0);                  
                        }
                  
          }        
      
     }
}

/********************************/

/* .od$$$$$$$$$$$$$$$$bo.
  .X^                  `X.
  cC [ the functions! ] Ob   
  `X.                  .X'      
   `"$$$$$$$$$$$$$$$$$$"'  */

/*************************[dir]*/

void comm_dir(args a){
         
      /*[ system() artifacts ]************************************************************
       *
       *  const char *ls = {"ls -al "};  
       *  char path[MAX_BUFFER];  
       *  strcpy(path, a.arguments[0]);
       *
       *  char *dir = (char*)malloc(sizeof(char) * (strlen(test) + strlen(path) + 2));
       *
       *  if(dir == NULL){
       *  
       *       fprintf(stderr, "ERROR-- malloc failed in function comm_dir");
       *      exit(EXIT_FAILURE);
       *  }
       *
       *  strcat(dir, test);
       *  strcat(dir, path);           
       *  system(dir);
       *     
       *  free(dir);
       *  dir = NULL;          
       *
       ***********************************************************************************/

     int flag_r = a.flag_r;
     int flag_w = a.flag_w;
     int flag_a = a.flag_a;
     
     if(flag_r >= 0){
          
          char f_read[MAX_BUFFER];
          
          strcpy(f_read, a.arguments[flag_r + 1]);
          freopen(f_read, "r", stdin);

          if(flag_r == 0){

               a.arguments[0] = 0;
          }
     }

     if(flag_w >= 0){
   
          char f_write[MAX_BUFFER];
          strcpy(f_write, a.arguments[flag_w + 1]);          
          freopen(f_write, "w", stdout);

          if(flag_w == 0){

               a.arguments[0] = 0;
          }
     }
   
     if(flag_a >= 0){

          char f_append[MAX_BUFFER];
          
          strcpy(f_append, a.arguments[flag_a + 1]);
          freopen(f_append, "a", stdout);

          if(flag_a == 0){

               a.arguments[0] = 0;
          } 
     }
    
     if(a.arguments[0]){

/* went with execlp as it was easier to use from my struct */         
 
          execlp("ls", "ls", "-a", "-l", a.arguments[0], NULL);
     }

     else{

          execlp("ls", "ls", "-a", "-l", NULL);
     }

}


/*******************************[clear]*/

void comm_clear(){

  /*[ system() nostalgia ]*******************************************************************************  
   *
   * system("clear");
   *
   *****************************************************************************************************/

/* did this before i saw that it was unnecessary */

     execlp("clear", "clear", NULL);
}


/***************************[quit]*/

void comm_quit(){
     
     exit(EXIT_SUCCESS);
}


/*************************[execv]*/

void comm_system(args a){

  /*[ system() on sale at goodwill ]********************************************************************
   *  
   * system(a);
   *
   ****************************************************************************************************/

/* this way of handling redirection is janky, i know */
   
     int count = a.count;
     
     int flag_r = a.flag_r;
     int flag_w = a.flag_w;
     int flag_a = a.flag_a;
     
     if(flag_r >= 0){
          
          char f_read[MAX_BUFFER];
          
          strcpy(f_read, a.arguments[flag_r + 1]);
          freopen(f_read, "r", stdin);
          count = count - 2;
     }

     if(flag_w >= 0){
   
        char f_write[MAX_BUFFER];
        strcpy(f_write, a.arguments[flag_w + 1]);          
        freopen(f_write, "w", stdout);
        count = count - 2;
     }

     if(flag_a >= 0){

          char f_append[MAX_BUFFER];
          
          strcpy(f_append, a.arguments[flag_a + 1]);
          freopen(f_append, "a", stdout);
          count = count - 2;
     } 

     char command[MAX_BUFFER];
     strcpy(command, a.command);

/* adding users requested command to a string for execvp() */

     char *arg_string[MAX_ARGS]; 
     
     arg_string[0] = command;

     int i = 1;
     int j = 0;

/* pushing through all users arguments and adding them to string as well */
 
     while(i <= count){
          
          arg_string[i] = a.arguments[j];          
          i++;
          j++;
     }
     
/* exec needs a null */

     arg_string[i++] = NULL;

/* issuing users command/arguments couple with error handling */

     if(execvp(a.command, arg_string)){

          fprintf(stderr, "ERROR-- attempted command failed [(prohibited/non-applicable/misunderstood)(your choice)]\n");
     }
}


/*********************[environ]*/

void comm_environ(){
     
     char **list = environ;

     while(*list != NULL){
          
          printf("%s\n", *list);
          list++;
     }
}


/********************[command line construction]*/

void comm_line(){
     
     char line[MAX_BUFFER] = { 0 };
     char cwd[MAX_BUFFER];
     const char *front = {"[aWW][]["};
     const char *end = {"][]#: "};
 
/* now with error handling!! */

     if (getcwd(cwd, sizeof(cwd)) == NULL){
     
          fprintf(stderr, "ERROR-- getcwd failed in function comm_line");
          exit(EXIT_FAILURE);
     }
     
/* assembling the command line */

     strcat(line, front);
     strcat(line, cwd);
     strcat(line, end);
     
     printf("%s", line);
     fflush(stdout);

/* 200% less double frees! (hopefully) */

}


/****************[cd]*/

void comm_cd(args a){

     if(a.arguments[0]){
     
          char path[MAX_BUFFER];
          strcpy(path, a.arguments[0]); 
     
/* does the path exist? was the command issued correctly? */

          if(chdir(path)){
               
               if(a.count > 1){
                    
                    fprintf(stderr, "ERROR-- change directory failed; too many arguments(%d)\n", a.count);
               }
                 
               else{

                    fprintf(stderr, "ERROR-- change directory failed; no such file or directory(\"%s\")\n", path);
               }
          }

/* error handling for retrieving cwd and set path pwd request */
     
          char cwd[MAX_BUFFER];

          if(getcwd(cwd, sizeof(cwd)) == NULL){
     
               fprintf(stderr, "ERROR-- getcwd failed in function comm_c\n");
               exit(EXIT_FAILURE);
          }     

          if(setenv("PWD", cwd, 1)){

               fprintf(stderr, "ERROR-- setenv failed in function comm_cd\n"); 
          }
     }
     
/* vanilla cd request-- displays the current working directory */

     else{
         
         char *pwd;

         if((pwd = getenv("PWD")) == NULL){
         
              fprintf(stderr, "ERROR-- getenv failed in function comm_cd\n");          
         }
         
         printf("%s\n", pwd);
         fflush(stdout);
     }
}


/*******************[pause]*/

void comm_pause(){

/* getpass() is awesome since it turns off keyboard echo */

     char *garbage;
     garbage = getpass("Press ENTER to continue...");

     if(!garbage){

          fprintf(stderr, "ERROR-- getpass failed in function comm_pause\n");
     }
}


/********************[echo]*/

/* tbh this entire function was a complete pain.
 * stripping the quotes in the tokenizer would have
 * required a complete overhaul of the code. */

void comm_echo(args a){


     int count = a.count;
     
     int flag_r = a.flag_r;
     int flag_w = a.flag_w;
     int flag_a = a.flag_a;
     
     if(flag_r >= 0){
          
          char f_read[MAX_BUFFER];
          
          strcpy(f_read, a.arguments[flag_r + 1]);
          freopen(f_read, "r", stdin);
          count = count - 2;
     }

     if(flag_w >= 0){
   
        char f_write[MAX_BUFFER];
        strcpy(f_write, a.arguments[flag_w + 1]);          
        freopen(f_write, "w", stdout);
        count = count - 2;
     }

     if(flag_a >= 0){

          char f_append[MAX_BUFFER];
          
          strcpy(f_append, a.arguments[flag_a + 1]);
          freopen(f_append, "a", stdout);
          count = count - 2;
     } 

     char hold[MAX_BUFFER] = { 0 };
     char* space = " ";

     int i = 0;
     
     while(i < count){
          strcat(hold, a.arguments[i]);
          strcat(hold, space);
          i++;
     }         

     char* plow;
     char clip = ' ';

     plow = strchr(hold, '\"');

     while(plow != NULL){
          
          *plow = clip;
          plow = strchr(plow + 1, '\"');
     }

     char* stacking[MAX_BUFFER];
     char place[MAX_BUFFER];
     char* tkn;
     int cnt = 1;    

     strcpy(place, a.command);     
     stacking[0] = place;     

     tkn = strtok(hold, SEPARATORS);
     stacking[cnt] = tkn; 
     cnt++;

     while((tkn = strtok(NULL, SEPARATORS))){
        
          stacking[cnt] = tkn;
          cnt++;
     }

     stacking[cnt++] = NULL;

     if(execvp("echo", stacking)){

          fprintf(stderr, "ERROR-- execvp failed in function comm_echo\n");
     }
}

/*****************[setting env]*/

void init_env(){

/* getting both the path for the SHELL environment string 
 * and the working directory to feed the help file */

     const char *wd = "PATH";
     const char *start = "PWD";
     
     char *path;
          
     home  = getenv(start);     
     
     path = getenv(wd);
   
     if(path ==  NULL){  
   
          fprintf(stderr, "ERROR-- getcwd failed in function comm_c\n");
          exit(EXIT_FAILURE);
     }     

     if(setenv("SHELL", path, 1)){

          fprintf(stderr, "ERROR-- setenv failed in function comm_cd\n"); 
     }
}

/*******************[help]*/

void comm_help(args a){     

     char* bin[MAX_ARGS];
     char* more = "more";
     char* nfo = "/readme.nfo";
     char path[MAX_BUFFER];
          
     strcpy(path, home); 
     strcat(path, nfo);
     
     bin[0] = more;
     bin[1] = path;
     bin[2] = NULL;
     
     int flag_r = a.flag_r;
     int flag_w = a.flag_w;
     int flag_a = a.flag_a;

/* redirection is allowed */
     
     if(flag_r >= 0){
          
          char f_read[MAX_BUFFER];
          
          strcpy(f_read, a.arguments[flag_r + 1]);
          freopen(f_read, "r", stdin);
     }

     if(flag_w >= 0){
   
        char f_write[MAX_BUFFER];
        strcpy(f_write, a.arguments[flag_w + 1]);          
        freopen(f_write, "w", stdout);
     }

     if(flag_a >= 0){

          char f_append[MAX_BUFFER];
          
          strcpy(f_append, a.arguments[flag_a + 1]);
          freopen(f_append, "a", stdout);
     } 


      if(execvp("more", bin)){

          fprintf(stderr, "ERROR-- execvp failed in function comm_help\n");
     }

}

/*************[SIGINT trap]*/

void sig_trap(){

/* control-c returns the user to the shell's command line */

}
