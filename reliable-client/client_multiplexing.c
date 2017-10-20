#define _GNU_SOURCE
#include <sys/types.h>  

#include <assert.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <string.h>  
#include <stdlib.h>  
#include <fcntl.h>  

#ifdef WIN32
   #include <ws2tcpip.h>
   #include <winsock.h>
   #include <windows.h>
#else
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <poll.h>
   #include <arpa/inet.h>  
   #include <netinet/tcp.h>
#endif

#define SEP ":"
#define BUFFER_SIZE 1024 
#define INDEX_SIZE 200
#define WRITE_SIZE (BUFFER_SIZE - INDEX_SIZE)
#define ACK "ACK"
#define NAK "NAK"

typedef struct Node
{
	int index;
	char buf[BUFFER_SIZE];
	struct Node* next;
}Node ;

typedef struct LinkedList
{
	Node* head;
	Node* tail;
	int size;
} LinkedList;

void initialize(LinkedList* list)
{
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

Node*  seek(LinkedList *list, int index)
{
   Node* iter = list->head;
   while(iter != NULL)
   {
       if(iter->index == index)
           return iter;
       iter = iter->next;
   }
   return NULL; // element with code doesn't exist
}

void printList(LinkedList *list)
{
	Node* iter = list->head;
   	while(iter != NULL)
   	{
		printf("%s\n", iter->buf);
       	iter = iter->next;
   	}
   	if (list->tail != NULL)
   		printf("tail:%s\n", list->tail->buf);
}

void removeNode(LinkedList *list, int index) 
{
   Node* iter = list->head;
   //for head 
   if (iter != NULL && iter->index == index) 
   {
   		if (list->head == list ->tail)
   		{
   			list->tail= iter->next;		
		}
		list->head = iter->next;
   		free(iter);
   		list->size --;
   		return ;
   }
   Node* before = iter;
   while(iter != NULL)
   {
       if(iter->index == index) 
	   {
	   		//for tail
	   		if (list->tail == iter) 
	   			list->tail = iter->next; 
       		before->next = iter->next;
       		free(iter);
   			list->size --;
			break;       	
	   }
	   before = iter; 
       iter = iter->next;
   }
}

void addNode(LinkedList *list, int index, char buf[])
{
    // set up the new node
    Node* node = (Node*)malloc(sizeof(Node));
    node->index = index;
    strncpy(node->buf, buf, sizeof(node->buf));
    node->next = NULL;

    if(list->head == NULL){ // if this is the first node, gotta point head to it
        list->head = node;
        list->tail = node;  // for the first node, head and tail point to the same node
    }else{
        list->tail->next = node;  // append the node
        list->tail = node;        // point the tail at the end
    }
    list->size ++;
}

void testList() 
{
	LinkedList *testList;
	testList = (LinkedList*)malloc(sizeof(LinkedList));
	initialize(testList);
	addNode(testList, 1, "1");
	addNode(testList, 2, "2");
	addNode(testList, 3, "3");
	addNode(testList, 5, "5");
	removeNode(testList, 3);
	removeNode(testList, 2);
	removeNode(testList, 1);
//	removeNode(testList, 5);
	printList(testList);
}

char* subString(char* buf, int start, int end) {
	int size = strlen(buf);
	if (end > size - 1)
		end = size - 1;
	
	int subSize = end - start + 1;
	char* subbuff = malloc(subSize);
   	memcpy( subbuff, &buf[start], subSize - 1);
	subbuff[subSize] = '\0';
//	printf( "sub[%s][%s]%d %d", subbuff, buf, start, end);  	
	return subbuff;
}

int main( int argc, char* argv[] )  
{  
	testList();//for test list

    if( argc <= 2 )  
    {  
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );  
        return 1;  
    }  
    const char* ip = argv[1];  
    int port = atoi( argv[2] );  
  
    struct sockaddr_in server_address;  
    bzero( &server_address, sizeof( server_address ) );  
    server_address.sin_family = AF_INET;  
    inet_pton( AF_INET, ip, &server_address.sin_addr );  
    server_address.sin_port = htons( port );  
  
    int sockfd = socket( PF_INET, SOCK_STREAM, 0 );  
    assert( sockfd >= 0 );  
//    int i = 1;
//	setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
    if ( connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) ) < 0 )  
    {  
        printf( "connection failed\n" );  
        close( sockfd );  
        return 1;  
    }  
  
    struct pollfd fds[2];  
    fds[0].fd = fileno(stdin);  
    fds[0].events = POLLIN;  
    fds[0].revents = 0;  
    fds[1].fd = sockfd;  
    fds[1].events = POLLIN | POLLRDHUP;  
    fds[1].revents = 0;  
    char read_buf[BUFFER_SIZE];
    char write_buf[WRITE_SIZE];
    char index_buf[INDEX_SIZE];
    char snd_buf[BUFFER_SIZE];
    int pipefd[2];  
    int ret = pipe( pipefd );  
    assert( ret != -1 );  
  
  	int msgIndex = 0;
  	int curRecvInx = 0; 
  	
	LinkedList *sndList;
	sndList = (LinkedList*)malloc(sizeof(LinkedList));
	initialize(sndList);

	LinkedList *recvList;
	recvList = (LinkedList*)malloc(sizeof(LinkedList));
	initialize(recvList);
		
    while( 1 )  
    {  
        ret = poll( fds, 2, -1 );  
        if( ret < 0 )  
        {  
            printf( "poll failure\n" );  
            break;  
        }  
  
        if( fds[1].revents & POLLRDHUP )  
        {  
            printf( "server close the connection\n" );  
            break;  
        }  
        else if( fds[1].revents & POLLIN )  
        {  
            memset( read_buf, '\0', BUFFER_SIZE );  
            recv( fds[1].fd, read_buf, BUFFER_SIZE-1, 0 );  
            int i = strcspn (read_buf, SEP);
            if (i < 0)
            	printf("Error, ask for resend");
            else 
			{
				char* indexString = subString(read_buf, 0, i); 
				int index = atoi( indexString); 
				char* recvmsg =  subString(read_buf, i + 1, BUFFER_SIZE); 
            	printf( "recv: %s %d\n", recvmsg, index );  
        	}
        }  

        if( fds[0].revents & POLLIN )  
        {  
          	bzero(write_buf,WRITE_SIZE);
          	bzero(index_buf,INDEX_SIZE);
          	bzero(snd_buf,BUFFER_SIZE);
          	
          	sprintf(index_buf, "%d:", msgIndex);
          	fgets(write_buf,WRITE_SIZE,stdin);
          	sprintf(snd_buf, "%s%s", index_buf, write_buf);
          	
			printf( "msg:[%s]\n", snd_buf );  
        	int n = write(sockfd,snd_buf,strlen(snd_buf));
			if (n < 0) 
				 error("ERROR writing to socket");
			else
				msgIndex ++; 
//            ret = splice( 0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );  
//            ret = splice( pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );  
        }  
        
    }  
    shutdown(sockfd, SHUT_WR);  
    close( sockfd );  
    return 0;  
} 
