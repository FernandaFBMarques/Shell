#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_LINE 80

void process_seq(char **split_commands, int flag_dash, char *index_dash[], int flag_tee, int flag_write, char *index_tee_w[2],
    int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]);

void process_par(char **split_commands, int flag_dash, char *index_dash[], int flag_tee, int flag_write, char *index_tee_w[2],
    int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]);

void interactive(char *input);
void pipe_p(char *index_dash[]);
void* foo(void* combined_ptr);
void tee( int flag_write, char *index_tee_w[2], int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]);

int flag_par = 0; //0 se for seq e 1 se for par
int flag_dash = 0; //0 ser não tiver e 1 se tiver
int flag_write = 0; //0 se não tiver e 1 se tiver 
int flag_write_end = 0; //0 se não tiver e 1 se tiver
int flag_read = 0; //0 se não tiver e 1 se tiver
int flag_tee = 0;
int flag_history = 0;
int should_exit = 0; // Variável para verificar se o programa deve ser encerrado
int flag_background = 0;
char *last_command = NULL;
FILE *fp;

int main(int argc, char *argv[]) {
    int should_run = 1;
    char *input = (char*)malloc(MAX_LINE * sizeof(char));
    FILE *input_stream = stdin; // Define o stream de entrada padrão para stdin

    // Se um argumento de arquivo foi fornecido, abra o arquivo e ajuste o stream de entrada
    if(argc > 1) {
        fp = fopen(argv[1], "r");
        if(fp == NULL) {
            perror("Error in the file reading");
            
        }
        input_stream = fp; // Ajusta o stream de entrada para o arquivo
    }

    while (should_run) {
        if (input_stream == stdin) { // Só imprime o prompt se estiver aguardando entrada do usuário
            if(flag_par == 0){
                printf("mffbm seq>");
            } else if(flag_par == 1){
                printf("mffbm par>");
            }
            fflush(stdout); // vai fazer com que o prompt seja impresso imediatamente
        }

        // Usa o stream de entrada atual para ler o input
        if (fgets(input, MAX_LINE, input_stream) == NULL) {
            if (input_stream != stdin) { // Se estiver lendo de um arquivo e chegou ao fim
                fclose(fp);
                input_stream = stdin; // muda para o stdin
                continue; // Continua e vai para ler do stdin
            } else {
                // Se estiver lendo do stdin e fgets retorna NULL (por exemplo, EOF) ai ele sai
                should_exit = 1;
            }
        }
        interactive(input);

        // Verifica se deve encerrar após processar os comandos
        if (should_exit) {
            if (input_stream != stdin) {
                fclose(fp);
            }
            free(input);
            exit(0);
        }
    }
    free(input);
    return 0;
}

void interactive(char *input) {

    if (strncmp(input, "!!\n", 3) == 0) {
        if (last_command) {
            strcpy(input, last_command);
        } else {
            printf("No commands in history.\n");
            return;
        }
    }
    if (strncmp(input, "!!\n", 3) == 0) {
        if (last_command) {
            strcpy(input, last_command);
        } else {
            printf("No commands in history.\n");
            return;
        }
    }

    if (flag_history == 1) {
        free(last_command);
        last_command = NULL;
    }
    last_command = strdup(input);
    
    input[strlen(input) - 1] = '\0';//substitui o /n por /0
    char **split_commands = (char**)malloc(MAX_LINE * sizeof(char*));
    char *index_dash[2];
    char *index_tee_w[2];
    char *index_tee_we[2];
    char *index_tee_r[2];
    int cmd_count = 0;
    split_commands[cmd_count] = strtok(input, ";");
    int found_exit_command = 0; //flag exit


    while (split_commands[cmd_count] != NULL) {

        if (!strcasecmp(split_commands[cmd_count], "!!")) {
            flag_history = 1;
            if (last_command) {
                split_commands[cmd_count] = last_command;
                split_commands[cmd_count] = strtok(input, "!!");
            } else {
                perror("No command");
            }
        }
        while(isspace((unsigned char)*split_commands[cmd_count])) {
            split_commands[cmd_count]++;
        }

        if (!strcasecmp(split_commands[cmd_count], "exit")) {
            found_exit_command = 1;


        }else if (!strcasecmp(split_commands[cmd_count], "|")) {
            flag_dash = 1;
            split_commands[cmd_count] = strtok(input, "|");
            index_dash[0] = split_commands[cmd_count];
            index_dash[1] = split_commands[cmd_count + 1];

        }else if (!strcasecmp(split_commands[cmd_count], ">")) {//escrita
            flag_write = 1;
            flag_tee = 1;
            split_commands[cmd_count] = strtok(input, ">");
            index_tee_w[0] = split_commands[cmd_count];
            index_tee_w[1] = split_commands[cmd_count + 1];

        }else if (!strcasecmp(split_commands[cmd_count], ">>")) {//escrita no final
            flag_write_end = 1;
            flag_tee = 1;
            split_commands[cmd_count] = strtok(input, ">>");
            index_tee_we[0] = split_commands[cmd_count];
            index_tee_we[1] = split_commands[cmd_count + 1];

        }else if (!strcasecmp(split_commands[cmd_count], "<")) {//leitura
            flag_read = 1;
            flag_tee = 1;
            split_commands[cmd_count] = strtok(input, "<");
            index_tee_r[0] = split_commands[cmd_count];
            index_tee_r[1] = split_commands[cmd_count + 1];

        }else if (!strcasecmp(split_commands[cmd_count], "&")) {//leitura
            flag_background = 1;

        }else if (!strcasecmp(split_commands[cmd_count], "style parallel")) {
            split_commands[cmd_count] = NULL;
            if (flag_par == 0) {
                process_seq(split_commands, flag_dash, index_dash, flag_tee, flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
            }
            flag_par = 1;

            for (int i = 0; i <= cmd_count; i++) {
                split_commands[i] = NULL;
            }
            cmd_count = -1;

        } else if (!strcasecmp(split_commands[cmd_count], "style sequential")) {
            split_commands[cmd_count] = NULL;
            if (flag_par == 1) { 
                
                process_par(split_commands, flag_dash, index_dash, flag_tee, flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
            }
            flag_par = 0;  // muda para sequential

            for (int i = 0; i <= cmd_count; i++) {
                split_commands[i] = NULL;
            }
            cmd_count = -1;
        }

        cmd_count++;
        split_commands[cmd_count] = strtok(NULL, ";");
    }

    if (!should_exit) {
        if (flag_par == 1) {
            
            process_par(split_commands, flag_dash, index_dash, flag_tee, flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
        } else {
            
            process_seq(split_commands, flag_dash, index_dash, flag_tee, flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
        }
    }
    if (found_exit_command) {
        should_exit = 1;
    }
    
    flag_dash = 0;
    flag_write = 0; 
    flag_write_end = 0;
    flag_read = 0;
    flag_tee = 0;
    flag_history = 0;
    free(split_commands);
}

void process_seq(char **split_commands, int flag_dash, char *index_dash[], int flag_tee, int flag_write, char *index_tee_w[2],
    int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]) {

    if(flag_dash == 1){
        pipe_p(index_dash);
    }
    if(flag_tee == 1){
        tee(flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
    }
    pid_t pid, pid1;
    pid = fork();

    if (pid == 0) { //child
        char *aux = (char*)malloc(MAX_LINE * sizeof(char) * MAX_LINE);

        int i = 0;
        while (split_commands[i] != NULL) {
            strcat(aux, split_commands[i]);
            i++;
            if (split_commands[i] != NULL) 
                strcat(aux, " && ");
        }

        pid1 = getpid();
        execlp("/bin/sh", "/bin/sh", "-c", aux, (char *)NULL);//chama o exec
        perror("Exec failed");
        
    } else if (pid < 0) {
        perror("Fork failed");
    } else {//parent
        pid1 = getpid();
        //printf("parent: pid1 = %d\n", pid1);
        wait(NULL);
    }
}

void* foo(void* combined_ptr) {
    char* combined = (char*)combined_ptr;//converte o *void de volta para um *char
    system(combined); // Chama o comando
    return NULL;
}

void process_par(char **split_commands, int flag_dash, char *index_dash[], int flag_tee, int flag_write, char *index_tee_w[2],
    int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]) {

    if(flag_dash == 1){
        pipe_p(index_dash);
    }
    if(flag_tee == 1){
        tee(flag_write, index_tee_w, flag_write_end, index_tee_we, flag_read, index_tee_r);
    }

    int i = 0;
    pthread_t threads[MAX_LINE];

    // Alocação de espaço para armazenar ponteiros para comandos duplicados.
    char *duplicated_cmds[MAX_LINE] = {NULL};

    while (split_commands[i] != NULL) {
        // String do comando duplicada para garantir que a thread tenha uma cópia independente.
        duplicated_cmds[i] = strdup(split_commands[i]);
        if (!duplicated_cmds[i]) {
            perror("Failed to duplicate string");
            
        }

        // Cria uma thread para cada comando
        if(pthread_create(&threads[i], NULL, foo, (void*)duplicated_cmds[i]) != 0) {
            perror("Failed to create thread");
        }
        i++;
    }

    // espero todas as threads finalizarem
    for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
        free(duplicated_cmds[j]); 
    }
}

void pipe_p(char *index_dash[]){
    int pipefd[2];
    
    //output de pipefd1 se torna o output de pipefd0
    pid_t childpid;

    pipe(pipefd);
    
    if ((childpid = fork()) == -1) { // estabelece a pipeline e agoa faz o fork no novo processo filho
        perror("fork");
    }
    
    if (childpid == 0) {  /* Processo filho */
        close(pipefd[1]);  // Fecha o lado de escrita do pipe no filho

        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);  

        system(index_dash[0]);

    } else {  /* Processo pai */
        close(pipefd[0]);  // Fecha o lado de leitura do pipe no pai

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);  

        system(index_dash[1]);
    }
}

void tee( int flag_write, char *index_tee_w[2], int flag_write_end, char *index_tee_we[2],int flag_read, char *index_tee_r[2]){
    FILE *input = stdin, *output = stdout, *file = NULL;

    // Abra o arquivo em modo de escrita (crie-o se não existir)
    if(flag_write == 1){
        file = fopen(index_tee_w[1], "w");

    }else if(flag_write_end == 1){
        file = fopen(index_tee_we[1], "a");

    }else if(flag_read == 1){
        file = fopen(index_tee_r[1], "r");

    }
    if (file == NULL) {
        perror("Error while opening the file");
        
    }
    // Redireciona a saída padrão para o arquivo
    if (dup2(fileno(file), STDOUT_FILENO) == -1) {
        perror("Error in dup2");
        
    }

    fclose(file);
}
