//Rui Santos - 2020225542
//Tomás Dias - 2020215701

#include "funcoes.h"


int main(int argc, char *argv[]){
	sigset_t mask;
	sigfillset(&mask);
	sigdelset(&mask, SIGINT);
	sigdelset(&mask, SIGTSTP);
	sigdelset(&mask, SIGTERM);
	sigprocmask(SIG_SETMASK, &mask, NULL);


	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

    if(argc != 2){ //por no log???
        printf("Erro: número de argumentos inválido\n");
        return 0;
    }
    inicia();
    if(fork()==0){
        systemManager(argv[1]);
    }

    wait(NULL);
    return 0;
}
