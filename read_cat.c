//called from read_file()
int myread(int fd, char * buf, int nbytes){
	int count = 0;
	OFT * oftp = running->fd[fd];
	MINODE * mip = oftp->mptr;
	char readbuf[BLKSIZE];
	
	int avail = mip->INODE.i_size - oftp->offset;
	char *cq = buf;
	//While still have bytes to add and space available
	while(nbytes && avail){
		int lbk = oftp->offset / BLKSIZE;
		int startByte = oftp->offset % BLKSIZE;
		int blk;
		//sets blk to the i block value
		if (lbk < 12){
			blk = mip->INODE.i_block[lbk];
		}
		//if it is in iblock[12] we do an indirect block pattern
		else if (lbk >= 12 && lbk < 256 + 12){
			//indirect blocks
			char indirbuf[BLKSIZE];
			get_block(mip->dev, mip->INODE.i_block[12],indirbuf);
			int * ip = indirbuf;
			//offset the lbk so it starts at 0 and assign blk to the ip
			ip += (lbk - 12);
			blk = *ip;
		}
		else{
			//	double indirect blocks
			char indirbuf[BLKSIZE];
			get_block(mip->dev, mip->INODE.i_block[13],indirbuf);
			int * ip = indirbuf;
			//the lbk leftover offset helps offset the lbk value and we assign to ip
			ip += (lbk - 13 - (lbk - 12) % 256) / 256;

			char dindirbuf[BLKSIZE];
			int * dip;
			//the local offset once in correct indir buf calculates how far over the buf has to be
			get_block(mip->dev, *ip, dindirbuf);
			dip = dindirbuf;
			dip = dip + ((lbk - 12) % 256);
			//get values for dindir

			//blk is wherever the dip is at
			blk = *(dip);

			//getchar();
		}
		//once we have the blk that we are using
		get_block(mip->dev, blk, readbuf);
		
		char *cp = readbuf + startByte;
		int remain = BLKSIZE - startByte;
		
		//assigns a value min that is the amount that this block will hold after read
		int min;
		if (avail > nbytes){
			min = nbytes;
		}
		else min = avail;
		//while there is still room
		if (remain > 0){
			//Makes a readbytes that copies over the max amount of bytes that is can into memory
			int readbytes;
			if (min > remain){
				readbytes = remain;
			}
			else{
				readbytes = min;
			}
			memcpy(cq, cp, readbytes);
			//cp and cq keep track of where in the buf we are
			cq += readbytes;
			cp += readbytes;
			oftp->offset += readbytes;
			count += readbytes;
			avail -= readbytes;
			nbytes -= readbytes;
			remain -= readbytes;
		}
		/*
		while (remain > 0){
			*cq++ = *cp++;
			oftp->offset++;
			count++;
			avail--; nbytes--; remain--;
			if(nbytes <= 0 || avail <= 0){
				break;
			}
		}*/
	}
	//printf("myread: read %d char from file descriptor %d\n", count, fd);
	return count;

}

//read from main
int read_file(int fd, char * buf, int nbytes){
	//basically just maintains that all inputs are valid
	if(running->fd[fd] == 0){
		printf("fd: %d is not open\n", fd);
		return -1;
	}
	int mode = running->fd[fd]->mode;
	if(mode != 0 && mode != 2){
		printf("file discriptor %d is not open for read or right\n", mode);
		return -1;
	}
	return myread(fd, buf, nbytes);
}

// Called from main
int my_cat(char * filename){
	//insert jpg of my cat Homer
	char mybuf[1024], dummy = 0;
	int n;
	//open the file
	int fd = open_file(filename, 0);
	//while it still has more to write
	while (n = read_file(fd, mybuf,1024)){
		//printf("read %d bytes\n",n);
		//leaves current spot empty
		mybuf[n] = 0;
		//puts chars to the sreen one at a time
		for(int i = 0; i < n;++i){
			if(mybuf[i] == '\n'){
				putchar('\r');
			}
			putchar(mybuf[i]);
		}

	}
	//closes the file
	close_file(fd);
}
