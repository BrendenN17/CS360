/*********** util.c file ****************/

int get_block(int dev, int blk, char *buf)//get info from desired block
{
   //skip forward to desired block
   lseek(dev, (long)blk*BLKSIZE, 0);
   //read desired block
   read(dev, buf, BLKSIZE);
}   
int put_block(int dev, int blk, char *buf)//put info into desired block
{
   //skip forward to desired block
   lseek(dev, (long)blk*BLKSIZE, 0);
   //write back to desired block 
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }

  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){//searching for specific minode (unique dev and ino numbers)
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){//updating a MINODE to show its in use and setting dev, ino.
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]    
       blk    = (ino-1)/8 + inode_start;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)
{
   int i, block, offset;
   char buf[BLKSIZE];
   INODE *ip;

   mip->refCount--;//lower use count by 1

   if (mip->refCount > 0)  // minode is still in use
      return;
   if (!mip->dirty)        // INODE has not changed; no need to write back
      return;

   /* write INODE back to disk */
   /***** NOTE *******************************************
   For mountroot, we never MODIFY any loaded INODE
                  so no need to write it back
   FOR LATER WORK: MUST write INODE back to disk if refCount==0 && DIRTY

   Write YOUR code here to write INODE back to disk
   ********************************************************/
   // int blk =  (mip->ino - 1) / 8 + inode_start;
   // offset = (mip->ino-1) % 8;
   // char ibuf[BLKSIZE];

   // // load inode
   // get_block(mip->dev, blk, ibuf);
   
   // ip = (INODE*) ibuf + offset;
   // *ip = mip->INODE;

   // put_block(dev, blk, ibuf);
} 

int search(MINODE *mip, char *name)//returns inode number if found
{
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE)
   {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;//terminating temp with null char
      printf("%4d  %4d  %4d    %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if (strcmp(temp, name)==0)
      {
         printf("found %s : ino = %d\n", temp, dp->inode);
         return dp->inode;
      }
      //advance to next data block
      cp += dp->rec_len;
      dp = (DIR *)cp;
   }
   return 0;
}

int getino(char *pathname)
{
  int i, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname);

  for (i=0; i<n; i++)
  {
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);                // release current mip
      mip = iget(dev, ino);     // get next mip
   }

   iput(mip);                   // release mip  
   return ino;
}

int findmyname(MINODE *parent, u32 myino, char *myname) 
{
  // WRITE YOUR code here:
  // search parent's data block for myino;
  // copy its name STRING to myname[ ];
   char buf[BLKSIZE];
   char name[256];

   get_block(dev, parent->INODE.i_block[0], buf);
   dp = (DIR *)buf;
   char *cp = buf;

   while (cp < buf + BLKSIZE)
   {
      // get current dir name
      if (dp->inode == myino)//found
      {
         strncpy(name, dp->name, dp->name_len);
         name[dp->name_len] = 0;
         strcpy(myname, name);
         return 1;
      }

      cp += dp->rec_len; // increment by length of record
      dp = (DIR *)cp;
   }
   return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  char buf[BLKSIZE], *cp;   
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);//get desired block
  cp = buf; 
  dp = (DIR *)buf;
  *myino = dp->inode;
  cp += dp->rec_len;
  dp = (DIR *)cp;
  return dp->inode;//return dp inode number
}
