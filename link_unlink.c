

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
