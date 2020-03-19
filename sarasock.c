/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
 *                                                                                
 * Copyright (c) 2020, This Fackin Guy, gitluin on github (no email for you!)     
 *                                                                                
 * Please refer to the complete LICENSE file that should accompany this software.
 * Please refer to the MIT license for details on usage: https://mit-license.org/
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/un.h>
#include <unistd.h>

/* this NEEDS to match with sara.c */
#define INPUTSOCK			"/tmp/sara.sock"

int
slen(const char* str){
	int i = 0;

	while (*str){ str++; i++; }

	return i;
}

void
die(const char* e, ...){
	fprintf(stdout, "sarasock: %s\n", e);
	exit(1);
}

int
main(int argc, char* argv[]){
	int sfd, n;
	struct sockaddr saddress = {AF_UNIX, INPUTSOCK};

	if (argc != 2)
		die("please provide one argument!");
	if ( (sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		die("failed to create socket!");
	if (connect(sfd, &saddress, sizeof(saddress)) < 0)
		die("failed to connect to socket!");
	if (send(sfd, argv[1], slen(argv[1])+1, 0) < 0)
		die("failed to send to socket!");

	close(sfd);

	return 0;
}
