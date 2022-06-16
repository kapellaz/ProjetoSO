//Rui Santos - 2020225542
//Tomás Dias - 2020215701


#include "funcoes.h"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scheduler_work = PTHREAD_COND_INITIALIZER;
pthread_cond_t scheduler_end = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dispatcher_work = PTHREAD_COND_INITIALIZER;
pthread_cond_t dispatcher_free = PTHREAD_COND_INITIALIZER;
pthread_mutex_t vcpu_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vcpu_free = PTHREAD_COND_INITIALIZER;
pthread_cond_t go_vcpu = PTHREAD_COND_INITIALIZER;
pthread_mutexattr_t mattr;
pthread_condattr_t cattr;


int control;



void colocar(struct tarefas *p, tarefa t){
    struct no_fila *aux;
    aux = (struct no_fila*) malloc(sizeof(struct no_fila));
    aux->t=t;
    aux->pseg=NULL;

    if(p->raiz==NULL){
        p->raiz=aux;
        p->tamanho = 1;
    }
    else{
        struct no_fila *temp;
        for(temp = p->raiz; temp->pseg!=NULL; temp = temp->pseg);
        temp->pseg = aux;
        p->tamanho+=1;
    }
}

void ordenar(struct tarefas *p){
    struct no_fila *atual = p->raiz, *index = NULL;

    tarefa t;
    if(p->raiz==NULL){
        return;
    }
    else{
        while(atual!=NULL){
            index = atual->pseg;

            while (index!=NULL){
                if(atual->t.maxtime>index->t.maxtime){
                    t = atual->t;
                    atual->t=index->t;
                    index->t=t;
                }
                index=index->pseg;
            }
            atual=atual->pseg;
        }
    }
}



void retirar(struct tarefas *p, tarefa t){
    struct no_fila *temp = p->raiz, *anterior;

    if(temp!=NULL && temp->t.ID==t.ID){
        p->raiz=temp->pseg;
        free(temp);
        p->tamanho--;
        return;
    }
    while(temp!=NULL && !(temp->t.ID==t.ID)){
        anterior=temp;
        temp=temp->pseg;
    }
    if(temp==NULL){
        return;
    }
    anterior->pseg=temp->pseg;
    p->tamanho--;
    free(temp);

}

bool verifica_num(char s[]){
	for (int i = 0; s[i]!= '\0'; i++){
		if (isdigit(s[i]) == 0)
			return false;	
		}
	return true;
}


void ler_file(char nome[]){
    FILE* ptr;
    int qp, mw, esn;
    
    ptr = fopen(nome, "r");
    if(ptr == NULL) printf("o ficheiro não existe");

    char ch[30];
    int control=0;
    int control2 = 0;
    while(fscanf(ptr, "%s", ch)==1){
        if (control<3 && !verifica_num(ch)){
        	logW("Ficheiro de configuracoes incorreto\n");
        	exit(0);
        }
        else if (control==0) {
             qp = (int)strtol(ch, NULL, 10);
        }
        else if (control==1) {
            mw = (int)strtol(ch, NULL, 10);
        }
        else if (control==2) {
            esn = (int) strtol(ch, NULL, 10);
            //criar shared memory
    		shmid = shmget(IPC_PRIVATE, sizeof(memoria)+esn*sizeof(edge_Servers)+sizeof(struct tarefas)+qp*sizeof(struct no_fila), IPC_CREAT | 0700);
		
    		if(shmid < 0){
        		logW("Erro ao criar a shared memory");
        		exit(0);
    		}
		
    		//attatch
    		mem = (memoria*) shmat(shmid, NULL, 0);
    		if(mem<0){
        		logW("Erro no attatchment");
        		exit(0);
    		}
		
    		logW("Shared memory criada\n");
            mem->QUEUE_POS=qp;
            mem->MAX_WAIT=mw;
            mem->EDGE_SERVER_NUMBER=esn;
        }
        else{
            char *token = strtok(ch, "  .,;:!(-<>)#/[]{}\"?\r\n\t");
            int x = 1;
            while (token != NULL) {
                edge_Servers aux;
                if(x>1 && !verifica_num(token)){
                	logW("Ficheiro de configuracoes incorreto\n");
        			exit(0);
                }
                if(x==1) strcpy(aux.nome, token);
                if(x==2) aux.c1 = (int)strtol(token, NULL, 10);
                if(x==3) {
                    aux.c2 = (int)strtol(token, NULL, 10);
                    mem->edge_servers[control2] = aux;
                    control2++;
                }
                token = strtok(NULL, " .,;:!(-<>)#/[]{}\"?\r\n\t");
                x++;
            }
        }
        control++;
    }
    fclose(ptr);
}

void handler_sigint(int num){
	logW("SIGNAL SIGINT RECEIVED\n");
	logW("WAITING FOR ALL TASKS TO FINISH\n");
	kill(0, SIGTERM);
	while(wait(NULL)!=-1);
	printf("Tarefas que não se chegaram a realizar: %d\n",mem->tarefas.tamanho);
	kill(sys, SIGTSTP);
	termina();
}

void handler_sigterm_tm(int num){
    char tolog[100];
	for(int i = 0; i<mem->EDGE_SERVER_NUMBER; i++){
		while(mem->edge_servers[i].flag[0]==1 && mem->edge_servers[i].flag[0]==1);
		kill(mem->edge_servers[i].pid, SIGKILL);
		sprintf(tolog, "Servidor %d e seus vcpus terminados\n", i+1);
		logW(tolog);
		wait(NULL);
	}
	pthread_cancel(scheduler);
	pthread_cancel(dispatcher);
	pthread_join(scheduler,NULL);
	pthread_join(dispatcher,NULL);
	logW("Threads scheduler and dispatcher killed\n");
	logW("Task Manager terminado\n");
	kill(task_manager, SIGKILL);
}


void handler_sigterm_mm(int num){
	logW("Maintenance Manager terminado\n");
	kill(maintenance_manager, SIGKILL);
}


void handler_sigterm_m(int num){	
	logW("Monitor terminado\n");
	kill(monitorr, SIGKILL);
}

void handler_estatistica(int num){

	logW("SIGNAL SIGTSTP RECEIVED\n");
	sem_wait(write_mutex);
	char buffer[100];
	sprintf(buffer, "Total tarefas executadas: %d\n", mem->total_tarefas_realizadas);
	logW(buffer);
	sprintf(buffer,"Total medio de resposta: %f\n", (float)mem->tempo_medio/mem->total_tarefas_realizadas);
	logW(buffer);
	for(int i = 0; i<mem->EDGE_SERVER_NUMBER; i++){
		sprintf(buffer,"Total tarefas executadas pelo server_%d: %d\n", i+1, mem->edge_servers[i].numero_executadas);
		logW(buffer);
		sprintf(buffer,"Total operações de manutencao no server_%d: %d\n", i+1, mem->edge_servers[i].numero_manutencoes);
		logW(buffer);
	}
	sprintf(buffer, "Total tarefas não executadas: %d\n", mem->total_tarefas_nao_realizadas);
	logW(buffer);
	
	sem_post(write_mutex);
	
}




void systemManager(char fileConfig[]){		
	sys = getpid();

    logW("OFFLOAD SIMULATOR STARTING\n");
    

    //ler ficheiro configuracoes
    ler_file(fileConfig);
	
	mem->keep_going=0;
	mem->tarefas.tamanho=0;
	mem->tarefas.raiz=NULL;
	mem->flag=0;
	mem->total_tarefas_realizadas=0;
	mem->tempo_medio=0;
	mem->ativo=true;
	mem->tempo_medio=0;
	mem->total_tarefas_nao_realizadas=0;
	mem->stop=0;
	mem->keep_going_monitor=0;
	mem->keep_going_manutencao=0;
	control=0;
	
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);  
	pthread_mutex_init(&mem->monitor_mutex, &mattr);
	pthread_mutex_init(&mem->manutencao_mutex, &mattr);

	pthread_condattr_init(&cattr);
	pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);  
	pthread_cond_init(&mem->monitor_go, &cattr); 
	pthread_cond_init(&mem->manutencao_go, &cattr); 
	pthread_cond_init(&mem->vcpu_go, &cattr);

    // Creates the named pipe if it doesn't exist yet
    if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0777)<0) && (errno!= EEXIST))
    {
        perror("Cannot create pipe\n");
        exit(0);
    }
    
    //create message queue
    
    mqid = msgget(IPC_PRIVATE, IPC_CREAT|0777);
    if(mqid<0){
    	logW("Error creating message queue\n");
    	exit(0);
    }
    

    //criar processo taskmanager
    if((fork()) == 0){
        taskmanager();
        exit(0);
    }
    //criar processo maintenance manager
    if((fork()) == 0){
        maintenanceManager();
        exit(0);
    }

    //criar processo monitor
    if((fork()) == 0){
        monitor();
        exit(0);
    }
    signal(SIGINT, handler_sigint);
    signal(SIGTSTP, handler_estatistica);
    signal(SIGTERM, SIG_IGN);

    printf("limpou os main processes\n");
}


void *vCPU(void *n){
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	vcpu_args va = *((vcpu_args*)n);
    char *nome = va.nome;
    char output[] = "vcpu de ";
    strcat(output, nome);
    strcat(output, " criado\n");
    logW(output);
	char out[100];
	if(va.i==0){
		while(control==0){
			pthread_mutex_lock(&mem->edge_servers[va.index].vcpu_mutex[va.i]);
			while(mem->edge_servers[va.index].flag[0]!=1){
				pthread_cond_wait(&mem->edge_servers[va.index].vcpu_work[va.i],&mem->edge_servers[va.index].vcpu_mutex[va.i]);
			}
			sprintf(out,"VCPU %d DE SERVER %d a trabalhar\n", va.i+1, va.index+1);
			logW(out);
			sleep(mem->edge_servers[va.index].tempos[0]);
			sprintf(out,"VCPU %d DE SERVER %d acabou o trabalho\n", va.i+1, va.index+1);
			logW(out);
			sem_wait(write_mutex);
			mem->edge_servers[va.index].tempos[0]=0;
			if(mem->edge_servers[va.index].em_man==0){
				mem->edge_servers[va.index].flag[va.i]=0;
			}else{
				mem->edge_servers[va.index].flag[va.i]=2;
			}
			sem_post(write_mutex);
			pthread_mutex_unlock(&mem->edge_servers[va.index].vcpu_mutex[va.i]);
		}
	}
	if(va.i==1){
		while(control==0){
			pthread_mutex_lock(&mem->monitor_mutex);
			while(mem->flag==0){
				pthread_cond_wait(&mem->vcpu_go, &mem->monitor_mutex);
			}
			pthread_mutex_unlock(&mem->monitor_mutex);
			pthread_mutex_lock(&mem->edge_servers[va.index].vcpu_mutex[va.i]);
			while(mem->edge_servers[va.index].flag[1]!=1){
				pthread_cond_wait(&mem->edge_servers[va.index].vcpu_work[va.i],&mem->edge_servers[va.index].vcpu_mutex[va.i]);
			}
			sprintf(out,"VCPU %d DE SERVER %d a trabalhar\n", va.i+1, va.index+1);
			logW(out);
			sleep(mem->edge_servers[va.index].tempos[1]);
			sprintf(out,"VCPU %d DE SERVER %d acabou o trabalho\n", va.i+1, va.index+1);
			logW(out);
			sem_wait(write_mutex);
			mem->edge_servers[va.index].tempos[1]=0;
			if(mem->edge_servers[va.index].em_man==0){
				mem->edge_servers[va.index].flag[va.i]=0;
			}else{
				mem->edge_servers[va.index].flag[va.i]=2;
			}
			sem_post(write_mutex);
			pthread_mutex_unlock(&mem->edge_servers[va.index].vcpu_mutex[va.i]);
		}
	}

    pthread_exit(NULL);
}




void edgeServer(char nome[], int capacidade1, int capacidade2, int index){
	mem->edge_servers[index].pid = getpid();
	signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    char output[20];
    strcpy(output, nome);
    strcat(output, " pronto\n");
    logW(output);
    mem->edge_servers[index].flag[0]=0;
    mem->edge_servers[index].flag[1]=0;
    mem->edge_servers[index].numero_executadas=0;
    mem->edge_servers[index].numero_manutencoes=0;
    mem->edge_servers[index].em_man=0; 

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
    
    mem->edge_servers[index].vcpu_mutex[0]= mutex1;
    mem->edge_servers[index].vcpu_mutex[1]= mutex2;
	mem->edge_servers[index].vcpu_work[0]= cond1;
    mem->edge_servers[index].vcpu_work[1]= cond2;
    
    
    tarefa t;
    //criar 2 threads
    pthread_t vCPUs[2];
    
    vcpu_args va1;
    va1.nome = nome;
    va1.index = index;
    va1.i=0;
    
    vcpu_args va2;
    va2.nome = nome;
    va2.index = index;
    va2.i=1;
    
    
	pthread_create(&vCPUs[0], NULL, vCPU, &va1);
    pthread_create(&vCPUs[1], NULL, vCPU, &va2);
    
    msg msg_rcv;
    msg msg_send;
    sem_wait(write_mutex);
    mem->edge_servers[index].done=0;
	sem_post(write_mutex);
    
    while(control==0){
    	if(msgrcv(mqid, &msg_rcv, sizeof(msg), index+1,IPC_NOWAIT)>0){
    		printf("leu aqui\n");
    		sem_wait(write_mutex);
		    mem->edge_servers[index].em_man=1;
			sem_post(write_mutex);
    		sem_wait(write_mutex);

			sem_post(write_mutex);
    		while(!(mem->edge_servers[index].flag[0]==2 && mem->edge_servers[index].flag[1]==2)){
    			if(mem->edge_servers[index].flag[0]==0){
    				mem->edge_servers[index].flag[0]=2;
    			}
    			if(mem->edge_servers[index].flag[1]==0){
    				mem->edge_servers[index].flag[1]=2;
    			}
    		}

    		msg_send.mytype=index+1+100;
    		msgsnd(mqid, &msg_send, sizeof(msg),0);
    		char out[100];
    		sprintf(out, "SERVER %d vai entrar em manutencao\n", index+1);
    		logW(out);
    		sleep(msg_rcv.sleeptime);
    		sem_wait(write_mutex);
    		mem->edge_servers[index].done=1;
    		sem_post(write_mutex);
    		msgrcv(mqid, &msg_rcv, sizeof(msg), index+1,0);
    		sem_wait(write_mutex);
    		mem->edge_servers[index].em_man=0;
    		mem->edge_servers[index].numero_manutencoes+=1;
    		sem_post(write_mutex);
    		sprintf(out, "SERVER %d vai sair de manutencao\n", index+1);
    		logW(out);	
    	
    	}
    		if(mem->edge_servers[index].em_man==0 && read(mem->edge_servers[index].fds[0], &t, sizeof(tarefa))>0){
    			char output[100];
    			sprintf(output, "TAREFA DE ID %d SELECIONADA PARA EXECUCAO NO SERVER %d\n", t.ID, index+1);
    			logW(output);
		    	if(mem->edge_servers[index].flag[0]==0){
		    		pthread_mutex_lock(&mem->edge_servers[index].vcpu_mutex[0]);
					sem_wait(write_mutex);
					mem->edge_servers[index].flag[0]=1;
					mem->edge_servers[index].tempos[0]= (float)t.instructionsNumber/capacidade1;
    		    	mem->edge_servers[index].numero_executadas+=1;
    				mem->total_tarefas_realizadas+=1;
    				sem_post(write_mutex);
    				pthread_cond_signal(&mem->edge_servers[index].vcpu_work[0]);
    				pthread_mutex_unlock(&mem->edge_servers[index].vcpu_mutex[0]);
    			}else if(mem->edge_servers[index].flag[1]==0 && mem->flag==1){
    				pthread_mutex_lock(&mem->edge_servers[index].vcpu_mutex[1]);
    		    	sem_wait(write_mutex);
    		    	mem->edge_servers[index].flag[1]=1;
    		    	mem->edge_servers[index].tempos[1]= (float)t.instructionsNumber/capacidade2;
    				mem->edge_servers[index].numero_executadas+=1;
    				mem->total_tarefas_realizadas+=1;
					sem_post(write_mutex);
					pthread_cond_signal(&mem->edge_servers[index].vcpu_work[1]);
					pthread_mutex_unlock(&mem->edge_servers[index].vcpu_mutex[1]);
    			}
    		}
    	}	
	
}



int isFree(time_t atual){

	for(int i = 0; i<mem->EDGE_SERVER_NUMBER; i++){
		if(mem->edge_servers[i].flag[0]==0 && (((float)mem->tarefas.raiz->t.instructionsNumber/mem->edge_servers[i].c1)<=(atual-mem->tarefas.raiz->t.maxtime))){
			return i;
		}else if(mem->edge_servers[i].flag[1]==0 && mem->flag==1 && ((mem->tarefas.raiz->t.instructionsNumber/mem->edge_servers[i].c2)<=(atual-mem->tarefas.raiz->t.maxtime))){
			return i;
		}
	}
	return -1;
}



bool areFrees(){

	for(int i = 0; i<mem->EDGE_SERVER_NUMBER; i++){
		if(mem->edge_servers[i].flag[0]==0 || (mem->edge_servers[i].flag[1]==0 && mem->flag==1)){
			sem_wait(write_mutex);
			mem->ativo=true;
			sem_post(write_mutex);
			return true;
			}
	}
	sem_wait(write_mutex);
	mem->ativo=false;
	sem_post(write_mutex);
	return false;
}



void *dispatcher_func(){
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
    logW("thread dispatcher criada\n");
    
    while(control==0){
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
        pthread_mutex_lock(&dispatcher_mutex);
    	while((mem->keep_going_dispatcher==0)){
    		pthread_cond_wait(&dispatcher_work, &dispatcher_mutex);    		
    	}
    	
		time_t agora = time(NULL);
        int i = isFree(agora);
    	struct no_fila *aux = mem->tarefas.raiz;
		char tolog[500];
    	
    	if(aux->t.maxtime -(float)(agora-aux->t.tempo)>=0 && i!=-1){
    		sem_wait(write_mutex);
    		aux->t.tempo_executada+=(float)(agora - aux->t.tempo);
    		mem->tempo_medio+=aux->t.tempo_executada;
    		write(mem->edge_servers[i].fds[1], &aux->t, sizeof(tarefa));
			retirar(&mem->tarefas, aux->t);
			sem_post(write_mutex);
			sprintf(tolog,"Dispatcher: A tarefa de ID=%d foi enviada pelo unnamed pipe\n", aux->t.ID);
			logW(tolog);
			
    	}else{
			sprintf(tolog,"Dispatcher: A tarefa de ID=%d foi removida devido a exceder o tempo de execução\n", aux->t.ID);
			sem_wait(write_mutex);
			mem->total_tarefas_nao_realizadas+=1;
    		retirar(&mem->tarefas, aux->t);
    		sem_post(write_mutex);
    		logW(tolog);
    	}
    	sem_wait(write_mutex);
    	mem->keep_going_dispatcher=0;
    	sem_post(write_mutex);
    	pthread_cond_signal(&dispatcher_free);
    	pthread_mutex_unlock(&dispatcher_mutex);
    }
}



void *sheduler_func(){
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

    logW("Scheduler: thread scheduler criada\n");
    
    while(control==0)
    {	signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
    	pthread_mutex_lock(&scheduler_mutex);
    	while(mem->keep_going==0){
    		pthread_cond_wait(&scheduler_work, &scheduler_mutex);
    	}
		time_t agora = time(NULL);
		for (struct no_fila *aux = mem->tarefas.raiz; aux!=NULL; aux = aux->pseg) {
			sem_wait(write_mutex);
			aux->t.maxtime = aux->t.maxtime-((float)(agora - aux->t.tempo));
			aux->t.tempo_executada+=(float)(agora - aux->t.tempo);
			aux->t.tempo=agora;
			sem_post(write_mutex);
			if(aux->t.maxtime < 0){
				char tolog[500];
				sprintf(tolog,"Scheduler: A tarefa de ID=%d foi removida devido a exceder o tempo de execução\n", aux->t.ID);
				sem_wait(write_mutex);
				mem->total_tarefas_nao_realizadas+=1;
				retirar(&mem->tarefas, aux->t);
				sem_post(write_mutex);
				logW(tolog);
			}

    	}
    	sem_wait(write_mutex);
		ordenar(&mem->tarefas);
		mem->keep_going=0;
		sem_post(write_mutex);
		pthread_cond_signal(&scheduler_end);
		pthread_mutex_unlock(&scheduler_mutex);
    }
}



void taskmanager(){
	task_manager=getpid();
    logW("Task Manager: Processo taskmanager criado\n");
    //recebe as tarefas e adiciona à struct tarefas
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, handler_sigterm_tm);
	signal(SIGTSTP, SIG_IGN);
    //criar os processos edge servres
    for (int i = 0; i < mem->EDGE_SERVER_NUMBER; ++i) {
    	sem_wait(write_mutex);
    	pipe(mem->edge_servers[i].fds);
    	sem_post(write_mutex);
        if((fork())==0){
            edgeServer(mem->edge_servers[i].nome, mem->edge_servers[i].c1, mem->edge_servers[i].c2, i);
            exit(0);
        }
        sleep(1);
    }
    // abre o pipe em modo leitura
    logW("Task Manager: Going to open the pipe!\n");
    int fd;
    if ((fd=open(PIPE_NAME, O_RDONLY/*|O_NONBLOCK*/)) < 0)
    {
        logW("Task Manager: Cannot open pipe for reading: \n");
        exit(0);
    }
    logW("Task Manager: Pipe is open!\n");
       
    //criar o dispatcher
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, dispatcher_func, NULL);
    
    //criar o scheduler
    pthread_t scheduler;
    pthread_create(&scheduler, NULL, sheduler_func, NULL);

    
	char mob[100];
	tarefa t;
	int num=1;
    while(control==0){ 
    	char mob[100];
		if(mem->tarefas.tamanho>0 && areFrees()){
		    pthread_mutex_lock(&dispatcher_mutex);
        	sem_wait(write_mutex);
            mem->keep_going_dispatcher=1;
            sem_post(write_mutex);
        	pthread_cond_signal(&dispatcher_work);
        	pthread_cond_wait(&dispatcher_free, &dispatcher_mutex);
        	pthread_mutex_unlock(&dispatcher_mutex);
        }
		if((mem->tarefas.tamanho > 0.7*mem->QUEUE_POS && mem->keep_going_monitor==0 && mem->flag==0)
		|| (mem->tarefas.tamanho < 0.3*mem->QUEUE_POS && mem->keep_going_monitor==0 && mem->flag==1)){
                pthread_mutex_lock(&mem->monitor_mutex);
                mem->keep_going_monitor=1;
				pthread_cond_signal(&mem->monitor_go);
				pthread_mutex_unlock(&mem->monitor_mutex);
			}
			
        if(mem->tarefas.tamanho>0 && mem->keep_going_manutencao==0){
           		pthread_mutex_lock(&mem->manutencao_mutex);
                mem->keep_going_manutencao=1;
				pthread_cond_signal(&mem->manutencao_go);
				pthread_mutex_unlock(&mem->manutencao_mutex);
			}
    	if(read(fd, &mob, sizeof(mob))>0) {
    	    sem_wait(write_mutex);
            mem->keep_going_dispatcher=0;
            sem_post(write_mutex);
    	    pthread_mutex_lock(&scheduler_mutex);
    	    if(mem->tarefas.tamanho >= mem->QUEUE_POS){
    	    	logW("Fila cheia, a tarefa vai ser eliminada\n");
    	    }else{
				if(strcmp(mob, "EXIT \n")==0){
					kill(sys, SIGINT);
				}else if(strcmp(mob, "STATS\n")==0){
					kill(sys, SIGTSTP);
				}else{
					//printf("receiving\n");
    	        	t.ID=num;
    	    		t.tempo=time(NULL);
    	    		t.tempo_executada=0;
    	    		num++;
    	    		char *token = strtok(mob, ",");
					t.instructionsNumber=atoi(token);
					token=strtok(NULL, ",");
					t.maxtime=atof(token);
            		sem_wait(write_mutex);
					colocar(&mem->tarefas, t);
            		mem->keep_going=1;
            		sem_post(write_mutex);
            		pthread_cond_signal(&scheduler_work);   
            		pthread_cond_wait(&scheduler_end, &scheduler_mutex);
            	}
            }
                pthread_mutex_unlock(&scheduler_mutex);
        }    
    }
}



void monitor(){
	monitorr=getpid();
    signal(SIGINT, SIG_IGN);
	signal(SIGTERM, handler_sigterm_m);
	signal(SIGTSTP, SIG_IGN);

    logW("Monitor: Processo monitor criado\n");
    sem_wait(write_mutex);
    mem->flag=0;
    sem_post(write_mutex);
    while(control==0){
    	pthread_mutex_lock(&mem->monitor_mutex);
   		while(mem->keep_going_monitor==0){
   			pthread_cond_wait(&mem->monitor_go, &mem->monitor_mutex);
   		}
   		pthread_mutex_unlock(&mem->monitor_mutex);
    	if(mem->flag==0 && (mem->tarefas.tamanho > 0.8*mem->QUEUE_POS) /*|| mem->tempoespera > mem->MAX_WAIT)*/){
    		logW("Monitor: Modo performance ativado\n");
    		sem_wait(write_mutex);
    		mem->flag=1;
    		mem->keep_going_monitor=0;
			sem_post(write_mutex);
			pthread_cond_broadcast(&mem->vcpu_go);
    	}if(mem->flag==1 && mem->tarefas.tamanho<=0.2*mem->QUEUE_POS){
    	    logW("Monitor: Modo normal ativado\n");
    	    sem_wait(write_mutex);
    		mem->flag=0;
    		mem->keep_going_monitor=0;
    		sem_post(write_mutex);
    	}if(mem->tarefas.tamanho==0){
    	    sem_wait(write_mutex);
       		mem->keep_going_monitor=0;
    		sem_post(write_mutex);
    	}
    }
}


void maintenanceManager(){
	maintenance_manager=getpid();
	signal(SIGTERM, handler_sigterm_mm);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
    logW("Maintenance Manager: Processo maintenanceManager criado\n");
    int em_man=0;
    msg msg_send;
    msg msg_rcv;
    while(control==0){
    	pthread_mutex_lock(&mem->manutencao_mutex);
        while(mem->keep_going_manutencao==0){
   			pthread_cond_wait(&mem->manutencao_go, &mem->manutencao_mutex);
   		}
   		pthread_mutex_unlock(&mem->manutencao_mutex);
    	int lower = 1, upper = 5;
    	srand(time(0));
    	int num = 1+rand()%upper+1;
    	int sleeptime = 1+rand()%upper+1;
		long server = 1+rand()%3;

		if(em_man<mem->EDGE_SERVER_NUMBER-1 && mem->edge_servers[server-1].em_man==0 && mem->tarefas.tamanho>0){
			//printf("server=%ld\n", server);
			msg_send.mytype=server;
			msg_send.sleeptime=sleeptime;
			em_man++;
			
			if(msgsnd(mqid, &msg_send, sizeof(msg),0)<0){
				printf("erro a enviar msg\n");
			}
			//printf("enviou\n");
			msgrcv(mqid, &msg_rcv, sizeof(msg),server+100, 0);
			//printf("leu aqui\n");
			sleep(num);
		}
		if(em_man>0){
			for(int i = 0; i < mem->EDGE_SERVER_NUMBER; i++){
				if(mem->edge_servers[i].done==1 && mem->edge_servers[i].em_man==1){
					sem_wait(write_mutex);
    				mem->edge_servers[i].flag[0]=0;
    				mem->edge_servers[i].flag[1]=0;
    				mem->edge_servers[i].done=0;
    				sem_post(write_mutex);
    				msg_send.mytype=i+1;
    				em_man--;
    				msgsnd(mqid, &msg_send, sizeof(msg),0);
				}
			}
		
		}
		    sem_wait(write_mutex);
		    if(mem->tarefas.tamanho==0){
    			mem->keep_going_manutencao=0;
    			}
    		sem_post(write_mutex);
    }
}


void logW(char s[]){
    sem_wait(log_mutex);
    printf("%s", s);
    FILE *f;
    f=fopen("log.txt","a+");
    time(&horas);
    struct tm *tempo = localtime(&horas);
    fprintf(f,"%d:%d:%d %s",tempo->tm_hour, tempo->tm_min, tempo->tm_sec, s);
    fclose(f);
    sem_post(log_mutex);
}

void inicia(){
    remove("log.txt");
    remove("TASK_PIPE");
    sem_unlink("LOG_MUTEX");
    sem_unlink("WRITE_MUTEX");  
    log_mutex = sem_open("LOG_MUTEX",O_CREAT|O_EXCL,0700,1);
    write_mutex = sem_open("WRITE_MUTEX",O_CREAT|O_EXCL,0700,1);
}

void termina(){
	logW("OFFLOAD SIMULATOR CLOSING\n");
    sem_close(log_mutex);
    sem_unlink("LOG_MUTEX");
    sem_close(write_mutex);
    sem_unlink("WRITE_MUTEX");
    //printf("done\n");
    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&scheduler_mutex);
    pthread_mutex_destroy(&dispatcher_mutex);
    pthread_mutex_destroy(&vcpu_mutex);
    pthread_cond_destroy(&scheduler_work);
    pthread_cond_destroy(&scheduler_end);
    pthread_cond_destroy(&dispatcher_work);
    pthread_cond_destroy(&dispatcher_free);
    pthread_cond_destroy(&vcpu_free);
    pthread_cond_destroy(&go_vcpu);
    unlink(PIPE_NAME);
    remove("TASK_PIPE");
    shmdt(mem);
	shmctl(shmid,IPC_RMID,NULL);
    msgctl(mqid, IPC_RMID,0);
    //printf("done2\n");
}
