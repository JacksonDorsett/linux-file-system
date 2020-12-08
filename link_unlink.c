

int mylink(char * oldFileName, char * newFileName){
	MINODE *start;
	if (oldFileName[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	

	int oldino = getino(oldFileName);
	printf("oldino = %d\n", oldino);
	if(oldino == 0){
		printf("error: %s does not exist\n", oldFileName);
		return 0;
	}
	
	MINODE * old = iget(dev, oldino);
	
	if(S_ISDIR(old->INODE.i_mode)){
		printf("error: %s cannot be a directory\n", oldFileName);
		iput(old);
		return -1;
	}
	char parent[128], child[128];
	char buf[BLKSIZE];
	strcpy(buf, newFileName);
	strcpy(parent, dirname(buf));
	printf("buf = %s\n", parent);
	if (strlen(buf) == 1 && parent[0] == '.')
		parent[0] = '/';
	int newpino = getino(parent);
	
	if (newpino == 0 ){
		printf("error: %s does not exist\n", parent);
		return 0;
	}
	if (getino(newFileName) != 0){
		printf("error: %s already exists\n", newFileName);
		return 0;
	}
	
	MINODE * new = iget(dev, newpino);
	
	if(!S_ISDIR(new->INODE.i_mode)){
		printf("error: %s is not a directory\n", buf);
		iput(new);
		return -1;
	}
	
	enter_name(new, oldino, basename(newFileName));
	
	old->INODE.i_links_count++;
	
	iput(old);
	iput(new);
}

int my_unlink(char * pathname){
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    int isDir = (S_ISDIR(mip->INODE.i_mode));
    if(isDir){
        printf("Tried to unlink a dir\n");
        return -1;
    }
    //removing name entry from parent Dirs Data block
 	mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count>0){
        mip->dirty=1;
    }
    //if links==0 remove the file name
    else{
        //This deallocates the inode and all of its data blocks 
        idalloc(mip->dev,mip->ino);
        printf("enter truncate");
        truncate(mip);

    }
    iput(mip);
    
    char buf[256];

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
    rm_child(pmip, child);
    pmip->dirty = 1;
    iput(pmip);
    //Now We got to decrement the INODEs link count by 1
   
    // update the dirty flag for when write back to disk
   
}
