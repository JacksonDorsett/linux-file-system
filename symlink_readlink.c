

int mysymlink(char * oldName, char * newName){
	MINODE *start;
	char * buf[BLKSIZE];
	if (oldName[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	
	int oino = getino(oldName);
	if (oino == 0){
		printf("error: %s does not exist", oino);
		return -1;
	}
	if (strlen(oldName) > 60){
		printf("error: %s is longer than 60 characters\n", oldName);
		return -1;
	}
	MINODE * mip = iget(dev, oino);
	ls_file(mip, "the file");
	if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode)){
		printf("error: %s is neither a REG or DIR file\n", oldName);
		iput(mip);
		return -1;
	}
	iput(mip);
	
	int ino = creat_file(newName);
	MINODE * nMip = iget(dev, ino);
	
	nMip->INODE.i_mode = 0120644;
	printf("ino = %d, mode = %x\n", ino, nMip->INODE.i_mode);
	memcpy((char *)nMip->INODE.i_block, oldName, strlen(oldName) + 1);
	
	nMip->INODE.i_size = strlen(oldName);
	nMip->dirty = 1;
	iput(nMip);
	
	return 1;
}

char * read_link(char * pathname, char * link){
	int ino = getino(pathname);
	if(ino == 0){
		printf("error: %s does not exist\n", pathname);
		return 0;
	}
	MINODE * mip = iget(dev, ino);
	
	if(!S_ISLNK(mip->INODE.i_mode)){
		printf("error: %s is not a symbolic link file\n", pathname);
		return 0;
	}
	
	memcpy(link, mip->INODE.i_block, mip->INODE.i_size);
	return link;
	
}
