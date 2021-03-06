/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)   
{
  printf("chdir %s\n", pathname);
  //printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
  int ino = getino(pathname);
  printf("ino = %d\n", ino);
  if (!ino) return -1;
  MINODE *mip = iget(dev,ino);
  //verify is dir
   if (!S_ISDIR(mip->INODE.i_mode)){
      printf("not a directory\n");
      iput(mip);
      return -1;
   }
   //puts the changes to the dev and moves on
   iput(running->cwd);
   //changes the running to the minode
   running->cwd = mip;
   return 1;
}


int ls_file(MINODE *mip, char *name)
{
  char * t1 = "xwrxwrxwr-------";
  char * t2 = "----------------";
 
  int r, i;
  char ftime[64];

  //prints the stuff to signify what type of ffile it is 
  if(S_ISDIR(mip->INODE.i_mode)){
     printf("%c",'d');
  }
  if(S_ISREG(mip->INODE.i_mode)){
     printf("%c",'-');
  }
  if(S_ISLNK(mip->INODE.i_mode)){
     printf("%c",'l');
  }
  //goes thru the 8 steps to check
  for (i = 8; i >= 0; i--){ 
     if (mip->INODE.i_mode & (1 << i)) // print r | w | x 
        printf("%c", t1[i]); 
     else printf("%c", t2[i]); // or print - 
  }
  //prints the values for INODE info
  printf("%4d ", mip->INODE.i_links_count); // link count 
  printf("%4d ", mip->INODE.i_gid); // gid 
  printf("%4d ", mip->INODE.i_uid); // uid 

  printf("%8d ", mip->INODE.i_size);

  // print time 
  strcpy(ftime, ctime(&mip->INODE.i_mtime) ); // print time in calendar form 
  ftime[ strlen( ftime)-1] = 0; // kill \n at end 
  printf("%s ", ftime); // print name 
  printf("%s",  name ); // print file basename // print -> linkname if symbolic file 
  if (S_ISLNK(mip->INODE.i_mode)){ // use readlink() to read linkname 
  	char lnk[64];
  	myreadlink(mip, lnk);
  	printf(" -> %s", lnk ); // print linked name 
  }
  printf("\n"); 
  //getchar();
}

int ls_dir(MINODE *mip)
{
  //printf("ls_dir: list CWD's file names; YOU do it for ls -l\n");
  	
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  
  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf); 
  dp = (DIR *)buf;
  cp = buf;

  //printf("hello");
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
	
     //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
     int ino = search(mip, temp);
     //printf("ino = %d", ino);
     
     MINODE * node = iget(dev, ino);
      //simply calls the ls
     ls_file(node, dp->name);
     iput(node);
     
     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  printf("\n");
}

int ls(char *pathname)  
{
  printf("ls %s\n", pathname);
  //printf("ls CWD only! YOU do it for ANY pathname\n");
  if(pathname[0] == 0)	ls_dir(running->cwd);
  else{
     int ino = getino(pathname);
     //printf("ino = %d", ino);
     //getchar();
     if (ino == 0){
        printf("Error: %s does not exist\n",pathname);
        return -1;
     
     }
     //wd is the inode representing ino
     MINODE * wd = iget(dev, ino);
     if(S_ISDIR(wd->INODE.i_mode)){
     	ls_dir(wd);
  	 }
     else{
     	printf("error: %s is not a directory\n", pathname);
     }
     //puts info back to the wd and dec refcount
     iput(wd);
  }
}

char dirbuf[16][64];
int ndir;

int rpwd(MINODE *wd){
  char buf[64];
  char temp[64];
  //wd is work dir
  if(wd == root){
    return;
  }
  u32 cur;
  u32 parent = findino(wd, &cur);
  //printf("cur = %d, parent = %d\n", cur, parent);
  MINODE * pip = iget(dev, parent);
  u32 len = findmyname(pip, cur, &buf);
  /*
  strcpy(dirbuf[ndir], buf);
  ndir++;
  */
  strncpy(temp, buf, len);
  //recursive call to look into the dir
  rpwd(pip);
  //buf[strlen(buf)] = 0;
  //printf("/%s", temp);
  putchar('/');
  //used for building the path
  for(int i = 0; i < len; i++){
     if(isalnum(buf[i])) putchar(buf[i]);
  }
  //printf("hello");
  iput(pip);
  
}

char *pwd(MINODE *wd)
{
  //printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
  else{
     ndir = 0;
     //this calls a recursive function that breaks down the working directory
     rpwd(wd);
     //for(int i = ndir-1; i >= 0; --i){
     //	printf("/%s",dirbuf[i]);
     //}
     //printf("%s\n", dirbuf);
  }
  printf("\n");
  
}

