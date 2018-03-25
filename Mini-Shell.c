#define _GNU_SOURCE
// Para las funciones pause y alarm:
#include <unistd.h>
// Para las constantes SIGALRM y similares
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>	/* para key_t */
#include <semaphore.h>
#include <sysexits.h>
#include <sys/mman.h>
#include <string.h> 	/* memset */
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio_ext.h>
#include <pwd.h>

#define NEW (O_WRONLY|O_CREAT|O_EXCL)
#define PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

//Define of type bool.
typedef enum { false, true } bool;

struct command {
    int numCom;
    char pipe_command[300];
    char original_Command[150];
    char *args[8];
    int numArg;
    bool pipe;              // We have a pipe |
    bool background;        // More complex, instruction must be executed in background.
    bool input;             // <<              
    bool output;            // >>
};

char terminal_Command[150];
struct command historyCommand[20];
char *file;

void showCommand(struct command comando){
    if(comando.pipe==1){
        printf("Comando tubería: %s \n", comando.pipe_command);
        printf("Num comandos: %d \n", comando.numCom);
    }
    
    printf("Comando: %s \n",        comando.original_Command);
    printf("Num Argumentos: %d \n", comando.numArg);
    for(int i=0; i<comando.numArg; i++){
        printf("Arg %d: %s \n",i ,comando.args[i]);
    }


    printf("Comando asdasda %s \n", comando.original_Command);
    printf("historia  %s \n", historyCommand[1].original_Command);
}

void clearTerminal(){
    __fpurge(stdin);                    // Clean output terminal
    //for(int i=0; i<151;i++){ terminal_Command[i] = '\0'; } // Clean input array
    memset(terminal_Command,'\0',150);  // Clean input array
}

void redirection(struct command comando){
    FILE *fp;

    if(comando.output == 1){
        int file = creat(comando.args[3], 0644);
        dup2(file, STDOUT_FILENO);
        close(file);
    }else if(comando.input == 1){
        int file = open(comando.args[3], O_RDONLY);
        dup2(file, STDIN_FILENO);
        close(file);
    }
}

void executeCommand(struct command comando){
    int status = 100;
    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
            perror ("Error to executed a process");
            exit(-1);
        case 0:
            if(comando.pipe == 1){
                // 0 => STD_INPUT
                // 1 => STD_OUTPUT
                // 2 => STD_ERROR
                /*  
                    int dup2 (int fd, int copia_fd) => -1 error 
                    dup2: cierra el descriptor que corresponde a copia_fd y luego copia la entrada
                    del correspondiente al primer parámetro fd sobre la del segundo parámetro
                    copia_fd

                    mypipe[0] => descriptor de fichero válido para leer en la tubería de salida
                    mypipe[1] => descriptor de fichero válido para escribir en la tubería de entrada
                */

                pid_t pide;
                int mypipe[2];
                pipe(mypipe); //Creación de una tubería.
                pide=fork();
                switch (pide) {
                    case -1:
                        perror("Error to executed a process");
                        exit(-1);
                    case 0:
                        
                        close(mypipe[0]);   //Close channel output
                        dup2(mypipe[1],1);  //Redirect output to mypipe
                        if(historyCommand[comando.numCom-1].output == 1){
                            redirection(historyCommand[comando.numCom-1]); //Execute
                        } else if(execvp(historyCommand[comando.numCom-1].args[0],historyCommand[comando.numCom-1].args)<0) {
                            exit(-1);
                        }
                    default:
                        close(mypipe[1]);
                        dup2(mypipe[0],0);
                        if(comando.output == 1){
                            redirection(comando);
                        } else if(execvp(comando.args[0],comando.args)<0) {
                            exit(-1);
                        }
                }
                
            }else if((comando.input == 1) || (comando.output ==1)){
                redirection(comando);
            }else if(execvp(comando.args[0],comando.args)<0) {
                exit(-1);
            }
        default:
            if(comando.background == 1){
                clearTerminal();
                waitpid(pid,&status,WNOHANG); 
            }else{
                clearTerminal();
                waitpid(pid,&status,0); 
            }
    }

}

void splitArg(struct command comando, int execute){
    char *original_commands = strdup(comando.original_Command);
    char *argCommand = strtok(original_commands, " ");
    int cont = 0;
    char *comprobationCommand = strstr(terminal_Command, "&");   
    if(comprobationCommand != NULL){
        comando.background = true;
    }

    comprobationCommand = strstr(terminal_Command, "<");   
    if(comprobationCommand != NULL){
        comando.input = true;
    }

    comprobationCommand = strstr(terminal_Command, ">");   
    if(comprobationCommand != NULL){
        comando.output = true;
    }
    
    while(argCommand != NULL){
        comando.args[cont] = argCommand;
        argCommand = strtok(NULL, " ");
        cont++;
    }
    comando.numArg = cont;

    //Execute == 1 means that it will process the pipe.
    if(execute == 0){
        executeCommand(comando);
    }
}

    

void splitCommands(){
    /*  Mark:
            BackGround (&)  <= Done!
            Pipe (|)        <= Done!
            input (<)       <= Done!
            output (>)      <= Done!
    */

    struct command com = {.numCom = 0, .numArg = 0, .args = NULL, .pipe = false, .background = false, .input = false, .output = false};
    int execute = 0;
    char *comprobationCommand = strstr(terminal_Command, "|");   
    if(comprobationCommand != NULL){
        strcpy(com.pipe_command,terminal_Command); //Copy the original command  
        com.pipe = true;
        execute = 1;
    }
    
    char *pipeComprobation = strtok(terminal_Command, "|");
    while (pipeComprobation != NULL){
        strcpy(com.original_Command,pipeComprobation);
        historyCommand[com.numCom] = com;
        pipeComprobation = strtok(NULL, "|");
        if(pipeComprobation == NULL) execute = 0;
        splitArg(historyCommand[com.numCom], execute);
        com.numCom++;
    }
}

int main (){
    while(1){
		printf("%s@[%s]> ",getpwuid(getuid())->pw_name,get_current_dir_name()); 
        clearTerminal();
        scanf("%[^\n]s",terminal_Command); //read input stream from terminal.
        if(strcmp(terminal_Command,"exit") == 0 || strcmp(terminal_Command,"quit") == 0){
            exit(0);
        }

        splitCommands();
    };

    return 0;
}
