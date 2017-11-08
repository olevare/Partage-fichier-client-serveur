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

int main (int argc, char** argv)
{
  //création du buffer
  int brclient = socket(AF_INET, SOCK_STREAM, 0);
  if(brclient == -1)
  {
    perror("socket() : error\n");
    return EXIT_FAILURE;
  }
  else
    printf("Client : La socket est ouverte en mode TCP.\n");
  
  
  //création et configuration de la socket du client
  struct  sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = inet_addr("127.0.01"); // ou INADDR_ANY; ou htonl(INADDR_ANY);
  client.sin_port = htons((short)0);
  
  
  //le bind = liaison entre le buffer et le socket
  int res = bind(brclient,(struct sockaddr*)&client, sizeof(client));
  if(res < 0)
  {
    perror("bind() : error\n");
    close(brclient);
    return EXIT_FAILURE;
  }
  
  printf("Client : le bind a été effectuer.\n");
  
  
  //création et configuration de la socket du serveur
  struct sockaddr_in Server;
  Server.sin_family = AF_INET;
  Server.sin_addr.s_addr = inet_addr("127.0.01"); // ou INADDR_ANY; ou htonl(INADDR_ANY);
  Server.sin_port = htons(31469);
  
  printf("Client : L'initialisation de la socket distante est terminé.\n");
  
  
  //demande de connection au serveur
  if(connect(brclient, (struct sockaddr*) &Server, sizeof(Server)) == -1)
  {
    perror("connect() : error\n");
    close(brclient);
    return EXIT_FAILURE;
  }
  else
    printf("Client : Connexion établit avec %s sur le port %d.\n",inet_ntoa(Server.sin_addr), htons(Server.sin_port));
  
  
  //initialisation tampon envoie en reception
  char message[1024] = {0};
  char buffer[1024] = {0};
  off_t size; //taille du fichier demande
  struct stat status; //données du fichier
  int resp=0; //nombre d'octets reçut lors du recv()
  int rcv=0; //nombre d'octets total reçut
  int i = 0;
  char c;

  //explication
  printf("Client : Bonjour.\n");
  printf("Client : Vous pouvez importer un fichier, pour cela taper import.\n");
  printf("Client : Pour changer de mode taper quitter.\n");
  printf("Client : Pour quitter le programme taper exit (quitter le mode d'abord).\n");
  

  //boucle principale de réception et d'envoie
  while(1)
  {
    printf("Client : import ou exit ??\n");
    fgets(message, sizeof(message), stdin);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                            LE CLIENT TELECHARGE UN FICHER
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(strncmp(message, "import", 6) == 0)
    {
      //envoie du message import au serveur
      if(send(brclient, message, sizeof(message), 0) == -1)
      {
        perror("Client : erreur du send");
        close(brclient);
        return EXIT_FAILURE;
      }
      printf("Client : Envois du message %s\n", message);

      //variable pour un fichier
      FILE* fd;

      //recois la taille du fichier
      if(recv(brclient, &size, sizeof(off_t), 0)<=0)
      {
        perror("Client : erreur reception taille fichier.\n");
        close(brclient);
        return EXIT_FAILURE;
      }

      //cree le nouveau fichier
      if((fd = fopen("liste.txt", "w+")) == NULL)
      {
        perror("Client : erreur création nouveau fichier.\n");
        close(brclient);
        return EXIT_FAILURE;
      }

      do 
      {
        if( (resp = recv(brclient, buffer, sizeof(buffer), 0)) <= 0)
        {
          perror("Client : erreur reception fichier.\n");
          close(brclient);
          return EXIT_FAILURE;
        }
        fwrite(buffer, 1, resp, fd);
        rcv += resp;//on additionne les octets juste recut et totaux deja recut 
      }while(rcv < size);//tant que taille recu n'est pas taille du fichier


      //on remet la variable à 0
      rcv = 0;

      //on se remet au début du fichier
      fseek(fd, 0, SEEK_SET);

      printf("Client : voici les fichier disponible :\n");

      //affiche tous les fichier disponible sur le serveur
      while(fgets(message, 1024, fd) != NULL)
        printf("%s", message);

      //on ferme le fichier
      fclose(fd);

      printf("Client : que voulez-vous télécharger ?\n");

      //envoie du nom du fichier a télécharger
      fgets(message, sizeof(message), stdin);

      //si on envoie quitter
      if(strncmp(message, "quitter", 7) == 0)
      {
        //envoie quitter au serveur
        if(send(brclient, message, sizeof(message), 0) == -1)
        {
          perror("Client : erreur du send");
          close(brclient);
          return EXIT_FAILURE;
        }
      }
      else
      {
        //envoie le nom du fichier a telecharger
        if(send(brclient, message, sizeof(message), 0) == -1)
        {
          perror("Client : erreur du send");
          close(brclient);
          return EXIT_FAILURE;
        }

        //recois la taille du fichier
        if(recv(brclient, &size, sizeof(off_t), 0)<=0)
        {
          perror("Client : erreur reception taille fichier.\n");
          close(brclient);
          return EXIT_FAILURE;
        }

        //cree le nouveau fichier
        if((fd = fopen(strtok(message, "\n"), "w+")) == NULL)
        {
          perror("Client : erreur création nouveau fichier.\n");
          close(brclient);
          return EXIT_FAILURE;
        }

        do {
            if( (resp = recv(brclient, buffer, sizeof(buffer), 0)) <= 0)
            {
              perror("Client : erreur reception fichier.\n");
              close(brclient);
              return EXIT_FAILURE;
            }
            fwrite(buffer, 1, resp, fd);
            rcv += resp;//on additionne les octets juste recut et totaux deja recut 
        }while(rcv < size);//tant que taille recu n'est pas taille du fichier


        //on remet la variable à 0
        rcv = 0;

        //on ferme le fichier recu
        fclose(fd);
        printf("Client : fichier télécharger !!\n");
      }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                            LE CLIENT VEUT PARTIR
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if(strncmp(message, "exit", 4) == 0)
    {
      //envoie exit au serveur
      if(send(brclient, message, sizeof(message), 0) == -1)
      {
        perror("Client : erreur du send");
        close(brclient);
        return EXIT_FAILURE;
      }
      close(brclient);
      return EXIT_FAILURE;
    }

    else
      printf("Client : Je n'ai pas compris votre demande désoler !!\n");
  }
  close(brclient);
  return EXIT_FAILURE;
}
