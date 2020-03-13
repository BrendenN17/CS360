// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>

#define  MAX 256
#define SERVER_HOST "localhost"
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 1234

#define BLKSIZE 1024

int n, ncommand;
char line[MAX], ans[MAX], cwd[MAX];
char *s, *command[MAX];
char lines[BLKSIZE];
char cwd[MAX], curdir[MAX];
// Define variables:
struct sockaddr_in  server_addr, client_addr;
int mysock, csock;
int r, len, n;

//functions for commands below
void do_pwd()
{
  //getcwd(cwd, MAX);
  printf("cwd = %s\n", curdir);
  strcpy(lines, curdir);
  //write(csock, lines, MAX);
}

void do_mkdir()
{
  int r = mkdir(command[1], 0700);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    strcpy(lines, command[1]);//user given directory put in lines
    strcat(lines, " directory has been created ");//add text to print out better on client side
    //write(csock, lines, MAX);
  }
}

void do_rmdir()
{
  int r = rmdir(command[1]);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    strcpy(lines, command[1]);
    strcat(lines, " directory has been removed ");
    //write(csock, lines, MAX);
  }
}

void do_rm()
{
  int r = unlink(command[1]);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    strcpy(lines, command[1]);
    strcat(lines, " pathname has been removed ");
    //write(csock, lines, MAX);
  }
}

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
int ls_file(char *fname)
{
  struct stat fstat, *sp;
  int r, i;
  char ftime[64];
  sp = &fstat;
  if ( (r = lstat(fname, &fstat)) < 0)
  {
    // printf("can’t stat %s\n", fname);
    strcat(lines, "can’t stat %s\n");
    exit(1);
  }  

  strcat(lines, "\n");  

  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
  {
    // printf("%c",'-');
    strcat(lines, "-");
  }
  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
  {
    // printf("%c",'d');
    strcat(lines, "d");
  }
  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
  {
    // printf("%c",'l');
    strcat(lines, "l");
  }
  for (i=8; i >= 0; i--)
  {
    if (sp->st_mode & (1 << i)) // print r|w|x
    {
      // printf("%c", t1[i]);
      strncat(lines, &t1[i], 1);
    }
    else
    {
      // printf("%c", t2[i]);
      strncat(lines, &t2[i], 1);
    }
  }

  char temp[MAX];

  // printf("%4d ",sp->st_nlink); // link count
  sprintf(temp, "%4d", sp->st_nlink);
  strcat(lines, temp);
  strcat(lines, " ");

  // printf("%4d ",sp->st_gid);
  sprintf(temp, "%4d", sp->st_gid);
  strcat(lines, temp);
  strcat(lines, " ");
  // gid

  // printf("%4d ",sp->st_uid);
  sprintf(temp, "%4d", sp->st_uid);
  strcat(lines, temp);
  strcat(lines, " ");
  // uid

  // printf("%8d ",sp->st_size);
  sprintf(temp, "%4d", sp->st_size);
  strcat(lines, temp);
  strcat(lines, " ");
  // file size

  // print time
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0;
  // kill \n at end
  // printf("%s ",ftime);
  strcat(lines, ftime);
  strcat(lines, " ");

  // print name
  // printf("%s", basename(fname)); // print file basename
  strcat(lines, basename(fname));
  // strcat(lines, " ");
  // print -> linkname if symbolic file
  if ((sp->st_mode & 0xF000)== 0xA000)
  {
    char linkname[1024];
    if (readlink(fname, linkname, 1024) > 0)
    {
      // printf(" -> %s", linkname); // print linked name
      strcat(lines, " -> ");
      strcat(lines, linkname);
    }
  }
  // printf("\n");
  //write(csock, lines, MAX);
}

int ls_dir(char *dname)
{
  DIR *dir;
  struct dirent *content;

  dir = opendir(dname);
  if (dir == 0)
  {
    printf("opendir %s failed\n", dname);
    return -1;
  }
  while (content = readdir(dir))
  {
    if (strcmp(content->d_name, ".") && strcmp(content->d_name, ".."))
      ls_file(content->d_name);
  }
}

void do_ls()
{
  struct stat mystat, *sp = &mystat;
  int r;
  char *filename, path[1024], cwd[256];

  filename = "./"; // default to CWD
  if (command[2] != '\0')
    filename = command[2]; // if specified a filename
  if (r = lstat(filename, sp) < 0)
  {
    printf("no such file %s\n", filename);
    exit(1);
  }
  strcpy(path, filename);
  if (path[0] != '/') // filename is relative : get CWD path
  {
    getcwd(cwd, 256);
    strcpy(path, cwd); strcat(path, "/"); strcat(path, filename);
  }
  if (S_ISDIR(sp->st_mode))
    ls_dir(path);
  else
    ls_file(path);

  //write(csock, "END ls", MAX);
}

void do_cd()
{
  if (!command[1])//change to home directory
  {
    chdir("/home");
    strcpy(lines, "/home");
  }
  else//changes directory to user specified
  {
    chdir(command[1]);
    strcpy(lines, command[1]);
  }
  strcat(lines, " is our new current directory");

  //write(csock, lines, MAX);
}

void do_quit()
{
  //should disconnect the client
  bzero(lines, MAX);
  strcpy(lines, "quit");
  //write(csock, lines, MAX);
}

void do_get()
{
  if (command[1] == "\0")
  {
    printf("No FILE\n");
  }

  struct stat buff;
  int ret = stat(command[1], &buff);

  char size[MAX];

  if (ret == -1)
  {
    printf("Stat failed!\n");
  }
  else
  {
    sprintf(size, "%4d", buff.st_size);
    write(csock, size, BLKSIZE);
  }

  FILE *src;
  char buf[BLKSIZE];
  src = fopen(command[1], "r");

  while (fgets(buf, BLKSIZE, src))
  {
    write(csock, buf, BLKSIZE);
  }

  write(csock, "END get", BLKSIZE);

  fclose(src);
}

void do_put()
{
  //copy path from client to server
  int n, size, total = 0;
  char buf[BLKSIZE];
  n = read(csock, buf, BLKSIZE);

  size = buf;

  FILE *dest;

  dest = fopen(command[1], "w");

  n = read(csock, buf, BLKSIZE);
  // printf("buf = %s\n", buf);
  while (strcmp(buf, "END get") != 0)
  {
    fputs(buf, dest);
    n = read(csock, buf, BLKSIZE);
    total += n;
  }

  fclose(dest);
}

char *cmds[] = {"pwd", "ls", "cd", "mkdir", "rmdir", "rm", "get", "put", "quit", "\0"};
int (*fptr[])() = {do_pwd, do_ls, do_cd, do_mkdir, do_rmdir, do_rm, do_get, do_put, do_quit};

int findCmd(char *cmd)
{
  int i = 0;
  while (cmds[i] != "\0")
  {
    if (strcmp(cmds[i], cmd) == 0)
      return i;
    i++;
  }
  return -1;
}

// ********* CODE FROM PRELAB 4 PART2

// Server initialization code:
int server_init()
{
  printf("==================== server init ======================\n");   
  // get DOT name and IP address of this host
  printf("1 : create a TCP STREAM socket\n");
  mysock = socket(AF_INET, SOCK_STREAM, 0);
  
  printf("2 : fill server_addr with host IP and PORT# info\n");
  // initialize the server_addr structure
  server_addr.sin_family = AF_INET;                  // for TCP/IP
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
  server_addr.sin_port = htons(SERVER_PORT);         // port number

  printf("3 : bind socket to server address\n");
  // bind syscall: bind the socket to server_addr info
  r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
      printf("bind failed\n");
      exit(3);
  }
  printf("host = %s port = %d\n", SERVER_HOST, SERVER_PORT);
  
  printf("4 : server is listening ....\n");
  listen(mysock, 5); // listen queue length = 5
  printf("===================== init done =======================\n");
}

char cmdLine[MAX];
int main(int argc, char *argv[])
{
  server_init(); 
  
  getcwd(curdir, MAX);       // get CWD into char curdir[ ];
  printf("server: chroot to %s\n", curdir);
  chroot(curdir); // change / to current DIR 

  while(1)
  { // Try to accept a client request
    printf("server: accepting new connection ....\n"); 

    // Try to accept a client connection as descriptor csock
    len = sizeof(client_addr);
    csock = accept(mysock, (struct sockaddr *)&client_addr, &len);
    if (csock < 0)
    {
      printf("server: accept error\n");
      exit(1);
    }
    printf("server: accepted a client connection from\n");
    printf("-----------------------------------------------\n");
    printf("Client: IP=%s  port=%d\n", "127.0.0.1", ntohs(client_addr.sin_port));
    printf("-----------------------------------------------\n");

    // Processing loop: csock <-- data --> client
    while(1)
    {
      bzero(line, MAX);
      bzero(lines, MAX);
      n = read(csock, line, MAX);
      strcpy(cmdLine, line);
      // process the line from clinet
      strcat(line, " ECHO");
      // send the echo line to client 
      n = write(csock, line, MAX);

      if (n==0)
      {
        printf("server: client died, server loops\n");
        close(csock);
        break;
      }
      // show the line string
      printf("server: read  n=%d bytes; line=[%s]\n", n, line);


      ncommand = 0;
      s = strtok(cmdLine, " ");

      while (s)
      {
          command[ncommand++] = s;
          s = strtok(NULL, " ");
      }

      int index = findCmd(command[0]);

      if (index >= 0)
          fptr[index]();
      
      //send result of simple command to client
      n = write(csock, lines, BLKSIZE);

      printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
      printf("server: ready for next request\n");
    }
  }
}
