/**************************************************
Instruction to run this code:
1) Always run USR1 before USR2.
2) Run as ./USR2 Port  (port=Integer denoting Port number)
3) Start typing message and press enter to send to USR2
    Simultaneously you will receive message from USR2 also
4) Press Ctrl+C , ou will be asked to quit y/n
    y-->terminate USR1 as well as USR2
    n-->continue the chat program 
****************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <time.h>
#include "g711.c"
#define BACKLOG 10     // how many pending connections queue will hold
#define size 1024
#define interval 1000
pid_t pid,parent_pid;
pa_simple *serve = NULL; //New server for playing or recording
int error;
int sockfd, new_fd,numbytes;  // listen on sock_fd, new connection on new_fd
uint16_t inmsg[size],outbuffer[size];
uint8_t outmsg[size],inbuffer[size];
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/*SIGINT signal handler for parent process*/
void term_p(int sig)
{
    char c;
    if(sig==SIGINT)
    {
        printf("\nDo you want to quit? y/n\n");
       	scanf(" %c",&c);
        if(c=='y'){
            printf("Terminating USR1\n");
            /*Signalling all Child process to rterminate*/
            kill(0,SIGTERM);
            exit(0);
        }
        else if(c=='n'){
            /*Signalling Child process to resume from pause()*/
            kill(pid,SIGCONT);
        }
    }
}
void noth()
{}
/*SIGINT signal handler for child process*/
void term_c(int sig)
{
    char c;
    signal(SIGCONT,noth);
    if(sig==SIGINT)
    {
        /*Sleeping till parent process signals to wakeup*/
        pause();
        printf("Continuing...Start Typing\n");
    }
}
void signal_handler_c(int signal){
	/*If there is incoming steam of bytes keep 
        reading and printing on user stdout*/
		int i;
        if(signal==SIGALRM&&numbytes!=0)
        {
            numbytes=recv(new_fd, inbuffer, sizeof(inbuffer), 0);
            if (numbytes== -1) 
            {
                perror("recv");
                exit(1);
            }
            //Conversion of  ulaw8 to pcm16 (G711 codec)
            for(i=0;i<sizeof(inbuffer)/sizeof(inbuffer[0]);i++)
        	{
            	outbuffer[i]=Snack_Mulaw2Lin(inbuffer[i]);
        	}
            if(numbytes!=0)
            {
				/*Writing to the Server from the buffer*/
            	if (pa_simple_write(serve, outbuffer, sizeof(outbuffer), &error) < 0) 
                {
                    fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
                    close(new_fd);
    				close(sockfd);
    				pa_simple_free(serve);
    				exit(1);
                }
            }
            else if(numbytes==0){printf("USR2's sending end is closed...Exiting USR1's receving end!!!\n");}
        }
        else kill(parent_pid,SIGTERM);
}
void signal_handler_p(int signal){
	/*Taking input from user and sending through socket*/
		int i;
        if(signal==SIGALRM)
        {
			/*Reading From the Server Into the msg Buffer*/
        	if (pa_simple_read(serve, inmsg, sizeof(inmsg), &error) < 0) 
        	{
            	fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            	close(new_fd);
    			close(sockfd);
    			pa_simple_free(serve);
    			exit(1);
        	}
        	//Conversion of pcm16 to ulaw8 (G711 codec)
        	for(i=0;i<sizeof(inmsg)/sizeof(inmsg[0]);i++)
        	{
            	outmsg[i]=Snack_Lin2Mulaw(inmsg[i]);
        	}
            if (send(new_fd, outmsg,sizeof(outmsg),0) == -1)
            {
                perror("send");
                exit(1);
            }
        }
}
int main(int argc,char *argv[])
{	
	/*Attributes of a Sample on the server*/
	static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    
    int ret = 1;
    parent_pid=getpid(); //Parent process pid
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    
	struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
	/*Initializing addrinfo struct*/
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    //IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;//TCP connection
    hints.ai_flags = AI_PASSIVE;    //use my IP
    
	/*Initializing and generating a linked 
    list of all address family specified by hins struct*/
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        /*Generating Socked id(Descriptors)*/
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("USR1: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
		/*Binding socket to the address family*/
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("USR1: bind");
            continue;
        }

        break;
    }
	
    /*Not required any more so freeing it*/
    freeaddrinfo(servinfo);

    if (p == NULL)  {
        fprintf(stderr, "USR1: failed to bind\n");
        exit(1);
    }
	
    /*Listening To the socket from incomming connection request*/
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("USR1: waiting for connections...\n");

    while(1) 
    {  
        sin_size = sizeof their_addr;
        /*Accepting a new connection and generating a new socket bound to same port address*/
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
        }
        else break;
	}
	
    /*Converting ip address from from struct to string and printing*/
    inet_ntop(their_addr.ss_family,
    	get_in_addr((struct sockaddr *)&their_addr),
    	s,sizeof s);
    printf("USR1: got connection from %s\nStart Typing...\n", s);
    timer_t timer_id;
    /*Initial value and interval period of timer*/
    struct itimerspec time;
    time.it_value.tv_sec=0;
    time.it_value.tv_nsec=interval;//Period of 10usec
    time.it_interval = time.it_value;
	pid=fork();
    numbytes  = 1;
    
	/*Child Process*/
    if(pid==0)
    {
    	if (!(serve = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "play", &ss, NULL, NULL, &error))) 
    	{
        	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}
        /*Registering SIGINT signal*/
    	if(signal(SIGINT,term_p)==SIG_ERR){
    		printf("Can't Catch SIGINT\n");
    	}
		/*Setting up server for playing into the connected speakers*/
    	/*Registering SIGALRM signal which will be generated on timer expiration*/
    	if(signal(SIGALRM,signal_handler_c)==SIG_ERR){
    		perror("Signal");
    	}
    	/*Timer creation with real time clock*/
    	if(timer_create(CLOCK_REALTIME,NULL,&timer_id)==-1){
    		perror("Timer_create");
    	}
    	/*Setting Timer value and interval*/
    	if(timer_settime(timer_id,0,&time,NULL)==-1){
    		perror("Timer_settime");
    	}	
      	while(1);
    }
    
	/*Parent Process*/
    else
    {
    	/*Setting up server for recording from the connected microphone*/
    	if (!(serve = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) 
    	{
        	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}
    	/*Registering SIGCONT and SIGINT signals*/
    	if(signal(SIGCONT,noth)==SIG_ERR){
            printf("Can't Catch SIGCONT\n");
        }
    	if(signal(SIGINT,term_c)==SIG_ERR){
    		printf("Can't Catch SIGINT\n");
    	}
    	/*Registering SIGALRM signal which will be generated on timer expiration*/
    	if(signal(SIGALRM,signal_handler_p)==SIG_ERR){
    		perror("Signal");
    	}
    	/*Timer creation with real time clock*/
    	if(timer_create(CLOCK_REALTIME,NULL,&timer_id)==-1){
    		perror("Timer_create");
    	}
    	/*Setting Timer value and interval*/
    	if(timer_settime(timer_id,0,&time,NULL)==-1){
    		perror("Timer_settime");
    	}
    	while(1);
    }
    
	/*Closing the socket Descriptor freeing the Server*/
   	ret = 0;
	finish:
	close(new_fd);
    close(sockfd);
    pa_simple_free(serve);
    return ret;
}

