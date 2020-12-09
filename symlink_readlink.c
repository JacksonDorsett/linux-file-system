
//called from main
int mysymlink(char * oldName, char * newName){
	MINODE *start;
	char * buf[BLKSIZE];
	//is it root or cwd
	if (oldName[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	//gets the oldino
	int oino = getino(oldName);
	if (oino == 0){
		printf("error: %s does not exist", oino);
		return -1;
	}
	if (strlen(oldName) > 60){
		printf("error: %s is longer than 60 characters\n", oldName);
		return -1;
	}
	//the minode for the oldname
	MINODE * mip = iget(dev, oino);
	ls_file(mip, "the file");
	//checks to make sure dir or reg
	if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode)){
		printf("error: %s is neither a REG or DIR file\n", oldName);
		iput(mip);
		return -1;
	}
	//puts the mip back
	iput(mip);
	//creates a file
	int ino = creat_file(newName);
	MINODE * nMip = iget(dev, ino);
	//sets the mode
	nMip->INODE.i_mode = 0120644;
	//printf("ino = %d, mode = %x\n", ino, nMip->INODE.i_mode);
	//sets the oldName into the Iblock
	memcpy((char *)nMip->INODE.i_block, oldName, strlen(oldName) + 1);
	//marks it as dirty and puts it to the inodes
	nMip->INODE.i_size = strlen(oldName);
	nMip->dirty = 1;
	iput(nMip);
	
	return 1;
}

//called from read link
char * myreadlink(MINODE * node, char * link){
	//copies the iblock into the link char *
	memcpy(link, node->INODE.i_block, node->INODE.i_size);
	return link;
}

//called from main
char * read_link(char * pathname, char * link){
	//just checks if pathname exists and isn't a Link file
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
	
	return myreadlink(mip, link);
	
}
