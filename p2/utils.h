//Utility functions for chat client and server. Linux.
#include "local.h"

// Setup nonblocking socket
int setnonblocking(int sockfd)
{
   CHK(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK));
   return 0;
}

// Send full message
int sendall(int s, char *buf, int len)
{
    int total = 0;        
    int bytesleft = len; 
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }


    return n==-1?-1:0; // In case of error it return -1
} 
