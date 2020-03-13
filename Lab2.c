/*** This is my final working version of the SH simulator from Lab2 ***/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h> //prints correct error message for why function is failing

char cwd[256];
char *dir[128], *token[128]; //path and user input
char cmd[64], home[128];     //user chosen command & home path
char *cmdlist[64] = {"ls", "cd", "pwd", "cat", "emacs", "exit", NULL};
#define CMDNUM 6
#define MAX 4096
//prints startup menu
void printstartup(char *env[])
{
    char gpath[128];
    int i = 0;
    int n = 0;
    char *s;
    while (env[i])
    {
        if (strncmp(env[i], "PATH=", 5) == 0)
        {
            printf("found PATH : %s\n", env[i]);
            printf("copy PATH string to gpath[]\n");
            strcpy(gpath, &env[i][5]);
            break;
        }
        i++;
    }

    printf("\n1. SHOW PATH:\n");
    printf("PATH = %s\n", gpath);
    s = strtok(gpath, ":");
    while (s)
    {
        dir[n++] = s;
        s = strtok(NULL, ":");
    }
    printf("2.decompose path to %d dir strings:\n", n);
    for (i = 0; i < n; i++)
    {
        printf("dir[%d] = %s   ", i, dir[i]);
    }
    printf("\n\n");
    printf("3. Show the HOME directory: ");
    strcpy(home, "/root");
    printf(" HOME = %s\n\n", home);
}
//gets user commad line and parses into tokens
void getCmd_parse()
{
    int i, n;
    char *s;
    //char *token[128];
    char gpath[128], line[128];
    
    //prints out menu of available commands to user
    printf("Available commands are ***ls, cd, pwd, cat, emacs, exit***\n");

    //gets command line from user and puts into gpath
    printf("Enter a command line : ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;
    strcpy(gpath, line);
    printf("gpath = %s\n", gpath);

    //parses gpath into directory array
    n = 0;
    s = strtok(gpath, " ");
    while (s)
    {
        token[n++] = s;
        s = strtok(NULL, " ");
    }
    strcpy(cmd, token[0]);
    printf("n=%d\n", n);
    for (i = 0; i < n; i++)
    {
        printf("token[%d] = %s\n", i, token[i]);
    }
}

int whichcmdNum() //returns # cooresponding to which command
{
    for (int i = 0; i < CMDNUM; i++)
    {
        if (!strcmp(cmdlist[i], cmd))
        {
            return i;
        }
    }
    return -1; //not valid
}

//ls -l command and helper functions
struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
int ls_file(char *fname)
{
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0)
    {
        printf("can’t stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }
    printf("%4d ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);   // gid
    printf("%4d ", sp->st_uid);   // uid
    printf("%8d ", sp->st_size);  // file size
    // print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;        // kill \n at end
    printf("%s ", ftime);                // print name
    printf("%s", basename(fname));       // print file basename
    // print -> linkname if symbolic file
    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        char linkname[1024];
        // use readlink() to read linkname
        if (readlink(fname, linkname, 1024) > 0)
        {
            printf(" -> %s", linkname); // print linked name
        }
    }
    printf("\n");
}
//ls dir helper for ls-l command
int ls_dir(char *dname)
{
    // use opendir(), readdir(); then call ls_file(name)
    struct dirent *ep;
    DIR *dp = opendir(dname);
    if (dp == 0)
    {
        printf("failed to open %s directory\n", dname);
        return -1;
    }
    while (ep = readdir(dp))
    {
        if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, ".."))
        {
            ls_file(ep->d_name);
        }
    }
}
//function to call ls_file and ls_dir
int lscmd()
{
    struct stat mystat, *sp = &mystat;
    int r;
    char *filename, path[1024], cwd[256];
    filename = "./"; // default to CWD
    if (token[2] != '\0')
        filename = token[2]; // if specified a filename
    if (r = lstat(filename, sp) < 0)
    {
        printf("no such file %s\n", filename);
        exit(1);
    }
    strcpy(path, filename);
    if (path[0] != '/')
    { // filename is relative : get CWD path
        getcwd(cwd, 256);
        strcpy(path, cwd);
        strcat(path, "/");
        strcat(path, filename);
    }
    if (S_ISDIR(sp->st_mode))
    {
        ls_dir(path);
    }
    else
    {
        ls_file(path);
    }
}
//chang directory command
void cdcmd()
{
    if (token[1])
    {
        int r = chdir(token[1]);
            if (r < 0)
            {
                printf("errno=%d : %s\n", errno, strerror(errno));
            }
            else
            {
                printf("changing directory to /%s/\n\n", token[1]);
            }
    }
    else
    {
        int r = chdir(home);
        if (r < 0)
        {
            printf("errno=%d : %s\n", errno, strerror(errno));
        }
        else
        {
            printf("changing to home directory : %s\n\n", home);
        }
    }
}
//print working directory command
void pwdcmd()
{
    int r = getcwd(cwd, 256);
    if (r < 0)
    {
        printf("errno=%d : %s\n", errno, strerror(errno));
    }
    else
    {
        printf("cwd is : %s\n", cwd);
    }
}
//exit command
void exitcmd()
{
    printf("Exiting the program\n");
    exit(0);
}
//cat file command (possibly with <)
int catcmd(char *token[]) 
{
    int fd, m, n;
    char buf[MAX], dummy;
    fd = 0; // default to stdin
    if (token[1])
    {
        fd = open(token[1], O_RDONLY);
        if (fd < 0) //failed to open
            exit(1);
    }
    while (n = read(fd, buf, MAX)) //while reading from file
    {
        m = write(1, buf, n); //write to screen
    }
}
//file editor command
int emacscmd()
{
}
//I-O redirection checking function
int i_ocheck()
{

}
//Pipe checking function
int pipe_check()
{

}
//copy file contents (>)
int cpcmd()
{

}
//append file contents (>>)
int appndcmd()
{

}
int (*fptr[])() = {lscmd, cdcmd, pwdcmd, catcmd, emacscmd, exitcmd};

int main(int argc, char *argv[], char *env[])
{
    int index;
    printstartup(env); //will update global token
    while (1)//main loop
    {
        getCmd_parse(); //will update global cmd
        if (!strcmp(cmd, cmdlist[1]))//cd command (no fork needed)
        {
            fptr[1]();//calls cdcmd function
        }
        else if (!strcmp(cmd, cmdlist[5]))//exit command (no fork needed)
        {
            fptr[5]();//calls exitcmd function
        }
        else//non simple command, (will fork a child process)
        {
            index = whichcmdNum();
            if (index >= 0)//valid command
            {
                int pid, status;
                pid = fork(); //fork a child
                if (pid)//parent
                {
                    //must wait for child to die
                    pid = wait(&status);
                    //prints parent pid, status, and pid
                    printf("PARENT: %d, STATUS: %d -- CHILD: %d\n", getpid(), status, pid);
                }
                else//child
                {
                    //will execute the command
                    //using execve
                    //prints child pid, and parent pid
                    printf("CHILD %d -- PARENT %d\n", getpid(), (pid_t)getppid());
                    fptr[index](); //calls whatever command called
                }
            }
            else
            {
                printf("invalid command BUDDY\n Try again GUY\n");
            }
        }
    }
}