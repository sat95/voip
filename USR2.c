/**************************************************
Instruction to run this code:
1) Always run USR2 after USR1.
2) Run as ./USR2 IP Port  (ip=string denoting ip adress of USR1(192.168.0.102)
                            port=String denoting Port number(9999))
3) Start typing message and press enter to send to USR1
    Simultaneously you will receive message from USR1 also
4) Press Ctrl+C , you will be asked to quit y/n
    y-->terminate USR2 as well as USR1
    n-->continue the chat program 
****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#define size 400
#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
pid_t pid;
int fd;
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
            printf("Terminating USR2\n");
            /*Signalling all Child process to rterminate*/
            kill(0,SIGTERM);
            close(fd);
            exit(0);
        }
        else{
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
    if(sig==SIGINT)
    {
         /*Sleeping till parent process signals to wakeup*/
        pause();
        printf("Continuing...Start Typing\n");
    }
}
int main(int argc, char *argv[])
{
	/*Attributes of a Sample on the server*/
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    pa_simple *serve = NULL;//New server for playing or recording
    int ret = 1;
    int error;

    pid_t parent_pid=getpid();//Parent process pid
    int sockfd, numbytes;  // Communication on sock_fd
    char buf[size],msg[size];
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
	
    if (argc != 3) {
        fprintf(stderr,"usage: USR2 hostname\n");
        exit(1);
    }
	
	/*Initializing addrinfo struct*/
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    //IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;//TCP connection
    
	/*Initializing and generating a linked 
    list of all address family specified by hints struct*/
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        /*Generating Socked id(Descriptors)*/
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("USR2: socket");
            continue;
        }
		/*Connecting socket to the address family*/
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("USR2: connect");
            continue;
        }
		break;
    }

    if (p == NULL) {
        fprintf(stderr, "USR2: failed to connect\n");
        return 2;
    }
	
    /*Converting ip address from from struct to string and printing*/
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("USR2: connecting to %s\nStart Typing...\n", s);

    /*Not required any more so freeing it*/
    freeaddrinfo(servinfo);
	
    pid=fork();
    numbytes  = 1;
    
	/*Child Process*/
    if(pid ==0)
    {
        /*Registering SIGINT signal*/
        if(signal(SIGINT,term_p)==SIG_ERR){
                printf("Can't Catch SIGINT\n");
            }
		/*Setting up server for playing into the connected speakers*/
        if (!(serve = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "play", &ss, NULL, NULL, &error))) 
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        /*While there is incoming steam of bytes keep 
        reading and printing on user stdout*/
        while(numbytes!=0)
        {
            numbytes=recv(sockfd, buf, sizeof(buf), 0);
            if (numbytes== -1) 
            {
                perror("recv");
                exit(1);
            }
            if(numbytes!=0)
            {
				/*Writing to the Server from the buffer*/
                if (pa_simple_write(serve, buf, sizeof(buf), &error) < 0) 
                {
                    fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
                    goto finish;
                }
            }
            else if(numbytes==0){printf("USR1's sending end is closed...Exiting USR2's receving end!!!\n");}
        }
        kill(parent_pid,SIGTERM);
    }
    /*Parent Process*/
    else
    {
        /*Registering SIGCONT and SIGINT signals*/
        if(signal(SIGCONT,noth)==SIG_ERR){
            printf("Can't Catch SIGCONT\n");
        }
        if(signal(SIGINT,term_c)==SIG_ERR){
                printf("Can't Catch SIGINT\n");
        }
		/*Setting up server for recording from the connected microphone*/
        if (!(serve = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) 
        {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
        }
        /*Infinite loop of taking input from user and sending through socket*/
         while(1)
        {
			/*Reading From the Server Into the msg Buffer*/
            if (pa_simple_read(serve, msg, sizeof(msg), &error) < 0) 
            {
                fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
                goto finish;
            }
            if (send(sockfd, msg,sizeof(msg),0) == -1)
            {
                perror("send");
                exit(1);
            }
        }
    }
    /*Closing the socket Descriptor freeing the Server*/
    ret = 0;
    finish:
    close(sockfd);
    if (s)
        pa_simple_free(serve);
    return ret;
}
