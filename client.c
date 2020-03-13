// The echo client client.c
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

#define MAX          256
#define BLKSIZE      1024
#define SERVER_HOST "localhost"
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT  1234

char *s;
int ncommand;
char *command[MAX];
char cwd[MAX];

// Define variables
struct sockaddr_in  server_addr; 
int sock, r, n;
char line[MAX], ans[MAX], lines[MAX];

void do_lcat()
{
  FILE *file;

  int c;
  if (!command[1])
  {
    printf("No file specified to display\n");
  }
  else
  {
    file = fopen(command[1], "r");
    while ((c = fgetc(file)) != EOF)
    {
      putchar(c); //prints file to server side
    }
    close(file);
  }
}

void do_lpwd()
{
  getcwd(cwd, MAX);
  printf("cwd = %s\n", cwd);
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
      printf("%c", t2[i]);
  }
  printf("%4d ", sp->st_nlink); // link count
  printf("%4d ", sp->st_gid);
  // gid
  printf("%4d ", sp->st_uid);
  // uid
  printf("%8d ", sp->st_size);
  // file size
  // print time
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
  ftime[strlen(ftime) - 1] = 0;
  // kill \n at end
  printf("%s ", ftime);
  // print name
  printf("%s", basename(fname)); // print file basename
  // print -> linkname if symbolic file
  if ((sp->st_mode & 0xF000) == 0xA000)
  {
    char linkname[1024];
    if (readlink(fname, linkname, 1024) > 0)
    {
      printf(" -> %s", linkname); // print linked name
    }
  }
  printf("\n");
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

void do_lls()
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
    strcpy(path, cwd);
    strcat(path, "/");
    strcat(path, filename);
  }
  if (S_ISDIR(sp->st_mode))
    ls_dir(path);
  else
    ls_file(path);
}

void do_lcd()
{
  if (!command[1])
  { //change to home directory
    int r = chdir("/home");
    if (r < 0)
    {
      printf("errno = %d : %s\n", errno, strerror(errno));
    }
    else
    {
      printf("changing to home directory\n");
    }
  }
  else
  { //change to user specified directory
    int r = chdir(command[1]);
    if (r < 0)
    {
      printf("errno = %d : %s\n", errno, strerror(errno));
    }
    else
    {
      printf("changing to %s directory\n", command[1]);
    }
  }
}

void do_lmkdir()
{
  int r = mkdir(command[1], 0700);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    printf("%s was created as a directory\n", command[1]);
  }
}

void do_lrmdir()
{
  int r = rmdir(command[1]);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    printf("%s was removed as a directory\n", command[1]);
  }
}

void do_lrm()
{
  int r = unlink(command[1]);
  if (r < 0)
  {
    printf("errno = %d : %s\n", errno, strerror(errno));
  }
  else
  {
    printf("%s was removed successfully\n", command[1]);
  }
}

void do_cget()
{
  int n, size, total = 0;
  char buf[BLKSIZE];
  n = read(sock, buf, BLKSIZE);

  size = buf;
  printf("size = %d\n", atoi(size));

  FILE *dest;

  dest = fopen(command[1], "w");

  n = read(sock, buf, BLKSIZE);
  // printf("buf = %s\n", buf);
  while (strcmp(buf, "END get") != 0)
  {
    fputs(buf, dest);
    n = read(sock, buf, BLKSIZE);
    total += n;
  }

  fclose(dest);

  write(sock, "FINISH get", BLKSIZE);
}

void do_cput()
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
    printf("size = %4d\n", buff.st_size);
    sprintf(size, "%4d", buff.st_size);
    write(sock, size, BLKSIZE);
  }

  FILE *src;
  char buf[BLKSIZE];
  src = fopen(command[1], "r");

  while (fgets(buf, BLKSIZE, src))
  {
    write(sock, buf, BLKSIZE);
  }

  write(sock, "END get", BLKSIZE);

  fclose(src); 
}

char *cmds[] = {"lcat", "lpwd", "lls", "lcd", "lmkdir", "lrmdir", "lrm", "\0"};
int (*fptr[])() = {do_lcat, do_lpwd, do_lls, do_lcd, do_lmkdir, do_lrmdir, do_lrm};

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

//*** PRELAB 4 PART 2 stuff

// client initialization code
int client_init()
{
  printf("======= clinet init ==========\n");
  
  printf("1 : create a TCP socket\n");
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock<0){
     printf("socket call failed\n");
     exit(1);
  }

  printf("2 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  //server_addr.sin_addr.s_addr = htonl((127<<24) + 1);
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("3 : connecting to server ....\n");
  r = connect(sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(3);
  }
  printf("4 : connected OK to\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s PORT=%d\n", SERVER_HOST, SERVER_PORT);
  printf("---------------------------------------------------------\n");
  printf("========= init done ==========\n");
}

int main(int argc, char *argv[ ])
{
  client_init();

  printf("************************\n");
  printf("COMMAND MENU: \n");
  printf("pwd ls cd mkdir rmdir rm get put quit\n");
  printf("lcat lpwd lls lcd lmkdir lrmdir lrm\n");
  printf("************************\n");



  printf("********  processing loop  *********\n");
  while (1){
    printf("input a line : ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0)                  // exit if NULL line
       exit(0);

    char tempLine[MAX];
    strcpy(tempLine, line);
    ncommand = 0;
    s = strtok(tempLine, " ");
    while (s)
    {
      command[ncommand++] = s;
      s = strtok(NULL, " ");
    }

    int index = findCmd(command[0]);  
    
    if (index >= 0)
    {
      fptr[index](); //calls function locally
    }
    else
    { 
      // Send ENTIRE line to server
      n = write(sock, line, MAX);
      printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

      // // Read a line from sock and show it
      n = read(sock, ans, MAX);
      printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);

      if (!strcmp(command[0], "get"))
        do_cget();
      if (!strcmp(command[0], "put"))
        do_cput();

      // if (strcmp(command[1], "ls") == 0)
      // {
      //   int m;
      //   char buffer[MAX];
      //   m = read(sock, buffer, MAX);
      //   while (strcmp(buffer, "END ls") != 0)
      //   {
      //     n = read(sock, lines, BLKSIZE);
      //     printf("client: command result is : %s\n", lines);
      //   }
      // }
      // else
      {
        n = read(sock, lines, BLKSIZE);
        printf("client: command result is : %s\n", lines);
      }


      if (!strcmp(lines, "quit")) //if client chose quit, quit will be echoed
      {
        exit(0); //client exits
      }
    }
  }
}

