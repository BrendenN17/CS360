/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)
{
  // READ Chapter 11.7.3 HOW TO chdir
  if (pathname[0] != '\0')//cd with 2nd argument
  {
    int ino = getino(pathname);// get inode # of pathname
    if (ino == 0) return 0;//doesn't exist

    MINODE *mip = iget(dev, ino);//ptr set to loaded inode with correct ino #

    // VERIFY is mip->INODE is DIR
    if (S_ISDIR(mip->INODE.i_mode))
    {   
      iput(running->cwd);
      running->cwd = mip;
    }
    else//not a DIR
    {
      printf("%s is not a DIR\n", pathname);
    }
  }
  else // When user enters cd without a second argument
  {
    running->cwd = root;
  }
}

char *retPWD[NMINODE];//used to return cwd if not root
char *rpwd(MINODE *wd)
{
  if (wd == root) return;
  int my_ino, parent_ino, x = 0;
  char *my_name[NMINODE];

  // FROM wd->INODE.i_block[0] get, my_ino & parent_ino
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // Assume DIR has only one data block i_block[0]
  get_block(dev, wd->INODE.i_block[0], buf); 
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE && x < 2)//while still in data block
  {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    if (strcmp(temp, ".") == 0)
    {
      my_ino = dp->inode; //found my_ino number
      x++;
    }
    if (strcmp(temp, "..") == 0)
    {
      parent_ino = dp->inode;//found parent_ino number
      x++;
    }
    // printf("[%d %s]  ", dp->inode, temp); // print [inode# name]

    //advancing to next data block
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  // Clear buffers
  strcpy(buf, "");
  strcpy(temp, "");

  MINODE *pip = iget(dev, parent_ino);

  // FROM pip->INODE.i_block[ ]: get my_name string by my_ino as LOCAL
  findmyname(pip, my_ino, my_name);

  rpwd(pip);
  printf("/%s", my_name);
  strcat(retPWD, "/");
  strcat(retPWD, my_name);
}

char *pwd(MINODE *wd)
{
  strcpy(retPWD, "");
  if (wd == root){ //current directory is root
    printf("CWD = /\n");
    return "/";
  }
  else //current directory not root
  {
    printf("CWD = ");
    rpwd(wd);//updates retPWD (global var)
    printf("\n");
    return retPWD;
  }
}


char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
int ls_file(MINODE *mip, char *name)
{
  char ftime[64];
  // READ Chapter 11.7.3 HOW TO ls
  if (S_ISREG(mip->INODE.i_mode)) // if (S_ISREG())
  {
    printf("%c", '-');
  }
  if (S_ISDIR(mip->INODE.i_mode)) // if (S_ISDIR())
  {
    printf("%c", 'd');
  }
  if (S_ISLNK(mip->INODE.i_mode)) // if (S_ISLNK())
  {
    printf("%c", 'l');
  }
  for (int i = 8; i >= 0; i--)
  {
    if (mip->INODE.i_mode & (1 << i)) // print r|w|x
    {
      printf("%c", t1[i]);
    }
    else
    {
      printf("%c", t2[i]);
    }
  }

  printf("  %d  %d  %d", mip->INODE.i_links_count, mip->INODE.i_uid, mip->INODE.i_gid); // first number to show

  strcpy(ftime, ctime(&mip->INODE.i_mtime)); // print time in calendar form
  ftime[strlen(ftime) - 1] = 0;
  // kill \n at end
  printf("  %s", ftime);


  printf("  %d", mip->INODE.i_size);
  printf("  %s\n", name);
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf); 
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE)
  {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
    mip = iget(dev, dp->inode);
    ls_file(mip, temp);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");
}

int ls(char *pathname)  
{
  printf("ls %s\n", pathname);
  if (pathname[0] != '\0')//ls specific directory
  {
    char curdir[NMINODE];
    strcpy(curdir, pwd(running->cwd));//get current directory into curdir
    printf("curdir = %s\n", curdir);
    chdir(pathname);//change to user specified directory
    ls_dir(running->cwd);//ls specified directory
    chdir(curdir);//change back to previous current directory
  }
  else
  {
    ls_dir(running->cwd);//ls current directory
  }
}


