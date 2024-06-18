#include "client.h"

int quitter = 0;

// Initialisation des sockets pour les communications réseau
static void initialisation(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

// Nettoyage des ressources utilisées pour les sockets Windows
static void fin(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

// Fonction pour gérer les interruptions
static void gestionnaire_signal(int sig) 
{
   quitter = 1;
}

// Fonction principale gérant la communication avec le serveur
static void application(const char *address, int port)
{
   // Initialisation de la connexion au serveur et création d'une socket
   SOCKET sock = initialiser_connection(address, port);
   char buffer[BUF_SIZE]; // Buffer pour stocker les données à envoyer ou reçues

   fd_set rdfs; // Ensemble des descripteurs de fichier à surveiller avec select()

   // Saisie du pseudo du joueur depuis la console
   printf("\nSaisissez votre pseudo : ");
   fgets(buffer, BUF_SIZE - 1, stdin);
   buffer[strcspn(buffer, "\n")] = '\0'; // Supprimer le caractère de nouvelle ligne s'il est présent
   printf("\n");
   
   // Envoie du pseudo au serveur
   ecrire_serveur(sock, buffer);

   while(1)
   {
      // Interruptions
      signal(SIGINT, gestionnaire_signal);
      signal(SIGKILL, gestionnaire_signal);
      signal(SIGHUP, gestionnaire_signal);
      signal(SIGTERM, gestionnaire_signal);
      signal(SIGSTOP, gestionnaire_signal);
      signal(SIGQUIT, gestionnaire_signal);
       
      if(quitter == 1)
      {
         ecrire_serveur(sock, "/quitter");
      }

      FD_ZERO(&rdfs); // Initialisation de l'ensemble des descripteurs de fichier à surveiller

      /* Ajout de STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* Ajout de socket */
      FD_SET(sock, &rdfs);

      // Utilisation de select() pour surveiller les entrées utilisateur et les données reçues du serveur
      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("\nselect()");
      }
      else
      {
         // Si l'on reçoit une entrée clavier
         if(FD_ISSET(STDIN_FILENO, &rdfs))
         {
            fgets(buffer, BUF_SIZE - 1, stdin);
            {
               char *p = NULL;
               p = strstr(buffer, "\n");

               if(p != NULL)
               {
                  *p = 0;
               }
               else
               {
                  /* fclean */
                  buffer[BUF_SIZE - 1] = 0;
               }
            }

            // Envoi des données saisies au serveur
            ecrire_serveur(sock, buffer);

         }
         else if(FD_ISSET(sock, &rdfs))
         {
            int n = lire_serveur(sock, buffer);

            // Si la réception est vide, le serveur est déconnecté
            if(n == 0)
            {
               printf("\nVous êtes désormais déconnecté du serveur.\n\n");
               break;
            }

            puts(buffer); // Affichage des données reçues du serveur
         }
      }
   }

   terminer_connection(sock); // Fermeture de la connexion avec le serveur
}


// Initialisation de la connexion avec le serveur
static int initialiser_connection(const char *address, int port)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(port);
   sin.sin_family = AF_INET;

   // Connexion au serveur via la socket
   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock; // Retourne la socket pour la communication
}

// Fermeture de la connexion avec le serveur
static void terminer_connection(int sock)
{
   closesocket(sock);
}

// Lecture des données envoyées par le serveur
static int lire_serveur(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}

// Envoi des données au serveur
static void ecrire_serveur(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

// Fonction principale du programme
int main(int argc, char **argv)
{
   if(argc != 3)
   {
      printf("Usage : %s [adresse_du_serveur] [port_du_serveur]\n", argv[0]);
      return EXIT_FAILURE;
   }

   initialisation(); // Initialisation des sockets

   application(argv[1], atoi(argv[2])); // Exécution de l'application

   fin(); // Nettoyage des ressources des sockets

   return EXIT_SUCCESS;
}
