// Chat client. Linux.
#include "local.h"
#include "utils.h"

using namespace std;

// chat message buffer
char message[BUF_SIZE];

int main(int argc, char *argv[])
{


        // Define values
        int sock, pid, pipe_fd[2], fdmax;
	fd_set client_fds, read_fds;

        //     define ip & ports for server
        struct sockaddr_in addr;
        addr.sin_family = PF_INET;
        addr.sin_port = htons(SERVER_PORT);
        addr.sin_addr.s_addr = inet_addr(SERVER_HOST);

	
        //     if it's zero, we should shoud down client
        int continue_to_work = 1;

        //     Setup socket connection with server
        CHK2(sock,socket(PF_INET, SOCK_STREAM, 0));
        CHK(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0);

        //     Setup pipe to send messages from child process to parent
        CHK(pipe(pipe_fd));


        //     add server connetion to epoll to listen incoming messages from server
	FD_SET(sock, &client_fds);
	fdmax = sock;        

        //     add read part of pipe to main set
        //     to listen incoming messages from child process
	FD_SET(pipe_fd[0], &client_fds);
	if(pipe_fd[0] > fdmax)
		fdmax = pipe_fd[0];

        // Fork
        CHK2(pid,fork());
        switch(pid){
                case 0: // child process
                        close(pipe_fd[0]); // we dont need read pipe here
                        printf("Enter 'exit' to exit\n");
                        while(continue_to_work){
                                bzero(&message, BUF_SIZE);
                                fgets(message, BUF_SIZE, stdin);

                                // close while cycle for 'exit' command
                                if(strncasecmp(message, CMD_EXIT, strlen(CMD_EXIT)) == 0){
                                        continue_to_work = 0;
                                        // send user's message to parent process
                                }else CHK(write(pipe_fd[1], message, strlen(message) - 1));
                        }
                        break;
                default: //parent process
                        close(pipe_fd[1]); // we dont need write pipe here

                        // results of different functions(res)
                        int res;

                        // Main cycle with select
                        while(continue_to_work) {
				// Copy descriptor set
				read_fds = client_fds;
				CHK(select(fdmax+1, &read_fds, NULL, NULL, NULL));
                                
				for(int i = 0; i <= fdmax ; i++){
					if(FD_ISSET(i, &read_fds)){

		                                bzero(&message, BUF_SIZE);

		                                // new message from server
		                                if(i == sock){
		                                        CHK2(res,recv(sock, message, BUF_SIZE, 0));

		                                        // zero size of result means the server closed connection
		                                        if(res == 0){
		                                                CHK(close(sock));
		                                                continue_to_work = 0;
		                                        }
							else 
								printf("%s\n", message);

		                                        // message from child process(user's input message)
		                                }
						else{
		                                        CHK2(res, read(pipe_fd[0], message, BUF_SIZE));

		                                        // zero size of result means the child process going to exit
		                                        if(res == 0)
								continue_to_work = 0; // exit parent to
		                                        // send message to server
		                                        else
		                                                CHK(sendall(sock, message, BUF_SIZE));
		                                }
					}
                                }
                        }
        }
        if(pid){
                close(pipe_fd[0]);
                close(sock);
        }else{
                close(pipe_fd[1]);
        }

        return 0;
}
