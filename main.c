/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

// globals
MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;



MINODE *iget();

#include "symlink_readlink.c"
#include "util.c"
#include "open_close.c"
#include "rmdir.c"
#include "cd_ls_pwd.c"
#include "read_cat.c"
#include "mkdir_creat.c"
#include "link_unlink.c"
#include "write_cp.c"
#include "Misc_Level1.c"


int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");
  //clears all of the MINODES
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  //Just inits the procs
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "diskimage";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128], pathname2[128];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  //Opens any disks that may be passed in
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }
  //reads the amount inodes an blocks from the superblock
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  //get and assign the group descriptor
  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);
  // go init all the MINODES and PROCS
  init();  
  //mount the root node
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);
  //the main loop
  while(1){
    printf("[ls|cd|pwd|mkdir|read|write|cat|open|lseek|close|rmdir|creat|unlink|link|symlink|readlink|quit]\ninput command :  ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname2[0] = 0;
    
    //gather what was entered
    sscanf(line, "%s %s %s", cmd, pathname, pathname2);
    printf("cmd=%s pathname=%s pathname2 =%s\n", cmd, pathname, pathname2);
  
    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    if (strcmp(cmd, "cd")==0)
       chdir(pathname);
    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
	if (strcmp(cmd, "mkdir") == 0)
		make_dir(pathname);
	if (strcmp(cmd, "creat")==0)
		creat_file(pathname);
  if (strcmp(cmd, "creat")==0)
		(pathname);
	if (strcmp(cmd, "link")==0)
		mylink(pathname, pathname2);
	if (strcmp(cmd, "symlink") == 0)
		mysymlink(pathname, pathname2);
	if (strcmp(cmd, "readlink")== 0){
		char lnk[64];
		read_link(pathname, lnk);
		printf("link = %s\n", lnk);
	}
	if (strcmp(cmd, "unlink")==0){
		my_unlink(pathname);
	}
	if (!strcmp("open", cmd)){
		printf("%d\n\n", pathname2[0] - '0');
		open_file(pathname,pathname2[0] - '0');
	}
	if (!strcmp(cmd, "close")){
		int fd;
		sscanf(pathname, "%d", &fd);
		printf("fd = %d\n", fd);
		close_file(fd);
	}
	if(!strcmp(cmd, "pfd")){
		pfd();
	}
	if(!strcmp(cmd, "cat")){
		my_cat(pathname);
	}
	if (!strcmp(cmd, "read")){
		int fd;
		int nbytes;
		char buf[BLKSIZE];
		sscanf(pathname, "%d", &fd);
		sscanf(pathname2, "%d", &nbytes);
		read_file(fd, buf, nbytes);
		printf("%s\n",buf);
	}
	if(!strcmp(cmd, "lseek")){
		int fd;
		int position;
		sscanf(pathname, "%d", &fd);
		sscanf(pathname2, "%d", &position);
		my_lseek(fd, position);
	}
	if(strcmp(cmd, "rmdir") ==0){
		my_rmdir(pathname);
	}
	if(strcmp(cmd, "write")==0){
		int fd;
		int nbytes;
		char buf[BLKSIZE];
		bzero(buf, BLKSIZE);
		sscanf(pathname, "%d", &fd);
		sscanf(pathname2, "%d", &nbytes);
		strtok(line, "\"");
		char * p = strtok(0, "\"");
		strtok(0,"\"");
		strcpy(buf, p);
		printf("buf = %s, line = %s\n", buf, line);
		getchar();
		write_file(fd, buf, nbytes);
	}
	if(strcmp(cmd, "cp") == 0){
		my_cp(pathname, pathname2);
	}
  if(strcmp(cmd, "stat") == 0){
		mystat(pathname);
	}
  if(strcmp(cmd, "chmod") == 0){
		mychmod(pathname, pathname2);
	}
  if(strcmp(cmd, "utime") == 0){
		myutime(pathname);
	}
    if (strcmp(cmd, "quit")==0)
       quit();
  }
}

// Just shuts everything down
int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
