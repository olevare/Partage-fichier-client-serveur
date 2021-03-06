#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> // Pour inet_ntoa
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>

int nbTelechargement = 0;
pthread_mutex_t verrou =  PTHREAD_MUTEX_INITIALIZER;

void* TraitementCompteur(void)
{
  char buf[1024] = {0};
  while(1)
  {
    scanf("%s", &buf);
    if(strncmp(buf, "compteur", 8) == 0)
      printf("Il y a eu %d telechargement (import et export) ;)\n", nbTelechargement);
    else
      printf("Je n'ai pas compris votre demande désoler\n");
  }
}


int main (int argc, char** argv)
{
  //création du buffer
  int brserveur = socket(AF_INET, SOCK_STREAM, 0);
  if (brserveur == -1)
  {
    perror("socket() : error\n");
    return EXIT_FAILURE;
  }
  
  
  //création et configuration de la socket
  struct  sockaddr_in serveur;
  serveur.sin_family = AF_INET;
  serveur.sin_addr.s_addr = INADDR_ANY;
  serveur.sin_port = htons((short)31469);
  
  
  //le bind = liaison entre le buffer et le socket
  int res = bind(brserveur,(struct sockaddr*)&serveur, sizeof(serveur));
  if(res == -1)
  {
    perror("bind() : error\n");
    close(brserveur);
    return EXIT_FAILURE;
  }
  
  printf("Serveur : le bind a été effectuer.\n");
  
  
  //structure du client
  struct sockaddr_in client; 
  socklen_t lgclient = sizeof(client);

  //crée le thread qui attend la commande compteur
  pthread_t compteur;
  pthread_create(&compteur, 0, &TraitementCompteur, NULL); //creation du thread
  pthread_detach(compteur);//détache le thread se qui permet de pas utiliser pthread_join

  
  listen(brserveur, 5); //file d'attente

  // accept la connexion
  int acclient = accept(brserveur, (struct sockaddr *)&client, &lgclient);
  if(acclient == -1)
  {
    perror("accept() : error\n");
    close(brserveur);
    return EXIT_FAILURE;
  }

  //initialisation tampon envoie en reception
  char message[1024] = {0};
  char buffer[1024] = {0};
  off_t size; //taille du fichier demande
  struct stat status; //données du fichier
  int resp=0; //nombre d'octets reçut lors du recv()
  int rcv=0; //nombre d'octets total reçut
  int i = 0;
  char c;
  
  //boucle principale
  while(1)
  {
    // Réception d'un message
    printf("Serveur : Attente réception du message.\n");
    if(recv(acclient, buffer, sizeof(buffer), 0) == -1 )
    {
      perror("Serveur : erreur du recv.\n");
      close(acclient);
      return EXIT_FAILURE;
    }
    else
      printf("Serveur : Réception du message %s", buffer);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                            LE CLIENT TELECHARGE UN FICHER
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //si reception import
    if(strncmp(buffer, "import", 6) == 0)
    {
      //ouverture du dossier courant
      DIR *rep = NULL;
      struct dirent *fichierLu = NULL;
      rep = opendir(".");

      //variable pour un fichier
      FILE* fd;

      //ouverture du fichier
      if((fd = fopen("liste.txt", "w+")) == NULL)
      {
        perror("Serveur : erreur de l'ouverture du fichier.\n");
        close(acclient);
        return EXIT_FAILURE;
      }

      //liste les fichier disponible
      while ((fichierLu = readdir(rep)) != NULL)
      {
        if(strcmp(fichierLu->d_name, ".") && strcmp(fichierLu->d_name, "..") && strcmp(fichierLu->d_name,"serveur") &&
            strcmp(fichierLu->d_name,"serveur.c"))
        {
          fprintf(fd, "%s\n", fichierLu->d_name);
        }
      }

      //fermeture du dossier courant
      if (closedir(rep) == -1)
      {
        perror("Serveur : erreur de la fermeture du dossier courant.\n");
        close(acclient);
        return EXIT_FAILURE;
      }

      //on se remet au début du fichier
      fseek(fd, 0, SEEK_SET);

      //recupère la taille du fichier
      lstat("liste.txt", &status);
      size = status.st_size; //off_t est un long pour la taille de fichier en gros

      //envoie de la taiile du fichier
      if(send(acclient, &size, sizeof(off_t), 0) == -1)
      {
        perror("Serveur : erreur envoie taille fichier.\n");
        close(acclient);
        return EXIT_FAILURE;
      }

      //envoie le contenue du fichier
      while((i = fread(message, 1, 1024, fd))>0)//tant que l'on a pas envoyer tout le fichier
      {
        if(send(acclient, message, sizeof(message), 0) < 0)
        {
          perror("Serveur : erreur envoie du fichier.\n");
          close(acclient);
          return EXIT_FAILURE;
        }
        memset(message, 0, 1024);     
      }

      //on remet la variable à 0
      i = 0;

      //on ferme le fichier
      fclose(fd);

      //on recois la demande du client
      if(recv(acclient, buffer, sizeof(buffer), 0) == -1 )
      {
        perror("Serveur : erreur du recv.\n");
        close(acclient);
        return EXIT_FAILURE;
      }
      else
      {
        //si il veut quitter
        if(strncmp(buffer, "quitter", 7) == 0)
          break;

        else
        {
          //ouvre le fichier que l'on va envoyer
          if((fd = fopen(strtok(buffer, "\n"), "r")) == NULL)
          {
            perror("Serveur : erreur ouverture fichier.\n");
            close(acclient);
            return EXIT_FAILURE;
          }

          //recupère la taille du fichier
          lstat(buffer, &status);
          size = status.st_size; //off_t est un long pour la taille de fichier en gros

          //envoie de la taiile du fichier
          if(send(acclient, &size, sizeof(off_t), 0) == -1)
          {
            perror("Serveur : erreur envoie taille fichier.\n");
            close(acclient);
            return EXIT_FAILURE;
          }

          //envoie le contenue du fichier
          while((i = fread(message, 1, 1024, fd))>0)//tant que l'on a pas envoyer tout le fichier
          {
            if(send(acclient, message, sizeof(message), 0) < 0)
            {
              perror("Serveur : erreur envoie du fichier.\n");
              close(acclient);
              return EXIT_FAILURE;
            }
            memset(message, 0, 1024);     
          }

          //on remet la variable à 0
          i = 0;
          pthread_mutex_lock(&verrou);
          nbTelechargement ++;
          pthread_mutex_unlock(&verrou);
        }
      }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                            LE CLIENT VEUT PARTIR
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //si reception exit
    if(strncmp(buffer, "exit", 4) == 0)
    {
      break;
    }
  }
  close(brserveur);
  return EXIT_SUCCESS;
}
