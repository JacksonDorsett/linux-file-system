
int rmdir(char *pathname){
    //getting ino of the pathname
    int ino  = getino(&dev, pathname); 
	if (ino == 0){
		printf("%s does not exist\n", pathname);
		return;
	}
    //getting minode pointer
    MINODE *mip = iget(dev, ino);
    int super_user = 1;
	int same_uids = 1;
    //worry about Later
    //checks to see if the directory is a directory
    //int isDir = mip->INODE.i_mode == 0x41ED; //cause the 4 is important
    int isDir = (S_ISDIR(mip->INODE.i_mode));
    int isBusy = (mip->refcount)>1;
    int isEmpty = (mip->INODE.i_links_count == 2);
    //Try looking for a way to traverse the blocks to manually search later
    if(!isDir || isBusy || !isEmpty){
        printf("Either not a Dir, is busy or isn't empty");
        iput(mip);
        return -1;
    }
    //Assuming the check goes through
    for (int i=0; i<2;i++){
        if(mip->INODE.iblock[i]==0) //is null
            break;
        bdealloc(mip->dev, mip->INODE.i_Block[i]);
        idealloc(mip->dev, mip->ino);
        iput(mip);
    }
    idealloc(mip->dev, mip->ino);
    iput(mip);
    //Getting Parent ID's
    /*char * buf[1024];
    strcpy(buf, pathname);
    char parent[128]; 
	char child[128]; 
    strcpy(parent, dirname(buf));
    strcpy(buf, pathname);
    strcpy(child, buf);*/
    char * name;
    int pino = findino(mip, ino);
    MINODE * pmip = iget(mip->dev, pino);
    findmyname(pmip, ino, name);
    rm_child(pmip, name);

}


int rm_child(MINODE *parent, char *name){

}