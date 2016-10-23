/*
 * project_2 -- a simple webserver
 *
 * The second project of the 2016 Fall Semester course
 * CSI4106-01: Computer Networks at Yonsei University.
 *
 * Author: Vincent Au (2016840200)
 */

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	//make sure we have the right number of arguments
	if (argc != 3) {
		//remember: the name of the program is the first argument
		fprintf(stderr, "ERROR: Missing required arguments!\n");
		printf("Usage: %s <port> <folder>\n", argv[0]);
		printf("e.g. %s 8080 /var/www\n", argv[0]);
		return 1;
	}

	printf("Listening on port %s with %s as root directory...\n",
			argv[1], argv[2]);
	return 0;
}

