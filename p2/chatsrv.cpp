// Chat server. Linux.
#include "local.h"
#include "utils.h"

using namespace std;

list<int> clients_list;

int main(int argc, char *argv[])
{
	int listener;

	struct sockaddr_in addr, their_addr;
	//     set server parameters
	addr.sin_family = PF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = inet_addr(SERVER_HOST);

	socklen_t socklen;
	socklen = sizeof(struct sockaddr_in);

	//     event template for epoll_ctl
	//     storage array for incoming events from epoll_wait
	//     and maximum events count could be EPOLL_SIZE
	static struct epoll_event ev, events[EPOLL_SIZE];
	//     watch just incoming(EPOLLIN)
	//     and Edge Trigged(EPOLLET) events
	ev.events = EPOLLIN | EPOLLET;

	//     message buffer
	char message[BUF_SIZE];

	//     epoll descriptor to watch events
	int epfd;

	// other values:
	//     new client descriptor
	//     to keep the results of different functions
	//     to keep incoming epoll_wait's events count
	int client, res, epoll_events_count;
	int yes=1;

	//     Create listener with PF_INET and SOCK_STREAM
	CHK2(listener, socket(PF_INET, SOCK_STREAM, 0));
	printf("Main listener (fd = %d) created! \n",listener);

	//     No more "Alredy in use"!
	CHK(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))); 

	//     setup nonblocking socket
	setnonblocking(listener);

	//     bind listener to address
	CHK(bind(listener, (struct sockaddr *)&addr, sizeof(addr)));
	printf("Listener binded to: %s\n", SERVER_HOST);

	//     start to listen connections
	CHK(listen(listener, 1));
	printf("Start to listen: %s!\n", SERVER_HOST);

	//     Setup epoll
	CHK2(epfd,epoll_create(EPOLL_SIZE));
	printf("Epoll(fd=%d) created!\n", epfd);

	//     set listener to event template
	ev.data.fd = listener;

	//     add listener to epoll
	CHK(epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev));
	printf("Main listener(%d) added to epoll!\n", epfd);

	// Main cycle(epoll_wait)
	while(1)
	{
		CHK2(epoll_events_count,epoll_wait(epfd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT));

		for(int i = 0; i < epoll_events_count ; i++)
		{

			// EPOLLIN event for listener(new client connection)
			if(events[i].data.fd == listener){
				CHK2(client,accept(listener, (struct sockaddr *) &their_addr, &socklen));
				printf("accepted connection\n");
				// setup nonblocking socket
				setnonblocking(client);

				// set new client to event template
				ev.data.fd = client;

				// add new client to epoll
				CHK(epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev));

				// save new descriptor to further use
				clients_list.push_back(client); // add new connection to list of clients

				// send initial welcome message to client
				bzero(message, BUF_SIZE);
				res = sprintf(message, STR_WELCOME, client);
				CHK2(res, sendall(client, message, BUF_SIZE));
			}
			else{ // EPOLLIN event for others(new incoming message from client)
				CHK2(res,handle_message(events[i].data.fd));
			}
		}
	}

	close(listener);
	close(epfd);

	return 0;
}

// Handle incoming message from clients
int handle_message(int client)
{
	// get row message from client(buf)
	//     and format message to populate(message)
	char buf[BUF_SIZE], message[BUF_SIZE];
	bzero(buf, BUF_SIZE);
	bzero(message, BUF_SIZE);

	// to keep different results
	int len;

	// try to get new raw message from client
	CHK2(len,recv(client, buf, BUF_SIZE, 0));

	// zero size of len mean the client closed connection
	if(len == 0){
		CHK(close(client));
		clients_list.remove(client);
		printf("connection terminated\n");
	// populate message around the world
	}
	else{
		if(clients_list.size() == 1)
		{ // If noone connected to server except YOU!
			CHK(sendall(client, STR_NOONE_CONNECTED, strlen(STR_NOONE_CONNECTED)));
			sprintf(message, STR_MESSAGE, client, buf);
			printf("%s\n", message);
			return len;
		}
	   
		// format message to populate
		sprintf(message, STR_MESSAGE, client, buf);
		printf("%s\n", message);

		// populate message around the world
		list<int>::iterator it;
		for(it = clients_list.begin(); it != clients_list.end(); it++)
			CHK(sendall(*it, message, BUF_SIZE));
	}

	return len;
}


