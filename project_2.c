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
#include <ctype.h>


#define BACKLOG 10 //how many pending connections the queue will hold
#define MAX_BUF 1024 //the max size of messages
#define MAX_PATH_LEN 8192 //max size of the file path

int connfd; //file descriptor of connection socket

char *PORT;
char *ROOT;

void
write_file(char *path);

int
handle_redirect(char *site);

/*
 * Writes an error response to connfd depending on <errno>
 */
void
write_error(int errno, char *req)
{
	switch(errno){
		case 403:
			write(connfd, "HTTP/1.1 403 Forbidden\r\n", 24);
			write(connfd, "Content-Type: text/html\r\n", 25);
			write(connfd, "\r\n", 2);
			write(connfd, "<html><head><title>Access Forbidden</title></head><body><h1>403 Forbidden</h1><p>You don’t have permission to access the requested URL ", 135);
			write(connfd, req, strlen(req));
			write(connfd, ". There is either no index document or the directory is read-protected.</p></body></html>", 89);
			break;
		case 404:
			write(connfd, "HTTP/1.1 404 Not Found\r\n", 24);
			write(connfd, "Content-Type: text/html\r\n", 25);
			write(connfd, "\r\n", 2);
			write(connfd, "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><p>The requested URL ", 96);
			write(connfd, req, strlen(req));
			write(connfd, " was not found on this server.</p></body></html>", 48);
			break;
		default:
			fprintf(stderr, "Unrecognised error number <%d>\n", errno);
			break;
	}
}

/*
 * Returns the correct MIME type depending on file extention
 */
char *
get_mime(char *path)
{
	char *ptr;
	int ch = '.';

	ptr = strrchr(path, ch);
	printf("%s\n", ptr);

	//supported file types
	if (strcmp(ptr, ".html") == 0)
		return "text/html";
	if (strcmp(ptr, ".css") == 0)
		return "text/css";
	if (strcmp(ptr, ".js") == 0)
		return "text/javascript";
	if (strcmp(ptr, ".jpg") == 0)
		return "image/jpeg";
	if (strcmp(ptr, ".png") == 0)
		return "image/png";

	//arbitrary data
	return "application/octet-stream";
}

void
parse_request(char *request)
{

	//only handle GET requests for now
	if (strncmp(request, "GET", 3) != 0)
		return;

	char *token, *string, *tofree;
	tofree = string = strdup(request);

	char req[MAX_PATH_LEN];
	char path[MAX_PATH_LEN];
	char *p = path;

	memset(path, 0, sizeof(path));
	memset(req, 0, sizeof(req));
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
	free(tofree);
	printf("%s\n", request);

	char *res1 = strstr(request, "/");
	char *res2 = strstr(request, " HTTP");
	int req_len = res2 - res1; //length of the requested page name
	//printf("'%d'\n", req_len);
	if (req_len > 1) {
		strncpy(req, res1, req_len);
		strcpy(p, req);
		p += req_len;
	} else {
		strcpy(p, "/index.html");
		p += 11;
	}
	printf("file: %s\n", path);

	struct stat st;
	stat(path, &st);

	if (S_ISREG(st.st_mode)) { //normal file
		printf("is normal file\n");
		write_file(path);
		return;
	}

	//file not found
	printf("is not normal file\n");
	//only handle GET requests for now
	if (strncmp(req, "/go/", 4) == 0) {
		char *site;
		site = req + 4;
		if (handle_redirect(site) != -1) //successfully redirected
			return;
	}

	//if we've made it down here then just return error
	write_error(404, req);

}

/*
 * If <site> contains only alpha characters, set HTTP header to redirect
 * to www.<site>.com. Returns: -1 if nonalpha and 0 otherwise
 */
int
handle_redirect(char *site)
{
	for (int i = 0; i < (int)strlen(site); i++) {
		if (!isalpha(site[i]))
			return -1;
	}
	write(connfd, "HTTP/1.1 302 Found\r\n", 20);
	write(connfd, "Location: http://www.", 21);
	write(connfd, site, strlen(site));
	write(connfd, ".com/\r\n\r\n", 9);
	return 0;
}


void
write_file(char *path)
{
	FILE *file, *connfile;
	unsigned char bytes_to_send[MAX_BUF];
	size_t bytes_read;
	struct stat st;
	stat(path, &st);

	file = fopen(path, "r");
	connfile = fdopen(connfd, "w");

	int fsize = (int) st.st_size;
	printf("file size is %d bytes\n", fsize);

	fprintf(connfile,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"\r\n", get_mime(path), fsize);
	fflush(connfile);

	while ((bytes_read = fread(bytes_to_send, 1, MAX_BUF, file)) > 0) {
		fwrite(bytes_to_send, 1, bytes_read, connfile);
	}

	fclose(file);
	fclose(connfile);
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

