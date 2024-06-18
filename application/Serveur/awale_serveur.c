#include "awale_serveur.h"

// Initialiser une partie d'awale
void initialiser(int case_joueur1[], int case_joueur2[], int points[])
{
    int i;

    for (i=0; i < 6;i++)
    {
        case_joueur1[i] = 4;
        case_joueur2[i] = 4;
    }

    points[0] = 0;
    points[1] = 0;
}

// Afficher la partie d'awale à l'écran
void afficher_partie(int case_joueur1[], int case_joueur2[], int points[], char * nom_joueur1, char* nom_joueur2, char * output_1, char * output_2)
{
    output_1[0] = '\0';
    output_2[0] = '\0';
    int compteur_pipe = 0;
    snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), "\n");
    snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), "\n");

    for (int i = LONGUEUR - 1; i >= 0; i--)
    {
        snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), "%d", case_joueur2[i]);
        snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), "%d", case_joueur1[i]);
        
        if (compteur_pipe < LONGUEUR - 1)
        {
            snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), " | ");
            snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), " | ");
        }

        compteur_pipe++;
    }

    snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), "  %s : %d points\n", nom_joueur2, points[1]);
    snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), "  %s : %d points\n", nom_joueur1, points[0]);
    compteur_pipe = 0;

    for (int i = 0; i < LONGUEUR; i++)
    {
        snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), "%d", case_joueur1[i]);
        snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), "%d", case_joueur2[i]);
        
        if (compteur_pipe < LONGUEUR - 1)
        {
            snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), " | ");
            snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), " | ");
        }

        compteur_pipe++;
    }

    snprintf(output_1 + strlen(output_1), 256 - strlen(output_1), "  %s : %d points\n\n", nom_joueur1, points[0]);
    snprintf(output_2 + strlen(output_2), 256 - strlen(output_2), "  %s : %d points\n\n", nom_joueur2, points[1]);
}

// Déterminer si le coup est autorisé. 
// Retourne : 
//  * 1 si le coup est autorisé
//  * 0 si la case choisie n'est pas entre 1 et 6
//  * -1 si la case choisie n'a aucune graine
//  * -2 si le joueur ne donne pas de graine alors qu'il a l'obligation de le faire.
int coup_autorise(int case_joueur1[], int case_joueur2[], int * case_choisie, int joueur)
{
    if ((*case_choisie) < LONGUEUR + 1 && (*case_choisie) > 0)
    {
        (*case_choisie) = (*case_choisie) - 1;

        if (joueur == 0)
        {
            // Vérifie si l'adversaire n'a plus de graine
            if (case_joueur2[0] + case_joueur2[1] + case_joueur2[2] + case_joueur2[3] + case_joueur2[4] + case_joueur2[5] == 0)
            {
                if((*case_choisie) + case_joueur1[(*case_choisie)-1] < 7)
                {
                    return -2;
                }
            }

            if (case_joueur1[(*case_choisie)] > 0)
            {
                return 1; 
            }  
            else
            {
                return -1; 
            }   
        }
        else 
        {
            if (case_joueur1[0] + case_joueur1[1] + case_joueur1[2] + case_joueur1[3] + case_joueur1[4] + case_joueur1[5] == 0)
            {
                if((*case_choisie) - 1 + case_joueur2[(*case_choisie)-1] < 6)
                {
                    return -2;
                }
            }

            if (case_joueur2[(*case_choisie)] > 0)
            {
                return 1; 
            }
            else
            {
                return -1; 
            }  
        }
    }

    return 0; 
}

// Gérer le tour d'une partie d'awale
void jouer_tour(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie)
{
    if(joueur == 0)
    {
        coup_joueur1(case_joueur1, case_joueur2, points, joueur, case_choisie);
    }
    else
    {
        coup_joueur2(case_joueur1, case_joueur2, points, joueur, case_choisie);
    }
}

// Jouer le coup du joueur 1
void coup_joueur1(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie)
{
	int case_index = case_choisie;
	int nb_graines = case_joueur1[case_choisie]; // Nombre de graines dans la case à jouer

	case_joueur1[case_choisie] = 0;
	
	//Egrenement
	while(nb_graines != 0)
	{
		case_index++;

		if (case_index == 12)
		{
			case_index = 0;
		} 

  		if (case_index == case_choisie) 
		{
			case_index++; 
		}

		if (case_index > 5)
		{
			case_joueur2[case_index % LONGUEUR]++;
		}
		else
		{
			case_joueur1[case_index]++;
		}

		nb_graines--;
	}

	// Capture
    while ((case_index > 5) && ((case_joueur2[case_index % LONGUEUR] == 2) || (case_joueur2[case_index % LONGUEUR] == 3)))
    {
        points[joueur] += case_joueur2[case_index % LONGUEUR];
        case_joueur2[case_index % LONGUEUR] = 0;
        case_index--;
    }
}

// Jouer le coup du joueur 2
void coup_joueur2(int case_joueur1[], int case_joueur2[], int points[], int joueur, int case_choisie)
{
	int case_index = case_choisie;
	int nb_graines = case_joueur2[case_choisie];

	case_joueur2[case_choisie]=0;
	
	//Egrenement
	while(nb_graines!=0)
	{
		case_index++;

		if (case_index==12)
		{
			case_index=0;
		}

		if (case_index == case_choisie)
		{ 
			case_index++;
		}

		if (case_index > 5)
		{
			case_joueur1[case_index % LONGUEUR]++;
		}
		else
		{
			case_joueur2[case_index]++;
		}

		nb_graines--;
	}
	
	// Capture
    while ((case_index > 5) && ((case_joueur1[case_index % LONGUEUR] == 2) || (case_joueur1[case_index % LONGUEUR] == 3)))
    {
        points[joueur] += case_joueur1[case_index % LONGUEUR];
        case_joueur1[case_index % LONGUEUR] = 0;
        case_index--;
    }
}

// Déterminer si la partie est finie.
// Retourne : 
//  * 0 si la partie n'est pas finie
//  * 1 si la partie est remportée par le joueur 1
//  * 2 si la partie est remportée par le joueur 2
//  * 3 si la partie se termine en match nul
int partie_finie(int case_joueur1[], int case_joueur2[], int points[])
{
    if((points[0] > 24) || (points[1] > 24))
	{
        points[0] += case_joueur1[0] + case_joueur1[1] + case_joueur1[2] + case_joueur1[3] + case_joueur1[4] + case_joueur1[5];
        points[1] += case_joueur2[0] + case_joueur2[1] + case_joueur2[2] + case_joueur2[3] + case_joueur2[4] + case_joueur2[5];

		if(points[0] > points[1])
        {
            return 1;
        }
        else if(points[0] < points[1])
        {
            return 2;
        }
        else
        {
            return 3;
        }
	}

    int i = 0;

    // Fin par indétermination pour joueur 1
	if (case_joueur1[0] + case_joueur1[1] + case_joueur1[2] + case_joueur1[3] + case_joueur1[4] + case_joueur1[5] == 0)
	{
		while ((i+case_joueur2[i] < 6) && (i < 6)) // Vérifie si on peut donner des graines 
        {
            i++;
        } 

		if (i==6)
		{
            points[0] += case_joueur1[0] + case_joueur1[1] + case_joueur1[2] + case_joueur1[3] + case_joueur1[4] + case_joueur1[5];
            points[1] += case_joueur2[0] + case_joueur2[1] + case_joueur2[2] + case_joueur2[3] + case_joueur2[4] + case_joueur2[5];

            if(points[0] > points[1])
            {
                return 1;
            }
            else if(points[0] < points[1])
            {
                return 2;
            }
            else
            {
                return 3;
            }
		}
	}

    i = 0;

    // Fin par indétermination pour joueur 2
	if (case_joueur2[0] + case_joueur2[1] + case_joueur2[2] + case_joueur2[3] + case_joueur2[4] + case_joueur2[5] == 0)
	{
		while ((i+case_joueur1[i] < 6) && (i < 6))  // Vérifie si on peut donner des graines
		{
			i++; 
		}

		if (i==6)
		{
			points[0] += case_joueur1[0] + case_joueur1[1] + case_joueur1[2] + case_joueur1[3] + case_joueur1[4] + case_joueur1[5];
            points[1] += case_joueur2[0] + case_joueur2[1] + case_joueur2[2] + case_joueur2[3] + case_joueur2[4] + case_joueur2[5];

            if(points[0] > points[1])
            {
                return 1;
            }
            else if(points[0] < points[1])
            {
                return 2;
            }
            else
            {
                return 3;
            }
		}
	}

    return 0;
}
