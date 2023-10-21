/*******************************************************************************
                                  My Shell 2.0
 *******************************************************************************
    一个非常非常简陋的shell……

*******************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>

#define TOK_DELIM " \t\r\n"
#define TOK_BUFFER_SIZE 64
#define ECHO_OMIT '"'
#define MAX 1024

     //存储分割后的参数
// char **pipe_input;
// char **pipe_output;
 int pipe_flag = 0;
// int fd[2];
int pipe_fd[2];
int wc_pipe = 0;
FILE *fpipe;





void handler(int sig)
{
    wait(NULL);

}

char *usage[] = 
{
    "change directory\n",
    "looking for commands\n",
    "exit my shell\n",
    "identify commands\n",
    "repeat input\n",
    "output files' content\n",
    "search if the words you input is in the file\n",
    "edit the time of files\n",
    "statistical tool\n"
};

char *builtin_cmd[] =     
{
    "cd",
    "help",
    "exit",
    "which",
    "echo",         //-e-n
    "cat",          //-E
    "grep",
    "touch",
    "wc"            //-l -c -m -w
};

int cd(char **args);
int help(char **args);
int mysh_exit(char **args);
int which(char **args);
int echo(char **args);
char **split_line(char *line);
int execute(char **args);
void printfcmd(char *args[],int i);
void printfcmd_e(char *args[],int i);
int cat(char **args);
int grep(char **args);
int touch(char **args);
int wc(char **args);
int command_nums();
int popen_system(const char * cmd,char *pRetMsg, int msg_len);
void popen_read(const char* cmd);
void popen_write(const char * cmd);
void savehistory(char *command);


int (*builtin_func[])(char**) =      //函数指针
{
    cd,
    help,
    mysh_exit,
    which,
    echo,
    cat,
    grep,
    touch,
    wc
};

/*******************************************************************************
                            built-in commands
*******************************************************************************/

int cd(char **args)
{
    if(args[1] == NULL)
    {
        perror("\033[31mMyshell error at cd, lack of args\n\033[0m");   //只有cd的情况
    }
    else
    {
        if(chdir(args[1]) != 0)
        {
            perror("\033[31mMyshell error at chdir\n\033[0m");   //切换路径
        }
    }

    return 1;
}

int help(char **args)
{
    int book = 0;

    if(args[1] == NULL)
    {
        for(int i = 0; i < command_nums(); i++)
         {              
            printf("%s: ",builtin_cmd[i]);
            printf("%s",usage[i]);
        }
    }
   
    else
    {
        for(int i = 0; i < command_nums(); i++)
        {
            if(strcmp(args[1],builtin_cmd[i]) == 0)
            {
                printf("%s: ",builtin_cmd[i]);
                printf("%s",usage[i]);

                book++;
            }

        }

        if(book = 0)
        {
            printf("\033[31mNo such command!\n\033[0m");
        }
    }
    return 1;
}

int mysh_exit(char **args)
{
    //printf("exit shell\n");
    exit(0);
    return 0;
}

int count_cmd(char **args)
{
    int count = 0;
    int i = 0;
    if(args == NULL)
    {
        return 0;
    }

    while(args[i] != NULL)
    {
        count++;
        i++;
    }

    return count;
}

void do_fork(char **token)
{
    //printf("%s\n",token[1]);
    int status_ = 1;
    int status = 1;
    char **args;

    char buff[MAX];
    int buffer_size = TOK_BUFFER_SIZE;
    long len = 0;
    char **n_args = malloc(buffer_size*sizeof(char*));

    signal(SIGCHLD,handler); 

    if(token != NULL)
    {
        pid_t pid;
        pipe_flag = 1;
        int wpid;

        if(pipe(pipe_fd) < 0)
        {
            perror("Can't make pipe!\n");
            exit(1);
        }

        int forknum = 0;
        while(token[forknum] != NULL)
        {
            printf("\n---------------cmd %d --------------\n",forknum + 1);

            if(token[forknum] == NULL)
            {
                return;
            }

            if((pid = fork()) < 0)
            {
                perror("fail to fork");
                return;
            }

            if(pid == 0)    //子进程
            {
            // printf("fork\n");
                // if((args[0] == NULL))
                // {
                //     perror("no command!");
                //     exit(0);
                // }
            
                args = split_line(token[forknum]);

                status_ = execute(args);
                status = 0;
           //  printf("fork end\n");

                exit(0);
            }
            else
            {
           // printf("father\n");

                close(pipe_fd[1]);
                memset(buff,0,MAX);
                if(wc_pipe == 1)
                {
                    int buff[MAX];
                    memset(buff,0,MAX);

                    read(pipe_fd[0],buff,MAX);
                    if(token[forknum+1] == NULL)
                    {
                        for(int i = 0; i < sizeof(buff); i++)
                        {
                            printf("%d\t",buff[i]);
                        }
                        printf("\n");
                    }
                    wc_pipe = 0;
                    return;
                }
                else
                {


                    if(read(pipe_fd[0],buff,MAX) > 0)
                    {
                        printf("read message\n");
                        if(token[forknum+1] == NULL)
                        {
                            printf("%s",buff);
                            return;
                        }
                    }
                    else
                    {
                        printf("no message\n");
                    }
                }

                do
                {   
                    wpid = waitpid(pid,&status, WUNTRACED);
                           
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
           // printf("father end\n");
            }
           // forknum++;
            n_args = split_line(token[++forknum]);
            int num = count_cmd(n_args);
            n_args[num] = buff;
            pipe_flag = 1;
            execute(n_args);
            forknum++;
           
            free(n_args);
        }


    }
    return;
}

void print_pipe()
{
    char buf[1024];
    memset(buf,0,1024);
    while(fgets(buf,sizeof(buf),fpipe) != NULL)
    {
        printf("%s",buf);
    }
    //printf("\n");
    return;
}

char* read_pipe()
{
    int buffer_size = TOK_BUFFER_SIZE;
    char buf[1024];
    char ncmd[MAX];
    char **cmd = malloc(buffer_size*sizeof(char*));
    memset(buf,0,1024);
    while(fgets(buf,sizeof(buf),fpipe) != NULL)
    {
        strcat(ncmd,buf);
    }
    //printf("\n");
    cmd[0] = ncmd;
    return cmd[0];
}

char** cat_cmd(char* ncmd,char* args)
{
    int num = 0;
    char** n_args = split_line(args);
    num = count_cmd(n_args);
    n_args[num] = ncmd;

    return n_args;
}

int checkpipe(char *cmd)
{
    int buffer_size = TOK_BUFFER_SIZE;
    int position = 0;
    // char **args;
    char **tokens = malloc(buffer_size*sizeof(char*));
    char *token;
    char **n_args = malloc(buffer_size*sizeof(char*));
    char *n_cmd;
    int flag = 0;
    int status_ = 1;
    int status = 1;

    int tubeflag = 0;

   // printf("%d",TUBE_FLAG(1));
    if(strstr(cmd,"|") == NULL)
    {
        return 1;
    }

    token = strtok(cmd,"|");
    //args = split_line(cmd);

    while(token != NULL)
    {
        tokens[position++] = token;
        token = strtok(NULL, "|");
    }
    tokens[position] = NULL;
    pipe_flag = 1;

    int i = 0;
    while(tokens[i] != NULL)
    {
        if(tokens[i+1] == NULL)
        {
            // printf("tokens = %s\n",tokens[i]);
            n_args = split_line(tokens[i]);
            execute(n_args);
           // fpipe = popen(tokens[i],"r");
            
           // execcute(tokens[i]);
           
            print_pipe();
            return 0;
        }
        


        else
        {
            fpipe = popen(tokens[i++],"r");
           // printf("This is pipe read!\n");
            n_cmd = read_pipe();
            *(tokens[i] + (strlen(tokens[i])-1)) = ' ';
            strcat(tokens[i],n_cmd);
            savehistory(tokens[i]);

        }
    }
    free(tokens);
    return 0;
}

int wc(char **args)
{
    FILE *fp = NULL;
   // char *fname = args[1];
    char *fname = NULL;
    char ch;
    int count_line = 0;
    int count_words = 0;

    if(args[1] == NULL)
    {
        printf("\033[31mno content input!\n\033[0m");
        return 1;
    }

    if(pipe_flag == 0)
    {
        if((strcmp(args[1],"-w") == 0) 
            || (strcmp(args[1],"-l") == 0)
            || (strcmp(args[1],"-m") == 0)
            || (strcmp(args[1],"-c") == 0))
        {
            fname = args[2];
        }
        else
        {
            fname = args[1];
        }
        if((fp = fopen(fname,"r")) == NULL)
        {
            puts("\033[31mfail to open\033[0m");
            printf("\033[31m%s\n\033[0m",fname);
            return 1;
        }
        // printf("hhhhhh\n");
        fseek(fp,0L,SEEK_END);
        long len = ftell(fp);
        fseek(fp,0L,SEEK_SET);

        for(long i = 0L; i < len; i++)
        {
            fseek(fp,i,SEEK_SET);
            ch = getc(fp);
            if((ch != EOF) && (ch == '\n'))
            {
                count_line++;
                count_words++;
                // fseek(fp,i,SEEK_SET);
            }
            if((ch != EOF) && ((ch == '\t') || (ch == ' ')))
            {
                count_words++;
            // fseek(fp,i,SEEK_SET);
            }
        //putchar(ch);
        }

        if(strcmp(args[1],"-w") == 0)
        {
            printf("%d\n",count_words);
            return 1;
        }
        if(strcmp(args[1],"-l") == 0)
        {
            printf("%d\n",count_line);
            return 1;
        }
        if(strcmp(args[1],"-m") == 0)
        {
            printf("%ld\n",len);
            return 1;
        }
        if(strcmp(args[1],"-c") == 0)
        {
            printf("%ld\n",sizeof(fp));
            return 1;
        }

        printf("%d\t",count_line);
        printf("%d\t",count_words);
        printf("%ld\t",len);
        printf("%s\n",fname);

        return 1;

    }
    if(pipe_flag == 1)
    {   
        wc_pipe = 1;
        long len = sizeof(args);
       // char buf[len];
       // int pos = 0;
        int input_len = 0;
        //memset(buf,0,len);

        for(long i = 0L; i < len; i++)
        {
            fseek(fpipe,i,SEEK_SET);
            ch = getc(fpipe);
            if((ch != EOF) && (ch == '\n'))
            {
                count_line++;
                count_words++;
                // fseek(fp,i,SEEK_SET);
            }
            if((ch != EOF) && ((ch == '\t') || (ch == ' ')))
            {
                count_words++;
            // fseek(fp,i,SEEK_SET);
            }
        //putchar(ch);
        }
        printf("%d\t",count_line);
        printf("%d\t",count_words);
        printf("%ld\n",len);

        pipe_flag = 0;
        return 1;
        
    }

    return 1;
}


int cat(char **args)
{
    if(args[1] == NULL)
    {
       puts("\033[31mno content input\033[0m");
       return 1;
    }

    FILE *fp = NULL;

    if((strcmp(args[1],"-E")) == 0)
    {
        char *fname = args[2];
        
        if((fp = fopen(fname,"r")) == NULL)
        {
            puts("\033[31mfail to open\033[0m");
            printf("\033[31m%s\n\033[0m",fname);
            return 1;
        }
        fseek(fp,0L,SEEK_END);
        long len = ftell(fp);
        char ch[len];
        memset(ch,0,len);
        fseek(fp,0L,SEEK_SET);
        int pos = 0;
        for(long i = 0L; i < len; i++)
        {
            fseek(fp,i,SEEK_SET);
            ch[pos] = getc(fp);
            if((ch[pos] != EOF) && (ch[pos] == '\n'))
            {
                fseek(fp,-1L,SEEK_CUR);
                ch[pos++] ='$';
                ch[pos] = '\n';
                fseek(fp,i,SEEK_SET);
            }
            pos++;
          //  putchar(ch);
        }
        ch[pos-1] = '\n';
        if(pipe_flag == 0)
        {
            printf("%s",ch);
        }
        else
        {
            close(pipe_fd[0]);
            write(pipe_fd[1],ch,strlen(ch));
        }
        return 1;
    }

    char *fname = args[1];

    if((fp = fopen(fname,"r")) == NULL)
    {
        puts("\033[31mfail to open\033[0m");
        printf("\033[31m%s\n\033[0m",fname);
        return 1;
    }


    fseek(fp,0L,SEEK_END);
    long len = ftell(fp);
    char ch[len];
    memset(ch,0,len);
    fseek(fp,0L,SEEK_SET);

    int buffer_size = TOK_BUFFER_SIZE,position = 0;
    char **n_args = malloc(buffer_size*sizeof(char*));
    char *tokens = NULL;

    for(long i = 0L; i < len-1; i++)
    {
        fseek(fp,i,SEEK_SET);
        ch[i] = getc(fp);
    }



    if(pipe_flag == 0)
    {
        // printf("\nprint by cat\n");
        printf("%s\n",ch);
       return 1;

    }
    else
    {
        // close(pipe_fd[0]);
        // write(pipe_fd[1],ch,strlen(ch));
    }

    free(n_args);

    // if(write(pipe_fd[1],(char*)len,len) < len)
    // {
    //     printf("write error!\n");
    // }
    // if(write(pipe_fd[1],ch,strlen(ch)) < strlen(ch))
    // {
    //     printf("write error!\n");
    // }

    return 1;
}

int grep(char **args)
{
    if(args[1] == NULL)
    {
        puts("\033[31mno content input\033[0m");
        return 1;
    }

    if((args[1] != NULL) && (args[2] == NULL))
    {
        puts("\033[31mplease input file name!\033[0m");
        return 1;
    }

    FILE *fp = NULL;
    int buffer_size = TOK_BUFFER_SIZE;
    char *fname = NULL;
    char **n_args = malloc(buffer_size*sizeof(char*));
    int count = 0;

    

    n_args = &args[1];
    fname = args[2];
 

    if((fp = fopen(fname,"r")) == NULL)
    {
        printf("\033[31mfail to open \033[0m");
        printf("\033[31m'%s'\n\033[0m",fname);
        return 1;
    }

    char *command = NULL;
    int position = 0;
    char **cmd = malloc(buffer_size*sizeof(char*));
    size_t bufsize;
    fseek(fp,0L,SEEK_SET);
    int ch = getc(fp);
    fseek(fp,0L,SEEK_SET);


    while(ch != EOF)
    {
        getline(&command,&bufsize,fp);
        cmd[position++] = command;
        command = NULL;
        ch = getc(fp);
    }

    int book_print = 0;

    int i = 0;
    //int book = 0;
    while(cmd[i] != NULL)
    {
        if(strstr(cmd[i],*n_args) != NULL)
        {
            printf("%s",cmd[i++]);
            book_print++;
        }
        else
        {
            i++;
        }
    }

    if(book_print == 0)
    {
        printf("\033[31mno \"%s\" in file \"%s\" !\n\033[0m",args[1],fname);
        return 1;
    }
    //printf("%s\n",command);
    free(cmd);
    free(*n_args);

    return 1;
}

int touch(char **args)
{
    if(args[1] == NULL)
    {
        puts("\033[31mno content input\033[0m");
        return 1;
    }

    FILE *fp = NULL;

    char *fname = args[1];

    if((fp = fopen(fname,"a")) == NULL)
    {
        puts("\033[31mfail to open\033[0m");
        printf("\033[31m%s\n\033[0m",fname);
        return 1;
    }
    fclose(fp);

    return 1;
}

void savehistory(char *command)
{
    FILE *fp;
    char cmd[1024] = {0};

    strcpy(cmd,command);
    //puts(cmd);

       	if((fp = fopen("myshell_history","wt+")) == NULL)
       	{
            fprintf(stderr,"\033[31merror; failed to store input history!\n\033[0m");
            exit(EXIT_FAILURE);
       	}

        if((command != NULL) && ((fp = fopen("myshell_history","wt+")) != NULL))
        {
            fprintf(fp,"%-40s",cmd);
            //fprintf(fp,"\n");
        }
    
    
}

char *read_line()
{
    char *command = NULL;
    size_t bufsize;
    getline(&command,&bufsize,stdin);
    savehistory(command);
    return command;
}

char **split_line(char *line)
{
    int buffer_size = TOK_BUFFER_SIZE,position = 0;
    char **tokens = malloc(buffer_size*sizeof(char*));
    char *token;

    token = strtok(line,TOK_DELIM);
    //puts(token);
    while(token != NULL)
    {
        tokens[position++] = token;
     //   printf("token:%s,%p\n",token,token);
        token = strtok(NULL, TOK_DELIM);
        //puts(token);

    }

    tokens[position] = NULL;
    return tokens;
}

int echo(char **args)
{
    if(args[1] == NULL)
    {
        puts("\033[31mno content input\033[0m");
        return 1;
    }

    if(strcmp(args[1],"-n") == 0)
    {

        printfcmd(args,2);
        return 1;
    }

    if(strcmp(args[1],"-e") == 0)
    {

        printfcmd(args,2);
        printf("\n");
        return 1;

    }

    printfcmd(args,1);
    printf("\n");

    return 1;
}

void printfcmd_e(char *args[],int i)
{
    if(args[i] == NULL)
    {
        printf("\033[31mfail to print command!\n\033[0m");
    }

    while(args[i] != NULL)
    {
        int j = 0;
        while((args[i] + j != NULL) && (j < strlen(args[i])))
        {

            if((*(args[i]+j) == '\\') && (*(args[i] + (j+1)) == '\\') && (*(args[i] + (j+2)) == 't'))
            {
                printf("\t");
                j += 3;
            }
            if((*(args[i]+j) == '\\') && (*(args[i] + (j+1)) == '\\') && (*(args[i] + (j+2)) == 'a'))
            {
                printf("\a");
                j += 3;
            }
            if((*(args[i]+j) == '\\') && (*(args[i] + (j+1)) == '\\') && (*(args[i] + (j+2)) == 'n'))
            {
                printf("\n");
                j += 3;
            }
            if((*(args[i]+j) == '\\') && (*(args[i] + (j+1)) == '\\') && (*(args[i] + (j+2)) == 'r'))
            {
                printf("\r");
                j += 3;
            }
            if((*(args[i]+j) == '\\') && (*(args[i] + (j+1)) == '\\') && (*(args[i] + (j+2)) == 'b'))
            {
                printf("\b");
                j += 3;
            }
            if((*(args[i]+j) == '\\') || (*(args[i]+j) == '"'))
            {
                j++;
            }
            if(args[i] + j != NULL)
            {
                printf("%c",*(args[i] + j));
                j++;
            }
        }
        printf(" ");
        i++;
        j = 0;
    }

}

void printfcmd(char *args[],int i)     
{
    if(args[i] == NULL)
    {
        printf("\033[31mfail to print command!\n\033[0m");
    }

    while(args[i] != NULL)
    {
        int j = 0;
        while((args[i] + j != NULL) && (j < strlen(args[i])))
        {
            if((*(args[i]+j) == '\\') || (*(args[i]+j) == '"'))
            {
                j++;
            }
            if(args[i] + j != NULL)
            {
                printf("%c",*(args[i] + j));
                j++;
            }
        }
        printf(" ");
        i++;
        j = 0;
        //printf("%s ",args[i]);
        //i++;
    }

    return;

}


int command_nums()    //一共有多少内置命令
{
    return sizeof(builtin_cmd)/sizeof(builtin_cmd[0]);  //数组的大小除以数组第一个元素的大小，就是这个数组的side
}



int which(char **args)
{
    int book = 0;

    if(args[1] == NULL)
    {
        printf("\033[31mPlese input command!\n\033[0m");
    }
    else
    {
        for(int i = 0; i < command_nums(); i++)
        {
            if(strcmp(args[1],builtin_cmd[i]) == 0)
            {
                printf("%s: ",builtin_cmd[i]);
                printf("shell built-in command\n");
                book++;
            }

        }
        if(book == 0)
        {
            printf("\033[31mNo such command!\n\033[0m");
        }

    }

    return 1;

}

// int popen_system(const char * cmd,char *pRetMsg, int msg_len)
// {
//     printf("\n\n_______________%s %d  cmd = %s\n",__func__,__LINE__,cmd);
//     FILE * fp;
//     char *p = NULL;
//     int res = -1;

//     if((cmd == NULL) || (pRetMsg == NULL) || (msg_len < 0))
//     {
//         printf("Param Error!\n");
//         return -1;
//     }
//     if((fp = popen(cmd,"r")) == NULL)
//     {
//         printf("Popen Error!\n");
//         return -2;
//     }
//     else
//     {
//         memset(pRetMsg,0,msg_len);

//         while(fgets(pRetMsg,msg_len,fp) != NULL)
//         {
//             printf("Msg: %s",pRetMsg);
//         }

//         if((res = pclose(fp)) == -1)
//         {
//             printf("close popen error!\n");
//             return -3;
//         }

//         pRetMsg[strlen(pRetMsg)] = '\0';
//         return 0;
//     }
// }


// void popen_read(const char* cmd)
// {
//     printf("\n\n_______________%s %d cmd = %s\n",__func__,__LINE__,cmd);
//     FILE *fp;
//     char buf[200] = {0};

//     if((fp = popen(cmd,"r")) == NULL)
//     {
//         perror("Fail to popen\n");
//         exit(1);
//     }
//     while(fgets(buf,200,fp) != NULL)
//     {
//         printf("%s",buf);
//     }
//     pclose(fp);
// }

// void popen_write(const char * cmd)
// {
//     printf("\n\n_______________%s %d  cmd = %s\n",__func__,__LINE__,cmd);
//     FILE *fp;
//     char buf[200] = {0};

//     if((fp = popen(cmd,"w")) == NULL)
//     {
//         perror("Fail to popen\n");
//         exit(1);
//     }
//     fwrite("Read pipe successfully!",1,sizeof("Read pipe successfully!"),fp);
//     pclose(fp);
// }


int not_builtin(char **args)   //非内置命令
{
    //execvp

    pid_t pid,wpid;
    int status;

    pid = fork();
    if(pid == 0)        //son
    {
        if(execvp(args[0],args) == -1)
        {
            printf("\033[31mCommand '%s' not found\n\033[0m",args[0]);
           // puts(args[0]);
           // perror("\033[31mMyshell error at execvp\n\033[0m");    //成功后不会继续执行,相当于替换函数

            for(int i = 0; i < command_nums(); i++)
            {
                if(strstr(args[0],builtin_cmd[i]) != NULL)
                {
                    printf("Did you mean:\n");
                    printf("command '%s' from built-in commands?\n",builtin_cmd[i]);
                }
            }
        }
        exit(EXIT_FAILURE);     //若能执行到这里，说明前面出错

    }
    else    //father等待子进程的结束
    {
        do
        {
            wpid = waitpid(pid,&status, WUNTRACED);    //把等待状态存在status

        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        
    }

    return 1;
}

int execute(char **args)   //execcute built-in commands
{
    if(args[0] == NULL)
    {
        return 1;
    }

    for(int i = 0; i < command_nums();i++)    //判断是否是内置命令
    {
        if(strcmp(args[0],builtin_cmd[i]) == 0)
        {
            return (builtin_func[i])(args);     //第I个命令
        }
    }

    return not_builtin(args);
    
}

void loop()
{
      //储存输入命令

    int status = 1;  //status of commands



    while(status == 1)
    {

        char *command;
        char **args;   
        char path[100];
        char now[200];
        getcwd(path,100);

        now[0] = '\0';
        strcat(now,path);
        strcat(now,"$  ");

        printf("\033[32mLumos@2023091202024:\033[0m");
        printf("\033[34m%s\033[0m",now);


        command = read_line();
        if(checkpipe(command) == 1)
        {
            args = split_line(command);
            status = execute(args);
        }
        free(command);
        free(args);
    } 
    

}

int main(int argc, char *argv[])
{
    loop();

    
    return 0;
}
