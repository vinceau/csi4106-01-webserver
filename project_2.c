/*
 * project_2 -- a simple webserver
 *
 * The second project of the 2016 Fall Semester course
 * CSI4106-01: Computer Networks at Yonsei University.
 *
 * Author: Vincent Au (2016840200)
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>


#define BACKLOG 10 //how many pending connections the queue will hold
#define MAX_BUF 1024 //the max size of messages
#define MAX_PATH_LEN 8192 //max size of the file path

int connfd; //file descriptor of connection socket

char *PORT;
char *ROOT;

void
parse_request(char *request) {

	//only handle GET requests for now
	if (strncmp(request, "GET", 3) != 0)
		return;

	char *token, *string, *tofree;
	tofree = string = strdup(request);

	char path[MAX_PATH_LEN];
	char *p = path;

	memset(path, 0, sizeof(path));
	strcpy(path, ROOT);
	p += strlen(ROOT);

	while ((token = strsep(&string, "\r\n")) != NULL) {
		if (strncmp(token, "User-Agent: ", 12) == 0) {
			//we're looking at the user agent now
			if (strstr(token, "Mobile") != NULL) {
				//we're on mobile
				strcpy(p, "/mobile");
				p += 7;
			}
		}
	}

	char *res1 = strstr(request, "/");
	char *res2 = strstr(request, " HTTP");
	int req_len = res2 - res1; //length of the requested page name
	//printf("'%d'\n", req_len);
	if (req_len > 1) {
		strncpy(p, res1, req_len);
		p += req_len;
	} else {
		strcpy(p, "/index.html");
		p += 11;
	}
	printf("file: %s\n", path);

	FILE *file;
	char a;
	file = fopen(path, "r");

	if (file != NULL) { //file found
		struct stat st;
		stat(path, &st);
		int fsize = (int) st.st_size + 1;

		FILE *connfile = fdopen(connfd, "w");
		fprintf(connfile,
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: text/html\r\n"
				"\r\n\r\n", fsize);
		fflush(connfile);

		do {
			a = fgetc(file);
			fputc(a, connfile);
		} while (a != EOF);

		fclose(file);
		fclose(connfile);
	} else { //file not found
		send(connfd, "HTTP/1.1 404 Not Found\r\n", 24, 0);
	}

	printf("%s\n", request);
	free(tofree);
}

//get sockaddr, IPv4 or IPv6:
void
*get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//set up the server on socket <listener> using port <port>
void
setup_server(int *listener, char *port)
{
	//set up the structs we need
	int status;
	struct addrinfo hints, *p;
	struct addrinfo *servinfo; //will point to the results

	memset(&hints, 0, sizeof(hints)); //make sure the struct is empty
	hints.ai_family = AF_UNSPEC; //IPv4 or IPv6 is OK (protocol agnostic)
	hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
	hints.ai_flags = AI_PASSIVE; //fill in my IP for me

	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n",
			gai_strerror(status));
		exit(1);
	}

	//servinfo now points to a linked list of struct addrinfos
	//each of which contains a struct sockaddr of some kind
	int yes = 1;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		*listener = socket(servinfo->ai_family, servinfo->ai_socktype,
				servinfo->ai_protocol);
		if (*listener == -1) {
			perror("ERROR: socket() failed");
			//keep going to see if we can connect to something else
			continue; 
		}

		if (setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(yes)) == -1) {
			perror("ERROR: setsockopt() failed");
			exit(1);
		}

		if (bind(*listener, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
			perror("ERROR: bind() failed");
			//keep going to see if we can connect to something else
			continue;
		}

		//we have something, let's move on
		break;
	}

	//free the linked list since we don't need it anymore
	freeaddrinfo(servinfo);

	if (p == NULL) {
		fprintf(stderr, "ERROR: Failed to bind to anything!\n");
		exit(1);
	}

	//listen time
	if (listen(*listener, BACKLOG) == -1) {
		perror("ERROR: listen() failed");
		exit(1);
	}
}

int
main(int argc, char **argv)
{
	//make sure we have the right number of arguments
	if (argc != 3) {
		//remember: the name of the program is the first argument
		fprintf(stderr, "ERROR: Missing required arguments!\n");
		printf("Usage: %s <port> <folder>\n", argv[0]);
		printf("e.g. %s 8080 /var/www\n", argv[0]);
		exit(1);
	}

	PORT = argv[1]; //port we're listening on
	ROOT = argv[2]; //root directory

	int listener; //file descriptor of listening socket
	struct sockaddr_storage their_addr; //connector's address info
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN]; //the connector's readable IP address

	char buf[MAX_BUF]; //buffer for messages
	int nbytes; //the number of received bytes

	//set up the server on 
	setup_server(&listener, PORT);

	printf("SERVER: Listening on port %s for connections...\n", PORT);

	while(1) {
		sin_size = sizeof(their_addr);
		//accept()
		connfd = accept(listener, (struct sockaddr *) &their_addr,
				&sin_size);
		if (connfd == -1) {
			perror("ERROR: accept() failed");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
		printf("SERVER: received connection from %s\n", s);

		if (!fork()) { //this is the child process
			close(listener); //child doesn't need the listener

			if ((nbytes = recv(connfd, buf, MAX_BUF, 0)) > 0) {
				parse_request(buf);
				//printf("%.*s", nbytes, buf);
			}

//			if (send(connfd, "HTTP/1.1 302 Moved\n", 25, 0) == -1)
//				perror("ERROR: send() failed");
			close(connfd);
			exit(0);
		}
		close(connfd);  //parent doesn't need this

	}

	return 0;
}

