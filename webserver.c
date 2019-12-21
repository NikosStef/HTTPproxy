#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cnaiapi.h"

#define BUFFSIZE	256
#define PORT 20000
#define SERVER_NAME	"HTTP Proxy Server"

int	recvln(connection, char *, int);
void	send_head(connection, int, int);

/*-----------------------------------------------------------------------
 *
 * Program: HTTP Proxy Server
 * Purpose: serve requested website for client
 * Usage:   webserver <appnum>
 *
 *-----------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{

  FILE *logf;
	connection	conn, remoteconn;
	int		n;
	char		buff[BUFFSIZE], cmd[16], path[64], vers[16], hostTag[16], host[64];
	char		*timestr;
#if defined(LINUX) || defined(SOLARIS)
	struct timeval	tv;
#elif defined(WIN32)
	time_t          tv;
#endif
  char    *request;
  appnum   port;
  computer hostIP;

  remoteconn = -1;

  char *packet = malloc(1024);
  char *answer = malloc(1024);


	while(1) {


		/* opening log file in append mode */

		logf = fopen("log.txt", "a");


		/* wait for contact from a client on specified appnum */

		conn = await_contact((appnum) PORT);
		if (conn < 0)
			exit(1);

		printf("Connection established.\n");


		/* read and parse the request line */

		n = recvln(conn, buff, BUFFSIZE);
		sscanf(buff, "%s %s %s", cmd, path, vers);
    strcat(packet, cmd);
    strcat(packet, " ");
    strcat(packet, path);
    strcat(packet, " ");
    strcat(packet, vers);
    strcat(packet, "\r\n");
		memset(buff, 0, sizeof(buff));


		/* search for Host header */

    while((n = recvln(conn, buff, BUFFSIZE)) > 0) {

			sscanf(buff, "%s %s", hostTag, host);
			strcat(packet, buff);
			memset(buff, 0, sizeof(buff));

      if(strcmp(hostTag, "Host:") == 0){
        break;
      }
    }


    /* get the rest of the request */

		while((n = recvln(conn, buff, BUFFSIZE)) > 0) {

			strcat(packet, buff);
			if (n == 2 && buff[0] == '\r' && buff[1] == '\n')
        break;

			memset(buff, 0, sizeof(buff));
		}

		printf("Request is read and being redirected.\n");



		/* check for unexpected end of file */

		if (n < 1) {
			(void) send_eof(conn);
			continue;
		}


    /* create connection with remote webserver */

		printf("Host is: %s\n", host);

    hostIP = cname_to_comp(host);
    port = appname_to_appnum(host);


    while (remoteconn < 0) {
      remoteconn = make_contact(hostIP, 80);
    }
		printf("Remote connection established.\n");



		/* send request to remote webserver */

    (void) send(remoteconn, packet, strlen(packet), 0);



    /* recieve answer from remote server */

    while((n = recvln(remoteconn, buff, BUFFSIZE)) > 0) {

			strcat(answer, buff);
      if (n == 2 && buff[0] == '\r' && buff[1] == '\n')
        break;
			memset(buff, 0, sizeof(buff));

		}

		printf("Recieved response. Forwarding...\n");

		(void) send_eof(remoteconn);



    /* write logs to file */

		printf("Writing logs...\n");


#if defined(LINUX) || defined(SOLARIS)
    gettimeofday(&tv, NULL);
    timestr = ctime(&tv.tv_sec);
#elif defined(WIN32)
		time(&tv);
		timestr = ctime(&tv);
#endif
    fputs(timestr,logf);
    fputs(" - ", logf);
    fputs(host,logf);
    fputs("\n",logf);


		/* return the requested web page */

    (void) send(conn, answer, strlen(answer), 0);


    /*
		memset(buff,0,sizeof(buff));
		memset(packet,0,sizeof(packet));
		memset(answer,0,sizeof(answer));
		memset(cmd,0,sizeof(cmd));
		memset(path,0,sizeof(path));
		memset(vers,0,sizeof(vers));
		memset(host,0,sizeof(host));
		memset(hostTag,0,sizeof(hostTag));
    */

    /* close file and end connection */

    fclose(logf);

    (void) send_eof(conn);

		printf("Connection ended.\n");
		printf("\n\n");

	}


  return(0);
}
