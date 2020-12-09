
//Opens a file for something
int open_file(char * pathname, int mode){
	//decides if root or cwd
	if (pathname[0] == '/') dev = root->dev;
	else dev = running->cwd->dev;
	int ino = getino(pathname);
	
	printf("dev = %d", dev);
	MINODE * mip = iget(dev, ino);
	//Can only open regular files
	if(!S_ISREG(mip->INODE.i_mode)){
		iput(mip);
		printf("Not a regular file\n");
		return -1;
	}
	//invalid mode entered
	if (mode < 0 || mode > 4){
		printf("mode is not valid\n");
		iput(mip);
		return -1;
	}
	//OFTP is the open file table pointer
	OFT *oftp = malloc(sizeof(OFT));
	int fid = 0;
	while(running->fd[fid] != 0){
		//goes thru each fd to check if any are open still, heres the check
		if(fid == NFD){
			printf("no free FTP\n");
			iput(mip);
			return -1;
		}
		fid++;
	}
	//Once found an empty set the runnting to the oftp and set initial
	running->fd[fid] = oftp;
	printf("fd = %x",running->fd[fid]);
	oftp->mode = mode;
	oftp->refCount = 1;
	oftp->mptr = mip;
	
	//what to do if RD WR RW or APPend
	switch(mode){
		case 0: oftp->offset = 0;
				break;
		case 1: truncate(mip);
				oftp->offset = 0;
				break;
		case 2: oftp->offset = 0;
				break;
		case 3: oftp->offset = mip->INODE.i_size;
				break;
		default:printf("invalid mode\n");
				return -1;
	}
	//update time, and mark as dirty
	mip->INODE.i_atime = time(0);
	if (mode != 0){
		mip->INODE.i_mtime;
	}
	mip->dirty = 1;
	
	return fid;
}

//called from main 
int close_file(int fd){
	//if the fd isn't in range
	if(fd < 0 || fd >= NFD){
		printf("error: fd out of range\n");
		return -1;
	}
	
	OFT * oftp = running->fd[fd];
	if(oftp == 0){
		printf("error: fd %d is not allocated\n", fd);
		return -1;
	}
	//dealloc and drop the ref count
	oftp = running->fd[fd];
	running->fd[fd] = 0;
	oftp->refCount--;
	if(oftp->refCount > 0){
		return 0;
	}
	//puts the back to the block
	MINODE * mip = oftp->mptr;
	iput(mip);
	return 0;
	
}

int my_lseek(int fd, int position){
	//called from main
	OFT * oftp = running->fd[fd];
	if(oftp == 0){
		printf("oftp is null\n");
		return -1;
	}
	
	if(position < 0 || oftp->mptr->INODE.i_size <= position){
		printf("error position is outside of the bounds of file\n");
		return -1;
	}
	//save original spot and move the offset over to the new value.
	int original_position = oftp->offset;
	oftp->offset = position;
	
	return original_position;
}

//called from main
int pfd(){
	printf("%4s %8s %8s %8s\n", "fd","mode","offset","INODE");
	//for each file in fd[]
	for(int i = 0; i < NFD; i++){
		if(running->fd[i] == 0) continue;
		printf("%4d", i);
		//shows which mode it is in currently
		switch(running->fd[i]->mode){
			case 0:
				printf("%8s", "R");
				break;
			case 1:
				printf("%8s", "W");
				break;
			case 2:
				printf("%8s", "RW");
				break;
			case 3:
				printf("%8s", "APPEND");
				break;
			}
		//then prints the values for offset and the dev,ino
		printf("%8d", running->fd[i]->offset);
		printf("\t   [%d,%d]\n", running->fd[i]->mptr->dev,running->fd[i]->mptr->ino);
	}
	printf("\n");
}
