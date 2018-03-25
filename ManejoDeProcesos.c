/*
  Cada 4 segundos se establece una sincronización entre procesos, en el acceso a un recurso compartido
*/

#include <unistd.h> 
#include <signal.h> 
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <time.h>
#include <semaphore.h>


int semaforo, status, count = 0;
struct sembuf P,V; 
pid_t pid;

void show_date();
void semaphore();

int main() {
    signal(SIGALRM, show_date);
	
	time_t t = time(NULL);		//I want to show the initial time before exec the alarm	
	struct tm *tm = localtime(&t);
	printf("%s\n", asctime(tm));

	alarm(4);
    while(1){
		semop(semaforo,&P,1);
		fflush(NULL);
		printf("Recurso compartido\n");  
		semop(semaforo,&V,1); 
		usleep(1000000);
	}
}

void show_date() {
	semop(semaforo,&P,1);
    pid = fork();
	semop(semaforo,&V,1);

	if (pid == -1){
 		perror ("Error to create a process");
 	}else if(pid == 0){
		printf("child process\n");
		execl("/bin/date","date",NULL);	
	}else{
		wait(&status);		
		alarm(4); 
    }
}

void semaphore(){
	key_t key=ftok("/bin/date",1); 
	semaforo = semget(key, 1, IPC_CREAT | 0600); //rwx 101 owner
 	semctl(semaforo,0,SETVAL,1);
	P.sem_num = 0;
	P.sem_op = -1;
 	P.sem_flg = SEM_UNDO;
 	V.sem_num = 0;
 	V.sem_op = 1;
 	V.sem_flg = SEM_UNDO;
}
