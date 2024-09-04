/*
    Student name: //
    Student No.: //
    Development platform: Ubuntu(Virtual machine), Ubuntu(workbench2)
    Completion :
        - Process creation and execution
        - Use of pipes
        - Print process’s running statistics (using /proc)
        - Use of signals (SIGINT,SIGUSR1)
        - Built-in command: exit
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

int ignore=0;//ignore flag
int execute=0;//execute command flag

void siginthandler(int signum){//SIGINT detection for ignore flag
    printf("\n");
    ignore=1;
}

void sigusrhandler(int signum){//SIGUSR1 detection for executing command flag
    execute=1;
}

int main(){
    struct sigaction sint,susr;
    sigaction(SIGINT, NULL, &sint); //SIGINT handler
    sint.sa_handler=siginthandler;
    sigaction(SIGINT, &sint, NULL);
    sigaction(SIGUSR1, NULL, &susr); //SIGUSR1 handler
    susr.sa_handler=sigusrhandler;
    sigaction(SIGUSR1, &susr, NULL);

    while (1){//executes until break; by exit
        
        char *a[5][30]; //command input array
        char *s1[5]; //string storage for each command
        int j=0; //counts number of command
    
        ignore=0;
        int ppid=(int)getpid();
        printf("## JCShell [%d] ## ", (int)getpid());

        char *input=calloc(1024,sizeof(char));
        fflush(stdout);//flush stdout before receiving input
        
        if (read(0,input,1024*sizeof(char))<0) {ignore=1;}
        
        int pipecnt=0;

        if (input[0]=='\n' || input==NULL){ignore=1;}//check empty input

        if(!ignore){//protection against empty inputs
            /***************************************************/
            /*splits string into commands, check illegal inputs*/
            /***************************************************/
            int i=0;
            while(input[i]!='\n'){//count number of pipes
                if (input[i]=='|') pipecnt++;
                i++;
            }
            if (input[0]=='|' || input[i-1]=='|'){//ignores when pipe is at start/end
                ignore=1;
                printf("JCShell: syntax error near unexpected token `|'\n");
            }
            char *str=strtok(input, "\n"); //divide commands by "|" using token
            str=strtok(str,"|");  //collect full command into s1
            
            while (str){
                s1[j]=str;                
                str=strtok(NULL,"|");
                j++;
            } 

            for (int l=0;l<j;l++){ //divide each char by " " in command using token
                a[l][0]=(char*) NULL;//and store into array a
                char *fn=strtok(s1[l]," ");
                a[l][0]=(char*) fn;

                if ((a[l][0]==NULL || pipecnt!=j-1 ) && pipecnt==1 && ignore!=1){
                    //ignores if one of the commands is empty(contains space only) or pipe count!=command count-1 (1 pipe)
                    printf("JCshell: syntax error near unexpected token `|'\n");
                    ignore=1;
                }
                if ((a[l][0]==NULL || pipecnt!=j-1 ) && pipecnt>1 && ignore!=1){
                    //ignores if one of the commands is empty(contains space only) or pipe count!=command count-1 (>1 pipe)
                    printf("JCshell: should not have two | symbols without in-between command\n");
                    ignore=1;
                }

                int i=1;
                while (fn){//store into array a
                    fn=strtok(NULL, " ");            
                    a[l][i]=(char*) fn;
                    i++;
                }
                a[l][i]=(char*) NULL;//adds trailing NULL to vector a[l] as a requirement of execvp
            }
        }


        if (ignore==0){//ignore SIGINT or problematic input
                        //else execute child and parent logic

            if (!strcmp(a[0][0],"exit")){//exit with other args
                if (a[0][1]!=NULL){
                printf("JCshell: \"exit\" with other arguments!!!\n");}
                else {//exit properly
                    break;
                }
            }
            else {
                int pfd[j][2]; //n-1 pipe construction for n processes
                for (int l=0;l<j-1;l++){
                    pipe(pfd[l]);
                }

                int l=0;
                pid_t child[j];
                while (l<j){//create (command count) no of child processes by fork()                   
                    child[l]=fork();

                    if (child[l]==0){
                        /***************/
                        /*child process*/
                        /***************/
                        if (j>1){//pipe is used
                            for (int k=0;k<j-1;k++){//close all pipe ends except 1R1W (see below)
                                for (int m=1;m>=0;m--){
                                    if (!(k==l && l!=(j-1) && m==1) && !(k==(l-1) && l!=0 && m==0)){
                                        close(pfd[k][m]);
                                        }
                                }
                            }
                            if (l!=0) {dup2(pfd[l-1][0],STDIN_FILENO);}//pipe[child no-1] read end except first child
                            if (l!=(j-1)) {dup2(pfd[l][1],STDOUT_FILENO);}//pipe[child no] write end except last child
                        }

                        while(!execute){//wait for execute flag set by SIGUSR1 handler
                            usleep(100);
                        }

                        if (execvp(a[l][0], a[l])==-1){//command execution
                            //prints error through stderr if returns -1
                            fprintf(stderr,"JCshell: '%s': %s\n",a[l][0] ,strerror(errno));
                            exit(errno);
                        } 

                        exit(0);                        
                    }    
                    l++;
                } 

                if ((int)getpid()==ppid){
                    /****************/
                    /*parent process*/
                    /****************/
                    int status[j];
                     
                    for (int k=0;k<j-1;k++){//close all pipes
                        for (int m=0;m<2;m++){
                            close(pfd[k][m]);
                        }
                    }

                    char temp[j][50];//data storage for running statistics
                    int pid[j], exit_code[j], exit_sig[j], ppid[j], z;
                    char cmd[j][50];
                    char state[j], z2;
                    unsigned long utime[j], stime[j], z1;
                    float u1[j],s1[j];
                    int vctx[j], nvctx[j];
                    int m;
                    FILE *  file[2][j];
                        
                    usleep(10);
                    for (m=0;m<j;m++){//sends SIGUSR1 to all child
                        kill(child[m],SIGUSR1);                     
                    }
                        
                    int alldone=0;//saves sequence of completion
                    int indivdone[j];
                    int seq[j];
                    for (m=0;m<j;m++){
                        indivdone[m]=0;
                        seq[m]=-1;
                    }

                    siginfo_t siginfo;
                    while (alldone!=j){//keeps checking if each process is completed
                        for (m=0;m<j;m++){
                            pid_t chdpid=waitid(P_PID, child[m], &siginfo,  WNOHANG | WEXITED | WNOWAIT);
                            //does not block process
                            if (siginfo.si_pid==child[m] && indivdone[m]==0){
                                //saves only if si_pid equals to child pid (0 if process not completed)
                                seq[alldone]=m;
                                indivdone[m]=1;
                                alldone++;
                            }
                        }
                        if (alldone!=j) usleep(10000);//check process completion every 10ms until all terminated
                    }

                        
                    for (int k=0;k<j;k++){//print data according to return sequence
                        int m=seq[k];
                        int err=0;
                        sprintf(temp[m], "/proc/%d/stat", child[m]);//extract all data from /proc/pid/stat
                            file[0][m] = fopen(temp[m], "r");//except vctx, nvctx
                            if (file[0][m] == NULL) {
                                printf("Error in open my proc file %da\n",m);
                                }
                                
                        fscanf(file[0][m], "%d (%[^)]s",&pid[m] , cmd[m]);
                        fscanf(file[0][m], ") %c %d %d %d %d %d %u %lu %lu %lu %lu %f %f", 
                        &state[m], &ppid[m], &z, &z, &z, &z,
                            (unsigned *)&z, &z1, &z1, &z1, &z1, &u1[m], &s1[m]);
                            fclose(file[0][m]);
                        

                        char *line=NULL;//extract all data from /proc/pid/status
                        size_t len;
                        ssize_t read;
                        sprintf(temp[m], "/proc/%d/status", child[m]);
                            file[1][m] = fopen(temp[m], "r");
                            if (file[1][m] == NULL) {
                                printf("Error in open my proc file %db\n",m);
                                } 

                        while ((read=getline(&line, &len, file[1][m]))!=1){
                                //extract vctx, nvctx from /proc/pid/status with corresponding names
                                line=strtok(line,"\t");                                   
                            if (!strcmp(line,"voluntary_ctxt_switches:")){
                                line=strtok(NULL," ");
                                sscanf(line, "%d" ,&vctx[m]);
                                }
                            if (!strcmp(line,"nonvoluntary_ctxt_switches:")){
                                line=strtok(NULL," ");
                                sscanf(line, "%d" ,&nvctx[m]);
                                break;
                            }
                        }
                        fclose(file[1][m]);

                        waitpid(child[m], &status[m], WNOHANG);//clears zombie process
                        if (WIFSIGNALED(status[m])){//abnormal termination
                            printf("(PID)%d (CMD)%s (STATE)%c (EXSIG)%s (PPID)%d (USER)%.2f (SYS)%.2f (VCTX)%d (NVCTX)%d\n", 
                                pid[m], cmd[m], state[m], strsignal(WTERMSIG(status[m])), ppid[m], u1[m]/sysconf(_SC_CLK_TCK), s1[m]/sysconf(_SC_CLK_TCK), vctx[m], nvctx[m]);
                        }
                        else {//normal termination
                            printf("(PID)%d (CMD)%s (STATE)%c (EXCODE)%d (PPID)%d (USER)%.2f (SYS)%.2f (VCTX)%d (NVCTX)%d\n", 
                                pid[m], cmd[m], state[m], (int)WEXITSTATUS(status[m]), ppid[m], u1[m]/sysconf(_SC_CLK_TCK), s1[m]/sysconf(_SC_CLK_TCK), vctx[m], nvctx[m]);
                        }
                        
                    }
                }
                    
  

            }
        }
    }
    return 0;
}