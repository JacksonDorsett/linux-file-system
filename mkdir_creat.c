
//called from mk_dir
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
	//initializes all of the INODE calues tho their starting pints
	ip->i_mode = 040755;
	ip->i_uid = running->uid;
	ip->i_gid = running->gid;
	ip->i_size = BLKSIZE;
	ip->i_links_count = 2;
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
	ip->i_block[0] = bno;
	//runs thru and sets all iblocks to 0
	for(int i = 1; i < 15; i++){
		ip->i_block[i] = 0;
	}
	//sets the dirty, then puts to the block
	mip->dirty = 1;
	iput(mip);
	

	bzero(buf, BLKSIZE);
	//initializes the DIR values
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
	// now that the base has been set up add the ino to the node in enter name
	enter_name(pip, ino, name);
}

//called by functions: 
int enter_name(MINODE *pip, int ino, char *name){
	// creating base line for a dir
	INODE * ip = &pip->INODE;
	DIR * dp; 
	char * cp;
	char buf[BLKSIZE];
	int i;
	int need_length = 4*((8 + strlen(name) + 3)/4);
	int ideal_len;
	cp = buf;
	//searches thru the parents i blocks
	for (i = 0; i < 12; ++i){
		if (ip->i_block[i] == 0) break;
		bzero(buf, BLKSIZE);
		get_block(dev, ip->i_block[i], buf);
		
		dp = (DIR *)buf;
		
		
		printf("step to LAST entry in data block %d\n",ip->i_block[i]);
		while (cp + dp->rec_len < buf + BLKSIZE){
			//printf("%s\n", dp->name);
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		//takes the last item shortest rec_len and allows remain to be large
		int remain = dp->rec_len - 4*((8 + dp->name_len + 3)/4);
		dp->rec_len = 4*((8 + dp->name_len + 3)/4);
		//printf("%s rec_len: %d", dp->name, dp->rec_len); 
		//printf("remain: %d\n", remain);
		cp += dp->rec_len;
		dp = (DIR *)cp;
		// This ^^^ dp is an empty dir

		//if there is enough room left over
		if (remain >= need_length){
			//printf("cur name: %s\n", dp->name);	
			dp->inode = ino;
			short rlen = remain;
			//printf("rlen: %d\n", rlen);
			dp->rec_len = rlen;
			dp->name_len = strlen(name);
			
			strcpy(dp->name, name);
			//printf("%s\n\n", dp->name);
			//writes the info to the block here
			put_block(dev, ip->i_block[i], buf);
			return 1;
		}
		
		put_block(dev, ip->i_block[i], buf);
	}
	//probably unneeded this part is
	//once done running thru the for loop if it hasn't placed
	//printf("cur name: %s\n", dp->name);	
	int bno = balloc(dev);
	//set an empty block and 
	ip->i_block[i] = bno;
	dp->inode = ino;
	short rlen = BLKSIZE - 4*((8 + strlen(name) + 3) / 4);
	//printf("rlen: %d\n", rlen);
	dp->rec_len = rlen;
	dp->name_len = strlen(name);
	
	strcpy(dp->name, name);
	printf("%s\n\n", dp->name);
	//somehow allocates it to the dev
	put_block(dev, bno, buf);
}

//called from main
int make_dir(char * pathname){
	MINODE *start;
	char * buf[BLKSIZE];
	//determining if starting from root or the cwd
	if (pathname[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	//makes parent and child names
	strcpy(buf, pathname);
	char parent[128]; 
	char child[128]; 
	strcpy(parent,dirname(buf));
	strcpy(buf, pathname);
	strcpy(child, basename(buf));
	//printf("parent: %s, child: %s\n",parent,child);
	
	int pino = getino(parent);
	//printf("pino: %d\n", pino);
	
	MINODE * pip = iget(dev, pino);

	if(pino == 0){
		printf("Directory %s does not exist\n", parent);
		return 0;
	}
	//if not a dir then obviously dont make it something went wrong
	if (!S_ISDIR(pip->INODE.i_mode)){
      printf("not a directory\n");
      iput(pip);
      return ;
   }
	//search for the child if there it already exists and this will fail
	if(search(pip, child) != 0){
		printf("mkdir failed: %s already exists\n", child);
		return 0;
	}
	//call the my mkdir
	mymkdir(pip, child);
	//start the link update the time and mark as dirty
	pip->INODE.i_links_count++;
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	//put onto the block
	iput(pip);
	
}

//called from creat_file
int my_creat(MINODE * pip, char* name){
	MINODE *mip;
	char buf[BLKSIZE];
	int ino = ialloc(dev);

	int p;
	int pino = findino(pip, &p);
	
	printf("ino = %d\n", ino);
	mip = iget(dev,ino);
	INODE *ip = &mip->INODE;
	//creating a basic file
	ip->i_mode = 0100644;
	ip->i_uid = running->uid;
	ip->i_gid = running->gid;
	ip->i_size = 0;
	ip->i_links_count = 1;
	ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);

	//clears out the inodes iblocks
	for(int i = 1; i < 15; i++){
		ip->i_block[i] = 0;
	}
	//mark as dirty for when you push to block
	mip->dirty = 1;
	iput(mip);
	//enter enter name which make room for the file on the inode somewhere
	enter_name(pip, ino, name);
	
	return ino;
}

//called by main
int creat_file(char* pathname){
	MINODE *start;
	char * buf[BLKSIZE];
	//determining if pathname starts from the root or cwd
	if (pathname[0] == '/'){
	   start = root;
	   dev = root->dev;
	}
	else{
		start = running->cwd;
		dev = running->cwd->dev;
	}
	//makes child and parent name
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
	//return if the parent doesn exist
	if(pino == 0){
		printf("Directory %s does not exist\n", parent);
		return 0;
	}
	//If parent is not a dir return
	if (!S_ISDIR(pip->INODE.i_mode)){
      printf("not a directory\n");
      iput(pip);
      return ;
   }
	//search to see if name child already exists
	if(search(pip, child) != 0){
		printf("mkdir failed: %s already exists\n", child);
		return 0;
	}
	//call mycreate
	int rino = my_creat(pip, child);
	//update time mark as dirty and put to block
	pip->INODE.i_atime = time(0L);
	pip->dirty = 1;
	
	iput(pip);
	
	return rino;
}
