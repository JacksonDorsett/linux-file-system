
int myread(int fd, char * buf, int nbytes){
	int count = 0;
	OFT * oftp = running->fd[fd];
	MINODE * mip = oftp->mptr;
	char readbuf[BLKSIZE];
	
	int avail = mip->INODE.i_size - oftp->offset;
	char *cq = buf;
	
	while(nbytes && avail){
		int lbk = oftp->offset / BLKSIZE;
		int startByte = oftp->offset % BLKSIZE;
		int blk;

		if (lbk < 12){
			blk = mip->INODE.i_block[lbk];
		}
		else if (lbk >= 12 && lbk < 256 + 12){
			//indirect blocks
			char indirbuf[BLKSIZE];
			get_block(mip->dev, mip->INODE.i_block[12],indirbuf);
			int * ip = indirbuf;
			
			ip += (lbk - 12);
			blk = *ip;
		}
		else{
			//	double indirect blocks
			char indirbuf[BLKSIZE];
			get_block(mip->dev, mip->INODE.i_block[13],indirbuf);
			int * ip = indirbuf;
			ip += (lbk - 13) / 256;
			printf("ip: %d\n", (lbk-13)/256);
			getchar();
			get_block(mip->dev, *ip, indirbuf);
			ip = indirbuf;
			blk = (lbk - 12) % 256;
			printf("blk = %d\n", blk);
			getchar();
		}
		
		get_block(mip->dev, blk, readbuf);
		
		char *cp = readbuf + startByte;
		int remain = BLKSIZE - startByte;
		while (remain > 0){
			*cq++ = *cp++;
			oftp->offset++;
			count++;
			avail--; nbytes--; remain--;
			if(nbytes <= 0 || avail <= 0){
				break;
			}
		}
	}
	printf("myread: read %d char from file descriptor %d\n", count, fd);
	return count;

}

int read_file(int fd, char * buf, int nbytes){
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

int my_cat(char * filename){
	char mybuf[1024], dummy = 0;
	int n;
	
	int fd = open_file(filename, 0);

	while (n = read_file(fd, mybuf,1024)){
		printf("read %d bytes\n",n);
		mybuf[n] = 0;
		
		for(int i = 0; i < n;++i){
			if(mybuf[i] == '\n'){
				putchar('\r');
			}
			putchar(mybuf[i]);
		}

	}
	close_file(fd);
}
