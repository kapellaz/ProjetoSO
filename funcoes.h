//Rui Santos - 2020225542
//Tomás Dias - 2020215701

#ifndef SYSTEMMANAGER_C_AUX_H
#define SYSTEMMANAGER_C_AUX_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <ctype.h>

//#define DEBUG

#define PIPE_NAME  "TASK_PIPE"


typedef struct{
	long mytype;
	int sleeptime;
	
}msg;


typedef struct{
	char *nome;
	int index;	
	int i;
}vcpu_args;



typedef struct{
	time_t tempo_executada;
	time_t tempo;
    int ID;
    int instructionsNumber;
    float maxtime;
} tarefa;

struct no_fila{
    tarefa t;
    struct no_fila *pseg;
};

struct tarefas{
    int tamanho;
    struct no_fila *raiz;
};


typedef struct{
    char nome[20];
    int c1;
    int c2;
    int numero_manutencoes;
    int numero_executadas;
    int flag[2];
    int fds[2];
   	float tempos[2];
	int em_man;
	int done;
	pid_t pid;
	pthread_t vCPUs[2];
    pthread_mutex_t vcpu_mutex[2];
	pthread_cond_t vcpu_work[2];
}edge_Servers;


typedef struct{
    int QUEUE_POS, MAX_WAIT,EDGE_SERVER_NUMBER;
    int flag;
    int total_tarefas_realizadas;
    int total_tarefas_nao_realizadas;
    float tempo_medio;
    float tempoespera;
    struct tarefas tarefas;
    int keep_going;
    int keep_going_dispatcher;
    int keep_going_monitor;
    int keep_going_manutencao;
    bool ativo;
    int stop;
    pthread_mutex_t manutencao_mutex;
	pthread_cond_t manutencao_go;
    pthread_mutex_t monitor_mutex;
	pthread_cond_t monitor_go;
	pthread_cond_t vcpu_go;
    edge_Servers edge_servers[];
}memoria;

sem_t *log_mutex;
sem_t *write_mutex;


memoria *mem;
time_t horas;


sigset_t set;

pid_t sys;
pid_t task_manager;
pid_t maintenance_manager;
pid_t monitorr;

pid_t *pids;

int shmid;
int mqid;
key_t key;

pthread_t scheduler;
pthread_t dispatcher;

void handler_estatistica(int num);
void handler_sigint(int num);
void ler_file(char nome[]);
void logW(char s[1000]);
void maintenanceManager();
void taskmanager();
void monitor();
void systemManager(char fileConfig[]);
void inicia();
void termina();

#endif //SYSTEMMANAGER_C_AUX_H
