
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	//the name of the program is the first argument
	if (argc != 3) {
		fprintf(stderr, "ERROR: Missing required arguments!\n");
		printf("Usage: %s <port> <folder>\n", argv[0]);
		printf("e.g. %s 8080 /var/www\n", argv[0]);
		return 1;
	}

	printf("Listening on port %s with %s as root directory...\n",
			argv[1], argv[2]);
	return 0;
}

