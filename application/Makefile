# Spécification du compilateur à utiliser
CC = gcc

# Drapeaux de compilation
# -lm : Inclus le support pour la bibliothèque mathématique
# -w : Désactive tous les avertissements (warning)
CFLAGS = -lm -w

# Règles pour les cibles
default: all

# Règle pour générer toutes les cibles
all: clean client_exe serveur_exe

# Règle pour générer l'exécutable client_exe à partir du fichier client.c dans le répertoire Client/
client_exe: Client/client.c 
	$(CC) -o $@ $^ $(CFLAGS)

# Règle pour générer l'exécutable serveur_exe à partir des fichiers awale_serveur.c et serveur.c dans le répertoire Serveur/
serveur_exe: Serveur/awale_serveur.c Serveur/serveur.c
	$(CC)  -o $@ $^ $(CFLAGS)

# Règle pour supprimer les exécutables générés
clean:
	rm -f client_exe serveur_exe *.txt

# Indique que 'all' et 'clean' ne sont pas des fichiers/dossiers
.PHONY: all clean