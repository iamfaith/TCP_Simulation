#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFF_SIZE 1024

typedef struct data
{
	int len;
	char buf[1024];
};

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	int i;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	pid_t fpid;
    char buffer1[BUFF_SIZE];
    char buffer2[BUFF_SIZE];
	
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
	
	fpid=fork();
	if (fpid < 0)
	{
		return 0;
	}
	else if (fpid == 0)
	{
		while (1)
		{
			bzero(buffer1,BUFF_SIZE);
			fgets(buffer1,BUFF_SIZE,stdin);
			n = write(sockfd,buffer1,strlen(buffer1));
			if (n < 0) 
				 error("ERROR writing to socket");
		}
	}
	else if (fpid > 0)
	{		
		while (1)
		{
			bzero(buffer2,BUFF_SIZE);
			n = read(sockfd,buffer2,BUFF_SIZE);
			if (n < 0) 
				 error("ERROR reading from socket");
			printf("%s\n",buffer2);
			fflush(stdout);
			
		}
	}
    close(sockfd);
    return 0;
}
