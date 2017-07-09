#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>

#define PTHREAD_NUM 10
#define BUFFER_SIZE 128

sem_t sem;

void dieWithError(char *msg)
{
	printf("[-]ERROR: %s\n", msg);
	exit(0);
}

void *writer(void *arg)
{
	int fd, string_num = 1;
	char buffer[BUFFER_SIZE];
	time_t rawtime; 
	struct tm *timeinfo;

	while(1)
	{
		//decrements the semaphore
		sem_wait(&sem);
		time(&rawtime);   
		timeinfo = localtime(&rawtime);
		if ((fd = open("file1", O_CREAT|O_WRONLY|O_APPEND, 0744)) < 0)
			dieWithError("Can't open the file\n");

		//writes the time and the line number in the buffer and then writes the buffer to a file
		sprintf(buffer, "string number:%d %d:%d:%d\n", string_num, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		printf("Writer thread[%x]\n", pthread_self());
		if (write(fd, buffer, strlen(buffer)) < 0)
			dieWithError("write() failed"); 
		string_num++;
		//increments the semaphore
		sem_post(&sem);
		sleep(1);
	}
	pthread_exit(0);	 
}

void *reader(void *arg)
{
	int fd, buffer_read = 0, buffer_written = 0;
	char buffer[BUFFER_SIZE];

	//open file for reading
	if ((fd = open("file1", O_CREAT|O_RDONLY, 0744)) < 0)
		dieWithError("Can't open the file\n");

	while(1)
	{
		//decrements the semaphore
		sem_wait(&sem);

		//Displays the contents of the file
		printf("\nReader thread[%x]\n", pthread_self());
		while ((buffer_read = read(fd, buffer, BUFFER_SIZE)) > 0)
		{
		    buffer_written = write (1, buffer, buffer_read);
		    if (buffer_written != buffer_read)
		        dieWithError("write() failed\n");
		}	
		//increments the semaphore
		sem_post(&sem);
		sleep(5);
	}
	close(fd);	
	pthread_exit(0);   	
}

int main(void)
{
	pthread_t reader_tid[PTHREAD_NUM];
	pthread_t writer_tid;
	int i = 0;

	//initialize semaphore
	if (sem_init(&sem, 0 , 5) < 0)
		dieWithError("sem_init() failed");
	
	//creates 1 writer pthread and 10 reader threads
	if (pthread_create(&writer_tid, NULL, writer, NULL) < 0)
		dieWithError("Can't create writer thread\n");
	for (i = 0; i < PTHREAD_NUM; i++)
		if (pthread_create(&reader_tid[i], NULL, reader, NULL) < 0)
			dieWithError("Can't create reader thread\n");

	if (pthread_join(writer_tid, NULL) < 0)
		dieWithError("Join writer failed\n");
	for (i = 0; i < PTHREAD_NUM; i++)
		if (pthread_join(reader_tid[i], NULL) < 0)
			dieWithError("Join reader failed\n");
	sem_destroy(&sem);

	return 0;
}
