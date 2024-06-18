#include "serveur.h"

// Fonction d'initialisation du serveur
static void initialisation(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

// Fonction de fermeture du serveur
static void fin(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

// Fonction principale du serveur
static void application(int port)
{
   SOCKET sock = initialiser_connection(port); // Initialisation de la connexion
   char buffer[BUF_SIZE]; // Buffer pour stocker les données reçues

   // Index pour le tableau des clients
   int actual = 0;
   int max = sock;

   // Tableau pour stocker les clients connectés
   Client clients[MAX_CLIENTS];

   // Liste de toutes les commandes du client avec leurs descriptions
   Commande liste_commandes[] = 
   {
      {"/liste_commandes", "Consulter la liste des commandes disponibles"},
      {"/liste_joueurs", "Consulter la liste des joueurs en ligne."},
      {"/classement", "Consulter le classement elo des joueurs."},
      {"/challenger [pseudo]", "Challenger [pseudo] pour une partie."},
      {"/challenger_IA", "Challenger notre IA pour une partie."},
      {"/accepter", "Accepter un challenge en attente."},
      {"/refuser", "Refuser un challenge en attente."},
      {"/declarer_forfait", "Déclarer forfait au cours d'une partie."},
      {"/liste_parties", "Consulter la liste des parties d'awale en cours."},
      {"/observer [id_partie]", "Observer une partie d'awale avec l'identifiant [id_partie]."},
      {"/arreter_observer", "Arrêter d'observer une partie d'awale."},
      {"/ajouter_ami [pseudo]", "Envoyer une requête d'ami à [pseudo]."},
      {"/retirer_ami [pseudo]", "Retirer [pseudo] de vos amis."},
      {"/accepter_ami [pseudo]", "Accepter la requête d'ami de [pseudo]."},
      {"/refuser_ami [pseudo]", "Refuser la requête d'ami de [pseudo]."},
      {"/liste_amis", "Consulter la liste de ses amis."},
      {"/liste_requete_ami", "Consulter la liste des requêtes d'ami en attente."},
      {"/ecrire_bio [votre_bio]", "Ecrire sa bio avec [votre_bio]."},
      {"/consulter_bio [pseudo]", "Consulter la bio de [pseudo]."},
      {"/liste_joueurs", "Consulter la liste des joueurs en ligne."},
      {"/prive", "Passer en mode privé."},
      {"/public", "Passer en mode public."},
      {"/discuter_out [pseudo] [message]", "Envoyer le [message] à [pseudo]."},
      {"/discuter_in [message]", "Envoyer le [message] au joueur adverse lors d'une partie."},
      {"/sauvegarder_oui", "Sauvegarder la partie courante."},
      {"/sauvegarder_non", "Ne pas sauvegarder la partie courante."},
      {"/historique", "Consulter la liste des parties sauvegardées."},
      {"/partie_sauvegardee [id_partie]", "Consulter une partie sauvegardée avec l'identifiant [id_partie]."},
      {"/quitter", "Se déconnecter du serveur. Attention, vos informations seront perdues !"}
   };

   // Construction du message contenant les informations sur les commandes
   char informations_commandes[BUF_SIZE];
   strcpy(informations_commandes, "\nVoici la liste de l'ensemble des commandes disponibles :\n");

   int nb_commandes = sizeof(liste_commandes) / sizeof(Commande);
   for (int i = 0; i < nb_commandes; i++) 
   {
      strcat(informations_commandes, liste_commandes[i].commande);
      strcat(informations_commandes, " : ");
      strcat(informations_commandes, liste_commandes[i].description);
      strcat(informations_commandes, "\n");
   }

   // Initialisation du set de descripteurs de fichiers pour la gestion des entrées/sorties
   fd_set rdfs;

   // Création du client IA
   Client IA = {
                  .name = "IA",
                  .nb_parties = 0,
                  .elo = 0
               };

   // Boucle principale du serveur
   while(1)
   {
      // Initialisation de la variable de boucle
      int i = 0;
      FD_ZERO(&rdfs); // Réinitialisation du set de descripteurs

      // Ajout de l'entrée standard (clavier)
      FD_SET(STDIN_FILENO, &rdfs);

      // Ajout du socket de connexion
      FD_SET(sock, &rdfs);

      // Ajout du socket de chaque client connecté
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      // Utilisation de la fonction select() pour surveiller les descripteurs
      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      // Vérification des descripteurs prêts en entrée/sortie
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         // Nouveau client
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         // Après que le client ait envoyé son pseudo
         if (lire_client(csock, buffer) == -1)
         {
            continue;
         }

         // Vérification de si le pseudo existe déjà
         int pseudo_exist = 0;
         for (int i = 0; i < actual; i++) 
         {
            if (strcmp(clients[i].name, buffer) == 0) 
            {
               pseudo_exist = 1;
               break;
            }
         }

         if (pseudo_exist) // Si le pseudo existe déjà, empêcher la connection du client
         {
            char reject_message[] = "Ce pseudo est déjà attribué à un autre joueur. Connexion refusée.\n";
            ecrire_client(csock, reject_message);
            close(csock);
            continue;
         }

         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[actual] = c;
         actual++;

         ecrire_client(csock, "Tapez /liste_commandes afin de consulter la liste des commandes disponibles.\n");
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            // Gestion des interactions avec les clients connectés
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = lire_client(clients[i].sock, buffer);
               
               if (c == 0) // Client déconnecté
               {
                  se_deconnecter(clients, i, &actual);
               }
               else
               {
                  if (strcmp(buffer,"/liste_commandes") == 0)
                  {
                     ecrire_client(clients[i].sock, informations_commandes);
                  }
                  else if (strcmp(buffer, "/quitter") == 0) 
                  {
                     se_deconnecter(clients, i, &actual);
                  }
                  else if (strcmp(buffer, "/classement") == 0) 
                  {
                     afficher_liste_classement_joueurs(&clients[i], clients, actual);
                  }
                  else if (strcmp(buffer, "/liste_joueurs") == 0) 
                  {
                     afficher_liste_joueurs(&clients[i], clients, actual);
                  }
                  else if((strcmp(buffer, "/challenger_IA") == 0) && !clients[i].is_playing && !clients[i].is_playing_with_ia)
                  {
                     challenger_ia(&clients[i], &IA);
                  }
                  else if ((strstr(buffer, "/challenger") != NULL) && !clients[i].is_playing_with_ia && !clients[i].is_playing && !clients[i].is_watching && !clients[i].is_challenged && !clients[i].is_asked_to_save && !clients[i].want_to_save) 
                  {
                     challenger_joueur(&clients[i], clients, actual, buffer);
                  }
                  else if (strcmp(buffer, "/accepter") == 0 && clients[i].is_challenged) 
                  {
                     accepter_challenge(&clients[i]);
                  }
                  else if (strcmp(buffer, "/refuser") == 0 && clients[i].is_challenged) 
                  {
                     refuser_challenge(&clients[i]);
                  }
                  else if (strcmp(buffer, "/sauvegarder_oui") == 0 && clients[i].is_asked_to_save) 
                  {
                     sauvegarder_partie(&clients[i], 1);
                  }
                  else if (strcmp(buffer, "/sauvegarder_non") == 0 && clients[i].is_asked_to_save) 
                  {
                     sauvegarder_partie(&clients[i], 0);
                  }
                  else if (clients[i].is_playing && est_nombre(buffer))
                  {
                     if(clients[i].is_playing_with_ia)
                     {
                        gerer_partie_ia(&clients[i], buffer);
                     }
                     else
                     {
                        gerer_partie(&clients[i], buffer);
                     }
                  }
                  else if (clients[i].is_playing && strcmp(buffer, "/declarer_forfait") == 0 && !clients[i].is_asked_to_save)
                  {
                     declarer_forfait_partie(&clients[i], buffer); 
                  }
                  else if (strcmp(buffer, "/liste_parties") == 0)
                  {
                     afficher_liste_parties(&clients[i]); 
                  }
                  else if ((strstr(buffer, "/observer") != NULL) && !clients[i].is_watching && !clients[i].is_playing && !clients[i].is_playing_with_ia) 
                  {
                     observer_partie(&clients[i], buffer);
                  } 
                  else if (strcmp(buffer, "/arreter_observer") == 0 && clients[i].is_watching && !clients[i].is_playing && !clients[i].is_playing_with_ia)
                  {
                     arreter_observer_partie(&clients[i]);
                  }
                  else if ((strstr(buffer, "/ajouter_ami") != NULL)) 
                  {
                     gerer_demande_ami(&clients[i], clients, actual, buffer);
                  }
                  else if ((strstr(buffer, "/retirer_ami") != NULL)) 
                  {
                     retirer_ami(&clients[i], clients, actual, buffer);
                  }
                  else if ((strstr(buffer, "/accepter_ami") != NULL)) 
                  {
                     accepter_requete_ami(&clients[i], clients, actual, buffer);
                  }
                  else if ((strstr(buffer, "/refuser_ami") != NULL)) 
                  {
                     refuser_requete_ami(&clients[i], clients, actual, buffer);
                  }
                  else if (strcmp(buffer, "/liste_amis") == 0)
                  {
                     afficher_liste_amis(&clients[i]);
                  } 
                  else if (strcmp(buffer, "/liste_requete_ami") == 0)
                  {
                     afficher_liste_requete_ami(&clients[i]);
                  }
                  else if ((strstr(buffer, "/ecrire_bio") != NULL))
                  {
                     ecrire_bio(&clients[i], buffer); 
                  }
                  else if (strstr(buffer, "/consulter_bio")!= NULL)
                  {
                     afficher_bio(&clients[i], clients, actual,  buffer) ;
                  }
                  else if (strcmp(buffer, "/prive") == 0)
                  {
                     passer_prive(&clients[i]);
                  }
                  else if (strcmp(buffer, "/public") == 0)
                  {
                     passer_public(&clients[i]);
                  }
                  else if ((strstr(buffer, "/discuter_in") == NULL) && (strstr(buffer, "/discuter_out") != NULL) )
                  {
                     discuter_hors_partie(&clients[i], buffer, clients, actual);
                  }
                  else if (clients[i].is_playing && (strstr(buffer, "/discuter_in") != NULL) && !clients[i].is_playing_with_ia)
                  {
                     discuter_en_partie(&clients[i], buffer); 
                  }
                  else if (strcmp(buffer, "/historique") == 0)
                  {
                     liste_historique(&clients[i]);
                  }
                  else if ((strstr(buffer, "/partie_sauvegardee") != NULL) && !clients[i].is_playing_with_ia && !clients[i].is_playing && !clients[i].is_watching && !clients[i].is_challenged && !clients[i].is_asked_to_save && !clients[i].want_to_save)  
                  {
                     consulter_partie_sauvegardee(&clients[i], buffer);
                  } 
                  else 
                  {
                     ecrire_client(clients[i].sock, "\nVotre commande est incorrecte ou inutilisable dans l'état.\n");
                  }
               }  

               break;
            }
         }
      }
   }

   // Nettoyage et fermeture des connexions à la fin
   effacer_clients(clients, actual);
   terminer_connection(sock);
}

// Fonction pour fermer les sockets de tous les clients
static void effacer_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

// Fonction pour retirer un client du tableau des clients
static void retirer_client(Client * clients, int to_remove, int *actual)
{
   char nom_fichier[50];
   strcpy(nom_fichier, (clients+to_remove)->name);
   strcat(nom_fichier, ".txt");
   remove(nom_fichier);

   // Suppression du client dans le tableau en décalant les éléments suivants 
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));

   /* Décrémentation du nombre de clients */
   (*actual)--;
}

// Fonction pour envoyer un message à tous les clients sauf à l'envoyeur
static void envoyer_message_tous_clients(Client * clients, Client envoyeur, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      // Ne pas envoyer de message à l'envoyeur
      if (envoyeur.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, envoyeur.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }

         strncat(message, buffer, sizeof message - strlen(message) - 1);
         ecrire_client(clients[i].sock, message);
      }
   }
}

// Fonction pour initialiser la connexion du serveur
static int initialiser_connection(int port)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(port);
   sin.sin_family = AF_INET;

   if (bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

// Fonction pour terminer la connexion du serveur
static void terminer_connection(int sock)
{
   closesocket(sock);
}

// Fonction pour lire les données provenant du client
static int lire_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

// Fonction pour écrire des données vers le client
static void ecrire_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

// Fonction pour afficher la liste des joueurs en ligne avec leur bio si disponible
static void afficher_liste_joueurs(Client * envoyeur, Client * clients, int actual)
{
   char reponse[BUF_SIZE];
   reponse[0] = 0;

   // Parcours de la liste des clients pour afficher leur nom et leur bio (si disponible)
   for (int i = 0; i < actual; i++) 
   {
      strncat(reponse, clients[i].name, BUF_SIZE - 1);
   
      if (strlen(clients[i].bio) > 0) 
      {
         strncat(reponse, " - Bio : ", BUF_SIZE - strlen(reponse) - 1);
         strncat(reponse, clients[i].bio, BUF_SIZE - strlen(reponse) - 1);
      }

      if (i != actual -1) 
      {
         strncat(reponse, "\n", BUF_SIZE - strlen(reponse) - 1);
      }

   }

   strncat(reponse, "\n", BUF_SIZE - strlen(reponse) - 1);

   ecrire_client(envoyeur->sock, "\nListe des joueurs en ligne : \n");
   ecrire_client(envoyeur->sock, reponse);
}

// Fonction pour afficher le classement elo du joueur
static void afficher_liste_classement_joueurs(Client * envoyeur, Client * clients, int actual)
{
   char reponse[BUF_SIZE];
   reponse[0] = 0;

   // Initialisation de la liste à trier
   Client clients_tries[actual];
   int nb_joueurs = 0;
   for(int i = 0; i < actual; i++)
   {
      clients_tries[nb_joueurs] = clients[i];
      nb_joueurs++;
   }
   
   // Tri de la liste
   qsort(clients_tries, nb_joueurs, sizeof(Client), tri_elo);
   
   // Parcours de la liste des clients pour afficher leur nom et leur classement elo
   char nb;
   char elo[4];

   for (int i = 0; i < nb_joueurs; i++) 
   {
      nb = (i+1) + '0';
      strncat(reponse, &nb, 1);
      
      strncat(reponse, " - ", 3);
      strncat(reponse, clients_tries[i].name, BUF_SIZE - strlen(reponse) - 1);
      strncat(reponse, ", ", 2);
      sprintf(elo,"%d",clients_tries[i].elo);
      strncat(reponse, &elo, 4);

      if (i != nb_joueurs -1) 
      {
         strncat(reponse, "\n", BUF_SIZE - strlen(reponse) - 1);
      }

   }

   strncat(reponse, "\n", BUF_SIZE - strlen(reponse) - 1);

   ecrire_client(envoyeur->sock, "\nClassement des joueurs : \n");
   ecrire_client(envoyeur->sock, reponse);
}

// Fonction de tri pour le classement elo
static int tri_elo(const void* a, const void* b)
{
   Client* ca = (Client*)a;
   Client* cb = (Client*)b;
   return cb->elo - ca->elo;
}

// Fonction pour obtenir un client par son nom dans la liste des clients
static Client * obtenir_client_par_nom(Client * clients, const char * buffer, int actual)
{
   for (int i = 0; i < actual; i++) 
   {
      char * name = clients[i].name;

      if (strcmp(buffer, name) == 0) 
      {
         return &(clients[i]);
      }
   }

   return NULL;
}

// Fonction pour gérer les demandes de challenges venant d'un joueur
static void challenger_joueur(Client * envoyeur, Client * clients, int actual, const char * buffer)
{
   // Obtenir le client ciblé par la demande de challenge
   Client * joueur_challenged = obtenir_client_par_nom(clients, buffer + strlen("/challenger") + 1, actual);
   
   // Vérifications avant d'envoyer une demande de challenge

   if (joueur_challenged == NULL)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur n'existe pas.\n") ; 
      return; 
   }

   if (joueur_challenged->is_playing)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur est déjà en train de jouer.\n") ; 
      return; 
   }

   if (joueur_challenged == envoyeur)
   {
      ecrire_client(envoyeur->sock ,"\nIl vous est impossible de vous challenger vous même.\n") ;
      return; 
   }

   if (joueur_challenged->is_challenged)
   {
      ecrire_client(envoyeur->sock ,"\nCette personne a déjà été challengée. Veuillez attendre qu'elle réponde à sa demande de challenge.\n") ;
      return; 
   }

   // Création d'une nouvelle demande de challenge

   liste_challenges[total_challenges].challenger = envoyeur; 
   liste_challenges[total_challenges].challenged = joueur_challenged;    
   liste_challenges[total_challenges].etat = -1;  // -1 indique en attente d'une réponse.
   liste_challenges[total_challenges].nb_observeurs = 0;
   joueur_challenged->is_challenged = 1;

   char demande_challenge[BUF_SIZE];
   strcpy(demande_challenge, "\n");
   strcat(demande_challenge, envoyeur->name);
   strcat(demande_challenge, " vous a challengé.\n\nVeuillez saisir soit /accepter soit /refuser pour répondre.\n"); 

   ecrire_client(liste_challenges[total_challenges].challenged->sock, demande_challenge);
   ecrire_client(liste_challenges[total_challenges].challenger->sock , "\nDemande de challenge envoyée."); 

   total_challenges++;
}

// Fonction pour obtenir le challenge en cours d'un joueur challengé
static int obtenir_challenge_par_joueur_challenged(Client challenged)
{
   for (int i = 0; i < total_challenges; i++) 
   {
      if (liste_challenges[i].challenged->sock == challenged.sock && liste_challenges[i].etat == -1) 
      {
         return i;
      }
   }

   return -1; // Retourne l'indice du challenge, -1 s'il n'y en a pas
}

// Fonction pour accepter un challenge venant d'un joueur
static void accepter_challenge(Client * envoyeur)
{
   char reponse[BUF_SIZE];

   // Obtenir la position du challenge en cours pour le joueur challengé

   int position_challenge = obtenir_challenge_par_joueur_challenged(*envoyeur); 
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged = liste_challenges[position_challenge].challenged->sock;

   if(liste_challenges[position_challenge].challenger->is_playing == 1)
   {
      ecrire_client(socket_challenged, "\nMince, votre challengeur vient tout juste de commencer une partie. Réessayez de la challenger plus tard.\n");
      liste_challenges[position_challenge].etat = 0;
      envoyeur->is_challenged = 0; 
      return;
   }

   strcpy(reponse, envoyeur->name); 
   strcat(reponse, " a accepté votre challenge.\n"); 
   ecrire_client(socket_challenger, reponse);

   // Préparation pour le début de la partie

   liste_challenges[position_challenge].etat = 1;
   liste_challenges[position_challenge].challenged->is_asked_to_save = 1;
   liste_challenges[position_challenge].challenger->is_asked_to_save = 1;

   liste_challenges[position_challenge].challenged->is_playing = 1;
   liste_challenges[position_challenge].challenger->is_playing = 1;

   ecrire_client(socket_challenger,"\nVoulez-vous sauvegarder la partie ?\nVeuillez saisir soit /sauvegarder_oui soit /sauvegarder_non pour répondre.\n");
   ecrire_client(socket_challenged,"\nVoulez-vous sauvegarder la partie ?\nVeuillez saisir soit /sauvegarder_oui soit /sauvegarder_non pour répondre.\n");
}

// Fonction pour gérer la sauvegarde d'une partie
static void sauvegarder_partie(Client * envoyeur, int has_saved)
{
   char affichage_1[BUF_SIZE];
   char affichage_2[BUF_SIZE];

   envoyeur->is_asked_to_save = 0;
   int position_challenge = obtenir_challenge_par_joueur(*envoyeur); 
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged = liste_challenges[position_challenge].challenged->sock;

   if (has_saved == 1)
   {
      envoyeur->want_to_save = 1;
      liste_challenges[position_challenge].sauvegardee = 1;
   }
   else
   {
      envoyeur->want_to_save = 0;
      liste_challenges[position_challenge].sauvegardee = 0;
   }

   envoyeur->is_challenged = 0;

   if(!liste_challenges[position_challenge].challenger->is_asked_to_save && !liste_challenges[position_challenge].challenged->is_asked_to_save)
   {
      initialiser(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);
      
      liste_challenges[position_challenge].tour = rand() % 2; 

      ecrire_client(socket_challenger,"\nEcrivez le numéro de la case à égréner.\n1 pour la case tout à gauche, jusqu'à 6 pour la case la plus à droite\n");
      ecrire_client(socket_challenged,"\nEcrivez le numéro de la case à égréner.\n1 pour la case tout à gauche, jusqu'à 6 pour la case la plus à droite\n");
      afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
      
      ecrire_client(socket_challenged, affichage_1);
      ecrire_client(socket_challenger, affichage_2); 
      
      if (liste_challenges[position_challenge].tour == 0)
      { 
         ecrire_client(socket_challenged,"A votre tour\n");
         ecrire_client(socket_challenger,"Au tour de votre adversaire");
      } 
      else 
      {
         ecrire_client(socket_challenged,"Au tour de votre adversaire");
         ecrire_client(socket_challenger,"A votre tour\n");
      }
   }
   else
   {
      ecrire_client(envoyeur->sock, "\nVeuillez attendre que votre adversaire ait répondu...\n");
   }
}

// Fonction pour refuser un challenge venant d'un joueur
static void refuser_challenge(Client * envoyeur)
{
   char reponse[BUF_SIZE];

   // Obtenir le challenge associé au joueur
   int position_challenge = obtenir_challenge_par_joueur_challenged(*envoyeur); 
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged = liste_challenges[position_challenge].challenged->sock;
   
   // Notifier le joueur ayant initié le challenge du refus

   snprintf(reponse, BUF_SIZE, "\n%s a refusé votre challenge.\n", envoyeur->name);
   liste_challenges[position_challenge].etat = 0;

   ecrire_client(socket_challenger, reponse);
   envoyeur->is_challenged = 0; 
}

// Fonction pour vérifier si une chaîne de caractères est un nombre
static int est_nombre(const char * chaine) 
{
   // Vérification que la chaîne n'est pas vide et ne contient que des chiffres
   if (chaine == NULL || chaine[0] == '\0') 
   {
      return 0;
   }

   int i = 0;
   while (chaine[i] != '\0') 
   {
      if (chaine[i] < '0' || chaine[i] > '9') 
      {
         return 0;
      }

      i++;
   }

   return 1; // Retourne 1 si c'est un nombre, sinon 0
}

// Fonction pour obtenir le challenge en cours associé à un joueur
static int obtenir_challenge_par_joueur(Client joueur)
{
   // Recherche du challenge en cours associé à un joueur
   for (int i = 0; i < total_challenges; i++) 
   {
      if ((liste_challenges[i].challenger->sock == joueur.sock || liste_challenges[i].challenged->sock == joueur.sock) && liste_challenges[i].etat == 1 && liste_challenges[i].challenger->is_playing) 
      {
         return i;
      }
   }

   return -1; // Retourne l'indice du challenge, -1 s'il n'y en a pas
}

// Fonction pour envoyer le tableau de jeu courant à tous les observeurs
static void envoyer_partie_aux_observeurs(int position_challenge, const char * affichage, int coup)
{
   char coup_joue[BUF_SIZE]; 

   for (int i = 0; i < liste_challenges[position_challenge].nb_observeurs; i++) 
   {
      if(coup != -1)
      {
         if(liste_challenges[position_challenge].tour == 0)
         {
            snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n", liste_challenges[position_challenge].challenged->name, coup + 1);
         }
         else
         {
            snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n", liste_challenges[position_challenge].challenger->name, coup + 1);
         }
      }
      
      ecrire_client(liste_challenges[position_challenge].liste_observeurs[i]->sock, coup_joue);
      ecrire_client(liste_challenges[position_challenge].liste_observeurs[i]->sock, affichage);
   }
}

// Fonction pour gérer un tour d'une partie d'awale
static void gerer_tour(int position_challenge, int coup, char * affichage_1, char * affichage_2)
{
   jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].tour, coup); 
   afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
   
   ecrire_client(liste_challenges[position_challenge].challenged->sock, affichage_1);
   ecrire_client(liste_challenges[position_challenge].challenger->sock, affichage_2); 

   envoyer_partie_aux_observeurs(position_challenge, affichage_1, coup); 
}

// Fonction pour gérer la fin d'une partie d'awale
static void gerer_fin_partie(int position_challenge, Client * gagnant, Client * perdant, int issue, char * fin_match)
{
   int D;

   if (issue == 1 || issue == 2)
   {
      liste_challenges[position_challenge].gagnant = gagnant;

      if(perdant->is_playing_with_ia)
      {
         perdant->is_playing_with_ia = 0;
      }
      else
      {
         ecrire_client(gagnant->sock, "\nFélicitations, vous avez gagné.\n" );
      }

      if(gagnant->is_playing_with_ia)
      {
         gagnant->is_playing_with_ia = 0;
      }
      else
      {
         ecrire_client(perdant->sock, "\nVous avez perdu.\n" );
      }

      D = gagnant->elo - perdant->elo;

      if(D > 400)
      { 
         D = 400;
      }

      calcul_classement_elo(gagnant, 1, D);
      calcul_classement_elo(perdant, 0, -D);
      snprintf(fin_match, BUF_SIZE, "%s a gagné.\n", gagnant->name);
   }
   else
   {
      if(perdant->is_playing_with_ia)
      {
         perdant->is_playing_with_ia = 0;
      }
      else
      {
         ecrire_client(gagnant->sock, "\nMatch nul.\n" );
      }
      
      if(gagnant->is_playing_with_ia)
      {
         gagnant->is_playing_with_ia = 0;
      }
      else
      {
         ecrire_client(perdant->sock, "\nMatch nul.\n" );
      }

      D = gagnant->elo - perdant->elo;

      if(D > 400)
      { 
         D = 400;
      }

      calcul_classement_elo(gagnant, 0.5, D);
      calcul_classement_elo(perdant, 0.5, -D);
      snprintf(fin_match, BUF_SIZE, "\nMatch nul.\n", liste_challenges[position_challenge].challenged->name);
   }

   // Sauvegarde
   if (gagnant->want_to_save != 0 && liste_challenges[position_challenge].nb_coups != 0)
   {
      sauvegarder_fichier(gagnant, position_challenge);
      gagnant->want_to_save = 0;
   }

   if (perdant->want_to_save != 0 && liste_challenges[position_challenge].nb_coups != 0)
   {
      sauvegarder_fichier(perdant, position_challenge);
      perdant->want_to_save = 0;
   }

   envoyer_partie_aux_observeurs(position_challenge, fin_match, -1); 

   liste_challenges[position_challenge].etat = 2; 
   liste_challenges[position_challenge].challenger->is_playing = 0; 
   liste_challenges[position_challenge].challenged->is_playing = 0;

   // Reinitialiser les observeurs
   for (int i = 0; i < liste_challenges[position_challenge].nb_observeurs; i++) 
   { 
      liste_challenges[position_challenge].liste_observeurs[i]->is_watching = 0;
      liste_challenges[position_challenge].liste_observeurs[i] = NULL;
   }

   liste_challenges[position_challenge].nb_observeurs = 0;
}

//Fonction qui calcule le nouveau clasement elo d'un joueur
static void calcul_classement_elo(Client * client, float W, int D)
{
   int K = 10;
   float p = 1/(1+pow(10, -D/400));
   int en = client->elo;
   client->nb_parties++;
   
   if(client->nb_parties < 30)
   {
      K = 40;
   }
   else if(client->elo < 2400)
   {
      K = 20;
   }

   client->elo = (int)(en + K*(W - p));

   if(client->elo < 0)
   {
      client->elo = 0;
   }

   char message[BUF_SIZE];
   snprintf(message, BUF_SIZE, "\nClassement elo de %s : %d\n\n", client->name, client->elo);
   
   if(client->sock != NULL)
   {
      ecrire_client(client->sock, message);
   }
}

// Fonction pour gérer le déroulement d'une partie d'awale
static void gerer_partie(Client * envoyeur, char * buffer)
{
   char affichage_1[BUF_SIZE];
   char affichage_2[BUF_SIZE];
   char fin_match[BUF_SIZE];

   int position_challenge = obtenir_challenge_par_joueur(*envoyeur);
   int coup = atoi(buffer);

   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged = liste_challenges[position_challenge].challenged->sock;

   if (socket_challenger == envoyeur->sock)
   {
      if (liste_challenges[position_challenge].tour) 
      {
         int est_coup_autorise = coup_autorise(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, &coup, liste_challenges[position_challenge].tour);
         
         if (est_coup_autorise == 1)
         {
            // Sauvegarde

            if(liste_challenges[position_challenge].challenged->want_to_save == 1 || liste_challenges[position_challenge].challenger->want_to_save == 1)
            {
               if (liste_challenges[position_challenge].nb_coups == 0)
               {
                  liste_challenges[position_challenge].coups = (int *) malloc(MAX_COUPS*sizeof(int));
                  liste_challenges[position_challenge].premier_joueur = liste_challenges[position_challenge].tour;
               }

               liste_challenges[position_challenge].coups[liste_challenges[position_challenge].nb_coups] = coup;
               liste_challenges[position_challenge].nb_coups++;
            }
            
            gerer_tour(position_challenge, coup, affichage_1, affichage_2); 
            liste_challenges[position_challenge].tour = 0;
            int est_partie_finie = partie_finie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);
            
            if (est_partie_finie == 0)
            {
               ecrire_client(socket_challenger,"\nAu tour de votre adversaire\n");
               ecrire_client(socket_challenged,"\nA votre tour\n");
            }
            else if (est_partie_finie == 1)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 2)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 3)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
         }
         else
         {
            if (est_coup_autorise == 0)
            {
               ecrire_client(socket_challenger,"\nIl faut donner un nombre entre 1 et 6. Veuillez réessayer.\n");
            }
            else if (est_coup_autorise == -1)
            {
               ecrire_client(socket_challenger,"\nIl faut jouer une case non vide. Veuillez réessayer.\n");
            }
            else if (est_coup_autorise == -2)
            {
               ecrire_client(socket_challenger,"\nIl faut donner des graines à votre adversaire. Veuillez réessayer.\n");
            }
         }

      } 
      else 
      {
         ecrire_client(socket_challenger, "\nAu tour de votre adversaire\n");  
      }
   } 
   else if (socket_challenged == envoyeur->sock)
   {
      if (!liste_challenges[position_challenge].tour) 
      {
         int est_coup_autorise = coup_autorise(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, &coup, liste_challenges[position_challenge].tour);
         
         if (est_coup_autorise == 1)
         {
            // Sauvegarde

            if(liste_challenges[position_challenge].challenged->want_to_save == 1 || liste_challenges[position_challenge].challenger->want_to_save == 1)
            {
               if (liste_challenges[position_challenge].nb_coups == 0)
               {
                  liste_challenges[position_challenge].coups = (int *) malloc(MAX_COUPS*sizeof(int));
                  liste_challenges[position_challenge].premier_joueur = liste_challenges[position_challenge].tour;
               }

               liste_challenges[position_challenge].coups[liste_challenges[position_challenge].nb_coups] = coup;
               liste_challenges[position_challenge].nb_coups++;
                  
            }

            gerer_tour(position_challenge, coup, affichage_1, affichage_2); 
            liste_challenges[position_challenge].tour = 1;
            int est_partie_finie = partie_finie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);

            if (est_partie_finie == 0)
            {
               ecrire_client(socket_challenged,"\nAu tour de votre adversaire\n" );
               ecrire_client(socket_challenger,"\nA votre tour\n" );
            }
            else if (est_partie_finie == 1)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 2)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 3)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
         }
         else
         {
            if (est_coup_autorise == 0)
            {
               ecrire_client(socket_challenged,"\nIl faut donner un nombre entre 1 et 6. Veuillez réessayer.\n");
            }
            else if (est_coup_autorise == -1)
            {
               ecrire_client(socket_challenged,"\nIl faut jouer une case non vide. Veuillez réessayer.\n");
            }
            else if (est_coup_autorise == -2)
            {
               ecrire_client(socket_challenged,"\nIl faut donner des graines à votre adversaire. Veuillez réessayer.\n");
            }
         }
      }
      else
      {
         ecrire_client(socket_challenged,"\nAu tour de votre adversaire\n");
      }
   }
}

// Fonction pour déclarer forfait au cours d'une partie
static void declarer_forfait_partie(Client * envoyeur, char* buffer)
{
   // Obtention du challenge associé au joueur
   int position_challenge = obtenir_challenge_par_joueur(*envoyeur);
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged;
   
   if(!envoyeur->is_playing_with_ia)
   {
      socket_challenged = liste_challenges[position_challenge].challenged->sock;
   }

   char fin_match[BUF_SIZE]; 
   liste_challenges[position_challenge].gagner_forfait = 1;

   if(envoyeur->is_playing_with_ia)
   {
      gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, 1, fin_match);
   }
   else if (socket_challenged == envoyeur->sock)// Si le joueur est celui qui a initié le challenge
   {
      ecrire_client(socket_challenger, "\nVotre adversaire a déclaré forfait.\n"); 
      gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, 2, fin_match);
   }
   else // Si le joueur est celui qui a été défié
   {
      ecrire_client(socket_challenged, "\nVotre adversaire a déclaré forfait.\n"); 
      gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, 1, fin_match);
   }
}

// Fonction pour afficher la liste des parties en cours
static void afficher_liste_parties(Client * envoyeur)
{
   char affichage[BUF_SIZE] = "";
   int match_trouve = 0;

   for (int i = 0; i < total_challenges; i++)
   {
      if (liste_challenges[i].etat == 1) 
      {
         match_trouve = 1;

         char temp[BUF_SIZE]; 
         snprintf(temp, BUF_SIZE, "ID Partie n°%d : %s VS %s\n", i + 1, liste_challenges[i].challenged->name, liste_challenges[i].challenger->name);
         strncat(affichage, temp, BUF_SIZE - strlen(affichage) - 1);
      }
   }

   if (match_trouve == 0)
   {
      ecrire_client(envoyeur->sock, "\nAucune partie n'est en cours pour l'instant.\n");
   } 
   else 
   {
      ecrire_client(envoyeur->sock, "\nVoici la liste des parties en cours : \n");
      ecrire_client(envoyeur->sock, affichage);
   }
}

// Fonction pour vérifier si un joueur peut observer une partie d'awale
static int est_partie_observable(Client * envoyeur, int id_partie) 
{
   Client * challenged = liste_challenges[id_partie].challenged;
   Client * challenger = liste_challenges[id_partie].challenger;
   
   return(
            (!challenged->prive && !challenger->prive) ||
            (challenged->prive && !challenger->prive && est_ami(envoyeur, challenged)) ||
            (!challenged->prive && challenger->prive && est_ami(envoyeur, challenger)) ||
            (challenged->prive && challenger->prive && est_ami(envoyeur, challenged) && est_ami(envoyeur, challenger))
         );
}

// Fonction pour observer une partie d'awale
static void observer_partie(Client * envoyeur, const char * buffer) 
{
   char affichage_1[BUF_SIZE];
   char affichage_2[BUF_SIZE];
   char * partie = buffer + strlen("/observer") + 1;
   int id_partie = atoi(partie) - 1;

   if (id_partie >= 0 && id_partie < total_challenges) 
   {
      if(liste_challenges[id_partie].etat == 1)
      {
         if (liste_challenges[id_partie].nb_observeurs < MAX_OBSERVEURS) 
         {
            if (est_partie_observable(envoyeur, id_partie)) 
            { 
               liste_challenges[id_partie].liste_observeurs[liste_challenges[id_partie].nb_observeurs] = envoyeur;
               liste_challenges[id_partie].nb_observeurs++;
               envoyeur->is_watching = 1; 

               afficher_partie(liste_challenges[id_partie].case_joueur1, liste_challenges[id_partie].case_joueur2, liste_challenges[id_partie].points, liste_challenges[id_partie].challenged->name, liste_challenges[id_partie].challenger->name, affichage_1, affichage_2);
               ecrire_client(envoyeur->sock, affichage_1);
            }
            else 
            {
               ecrire_client(envoyeur->sock, "\nVous ne pouvez pas observer cette partie puisqu'elle est privée.\n"); 
            }
         } 
         else 
         {
            ecrire_client(envoyeur->sock ,"\nCette partie est observé par beaucoup trop d'observateurs.\n"); 
         }
      }
      else
      {
         ecrire_client(envoyeur->sock ,"\nCette partie n'a pas été trouvé. Assurez vous qu'elle soit bien en cours en faisant appel à la commande /liste_parties\n"); 
      }
   } 
   else 
   {
      ecrire_client(envoyeur->sock ,"\nCette partie n'a pas été trouvé. Assurez vous qu'elle soit bien en cours en faisant appel à la commande /liste_parties\n"); 
   }
}

// Fonction pour arrêter d'observer une partie d'awale
static void arreter_observer_partie(Client * envoyeur) 
{
   for (int i = 0; i < total_challenges; i++) 
   {
      for (int j = 0; j < liste_challenges[i].nb_observeurs; j++) 
      {
         if (liste_challenges[i].liste_observeurs[j] == envoyeur) 
         {
            for (int k = j; k < liste_challenges[i].nb_observeurs - 1; k++)  // Retirer le client du tableau des observateurs
            {
               liste_challenges[i].liste_observeurs[k] = liste_challenges[i].liste_observeurs[k + 1];
            }

            liste_challenges[i].nb_observeurs--;
            envoyeur->is_watching = 0;
            return;
         }
      }
   }
}

// Fonction pour trouver le challenge en cours associé à un joueur lors de la déconnexion
static int obtenir_challenge_par_joueur_deconnecte(Client joueur)
{
   for (int i = 0; i < total_challenges; i++) 
   {
      if ((liste_challenges[i].challenged->sock == joueur.sock || liste_challenges[i].challenger->sock == joueur.sock) && (liste_challenges[i].etat == 1 || liste_challenges[i].etat == -1)) 
      {
         return i;
      }
   }

   return -1; // Retourne l'indice du challenge, -1 s'il n'y en a pas
}

// Fonction pour vérifier si deux clients sont amis
static int est_ami(Client * envoyeur, Client * joueur_demande_ami) 
{
    for (int i = 0; i < envoyeur->nb_amis; i++) 
    {
        if (envoyeur->liste_amis[i] == joueur_demande_ami) 
        {
            return 1; // Cas amis
        }
    }

    return 0; // Cas pas amis
}

// Fonction pour vérifier si une requête d'ami est en attente
static int requete_ami_en_attente(Client * envoyeur, Client * joueur_demande_ami) 
{
   for (int i = 0; i < envoyeur->nb_requetes_ami_en_attente; i++) 
   {
      if (envoyeur->liste_requetes_ami_en_attente[i] == joueur_demande_ami) 
      {
         return 1; // Requete en attente
      }
   }
   
   return 0; // Requete pas en attente
}

// Fonction pour gérer les demandes d'ami
static void gerer_demande_ami(Client * envoyeur, Client * clients, int actual, const char * buffer)
{
   Client * joueur_demande_ami = obtenir_client_par_nom(clients, buffer + strlen("/ajouter_ami") + 1, actual);
   
   if (joueur_demande_ami == NULL)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur n'existe pas.\n"); 
      return; 
   }
   else if (est_ami(envoyeur, joueur_demande_ami))
   {
      ecrire_client(envoyeur->sock, "\nCe joueur est déjà votre ami.\n"); 
      return; 
   }
   else if (requete_ami_en_attente(envoyeur, joueur_demande_ami))
   {
      ecrire_client(envoyeur->sock, "\nCette personne vous a déjà envoyé une requête d'ami.\n"); 
      return; 
   }
   else if (requete_ami_en_attente(joueur_demande_ami, envoyeur))
   {
      ecrire_client(envoyeur->sock, "\nVous avez dejà envoyé une requête d'ami à cette personne.\n"); 
      return; 
   }
   else if (joueur_demande_ami == envoyeur)
   {
      ecrire_client(envoyeur->sock ,"\nVous ne pouvez pas vous envoyer une requête d'ami.\n") ;
      return; 
   }

   if (joueur_demande_ami->nb_requetes_ami_en_attente < MAX_REQUETE_AMIS_EN_ATTENTE) 
   {
      joueur_demande_ami->liste_requetes_ami_en_attente[joueur_demande_ami->nb_requetes_ami_en_attente] = envoyeur;
      joueur_demande_ami->nb_requetes_ami_en_attente++;

      char invitation[BUF_SIZE];
      strcpy(invitation, "\n");
      strcat(invitation, envoyeur->name);
      strcat(invitation, " souhaite vous ajouter en ami.\n\nVeuillez saisir soit /accepter_ami [pseudo] soit /refuser_ami [pseudo] pour répondre.\n"); 

      ecrire_client(joueur_demande_ami->sock, invitation);
      ecrire_client(envoyeur->sock, "\nRequête d'ami envoyée.\n"); 
   } 
   else 
   {
      ecrire_client(envoyeur->sock, "\nLa liste des requêtes d'ami de ce joueur est pleine. Veuillez réessayer plus tard.\n");
   }
}

// Fonction pour retirer un ami de la liste d'amis d'un client
static void retirer_ami(Client * envoyeur, Client * clients, int actual, const char * buffer)
{
   Client * joueur_retirer_ami = obtenir_client_par_nom(clients, buffer + strlen("/retirer_ami") + 1, actual);
   char message[BUF_SIZE];

   if (joueur_retirer_ami == NULL)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur n'existe pas.\n"); 
      return; 
   } 
   else if (joueur_retirer_ami == envoyeur)
   {
      ecrire_client(envoyeur->sock ,"\nVous ne faites pas partie de votre propre liste d'amis.\n") ;
      return; 
   }
   
   if (est_ami(envoyeur, joueur_retirer_ami)) 
   {
      for (int i = 0; i < envoyeur->nb_amis; i++) 
      {
         if (envoyeur->liste_amis[i] == joueur_retirer_ami) 
         {
            for (int j = i; j < envoyeur->nb_amis - 1; j++) 
            {
               envoyeur->liste_amis[j] = envoyeur->liste_amis[j + 1];
            }
            
            envoyeur->nb_amis--;
         }
      }

      for (int i = 0; i < joueur_retirer_ami->nb_amis; i++) 
      {
         if (joueur_retirer_ami->liste_amis[i] == envoyeur) 
         {
            for (int j = i; j < joueur_retirer_ami->nb_amis - 1; j++) 
            {
               joueur_retirer_ami->liste_amis[j] = joueur_retirer_ami->liste_amis[j + 1]; 
            }

            joueur_retirer_ami->nb_amis--;
         }
      }

      snprintf(message, BUF_SIZE, "\n%s a été retiré de vos amis.\n", joueur_retirer_ami->name);
      ecrire_client(envoyeur->sock, message);

      snprintf(message, BUF_SIZE, "\n%s vous a retiré de ses amis.\n", envoyeur->name);
      ecrire_client(joueur_retirer_ami->sock, message);
   } 
   else 
   {
      snprintf(message, BUF_SIZE, "\n%s ne fait pas partie de vos amis.\n", joueur_retirer_ami->name);
      ecrire_client(envoyeur->sock, message);
   }
}

// Fonction pour accepter une requête d'ami
static void accepter_requete_ami(Client * envoyeur, Client * clients, int actual, const char * buffer)
{
   Client * joueur_nouvel_ami = obtenir_client_par_nom(clients, buffer + strlen("/accepter_ami") + 1 , actual);
   int requete_trouvee = 0;
   char message[BUF_SIZE];

   for (int i = 0; i < envoyeur->nb_requetes_ami_en_attente; i++) 
   {
      if (envoyeur->liste_requetes_ami_en_attente[i] == joueur_nouvel_ami) 
      {
         if (envoyeur->nb_amis < MAX_AMIS && joueur_nouvel_ami->nb_amis < MAX_AMIS) 
         {
            envoyeur->liste_amis[envoyeur->nb_amis] = joueur_nouvel_ami; 
            envoyeur->nb_amis++;

            joueur_nouvel_ami->liste_amis[joueur_nouvel_ami->nb_amis] = envoyeur; 
            joueur_nouvel_ami->nb_amis++;

            strcpy(message, "\n");
            strcat(message, envoyeur->name);
            strcat(message, " a accepté votre requête d'ami.\n");
            ecrire_client(joueur_nouvel_ami->sock, message);

            snprintf(message, BUF_SIZE, "\nVous êtes maintenant ami avec %s.\n", joueur_nouvel_ami->name);
            ecrire_client(envoyeur->sock, message);

            for (int j = i; j < envoyeur->nb_requetes_ami_en_attente - 1; ++j) 
            {
               envoyeur->liste_requetes_ami_en_attente[j] = envoyeur->liste_requetes_ami_en_attente[j + 1];
            }

            envoyeur->nb_requetes_ami_en_attente--;
         } 
         else if(envoyeur->nb_amis >= MAX_AMIS)
         {
            ecrire_client(envoyeur->sock, "\nVotre liste d'ami est déjà pleine.\n");
         } 
         else if(joueur_nouvel_ami->nb_amis >= MAX_AMIS)
         {
            snprintf(message, BUF_SIZE, "\nLa liste d'ami de %s est déjà pleine.\n", joueur_nouvel_ami->name);
            ecrire_client(envoyeur->sock, message);
         }

         requete_trouvee = 1;
      }
   }

   if (!requete_trouvee) 
   {
      ecrire_client(envoyeur->sock, "\nCe joueur ne vous a pas encore envoyé de requête d'ami.\n");
   }
}

// Fonction pour refuser une requête d'ami
static void refuser_requete_ami(Client * envoyeur, Client * clients, int actual, const char * buffer)
{
   Client * joueur_retire_ami = obtenir_client_par_nom(clients, buffer + strlen("/retirer_ami") + 1, actual);
   int requete_trouvee = 0;
   char message[BUF_SIZE];

   for (int i = 0; i < envoyeur->nb_requetes_ami_en_attente; ++i) 
   {
      if (envoyeur->liste_requetes_ami_en_attente[i] == joueur_retire_ami) 
      {
         for (int j = i; j < envoyeur->nb_requetes_ami_en_attente - 1; ++j) 
         {
            envoyeur->liste_requetes_ami_en_attente[j] = envoyeur->liste_requetes_ami_en_attente[j + 1];
         }

         envoyeur->nb_requetes_ami_en_attente--;

         strcpy(message, "\n");
         strcat(message, envoyeur->name);
         strcat(message, " a refusé votre requête d'ami.\n"); 
         ecrire_client(joueur_retire_ami->sock, message);

         requete_trouvee = 1;
      }
   }
   
   if (!requete_trouvee) 
   {
      ecrire_client(envoyeur->sock, "\nCe joueur ne vous a pas envoyé de requête d'ami.\n");
   }
}

// Fonction pour afficher la liste des amis d'un joueur
static void afficher_liste_amis(Client * envoyeur) 
{
   char reponse[BUF_SIZE];
   reponse[0] = 0;

   if (envoyeur->nb_amis > 0) 
   {
        strcat(reponse, "\nListe de vos amis :\n");

        for (int i = 0; i < envoyeur->nb_amis; i++) 
        {
            strcat(reponse, envoyeur->liste_amis[i]->name);
            strcat(reponse, "\n");
        }
   } 
   else 
   {
      strcat(reponse, "\nPour l'instant, vous n'avez aucun amis.\n");
   }

   ecrire_client(envoyeur->sock, reponse); 
}

// Fonction pour afficher la liste des requêtes d'ami en attente pour un joueur
static void afficher_liste_requete_ami(Client * envoyeur) 
{
   char reponse[BUF_SIZE];
   reponse[0] = 0;

   if (envoyeur->nb_requetes_ami_en_attente > 0) 
   {
      strcat(reponse, "\nListe des joueurs qui vous ont demandé en ami :\n");

      for (int i = 0; i < envoyeur->nb_requetes_ami_en_attente; ++i) 
      {
         strcat(reponse, envoyeur->liste_requetes_ami_en_attente[i]->name);
         strcat(reponse, "\n");
      }
   } 
   else 
   {
      strcat(reponse, "\nPour l'instant, vous n'avez pas de requête d'ami pour le moment.\n");
   }

   ecrire_client(envoyeur->sock, reponse); 
}

// Fonction pour écrire la bio d'un joueur
static void ecrire_bio(Client * envoyeur, char * buffer)
{
   char * bio_start = buffer + strlen("/ecrire_bio") + 1;

   if (strlen(bio_start) > MAX_BIO) 
   {
      ecrire_client(envoyeur->sock, "\nVotre bio est trop longue (limite : 500 caractères).\n");
   } 
   else 
   {
      strcpy(envoyeur->bio, bio_start);
      ecrire_client(envoyeur->sock, "\nVotre bio a bien été enregistré.\n");
   }
}

// Fonction pour afficher la bio d'un joueur
static void afficher_bio(Client * envoyeur, Client * clients, int actual, char * buffer)
{
   Client * joueur_afficher_bio = obtenir_client_par_nom(clients, buffer + strlen("/consulter_bio") + 1, actual);

   if (joueur_afficher_bio == NULL)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur n'existe pas.\n") ; 
      return; 
   }
   else if (strlen(joueur_afficher_bio->bio) == 0)
   {
      ecrire_client(envoyeur->sock, "\nCe joueur n'a pas encore de bio.\n") ; 
      return; 
   }

   char bio[BUF_SIZE];
   strcpy(bio, "\n");
   strcat(bio, joueur_afficher_bio->bio);
   strcat(bio, "\n");
   ecrire_client(envoyeur->sock, bio); 
}

// Fonction pour passer en mode privé
static int passer_prive(Client * envoyeur) 
{
   if (envoyeur->prive == 1) 
   {
      ecrire_client(envoyeur->sock, "\nVous êtes déjà en privé.\n"); 
   } 
   else 
   {
      envoyeur->prive = 1;
      ecrire_client(envoyeur->sock, "\nVous êtes maintenant passé en privé.\n"); 
   }
}

// Fonction pour passer en mode public
static int passer_public(Client * envoyeur) 
{
   if (envoyeur->prive == 0)
   {
      ecrire_client(envoyeur->sock ,"\nVous êtes déjà en public.\n"); 
   } 
   else 
   {
      envoyeur->prive = 0;
      ecrire_client(envoyeur->sock ,"\nVous êtes maintenant passé en public.\n"); 
   }
}

// Fonction pour extraire la chaîne entre deux espaces
static void extraire_entre_deux_espaces(const char * chaine, char * resultat, size_t taille_resultat) 
{
   const char * premiere_espace = strchr(chaine, ' ');
   
   if (premiere_espace == NULL) 
   {
      resultat[0] = '\0';
      return;
   }
   
   const char* deuxieme_espace = strchr(premiere_espace + 1, ' ');
   
   if (deuxieme_espace == NULL) 
   {
      resultat[0] = '\0';
      return;
   }

   size_t longueur = deuxieme_espace - (premiere_espace + 1);

   if (longueur < taille_resultat) 
   {
      strncpy(resultat, premiere_espace + 1, longueur);
      resultat[longueur] = '\0';
   } 
   else 
   {
      strncpy(resultat, premiere_espace + 1, taille_resultat - 1);
      resultat[taille_resultat - 1] = '\0';
   }
}

// Fonction pour discuter avec d'autres joueurs en dehors d'une partie d'awale
static void discuter_hors_partie(Client * envoyeur, char * buffer, Client * clients, int actual)
{
   char affichage[BUF_SIZE];
   char pseudo[BUF_SIZE]; 
   extraire_entre_deux_espaces(buffer, pseudo, sizeof(pseudo)); 

   Client * destinataire = obtenir_client_par_nom(clients, pseudo, actual);
   if (destinataire == NULL)
   {
      ecrire_client(envoyeur->sock,"\nLe joueur avec lequel vous voulez discuter n'existe pas.\n"); 
      return; 
   } 

   char * buffer_start = buffer + strlen("/discuter_out") + strlen(destinataire->name) + 2;
   
   strcpy(affichage, "\n");
   strcat(affichage, envoyeur->name); 
   strcat(affichage, " : ");
   strcat(affichage, buffer_start);
   strcat(affichage, "\n");

   ecrire_client(destinataire->sock,affichage); 
}

// Fonction pour discuter avec son adversaire lors d'une partie d'awale
static void discuter_en_partie(Client * envoyeur, char * buffer)
{
   char affichage[BUF_SIZE];  
   int position_challenge = obtenir_challenge_par_joueur(*envoyeur);

   strcpy(affichage, "\n");
   strcat(affichage, envoyeur->name); 
   strcat(affichage, " : ");
   char * buffer_start = buffer + strlen("/discuter_in") + 1;
   strcat(affichage, buffer_start); 
   strcat(affichage, "\n");

   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   int socket_challenged = liste_challenges[position_challenge].challenged->sock;
   
   if (socket_challenger == envoyeur->sock)
   {
      ecrire_client(socket_challenged, affichage); 
   }
   else
   {
      ecrire_client(socket_challenger, affichage);    
   }
}

// Fonction pour permettre à un client de se deconnecter du serveur
static void se_deconnecter(Client clients[], int position_client, int * actual)
{
   int position_challenge = obtenir_challenge_par_joueur_deconnecte(clients[position_client]);
   char fin_match[BUF_SIZE];

   if (position_challenge != -1) 
   {
      liste_challenges[position_challenge].joueur_deconnecte = 1;

      if (strcmp(clients[position_client].name, liste_challenges[position_challenge].challenged->name) == 0)
      {
         if (liste_challenges[position_challenge].etat == -1)
         {
            ecrire_client(liste_challenges[position_challenge].challenger->sock, "\nLe client challengé s'est déconnecté.\n");
         }
         else if (liste_challenges[position_challenge].etat == 1)
         {
            ecrire_client(liste_challenges[position_challenge].challenger->sock, "\nVotre adversaire s'est déconnecté.\n");
            gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, 1, fin_match);
            liste_challenges[position_challenge].challenger->is_playing = 0; 
         }
      }
      else
      {
         if (liste_challenges[position_challenge].etat == -1)
         {
            ecrire_client(liste_challenges[position_challenge].challenged->sock , "\nLe client qui vous a challengé s'est déconnecté.\n"); 
            liste_challenges[position_challenge].challenged->is_challenged = 0 ;
         }
         else if (liste_challenges[position_challenge].etat == 1)
         {
            if(liste_challenges[position_challenge].challenged->sock != NULL)
            {
               ecrire_client(liste_challenges[position_challenge].challenged->sock , "\nVotre adversaire s'est déconnecté.\n");
            }
            
            gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, 1, fin_match);
            liste_challenges[position_challenge].challenged->is_playing = 0 ;
         }
      }

      liste_challenges[position_challenge].etat = 2;
   } 

   closesocket(clients[position_client].sock);
   retirer_client(clients, position_client, actual);
}

// Fonction pour sauvegarder une partie dans un fichier
static void sauvegarder_fichier(Client * envoyeur, int position_challenge)
{
   if (envoyeur->want_to_save != 0)
   {
      char nom[50];
      strcpy(nom, envoyeur->name);
      strcat(nom, ".txt");
      FILE * f = fopen(nom, "a");

      if(f == NULL)
      {
         ecrire_client(envoyeur->sock, "\nErreur de sauvegarde.\n");
         return;
      }
      else
      {
         int size = liste_challenges[position_challenge].nb_coups;
         char string_tab[MAX_FILE_LINE_SIZE]; 
         sprintf(string_tab, "%d\n", position_challenge + 1);

         fputs(string_tab, f);
      }

      fclose(f);
   }
}

// Fonction pour l'historique des parties sauvegardées par un joueur
static void liste_historique(Client * envoyeur)
{
   char affichage[BUF_SIZE] = "";
   int match_trouve = 0;

   char nom[50];
   strcpy(nom, envoyeur->name);
   strcat(nom, ".txt");
   FILE * f = fopen(nom, "r");

   if(f == NULL)
   {
      ecrire_client(envoyeur->sock, "\nAucune partie sauvegardée pour l'instant.\n");
      return;
   }
   else
   {
      char s[MAX_FILE_LINE_SIZE] = "";
      while (fgets(s, MAX_FILE_LINE_SIZE, f))
      {
         int position_challenge = -1;

         position_challenge = atoi(s) - 1;

         if (&liste_challenges[position_challenge] != NULL)
         {
            match_trouve = 1;

            char temp[BUF_SIZE]; 
            snprintf(temp, BUF_SIZE, "ID Partie n°%d : %s VS %s\n", position_challenge + 1, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name);
            strncat(affichage, temp, BUF_SIZE - strlen(affichage) - 1);
         }
      }

   }

   fclose(f);

   if (match_trouve == 0)
   {
      ecrire_client(envoyeur->sock, "\nAucune partie sauvegardée pour l'instant.\n");
   } 
   else 
   {
      ecrire_client(envoyeur->sock, "\nHistorique de vos parties sauvegardées : \n");
      ecrire_client(envoyeur->sock, affichage);
   }
}

// Fonction pour consulter une partie sauvgardée
static void consulter_partie_sauvegardee(Client * envoyeur, char * buffer)
{
   char nom[50];
   strcpy(nom, envoyeur->name);
   strcat(nom, ".txt");
   FILE * f = fopen(nom, "r");

   if(f == NULL)
   {
      ecrire_client(envoyeur->sock, "\nAucune partie sauvegardée pour l'instant.\n");
      return;
   }

   char * partie = buffer + strlen("/partie_sauvegardee") + 1;
   int id_partie = atoi(partie) - 1;
   int position_challenge = -1;
   int match_trouve = 0;

   char s[MAX_FILE_LINE_SIZE] = "";

   while (fgets(s, MAX_FILE_LINE_SIZE, f))
   {
      position_challenge = atoi(s) - 1;

      if (position_challenge == id_partie)
      {
         match_trouve = 1;
         break;
      }
      
   }

   fclose(f);

   // Simuler partie sauvegardée

   if (match_trouve == 1)
   {
      char coup_joue[100];
      char affichage_1[BUF_SIZE];
      char affichage_2[BUF_SIZE];

      initialiser(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);

      for(int i = 0; i < liste_challenges[position_challenge].nb_coups; i++)
      {
         if(liste_challenges[position_challenge].coups[i] >= 0 && liste_challenges[position_challenge].coups[i] < 6)
         {
            if (liste_challenges[position_challenge].premier_joueur) 
            {
               jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].premier_joueur, liste_challenges[position_challenge].coups[i]); 
               afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
               
               if(i == 0)
               {
                  snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n", liste_challenges[position_challenge].challenger->name, liste_challenges[position_challenge].coups[i] + 1);
                  ecrire_client(envoyeur->sock, coup_joue);
               }
               else
               {
                  snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n\n", liste_challenges[position_challenge].challenger->name, liste_challenges[position_challenge].coups[i] + 1);
                  ecrire_client(envoyeur->sock, coup_joue);
               }

               if(liste_challenges[position_challenge].challenged->sock == envoyeur->sock)
               {
                  ecrire_client(envoyeur->sock, affichage_1);
               } 
               else
               {
                  ecrire_client(envoyeur->sock, affichage_2); 
               }

               liste_challenges[position_challenge].premier_joueur = 0;
            }
            else
            {
               jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].premier_joueur, liste_challenges[position_challenge].coups[i]); 
               afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
               
               if(i == 0)
               {
                  snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n", liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].coups[i] + 1);
                  ecrire_client(envoyeur->sock, coup_joue);
               }
               else
               {
                  snprintf(coup_joue, 100, "\nCoup joué par %s : %d\n\n", liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].coups[i] + 1);
                  ecrire_client(envoyeur->sock, coup_joue);
               }

               if(liste_challenges[position_challenge].challenged->sock == envoyeur->sock)
               {
                  ecrire_client(envoyeur->sock, affichage_1);
               } 
               else
               {
                  ecrire_client(envoyeur->sock, affichage_2); 
               }

               liste_challenges[position_challenge].premier_joueur = 1;
            }
         }
         else
         {
            ecrire_client(envoyeur->sock, "\nLe fichier de sauvegarde est erroné. Arrêt du visionnage de la partie sauvegardée.\n");
            return;
         }
      }

      if(liste_challenges[position_challenge].gagnant == NULL)
      {
         ecrire_client(envoyeur->sock, "\nMatch nul.\n");
      }
      else if(liste_challenges[position_challenge].gagnant->sock == envoyeur->sock)
      {
         if(liste_challenges[position_challenge].gagner_forfait == 1)
         {
            ecrire_client(envoyeur->sock, "\nVous avez remporté la partie car l'adversaire a déclaré forfait.\n");
         }
         else
         {
            ecrire_client(envoyeur->sock, "\nVous avez remporté la partie\n");
         }
      }
      else
      {
         if(liste_challenges[position_challenge].gagner_forfait == 1)
         {
            ecrire_client(envoyeur->sock, "\nVous avez perdu la partie en déclarant forfait.\n");
         }
         else
         {
            ecrire_client(envoyeur->sock, "\nVous avez perdu la partie\n");
         }
      }
   }
   else
   {
      ecrire_client(envoyeur->sock, "\nAucune partie sauvegardée ne correspond à l'id de partie saisie.\n");
   } 
}

// Fonction pour challenger une IA
static void challenger_ia(Client * envoyeur, Client * IA)
{
   // Création d'une nouvelle demande de challenge

   liste_challenges[total_challenges].challenger = envoyeur; 
   liste_challenges[total_challenges].challenged = IA;    
   liste_challenges[total_challenges].etat = 1;  // l'IA accepte toujours les challenges
   liste_challenges[total_challenges].challenger->is_playing = 1;
   liste_challenges[total_challenges].challenger->is_playing_with_ia = 1;
   liste_challenges[total_challenges].nb_observeurs = 0;
   liste_challenges[total_challenges].tour = rand() % 2; 
   total_challenges++;

   char affichage_1[BUF_SIZE];
   char affichage_2[BUF_SIZE];
   char reponse[BUF_SIZE];
   
   // Obtenir la position du challenge en cours pour le joueur 
   int position_challenge = obtenir_challenge_par_joueur(*envoyeur); 
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   
   // Préparation pour le début de la partie
   initialiser(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);
   
   ecrire_client(socket_challenger,"\nEcrivez le numéro de la case à égréner.\n1 pour la case tout à gauche, jusqu'à 6 pour la case la plus à droite\n");
   afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
   
   ecrire_client(socket_challenger, affichage_2); 
   
   if (liste_challenges[position_challenge].tour == 1)
   { 
      ecrire_client(socket_challenger,"A votre tour\n");
   } 
   else 
   {
      ecrire_client(socket_challenger,"Au tour de l'IA\n");
      //gestion du 1er coup de l'IA
      int coup = (rand() % 6) + 1;

      jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].tour, coup); 
      afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);

      ecrire_client(socket_challenger, affichage_2);

      envoyer_partie_aux_observeurs(position_challenge, affichage_2, coup - 1); 
      liste_challenges[position_challenge].tour = 1;

      ecrire_client(socket_challenger,"A votre tour\n");
   }
   
}

// Fonction pour gérer le déroulement d'une partie d'awale avec une IA
static void gerer_partie_ia(Client * envoyeur, char * buffer)
{
   char affichage_1[BUF_SIZE];
   char affichage_2[BUF_SIZE];
   char fin_match[BUF_SIZE];

   int coup = atoi(buffer);
   int position_challenge = obtenir_challenge_par_joueur(*envoyeur); 
   int socket_challenger = liste_challenges[position_challenge].challenger->sock; 
   
   // Tour du joueur
   if (liste_challenges[position_challenge].tour == 1) 
   {
      int est_coup_autorise = coup_autorise(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, &coup, liste_challenges[position_challenge].tour);
      
      if (est_coup_autorise == 1)
      {
         // Gestion du tour
         jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].tour, coup); 
         afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
         ecrire_client(socket_challenger, affichage_2); 
         envoyer_partie_aux_observeurs(position_challenge, affichage_2, coup); 

         // Fin du tour
         liste_challenges[position_challenge].tour = 0;
         int est_partie_finie = partie_finie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);
         
         if (est_partie_finie == 0)
         {
            ecrire_client(socket_challenger,"Au tour de l'IA\n");
            
            // Tour de l'IA
            coup = (rand() % 6) + 1;
            est_coup_autorise = coup_autorise(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, &coup, liste_challenges[position_challenge].tour);
            
            while(est_coup_autorise != 1)
            {
               coup = (rand() % 6) + 1;
               est_coup_autorise = coup_autorise(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, &coup, liste_challenges[position_challenge].tour);
            }

            // Gestion du tour
            jouer_tour(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].tour, coup); 
            afficher_partie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points, liste_challenges[position_challenge].challenged->name, liste_challenges[position_challenge].challenger->name, affichage_1, affichage_2);
            ecrire_client(socket_challenger, affichage_2); 
            envoyer_partie_aux_observeurs(position_challenge, affichage_2, coup); 

            // Fin du tour
            liste_challenges[position_challenge].tour = 1;
            int est_partie_finie = partie_finie(liste_challenges[position_challenge].case_joueur1, liste_challenges[position_challenge].case_joueur2, liste_challenges[position_challenge].points);

            // Gestion de fin de partie
            if(est_partie_finie == 0)
            {
               ecrire_client(socket_challenger,"A votre tour\n");
            }
            else if (est_partie_finie == 1)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 2)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, est_partie_finie, fin_match);
            }
            else if (est_partie_finie == 3)
            {
               gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
            }
         }
         else if (est_partie_finie == 1)
         {
            gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
         }
         else if (est_partie_finie == 2)
         {
            gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenger, liste_challenges[position_challenge].challenged, est_partie_finie, fin_match);
         }
         else if (est_partie_finie == 3)
         {
            gerer_fin_partie(position_challenge, liste_challenges[position_challenge].challenged, liste_challenges[position_challenge].challenger, est_partie_finie, fin_match);
         }
         
      }
      else
      {
         if (est_coup_autorise == 0)
         {
            ecrire_client(socket_challenger,"\nIl faut donner un nombre entre 1 et 6. Veuillez réessayer.\n");
         }
         else if (est_coup_autorise == -1)
         {
            ecrire_client(socket_challenger,"\nIl faut jouer une case non vide. Veuillez réessayer.\n");
         }
         else if (est_coup_autorise == -2)
         {
            ecrire_client(socket_challenger,"\nIl faut donner des graines à votre adversaire. Veuillez réessayer.\n");
         }
      }
   } 
   else 
   {
      ecrire_client(socket_challenger, "\nAu tour de l'IA\n");  
   }
}

// Fonction pour obtenir le challenge en cours d'un joueur challengeur
static int obtenir_challenge_par_joueur_challenger(Client challenger)
{
   for (int i = 0; i < total_challenges; i++) 
   {
      if (liste_challenges[i].challenger->sock == challenger.sock) 
      {
         return i;
      }
   }

   return -1; // Retourne l'indice du challenge, -1 s'il n'y en a pas
}

// Fonction principale du programme
int main(int argc, char** argv)
{
   time_t t;
   srand((unsigned)time(&t));

   if(argc != 2)
   {
      printf("Usage : %s [port_du_serveur]\n", argv[0]);
      return EXIT_FAILURE;
   }

   initialisation(); // Initialise le programme

   application(atoi(argv[1])); // Exécute l'application principale

   fin(); // Termine l'exécution du programme

   return EXIT_SUCCESS; // Indique que le programme s'est terminé avec succès
}
