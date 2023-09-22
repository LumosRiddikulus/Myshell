#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
//#include <Windows.h>

#define TOK_DELIM " \t\r\n"
#define TOK_BUFFER_SIZE 64
#define ECHO_OMIT '"'


char *usage[] = 
{
    "change directory\n",
    "looking for commands\n",
    "exit my shell\n",
    "identify commands\n",
    "repeat input\n"
};

char *builtin_cmd[] =       //告诉支持哪些命令
{
    "cd",
    "help",
    "exit",
    "which",
    "echo"
};

int cd(char **args);
int help(char **args);
int mysh_exit(char **args);
int which(char **args);
int echo(char **args);


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
    }
    else
    {
        if(*args[1] == '"')
        {
            int i = 1;
            int j = 1;


            while(args[i] != NULL)
            {
                while((*(args[i] + j) != ECHO_OMIT) && (args[i] + j != NULL) && (j < strlen(args[i])))
                {
                    //printf("i = %d, j = %d\n",i,j);
                    printf("%c",*(args[i] + j));
                    j++;
                    //printf("\n");
                }
                printf(" ");
                i++;
                j = 0;
            }
            printf("\n");
           // return 1;
        }
        else
        {
            int i = 1;
            while(args[i] != NULL)
            {
                printf("%s ",args[i]);
                i++;
            }
            printf("\n");
        }
    }

    return 1;
}


int command_nums()    //一共有多少内置命令
{
    return sizeof(builtin_cmd)/sizeof(builtin_cmd[0]);  //数组的大小除以数组第一个元素的大小，就是这个数组的side
}


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
    exit(0);
    return 0;
}




int (*builtin_func[])(char**) =      //函数指针
{
    cd,
    help,
    mysh_exit,
    which,
    echo
};



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
                    printf("command '%s' from deb\n",builtin_cmd[i]);
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
    char *command;  //储存输入命令
    char **args;        //存储分割后的参数
    int status = 1;  //status of commands



    while(status == 1)
    {
        char path[100];
        char now[200];
        getcwd(path,100);

        now[0] = '\0';
        strcat(now,path);
        strcat(now,"$  ");

        printf("\033[32mLumos@2023091202024:\033[0m");
        printf("\033[34m%s\033[0m",now);
        //fflush(stdout);

        command = read_line();
        args = split_line(command);
        status = execute(args);

        //puts(args[1]);

       // printf("\n");
        free(command);
        free(args);


    } 
    

}

int main(int argc, char *argv[])
{
    loop();

    
    return 0;
}