
//mylink called when link is passed
int mylink(char * oldFileName, char * newFileName){
	MINODE *start;
	//sorts if the start should be root or cwd
	if (oldFileName[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	//get a old ino
	int oldino = getino(oldFileName);
	printf("oldino = %d\n", oldino);
	if(oldino == 0){
		printf("error: %s does not exist\n", oldFileName);
		return 0;
	}
	
	MINODE * old = iget(dev, oldino);
	//cant link a dir
	if(S_ISDIR(old->INODE.i_mode)){
		printf("error: %s cannot be a directory\n", oldFileName);
		iput(old);
		return -1;
	}
	char parent[128], child[128];
	char buf[BLKSIZE];
	strcpy(buf, newFileName);
	strcpy(parent, dirname(buf));
	//parent holds the dir of the new filename
	printf("buf = %s\n", parent);
	//checks if ./
	if (strlen(buf) == 1 && parent[0] == '.')
		parent[0] = '/';
	int newpino = getino(parent);
	//new pino is just the ino of the parent ~shocker~
	if (newpino == 0 ){
		printf("error: %s does not exist\n", parent);
		return 0;
	}
	if (getino(newFileName) != 0){
		printf("error: %s already exists\n", newFileName);
		return 0;
	}
	//new is the new link location
	MINODE * new = iget(dev, newpino);
	//can't link a dir
	if(!S_ISDIR(new->INODE.i_mode)){
		printf("error: %s is not a directory\n", buf);
		iput(new);
		return -1;
	}
	//basically enters the new ino into the old ino iblock
	enter_name(new, oldino, basename(newFileName));
	
	old->INODE.i_links_count++;
	//writess info to both the old and new
	iput(old);
	iput(new);
}

// removes a link
int my_unlink(char * pathname){
	// get the inode that is being looked for
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    int isDir = (S_ISDIR(mip->INODE.i_mode));
	//don't unlink dirs stupid
    if(isDir){
        printf("Tried to unlink a dir\n");
        return -1;
    }
    //removing name entry from parent Dirs Data block
 	mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count>0){
		// if its changed need to mark it as dirty so that it will adjust later
        mip->dirty=1;
    }
    //if links==0 remove the file name
    else{
        //This deallocates the inode and all of its data blocks 
        idalloc(mip->dev,mip->ino);
        printf("enter truncate");
        truncate(mip);
    }
	//writes back to the block
    iput(mip);
    
    char buf[256];
	//builds a parent and child name
    strcpy(buf, pathname);
	char parent[128]; 
	char child[128]; 
	strcpy(parent,dirname(buf));
	strcpy(buf, pathname);
	strcpy(child, basename(buf));
	printf("parent: %s, child: %s\n",parent,child);
	
	int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    //rm_child only takes in two parameters
    printf("pino: %d\n");
	//rm_child just gets rid of the child from the pmip data blocks
    rm_child(pmip, child);
	// mark it as dirty and put back to the block
    pmip->dirty = 1;
    iput(pmip);
   
}
