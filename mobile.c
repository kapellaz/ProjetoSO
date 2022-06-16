//Rui Santos - 2020225542
//Tomás Dias - 2020215701

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#define PIPE_NAME  "TASK_PIPE"


typedef struct
{
	time_t tempo;
    int ID;
    int instructionsNumber;
    float maxtime;
} tarefa;


bool verifica_num(char s[]){
	for (int i = 0; s[i]!= '\0'; i++){
		if (isdigit(s[i]) == 0)
			return false;	
		}
	return true;
}

int main(int argc, char *argv[])
{
    if(argc != 5){
        printf("Erro: Verifique os argumentos\n");
        return 0;
    }

    // Opens the pipe for writing
    int fd;
    if ((fd=open(PIPE_NAME, O_WRONLY)) < 0)
    {
        perror("Cannot open pipe for writing: ");
        exit(0);
    }

    int control = 1;
    if(verifica_num(argv[1]) && verifica_num(argv[2]) && verifica_num(argv[3]) && verifica_num(argv[4])){
    	while(control <= strtol(argv[1], NULL, 10)){
    		char to_send[100];
    		char aux[50];
    		sprintf(aux, "%d,", (int) strtol(argv[3], NULL, 10));
    		strcpy(to_send, aux);
    		sprintf(aux, "%f", atof(argv[4]));
    		strcat(to_send, aux);
        	write(fd, &to_send, sizeof(to_send));
        	usleep((float)(atoi(argv[2])*1000));
        	control++;
    	}
    }else{
    	printf("Comando inválido\n");
    }
    return 0;
}
