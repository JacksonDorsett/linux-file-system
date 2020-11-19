
int mymkdir(MINODE * pip, char * name){
	MINODE *mip;
	char buf[BLKSIZE];
	int ino = ialloc(dev);
	int bno = balloc(dev);
	int p;
	int pino = findino(pip, &p);
	
	printf("ino = %d, bno = %d\n", ino, bno);
	mip = iget(dev,ino);
	INODE *ip = &mip->INODE;
	
	ip->i_mode = 040755;
	ip->i_uid = running->uid;
	ip->i_gid = running->gid;
	ip->i_size = BLKSIZE;
	ip->i_links_count = 2;
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
	ip->i_block[0] = bno;
	
	for(int i = 1; i < 15; i++){
		ip->i_block[i] = 0;
	}
	mip->dirty = 1;
	iput(mip);
	

	bzero(buf, BLKSIZE);
	
	DIR *dp = (DIR *)buf;
	dp->inode = ino;
	dp->rec_len = 12;
	dp->name_len = 1;
	dp->name[0] = '.';
	
	dp = (char *) dp + 12;
	dp->inode = pino;
	dp->rec_len = BLKSIZE - 12;
	dp->name_len = 2;
	dp->name[0] = dp->name[1] = '.';
	put_block(dev, bno, buf);
	
	enter_name(pip, ino, name);
	
}

int enter_name(MINODE *pip, int ino, char *name){
	INODE * ip = &pip->INODE;
	DIR * dp; 
	char * cp;
	char buf[BLKSIZE];
	int i;
	int need_length = 4*((8 + strlen(name) + 3)/4);
	int ideal_len;
	cp = buf;
	
	for (i = 0; i < 12; ++i){
		if (ip->i_block[i] == 0) break;
		bzero(buf, BLKSIZE);
		get_block(dev, ip->i_block[i], buf);
		
		dp = (DIR *)buf;
		
		
		printf("step to LAST entry in data block %d\n",ip->i_block[i]);
		while (cp + dp->rec_len < buf + BLKSIZE){
			printf("%s\n", dp->name);
			
			
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		int remain = dp->rec_len - 4*((8 + dp->name_len + 3)/4);
		dp->rec_len = 4*((8 + dp->name_len + 3)/4);
		printf("%s rec_len: %d", dp->name, dp->rec_len); 
		printf("remain: %d\n", remain);
		cp += dp->rec_len;
		dp = (DIR *)cp;
		

		printf("remain: %d\n", remain);
		if (remain >= need_length){
			printf("cur name: %s\n", dp->name);	
			dp->inode = ino;
			short rlen = remain;
			printf("rlen: %d\n", rlen);
			dp->rec_len = rlen;
			dp->name_len = strlen(name);
			
			strcpy(dp->name, name);
			printf("%s\n\n", dp->name);
			put_block(dev, ip->i_block[i], buf);
			return 1;
		}
		
		put_block(dev, ip->i_block[i], buf);
	}
	
	printf("cur name: %s\n", dp->name);	
	int bno = balloc(dev);
	ip->i_block[i] = bno;
	dp->inode = ino;
	short rlen = BLKSIZE - 4*((8 + strlen(name) + 3) / 4);
	printf("rlen: %d\n", rlen);
	dp->rec_len = rlen;
	dp->name_len = strlen(name);
	
	strcpy(dp->name, name);
	printf("%s\n\n", dp->name);
	
	put_block(dev, bno, buf);
	
	
}

int make_dir(char * pathname){
	MINODE *start;
	char * buf[BLKSIZE];
	if (pathname[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	strcpy(buf, pathname);
	char parent[128]; 
	char child[128]; 
	strcpy(parent,dirname(buf));
	strcpy(buf, pathname);
	strcpy(child, basename(buf));
	printf("parent: %s, child: %s\n",parent,child);
	
	int pino = getino(parent);
	printf("pino: %d\n", pino);
	
	MINODE * pip = iget(dev, pino);

	if(pino == 0){
		printf("Directory %s does not exist\n", parent);
		return 0;
	}
	
	if (!S_ISDIR(pip->INODE.i_mode)){
      printf("not a directory\n");
      iput(pip);
      return ;
   }
	
	if(search(pip, child) != 0){
		printf("mkdir failed: %s already exists\n", child);
		return 0;
	}
	
	mymkdir(pip, child);
	
	pip->INODE.i_links_count++;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	
	iput(pip);
	
}

int my_creat(MINODE * pip, char* name){
	MINODE *mip;
	char buf[BLKSIZE];
	int ino = ialloc(dev);

	int p;
	int pino = findino(pip, &p);
	
	printf("ino = %d\n", ino);
	mip = iget(dev,ino);
	INODE *ip = &mip->INODE;
	
	ip->i_mode = 0100644;
	ip->i_uid = running->uid;
	ip->i_gid = running->gid;
	ip->i_size = 0;
	ip->i_links_count = 1;
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);

	
	for(int i = 1; i < 15; i++){
		ip->i_block[i] = 0;
	}
	mip->dirty = 1;
	iput(mip);
	
	enter_name(pip, ino, name);
	
	return ino;
}

int creat_file(char* pathname){
	MINODE *start;
	char * buf[BLKSIZE];
	if (pathname[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	strcpy(buf, pathname);
	char parent[128]; 
	char child[128]; 
	strcpy(parent,dirname(buf));
	strcpy(buf, pathname);
	strcpy(child, basename(buf));
	printf("parent: %s, child: %s\n",parent,child);
	
	int pino = getino(parent);
	printf("pino: %d\n", pino);
	
	MINODE * pip = iget(dev, pino);

	if(pino == 0){
		printf("Directory %s does not exist\n", parent);
		return 0;
	}
	
	if (!S_ISDIR(pip->INODE.i_mode)){
      printf("not a directory\n");
      iput(pip);
      return ;
   }
	
	if(search(pip, child) != 0){
		printf("mkdir failed: %s already exists\n", child);
		return 0;
	}
	
	int rino = my_creat(pip, child);
	
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	
	iput(pip);
	
	return rino;
}
