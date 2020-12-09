//write function called from write_file
int mywrite(int fd, char buf[], int nbytes){

    int count =0;
    char wbuf[BLKSIZE];
	OFT * oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
	INODE *ip   = &mip->INODE;
    //while there are still bytes to add
    while(nbytes){
        //getting the compute logical blockand the compute start byte
        int lbk = oftp->offset/BLKSIZE;
        int start = oftp->offset % BLKSIZE;
        int blk = 0;
        //if it can be fit in the 12 blks
        if (lbk<12){
            if(ip->i_block[lbk]==0){
                mip->INODE.i_block[lbk]=balloc(mip->dev);
               	clear_block(dev,mip->INODE.i_block[lbk]);
            }
            //gives the physical block number
            blk = mip->INODE.i_block[lbk];  //blk should be a disk block now
        }
        else if(lbk>=12 && lbk < 256 + 12){ //for indirect blocks
       		char ibuf[BLKSIZE];
            //alloc block if it 0
            if(mip->INODE.i_block[12]==0){
                mip->INODE.i_block[12] = balloc(mip->dev);
                // zeroes out the block
                clear_block(dev, mip->INODE.i_block[12]);
                
                //basically just need to make sure there is a block in the i_block
            }
            //get i_block[12] into a int ibuf[256]
            
            // = mip->INODE->i_block[12];
            int * p = ibuf;
            get_block(dev, ibuf, BLKSIZE);
            //adjusts the offset and assigns a blk value
            blk = *(p+(lbk - 12));
            if (blk==0){
                //if it = 0 then we need to balloc
             	blk = balloc(mip->dev);
                *(p + lbk) = blk;
               	put_block(dev, blk, BLKSIZE);
               	clear_block(dev, blk);

               	
            }
            //else needs to go to the next block
        }
        //this is for when it need double block dealloc
        else{
        	printf("lbk = %d\n", lbk);
            //Not implimented but should be likek double the amount.
            if(mip->INODE.i_block[13] == 0){
            	mip->INODE.i_block[13] = balloc(dev);
            	clear_block(dev, mip->INODE.i_block[13]);
            }
            //	double indirect blocks
			char indirbuf[BLKSIZE];
			get_block(mip->dev, mip->INODE.i_block[13],indirbuf);
			int * ip = indirbuf;
            //same offsetting to find truly where ip should be placed (first ref)
			ip += (lbk - 13 - (lbk - 12) % 256) / 256;
			if(*ip == 0){
				*ip = balloc(dev);
				clear_block(dev, *ip);
				printf("indir block allocated\n");
				put_block(dev, mip->INODE.i_block[13], indirbuf);
				
			}
            //second int * finds indexing for the second part
			char dindirbuf[BLKSIZE];
			int * dip;
			get_block(mip->dev, *ip, dindirbuf);
			dip = dindirbuf;
            //finds offset for the block wher to start writing
			dip = dip + ((lbk - 12) % 256);
			//get values for dindir
			if(*dip == 0){
				*dip = balloc(dev);
				clear_block(dev, *dip);
				printf("dindir block allocated\n");
				put_block(dev, *ip, dindirbuf);
			}
			blk = *(dip);
			printf("blk = %d\n", blk);
			//getchar();
            
        }
        get_block(mip->dev, blk, wbuf); //read disk block into wbuf[]
        char *cp = wbuf + start;
        int remain = BLKSIZE - start;
        //while space remains
        while (remain>0){
        	//printf("remain: %d\n", remain);
            //update of all the bytes, didn't have time to not do byte by byte
            *cp++ = *buf++;
            count++;
            nbytes--;
            remain--;
            oftp->offset++;
            if(oftp->offset > ip->i_size){
                mip->INODE.i_size++;
            }
            if(nbytes<=0){
                break;
            }
        }
        put_block(mip->dev, blk, wbuf);
    }
    //dirty because it updated and will notify
    mip->dirty = 1;
    //put back
    iput(mip);
    return count;
}

//called from main
int write_file(int fd, char buf[BLKSIZE], int nbytes){
    //basic checks, is open and is actually some kind of write node
	if (running->fd[fd] == 0){
		printf("fd %d is not open\n", fd);
		return -1;
	}
    int mode = running->fd[fd]->mode;
    if (mode != 1 && mode != 2 && mode != 3){ //checks for invalid indices
		printf("%d is an invalid fd\n", fd);
		return;
	}
    return mywrite(fd, buf, nbytes);
}

// copies from one spot to another, called from main
int my_cp(char * src, char * dst){
    char buf[BLKSIZE];
    int fd,gd;
    int n;
    //open the src
    fd=open_file(src,0);
    
    if(fd == -1){
    	printf("src file could not be opened\n");
    	return -1;
    }
    //create a file if there is none already
    int ino = getino(dst);
    if (ino == 0){
    	creat_file(dst);
    }
    //fd=open src for READ
    gd=open_file(dst,1);
    //gd=open dst for WR|CREAT;
    //while the readfile is being done write a file
    while(n=read_file(fd, buf, BLKSIZE)){
        write_file(gd, buf, n); //notice the n in write
    }
    //close both of the files
    close_file(fd);
    close_file(gd);
}
