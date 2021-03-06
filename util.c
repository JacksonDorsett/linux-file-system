/*********** util.c file ****************/
#define BLKSIZE 1024
#include "type.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

extern char gpath[128]; // global for tokenized components
extern char *name[32];  // assume at most 32 components in pathname
extern int   n;      
extern MINODE minode[NMINODE];

//for each MINODE finds first empty and available MINODE and makes it in use
MINODE *mialloc(){
  int i;
  for(i = 0; i < NMINODE; i++){
    MINODE *mp = &minode[i];
    if(mp->refCount == 0){
      mp->refCount = 1;
      return mp;
    }
  }
  printf("FS panic: out of minodes\n");
  return 0;
}

//just makes the refcount of a MINODE have a refcount of 0
int midalloc(MINODE *mip){
  mip->refCount = 0;
}

//get block takes a dev and blk, basically retrieves it and writes it into buffer
int get_block(int dev, int blk, char *buf)
{
    //lseek changes the offset of a file
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}
//same as get block but instead of reading it writes to the dev
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

//Just goes thru and parses the pathname into individual names , in name
int tokenize(char *pathname)
{
  // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
  // Code in Chapter 11.7.2
  strcpy(gpath, pathname);
  n = 0;
  for(char * c = strtok(gpath, "/"); c != 0; c = strtok(0,"/")){
    name[n] = c;
    n++;
  }
}

//retrieves a MINODE when given a dev and ino
MINODE *iget(int dev, int ino)
{
  // return minode pointer of loaded INODE=(dev, ino)
  // Code in Chapter 11.7.2
  MINODE *mip;
  MTABLE *mp;
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  for(i = 0; i < NMINODE;++i){
    MINODE *mip = &minode[i];
    if (mip->refCount && (mip->dev == dev) && (mip->ino==ino)){
      mip->refCount++;
      return mip;
    }
  }

  mip = mialloc();
  mip->dev = dev; mip->ino = ino;
  block = (ino - 1) / 8 + inode_start; // + iblock;
  offset = (ino-1) % 8;
  get_block(dev, block, buf);
  ip = (INODE *)buf + offset;
  mip->INODE = *ip;
  mip->refCount = 1;
  mip->mounted = 0;
  mip->dirty = 0;
  mip->mptr = 0;
  return mip;
}

//puts the inode back and records info
void iput(MINODE *mip)
{
  // dispose of minode pointed by mip
  // Code in Chapter 11.7.2
  
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  if(mip == 0) return 0;
  mip->refCount--;
  if (mip->refCount > 0) return;
  if (mip->dirty == 0) return;
  //write INODE back to disk
  block = (mip->ino - 1) / 8 + inode_start;//iblock
  offset = (mip->ino - 1) % 8;

  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset;
  *ip = mip->INODE;
  put_block(mip->dev, block, buf);
  midalloc(mip);
  
} 

int search(MINODE *mip, char *name)
{
  // search for name in (DIRECT) data blocks of mip->INODE
  // return its ino
  // Code in Chapter 11.7.2
  //printf("enter search\n");
  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  for(i = 0; i < 12; i++){
    if(mip->INODE.i_block[i] == 0){
      return 0;
    }
    get_block(mip->dev, mip->INODE.i_block[i], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while(cp < sbuf + BLKSIZE){
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      //printf("%s",temp);
      //printf("%8d%8d%8u\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if(strcmp(name, temp) == 0){
	//printf("found %s : inumber = %d\n", name, dp->inode);
	return dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
    return 0;
  }
  
}

int getino(char *pathname)
{
  // return ino of pathname
  // Code in Chapter 11.7.2
  //printf("enter getino");
  MINODE *mip;
  int i, ino;
  if (strcmp(pathname, "/") == 0)return 2;
  if (pathname[0] == '/') mip = root;
  else mip = running->cwd;
  mip->refCount++;

  tokenize(pathname);

  for(i = 0; i < n; i++){
    if (!S_ISDIR(mip->INODE.i_mode)){
      printf("%s is not a directory\n", name[i]);
      iput(mip);
      return 0;
    }

    ino = search(mip, name[i]);
    if(!ino){
      printf("no such component name %s\n");
      iput(mip);
      return 0;
    }
    iput(mip);
    mip = iget(dev, ino);
  }
  iput(mip);
  return ino;
  
}

//basically the inverse of search, looks thru parent for the inode and returns the name
int findmyname(MINODE *parent, u32 myino, char *myname) 
{
  // search parent's data block for myino;
  // copy its name STRING to myname[ ];
  
  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  
  for(i = 0; i < 12; i++){
    if(parent->INODE.i_block[i] == 0){
      return 0;
    }
    get_block(parent->dev, parent->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while(cp < sbuf + BLKSIZE){
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      //printf("%8d%8d%8u\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if(myino == dp->inode){
	//printf("found %s : inumber = %d\n", name, dp->inode);
	//printf("name = %s", dp->name);
	strcpy(myname, dp->name);
	//printf("%s\n", myname);
	return strlen(myname);
      }
       cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return -1;
  
}

// reverse of the getino, has the inode want to figure out wh
int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  
  if(mip->INODE.i_block[0] == 0){
     return 0;
   }
   get_block(mip->dev, mip->INODE.i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   while(cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     //just nulls out the end
     temp[dp->name_len] = 0;
     //if . it points to itself
     if(!strcmp(temp, ".")){
       *myino = dp->inode;
     }
     if(!strcmp(temp, "..")){
       return dp->inode;
     }
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
  
}

//Just checks what the bit is
int tst_bit(char *buf, int bit){
	return buf[bit / 8] & (1 << (bit % 8));
}
//sets a bit to one
int set_bit(char *buf, int bit){
	buf[bit/8] |= (1 << (bit % 8));
	return 1;
}
//clr_bit just makes a bit 0
int clr_bit(char *buf, int bit){
	buf[bit/8] &= (0 << (bit % 8));
}

//inode allocate; 
int ialloc(int dev){
	int i;
	char buf[BLKSIZE];
	
	//read inode_bitmap block
	get_block(dev, imap, buf);
	//allocates one of the nodes in the imap
	for(int i = 0; i < ninodes; i++){
		if(tst_bit(buf,i) == 0){
			set_bit(buf, i);
			put_block(dev, imap, buf);
			printf("allocated ino = %d\n", i + 1);
			return i + 1;
		}
	}
	return 0;
}

//inode deallocation, gets the imap clears the bit puts the block back to imap and Frees Inodes
int idealloc(int dev,int ino){
  //this might be poorly written just a heads up
  int i;
  char buf[BLKSIZE];
  //just gonna hope this is a predefined function

  /*MTABLE *mp = (MTABLE *)get_mtable(dev);
  if(ino > mp->ninodes){
    printf("inumber %d out of range\n", ino);
    return;
  }*/

  //get inode inside bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);
  //write buf back
  put_block(dev, imap, buf);
  incFreeInodes(dev);
}

//block allocation:
int balloc(int dev){
	int i;
	char buf[BLKSIZE];
	
	//read block bitmap block
	get_block(dev, bmap, buf);
	//allocates the block and puts it back to bmap
	for (int i = 0; i < nblocks; i++){
		if(tst_bit(buf,i) == 0){
			set_bit(buf, i);
			put_block(dev,bmap,buf);
			printf("allocated block = %d\n", i + 1);
			return i + 1;
		}
	}
	return 0;
}

//Literally just increases the free inodes count
int incFreeInodes(dev){
	char buf[BLKSIZE];
	get_block(dev, 1, buf);
	SUPER *sp = (SUPER *) buf;
	sp->s_free_inodes_count++;
	put_block(dev,1,buf);
	get_block(dev, 2,buf);
	GD * gp = (GD*) buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

//inode deallocate
int idalloc(int dev, int ino){
	int i;
	char buf[BLKSIZE];
	
	if (ino > ninodes){
		printf("inumber out of range\n");
		return;
	}
  //just get imap clear a map and put back
	get_block(dev, imap, buf);
	clr_bit(buf, ino-1);
	put_block(dev, imap, buf);
	incFreeInodes(dev);
}

//Literally just increases free blocks count
int incFreeBlocks(dev){
	char buf[BLKSIZE];
	get_block(dev, 1, buf);
	SUPER *sp = (SUPER *) buf;
	sp->s_free_blocks_count++;
	put_block(dev,1,buf);
	get_block(dev, 2,buf);
	GD * gp = (GD*) buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

//block dealloc:
int bdalloc(int dev, int bno){
	int i;
	char buf[BLKSIZE];
	
	if (bno > nblocks){
		printf("bnumber out of range\n");
		return;
	}
  //gets the bmap and sets the bno to zero and puts back
	get_block(dev, bmap, buf);
	clr_bit(buf, bno-1);
	put_block(dev, bmap, buf);
	incFreeInodes(dev);
}

//Truncate: Deletes all of the inner iblocks data, essentially zeroes it out
int truncate(MINODE *mip){
  //searches thru iblocks
	for(int i = 0; i < 14; i++){
		if(mip->INODE.i_block[i] == 0){
			break;
		}
		//If the 12th INODE, clear all of the sub-blocks
		if(i == 12){
			char buf[BLKSIZE];
			get_block(dev, mip->INODE.i_block[12], buf);
			int * ref = buf;
			for(i = 0; i<256;i++){
				//printf("ref = %d", *ref);
				if(*ref == 0){
					break;
				}
				bdalloc(dev, *ref);
				ref++;
				//getchar();
			}
      //put the block back
			put_block(dev, mip->INODE.i_block[12], buf);

		}
		//if the 13th block
		if(i == 13){
			char buf[BLKSIZE];
			get_block(dev, mip->INODE.i_block[12], buf);
			int * ref = buf;
      //repeat from when i==12
			for(i = 0; i<256;i++){
				if(*ref == 0){
					break;
				}
				char innerbuf[BLKSIZE];
				get_block(dev, mip->INODE.i_block[*ref], buf);
				int * ref2 = innerbuf;
        //this time need to go one step deaper and zero out all of the inner refs
				for(int j = 0; j < 256; j++){
					if(*ref2 == 0){
						break;
					}
					bdalloc(dev, *ref2);
					ref2++;
				}
				
				put_block(dev, *ref2, innerbuf);
				bdalloc(dev, *ref);
				ref++;
			}
			//basically puts it back to the block the fact that all refs have been zeroed out
			put_block(dev, mip->INODE.i_block[13], buf);
		}
		//dealloc the block
		bdalloc(dev, mip->INODE.i_block[i]);
	}
  //update time and zero out the size
	mip->INODE.i_mtime = mip->INODE.i_atime = mip->INODE.i_ctime = time(0);
	
	mip->INODE.i_size = 0;
	return 1;
}

//clear block zeroes out a block.
int clear_block(int dev, int blk){
	char buf[BLKSIZE];
	get_block(dev, blk, buf);
    bzero(buf,BLKSIZE);
    put_block(dev, blk, buf);
}
