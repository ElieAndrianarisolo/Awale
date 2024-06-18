#ifndef CLIENT_H
#define CLIENT_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <signal.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define CRLF     "\r\n"
#define BUF_SIZE 2048

static void initialisation(void);
static void fin(void);
static void application(const char *address, int port);
static int initialiser_connection(const char *address, int port);
static void gestionnaire_signal(int sig);
static void terminer_connection(int sock);
static int lire_serveur(SOCKET sock, char *buffer);
static void ecrire_serveur(SOCKET sock, const char *buffer);

#endif // CLIENT_H
