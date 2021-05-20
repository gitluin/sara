/*                                                                                
 * sara Window Manager
 * ______________________________________________________________________________ 
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

#include "common.h"


int
main(int argc, char* argv[]){
	int sfd;
	struct sockaddr saddress = {AF_UNIX, INPUTSOCK};

	// TODO: convert args array into single string, then write string

	if (argc != 2)
		die("please provide one argument!");
	if ( (sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		die("failed to create socket!");
	if (connect(sfd, &saddress, sizeof(saddress)) < 0)
		die("failed to connect to socket!");
	if (send(sfd, argv[1], MAXBUFF, 0) < 0)
		die("failed to send to socket!");

	close(sfd);

	return 0;
}
