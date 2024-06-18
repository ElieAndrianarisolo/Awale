#ifndef SERVEUR_H
#define SERVEUR_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

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

#define CRLF                        "\r\n"
#define MAX_CHALLENGES              1000
#define MAX_CLIENTS                 100
#define MAX_OBSERVEURS              5
#define MAX_AMIS                    10
#define MAX_REQUETE_AMIS_EN_ATTENTE 10
#define MAX_COUPS                   200
#define BUF_SIZE                    1024
#define MAX_FILE_LINE_SIZE          5
#define MAX_BIO                     500

#include "client.h"
#include "awale_serveur.h"

typedef struct 
{
    Client * challenger; 
    Client * challenged;  
    Client * gagnant;
    int gagner_forfait; // 0 : sans forfait | 1 : avec forfait
    int etat; // -1 : challenge en attente | 0 : challenge refusé | 1 : challenge accepté | 2 : challenge fini
    int case_joueur1[6];
	int case_joueur2[6];
    int points[2];
    int tour; // 0 : challenged | 1 : challenger
    Client * liste_observeurs[MAX_OBSERVEURS];
    int nb_observeurs;
    int joueur_deconnecte; 
    int premier_joueur; // 0 : challenged | 1 : challenger
    int nb_coups;
    int * coups;
    int sauvegardee; // 0 : aucune sauvegarde | 1 : au moins 1 joueur a sauvegardé
} Challenge;

typedef struct 
{
    char commande[50];
    char description[200]; 
} Commande;

Challenge liste_challenges[MAX_CHALLENGES];
int total_challenges = 0;

static void initialisation(void);
static void fin(void);
static void application(int port);
static int initialiser_connection(int port);
static void terminer_connection(int sock);
static int lire_client(SOCKET sock, char * buffer);
static void ecrire_client(SOCKET sock, const char * buffer);
static void envoyer_message_tous_clients(Client * clients, Client client, int actual, const char * buffer, char from_server);
static void retirer_client(Client * clients, int to_remove, int * actual);
static void effacer_clients(Client * clients, int actual);

static void afficher_liste_joueurs(Client * envoyeur, Client * clients, int actual);
static void afficher_liste_classement_joueurs(Client * envoyeur, Client * clients, int actual);
static int tri_elo(const void* a, const void* b);
static Client * obtenir_client_par_nom(Client * clients, const char * buffer, int actual);
static void calcul_classement_elo( Client * client, float W, int D);
static void challenger_joueur(Client * envoyeur, Client * clients, int actual, const char * buffer);
static void accepter_challenge(Client * envoyeur);
static void sauvegarder_partie(Client * envoyeur, int has_saved);
static int obtenir_challenge_par_joueur_challenged(Client challenged);
static void refuser_challenge(Client * envoyeur);

static int est_nombre(const char * chaine);
static void gerer_tour(int position_challenge, int coup, char * affichage_1, char * affichage_2);
static void gerer_partie(Client * envoyeur, char * buffer);
static int obtenir_challenge_par_joueur(Client joueur);
static void declarer_forfait_partie(Client * envoyeur, char* buffer);
static void gerer_fin_partie(int position_challenge, Client * gagnant, Client * perdant, int is_winner, char * fin_match);
static void afficher_liste_parties(Client * envoyeur);
static void observer_partie(Client * envoyeur, const char * buffer);
static int est_partie_observable(Client * envoyeur, int id_match);
static void arreter_observer_partie(Client * envoyeur);
static void envoyer_partie_aux_observeurs(int position_challenge, const char * affichage, int coup);

static int obtenir_challenge_par_joueur_deconnecte(Client joueur);
static void gerer_demande_ami(Client * envoyeur, Client * clients, int actual, const char * buffer);
static int est_ami(Client * envoyeur, Client * joueur_demande_ami);
static void retirer_ami(Client * envoyeur, Client * clients, int actual, const char * buffer);
static void accepter_requete_ami(Client * envoyeur, Client * clients, int actual, const char * buffer);
static void refuser_requete_ami(Client * envoyeur, Client * clients, int actual, const char * buffer);
static void afficher_liste_amis(Client * envoyeur);
static void afficher_liste_requete_ami(Client * envoyeur);

static void ecrire_bio(Client * envoyeur, char * buffer);
static void afficher_bio(Client * envoyeur, Client * clients, int actual, char * buffer);

static int passer_prive(Client * envoyeur);
static int passer_public(Client * envoyeur);

static void extraire_entre_deux_espaces(const char * chaine, char * resultat, size_t taille_resultat);
static void discuter_hors_partie(Client * envoyeur, char * buffer, Client * clients, int actual);
static void discuter_en_partie(Client * envoyeur, char * buffer);

static void se_deconnecter(Client clients[], int position_client, int * actual);

static void sauvegarder_fichier(Client * envoyeur, int position_challenge);
static void liste_historique(Client * envoyeur);
static void consulter_partie_sauvegardee(Client * envoyeur, char * buffer);

static void challenger_ia(Client * envoyeur, Client * IA);
static void gerer_partie_ia(Client * envoyeur, char * buffer);
static int obtenir_challenge_par_joueur_challenger(Client challenger);

#endif // SERVEUR_H
