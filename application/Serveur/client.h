#ifndef CLIENT_H
#define CLIENT_H

#include "serveur.h"

typedef struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[MAX_BIO];
   int nb_parties;
   int elo; 
   struct Client * liste_amis[MAX_AMIS]; 
   struct Client * liste_requetes_ami_en_attente[MAX_REQUETE_AMIS_EN_ATTENTE]; 
   int nb_amis;
   int nb_requetes_ami_en_attente;
   int is_challenged;
   int is_asked_to_save;
   int want_to_save;
   int is_playing;
   int is_playing_with_ia;
   int is_watching;
   int prive; // 1 : Mode privé | 0 : Mode public
} Client;

#endif // CLIENT_H
