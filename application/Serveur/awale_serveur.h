#ifndef AWALE_SERVEUR_H
#define AWALE_SERVEUR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LONGUEUR 6

void initialiser(int caseJoueur1[], int caseJoueur2[], int points[]);
void afficher_partie(int case_joueur1[], int case_joueur2[], int points[], char * nom_joueur1, char* nom_joueur2, char * output_1, char * output_2);
int coup_autorise(int caseJoueur1[], int caseJoueur2[], int * case_choisie, int joueur);
void jouer_tour(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie);
void coup_joueur1(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie);
void coup_joueur2(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie);
int partie_finie(int case_joueur1[], int case_joueur2[], int points[]);

#endif // AWALE_SERVEUR_H