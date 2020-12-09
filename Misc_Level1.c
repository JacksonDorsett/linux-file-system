
int mystat(char * pathname){
    struct stat myst;
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    INODE *ip=mip->INODE;
    myst.st_dev=dev;
    myst.st_ino=ino;
    myst.st_mode=ip->i_mode;
    myst.st_uid=ip->i_uid;
    myst.st_gid=ip->i_gid;
    myst.st_nlink=ip->links_count;
    myst.st_size=ip->i_size;
    myst.st_atime=ip->i_atime;
    myst.st_mtime=ip->i_mtime;
    myst.st_ctime=ip->i_ctime;
    myst.st_blksize=1024;
    int alloc_size   = 0;
	int i;
	for (i = 0; i < 15; i++){
		if (ip->i_block[i] != 0){
			alloc_size++;
		}
	}
	myst.st_blocks  = alloc_size; 
    iput(mip);
}

//not sure exactly how to work this
int mychmod(char * pathname, int mode){
    //rough start
    int ino = getino(pathname);
    MINODE * mip = iget(dev, ino);
    mip->INODE.i_mode |= mode;
    mip->dirty = 1;
    iput(mip);
}

int myutime(char * pathname){
    int ino = getino(pathname);
    MINODE * mip = iget(dev, ino);
    mip->INODE.i_mtime = mip->INODE.i_atime = mip->INODE.i_ctime = time(0);
    mip->dirty=1;
    iput(mip);
}