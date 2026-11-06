#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(void){
 printf("Inside namespace PID=%ld UID=%ld\n",(long)getpid(),(long)getuid());
 FILE *f=fopen("/etc/passwd","r");if(f){fclose(f);fprintf(stderr,"Host filesystem unexpectedly visible\n");return 1;}
 int fd=socket(AF_INET,SOCK_STREAM,0);struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(80)};inet_pton(AF_INET,"1.1.1.1",&a.sin_addr);
 if(fd>=0&&connect(fd,(struct sockaddr*)&a,sizeof a)==0){fprintf(stderr,"Network unexpectedly reachable\n");close(fd);return 1;}if(fd>=0)close(fd);
 puts("Host files and external network are inaccessible.");return 0;
}
