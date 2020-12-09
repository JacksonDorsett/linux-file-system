//function called by both unlink and rmdir
int rm_child(MINODE *parent, char *name){
	printf("ino = %d\n", parent->ino);
	printf("name: %s\n", name);
	char buf[BLKSIZE], *cp;
	DIR * dp1, * dp2 = 0;
	//goes thru each of the iblocks[]
	for(int i = 0; i < 12;++i){
		printf("block: %d\n",parent->INODE.i_block[i]);
		//if the iblock is empty just continue
		if(parent->INODE.i_block[i] == 0){
			continue;
		}
		//setting up the Block and two dir pointers to step thru the block
		get_block(parent->dev, parent->INODE.i_block[i], buf);
		int remain = BLKSIZE;
		char * namebuf[BLKSIZE];
		int counted = 0;
		dp1 =(DIR *)buf;
		cp = dp1;
		bzero(namebuf, BLKSIZE);
		strncpy(namebuf, dp1->name, dp1->name_len);
		namebuf[dp1->name_len] = 0;
		printf("dp1->name = %s\n",namebuf);
		//while there is still space on the block, checks to find dir with the correct name
		while(remain > 0 && strcmp(name, namebuf)){
			if(!strcmp(dp1->name, name)){
				break;
			}
			
			if(strcmp(dp1->name, name) == 0){
				break;
			}
			
			//printf("dp1->name = %s\n",namebuf);
			//printf("dp2->rec_len: %d, remain = %d\n", dp2->rec_len, remain);
			//moves the dir pointers along thru the dirs
			cp = dp1;
			dp2 = dp1;
			remain -= dp1->rec_len;
			counted += dp1->rec_len;
			cp += dp1->rec_len;
			dp1 = (DIR *)cp;
			
			/*
			dp2 = dp1;
			remain -= dp1->rec_len;
			dp1 = dp1 + dp1->rec_len; //may not work
			*/
			//getchar();
			bzero(namebuf, BLKSIZE);
			strncpy(namebuf, dp2->name, dp2->name_len);
			namebuf[dp2->name_len] = 0;
			printf("dp2->name after = %s\n",namebuf);
			printf("dp2->rlen after = %d\n",dp2->rec_len);
		}
		//if remain ==0 then it just continues
		if(remain == 0)
			continue;
		//only entry case
		if(dp1->rec_len == BLKSIZE){
			printf("\nenter only\n");
			bdalloc(dev, parent->INODE.i_block[i]);
			parent->INODE.i_block[i] = 0;
			return 1;
		}
		//end case
		else if(dp1->rec_len == remain){
			printf("enter end\n");
			dp2->rec_len += dp1->rec_len;
			put_block(dev,parent->INODE.i_block[i],buf);
			return 1;
		}
		//middle case
		else{
			//printf("enter middle\n");
			//the amount that we are going to shift by
			int len = dp1->rec_len;
			//printf("rec_len: %d", dp1->rec_len);
			//step forward
			
			//remain -= len;
			
			//char *cpmv = cp;
			/*while(dp1->rec_len < remain){
				
				
				//step forward dp1,2
				int next = dp1->rec_len;
				
				
				
				
				printf("dp2->rec_len: %d\n", dp2->rec_len);
				cp += next;
				cpmv += dp2->rec_len;
				dp1 = (DIR *)cp;
				dp2 = (DIR *)cpmv;

				remain -= next;
				printf("remain = %d", remain);
				getchar();
				

			}*/
			//sets name ready
			bzero(namebuf, BLKSIZE);
			strncpy(namebuf, dp1->name, dp1->name_len);
			namebuf[dp1->name_len] = 0;
			printf("dp1->name after = %s\n",namebuf);
			printf("dp1->rlen after = %d\n",dp1->rec_len);
			//sets the last to the buf current
			int bts = 0;
			DIR * last = buf;
			cp = last;
			DIR * dp;
			//looks for the last dir in the block to add the len to it
			while(bts + last->rec_len < BLKSIZE){
				bts += last->rec_len;
				cp += last->rec_len;
				last = (DIR *) cp;
				bzero(namebuf, BLKSIZE);
				strncpy(namebuf, last->name, last->name_len);
				namebuf[last->name_len] = 0;
				//if the name matches set the dp = the searched dir
				if(strcmp(name, namebuf) == 0){
					dp = last;
				}
			}
			
			last->rec_len += len;
			
			bzero(namebuf, BLKSIZE);
			strncpy(namebuf, last->name, last->name_len);
			namebuf[last->name_len] = 0;
			printf("dp1->name after = %s\n",namebuf);
			printf("dp1->rlen after = %d\n",last->rec_len);
			//have cp = the dp(dir of interest)
			cp = dp;
			//next is the item after the item of choice
			DIR * next = cp + dp->rec_len;
			//distance from start of next to the beginning. Subtract that value from BLKSIZE and thats the amount left over
			int bytes = BLKSIZE - ((char*)next - buf);
			
			printf("bytes: %d\n", bytes);
			memcpy(dp, next, bytes);
			printf("enter");

/*
			remain = bytes;
			while(remain > dp1->rec_len){
				remain -= dp1->rec_len; 
				cp += dp1->rec_len;
				dp1 = (DIR *)cp;
				
				
			}
			printf("dp2 final reclen before: %d\n",dp1->rec_len);
			dp1->rec_len += len;
			printf("dp2 final reclen after: %d\n",dp1->rec_len);
			*/
			
			//parent->INODE.i_size -= len;
			//put block back.
			put_block(dev,parent->INODE.i_block[i],buf);
			return 1;
		}
	}

}

//main
int my_rmdir(char *pathname){
    //getting ino of the pathname
    if (pathname[0] == '/'){
    	//start = root;
    	dev = root->dev;
    }
    else{
    	dev = running->cwd->dev;
		//start = running->cwd;
	}
    int ino = getino(pathname); 
	if (ino == 0){
		printf("%s does not exist\n", pathname);
		return;
	}
    //getting minode pointer
    MINODE *mip = iget(dev, ino);
    
    //check user status
    int super_user = 1;
	int same_uids = 1;
	if(running->uid != 0 && mip->INODE.i_uid == running-> uid){
		printf("uids do not match\n");
		return -1;
	}
    //checks to see if the directory is a directory
    //int isDir = mip->INODE.i_mode == 0x41ED; //cause the 4 is important
    int isDir = (S_ISDIR(mip->INODE.i_mode));
    int isBusy = (mip->refCount)>1;
    int isEmpty = (mip->INODE.i_links_count == 2);
    
    //Makes sur isDir isntBusy isEmpty
    if(!isDir || isBusy || !isEmpty){
        printf("Either not a Dir, is busy or isn't empty");
        iput(mip);
        return -1;
    }
    
    //Assuming the check goes through, search each of the iblocks
    for (int i=0; i<12;i++){
        if(mip->INODE.i_block[i]==0) //is null
            break;
		//deallocates to prep for deletion
        bdalloc(mip->dev, mip->INODE.i_block[i]);
        idalloc(mip->dev, mip->ino);
        iput(mip);
    }
	//deallocate the inode and put back to the block
    idalloc(mip->dev, mip->ino);
    iput(mip);

/*
    char name[256];
    char dname[256];
    char * bname;
    strcpy(dname, pathname);
    dirname(dname);
    printf("dname: %s\n", dname);
    strcpy(name, pathname);
    
    int pino = getino(dname);//
    MINODE * pmip = iget(dev, pino);
    findino(mip, &ino);
    printf("pino: %d\n", pino);
    findmyname(pmip, ino, name);
    printf("name: %s\n", name);
    getchar();*/
    
    char buf[256];
	//just gives parent and child names
    strcpy(buf, pathname);
	char parent[128]; 
	char child[128]; 
	strcpy(parent,dirname(buf));
	strcpy(buf, pathname);
	strcpy(child, basename(buf));
	printf("parent: %s, child: %s\n",parent,child);
	
	int pino = getino(parent);
    
    MINODE * pip = iget(dev, pino);
    

    pip = iget(dev, pino);
	//calls the rm_child and sends the inode and childname
    printf("pino: %d, ino %d\n", pino, ino);
    rm_child(pip, child);
    //touch the mtime and atime once we get these functions set up, and decrement link count
    pip->INODE.i_links_count--;
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0);
    pip->dirty = 0;
	//put the parent back without child
    iput(pip);

    return 1;
    //success
}


