#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "iniparser.h"

#define BACKLOG 3
#define RECVBUFFER 1024
#define LENUSERID 20
#define LENPASSWORD 20
#define DEBUG 0 /* For debugging set to 1 */


void *TcpClient(void *);
void AuthMsg(int);

int sfd;
struct sockaddr_in uzak_adres;

int main(){

  pid_t pid;
  pid = getpid();
  printf("PID: %d\n", pid);

  int cfd, size_uzak, port;
  struct sockaddr_in adres;
  pthread_t thr;
  dictionary *dict;
  char rootDir[256];
  DIR *dizin;


  //ServerRoot Tanımlı değil ise oluşturulacak
  dict = iniparser_load("server.ini");
  strcpy(rootDir, iniparser_getstring(dict, "Server:ServerRoot", "NULL"));
  iniparser_freedict(dict);

  dizin = opendir(rootDir);
  if(!dizin){
    mkdir(rootDir, 0700);
  }
  //ServerRoot Tanımlı değil ise oluşturulacak



  //Socket Oluşturma
  if((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    perror("Error: Socket creation");
    exit(EXIT_FAILURE);
  }

  //Reading port from server.ini
  dict = iniparser_load("server.ini");
  port = iniparser_getint(dict, "Server:ListenPort",0);
  if(port == 0){
    perror("Error: Reading port from server.ini");
    exit(EXIT_FAILURE);
  }
  iniparser_freedict(dict);

  //Adres Tanımlama
  memset( &adres, '\0', sizeof(adres) );
  adres.sin_family = AF_INET;
  adres.sin_port = htons(port);
  adres.sin_addr.s_addr = INADDR_ANY;  // use my IP address
  memset(adres.sin_zero, '\0', sizeof adres.sin_zero);

  //Socketin tekrar kullanımına izin vermek
  int optval = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  //Bind
  if((bind(sfd, (struct sockaddr *)&adres, sizeof adres)) == -1){
    perror("Error: Bind");
    exit(EXIT_FAILURE);
  }

  //Listen
  if((listen(sfd, BACKLOG)) == -1){
    perror("Error: Listen");
    exit(EXIT_FAILURE);
  }

  memset( &uzak_adres, '\0', sizeof(uzak_adres) );
  size_uzak = sizeof uzak_adres;

  //Wait for connection an  }d assign to a thread
  while((cfd = accept(sfd, (struct sockaddr *)&uzak_adres, &size_uzak))){
    printf("Wait for connection\n");
    if(pthread_create(&thr, NULL, TcpClient, (void*) &cfd) < 0){
      perror("Error: Thread");
      exit(EXIT_FAILURE);
    }
    printf("\nConnection assigned\n");
  }
  //Wait for connection and assign to a thread

  if(cfd < 0){
    perror("Error: Accept failed");
    exit(EXIT_FAILURE);
  }

  close(sfd);
  return 0;
}

void *TcpClient(void *arg){

  int len, bytes_recv, bytes_sent, i, a, b, c, intNum;
  int cfd = *(int*)arg;
  char recvbuf[RECVBUFFER];
  char msg[RECVBUFFER], tempMsg[RECVBUFFER];
  char value[100] = {0};
  char okErr[100] = "+OK ";
  char command[4];
  dictionary *dict;
  char *Command, *param1, *param2, *num;
  char user[LENUSERID];
  char targetUser[LENUSERID];
  char password[LENPASSWORD];
  char temp[256], temp2[256];
  int pass_len = 0;
  int auth = 0;
  int uzunluk;
  char dir[256], gDir[256], filename[256];
  DIR *pDIR;
  struct dirent *pDirEnt;
  FILE *fp;
  char timestamp[20];
  char tmpuser[20];
  char size[20];
  int totalsize = 0;
  int sizetmp;
  int *msgArray;
  int msgList = 0; //List çağrılırsa 1'e set edilcek
  int msgListSize = 0;
  int msgIndex = 0;

  //Dizin için
  struct dirent **namelist;
  int n;

  printf("Server: got connection from %s\n", inet_ntoa(uzak_adres.sin_addr));

  dict = iniparser_load("server.ini");

  //Send ServerMsg from server.ini BEGIN
  strcpy(value, iniparser_getstring(dict, "Server:ServerMsg", "NULL"));
  if((strcmp(value, "NULL")) == 0){
    perror("Error: Reading ServerMsg from server.ini");
    exit(EXIT_FAILURE);
  }
  strcat(okErr, value);
  strcpy(value, okErr);
  strcat(value, " Ready.\n");
  len = strlen(value);
  bytes_sent = send(cfd, value, len, 0);
  //Send ServerMsg from server.ini END

  iniparser_freedict(dict);

  while(1){

    //Recv
    memset( &recvbuf, '\0', sizeof(recvbuf) );
    bytes_recv = recv(cfd, recvbuf, RECVBUFFER, 0);
    printf("Buffer:%s\n",recvbuf);


    if(bytes_recv <= 0){
      close(cfd);
      pthread_exit(NULL);
    }


//COMMANDS BEGIN
    memset( &value, '\0', sizeof(value) );
    memset( &Command, '\0', sizeof(Command) );
    memset( &param1, '\0', sizeof(param1) );
    memset( &param2, '\0', sizeof(param2) );
    memset( &command, '\0', sizeof(command) );
    memset( &temp, '\0', sizeof(temp) );
    memset( &tempMsg, '\0', sizeof(tempMsg) );
    memset( &dir, '\0', sizeof(dir) );
    memset( &pDIR, '\0', sizeof(pDIR) );
    memset( &pDirEnt, '\0', sizeof(pDirEnt) );
    strncat(command, recvbuf, 4);


    //USER Command BEGIN +OK
    if((strcmp(command,"USER")) == 0){

      Command = strtok(recvbuf, " ");
      param1 = strtok(NULL, " ");
      param2 = strtok(NULL, " ");

      if((param1 == NULL) || (param2 == NULL)){
        strcpy(value, "-ERR UserID and Password is required.\n");
        len = strlen(value);
        bytes_sent = send(cfd, value, len, 0);
      }
      else{

        for(a = 0; a <= strlen(param1); a++){
          user[a] = param1[a];
        }
        param2[strlen(param2)-1] = '\0';
        for(a = 0; a <= strlen(param2); a++){
          password[a] = param2[a];
        }

        dict = iniparser_load("server.ini");
        strcpy(temp, "users:");
        strcat(temp, param1);
        strcpy(value, iniparser_getstring(dict, temp, "NULL"));
        iniparser_freedict(dict);

        //UserID and Password Control BEGIN
        if((strcmp(value, "NULL")) == 0){
          strcpy(value, "-ERR Either UserID or Password is wrong.\n");
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);
        }
        else if((strcmp(value, param2)) == 0){
          strcpy(value, "+OK UserID and password okay go ahead.\n");
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);
          auth = 1;


          dict = iniparser_load("server.ini");
          strcpy(gDir, iniparser_getstring(dict, "Server:ServerRoot", "NULL"));
          iniparser_freedict(dict);
          sprintf(gDir,"%s/%s",gDir,user);

          DEBUG == 0 ? : printf("Dizin:%s\n",gDir);

          pDIR = opendir(gDir);
          if(!pDIR){
            //dizin yok ise
            //id=001
            mkdir(gDir, 0700);

            DEBUG == 0 ? :  printf("\nDizin yok, dizin oluşturuluyor.\n",gDir);
          }
          else{
              DEBUG == 0 ? : printf("\nDizin mevcut.\n",gDir);
          }

          //Dizin kontrolu eğer yoksa oluşturulcak
        }
        else{
          strcpy(value, "-ERR Either UserID or Password is wrong.\n");
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);
          DEBUG == 0 ? : printf("Value:%s\n",value);
        }
        //UserID and Password Control END
      }//else
    }
    //USER Command END


    //SEND Command BEGIN +OK
    else if((strcmp(command,"SEND")) == 0){
      if(auth == 0){
        AuthMsg(cfd);
        continue;
      }

      a = 0;
      while(recvbuf[a] != '\0'){
        tempMsg[a] = recvbuf[a];
        a++;
      }
      Command = strtok(recvbuf, " ");
      param1 = strtok(NULL, " ");

      //Target UserID kontrolu
      if(param1 == NULL){
        continue;
      }
      //Boş değilse

      //UserID girilip girilmediği
      if(strlen(param1) >= 1){
        param1[(strlen(param1))] = '\0';
      }
      else{
        continue;
      }

      //Integer Control
      a = 0;
      int set = 0;
      while(param1[a] != '\0'){
        if(param1[a] >= '0' && param1[a] <= '9'){
          set++;
        }
        a++;
      }
      //Target UserID kontrolu

      //Hedef UserID digit'mi kontrolu
      if(set == strlen(param1)){
        dict = iniparser_load("server.ini");
        sprintf(temp, "users:%s", param1);
        strcpy(value, iniparser_getstring(dict, temp, "NULL"));
        iniparser_freedict(dict);

        if((strcmp(value, "NULL")) == 0){
          sprintf(value, "-ERR message can not send to %s. User %s not available.\n",param1, param1);
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);
          printf("Sended:%s\n",value);
        }
        else{//Hedef USER var ise çalışaca aşağısı

          uzunluk= (6 + strlen(param1)); //Çift tırnağın başlangıç indexi
          //Satır sonu kontrolu sonuncu tırnağın tespiti. ve Satır başı tırnak kontrolu
          if((tempMsg[uzunluk] == '\"') && (tempMsg[(strlen(tempMsg)-2)] == '\"') && (tempMsg[(strlen(tempMsg)-1)] == '\n') && (tempMsg[strlen(tempMsg)] == '\0')){
            i = 0;
            while(tempMsg[uzunluk] != '\0'){
              msg[i] = tempMsg[(uzunluk)+1];
              uzunluk++;
              i++;
            }
            sizetmp = totalsize = 0;
            //Path hatalımı kontrolu
            dict = iniparser_load("server.ini");
            strcpy(dir, iniparser_getstring(dict, "Server:ServerRoot", "NULL"));
            iniparser_freedict(dict);
            if((strcmp(dir, "NULL")) == 0){
              sprintf(value, "-ERR message can not send to %s. User %s not available.\n",param1, param1);
              len = strlen(value);
              bytes_sent = send(cfd, value, len, 0);
            }
            else{
              sprintf(dir, "%s/%s",dir,param1);//param1 is targetUserID

              pDIR = opendir(dir);
              memset( &temp, '\0', sizeof(temp) );
              sprintf(temp, "%d_%s_%d.msg",time(NULL),user,(strlen(msg)-2));
              if(!pDIR){
                //dizin yok ise
                //id=001
                mkdir(dir, 0700);
              }
              sprintf(dir,"%s/%s",dir,temp);
              //id=002

              msg[(strlen(msg)-2)] = '\0';
              fp = fopen(dir, "w");
              fputs(msg, fp);
              fclose(fp);

              sprintf(value, "+OK message sended to %s.\n",param1);
              len = strlen(value);
              bytes_sent = send(cfd, value, len, 0);
            }
          }
          else{
            strcpy(value,"-ERR TEXT must be between double-quote.\n");
            len = strlen(value);
            bytes_sent = send(cfd, value, len, 0);
          }
          //Satır sonu kontrolu sonuncu tırnağın tespiti. ve Satır başı tırnak kontrolu
        }
      }
      else{
        continue;
      }
      //Hedef UserID digit'mi kontrolu
    }
    //SEND Command END


    //LIST Command BEGIN *OK
    else if((strcmp(command,"LIST")) == 0){
      if(auth == 0){
        AuthMsg(cfd);
        continue;
      }

      msgList = 1; //List komutu çalıştırıldı.
      msgIndex = 0; //Index 0 lanacakk

      Command = strtok(recvbuf, " ");
      param1 = strtok(NULL, " ");
      printf("Param1:%s\n",param1);

      if(param1 == NULL){

        dict = iniparser_load("server.ini");
        strcpy(dir, iniparser_getstring(dict, "Server:ServerRoot", "NULL"));
        iniparser_freedict(dict);
        sprintf(dir,"%s/%s/",dir,user);
        a = 0;
        n = scandir(dir, &namelist, 0, alphasort);
        totalsize = 0;
        if (n < 0)
            perror("Error(1): scandir\n");
        else {
            for (i = 0; i < n; i++) {
              if(i > 1){
                sscanf(namelist[i]->d_name,"%[^_]_%[^_]_%d[^.]",timestamp, tmpuser, &sizetmp);
                totalsize += sizetmp;
                free(namelist[i]);
                a++;
              }
            }
        }
        free(namelist);

        //Aşağıda a yerine n ile yani n 2'den büyük ise dosya olduğunu söyleyebiliriz.


        //Mailbox'ta mesaj olup olmadığı
        if(a == 0){
          sprintf(value, "-ERR There are no message in your mailbox.\n");
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);

          if(DEBUG == 1){
            printf("Value:%s\n",value);
          }
        }
        else{//Mesaj varsa aşağısı çalışcak

          memset( &value, '\0', sizeof(value) );
          memset( &pDIR, '\0', sizeof(pDIR) );
          memset( &pDirEnt, '\0', sizeof(pDirEnt) );

          sprintf(value, "+OK %d messages (%d Octets)\n", a, totalsize);

          //For del command
          msgListSize = a;
          memset(&msgArray, '\0', sizeof(msgArray));
          msgArray = (int *) malloc(sizeof(int) * msgListSize);


          a = 1;
          n = scandir(dir, &namelist, 0, alphasort);
          if (n < 0)
              perror("Error(2): scandir\n");
          else {
              for (i = 0; i < n; i++) {
                if(i > 1){
                  sscanf(namelist[i]->d_name,"%[^_]_%[^_]_%[^.]",timestamp, tmpuser, size);
                  sprintf(value, "%s%d %s %s %s\n",value, a, tmpuser, timestamp, size);
                  free(namelist[i]);
                  a++;
                }
              }
          }
          free(namelist);

          if(DEBUG == 1){
            printf("Value:%s\n",value);
          }


          sprintf(value,"%s.\n",value);
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);

          if(DEBUG == 1){
            printf("Value:%s\n",value);
          }

        }//Mailbox mesaj var yok
      }
      else //LIST 2 gibi durumlarda çalışacak
      {

        b = 0;
        sscanf(param1, "%d%s", &b, temp);

        if((strcmp(temp, "")) == 0){
          dict = iniparser_load("server.ini");
          strcpy(dir, iniparser_getstring(dict, "Server:ServerRoot", "NULL"));
          iniparser_freedict(dict);
          sprintf(dir,"%s/%s/",dir,user);

          i = c = a = 0;
          n = scandir(dir, &namelist, 0, alphasort);
          if (n < 0)
              perror("Error(2): scandir\n");
          else {
              for (i = 0; i < n; i++) {
                if(i > 1){
                  a++;
                  if (a == b){//b is option which user selected
                    sscanf(namelist[i]->d_name,"%[^_]_%[^_]_%[^.]",timestamp, tmpuser, size);
                    sprintf(value, "+OK %d %s %s %s\n.\n", a, tmpuser, timestamp, size);
                    free(namelist[i]);

                    len = strlen(value);
                    bytes_sent = send(cfd, value, len, 0);
                    c = 1;

                    if(DEBUG == 1){
                      printf("Value:%s\n",value);
                    }

                  }
                }//if
              }//for
          }//else
          free(namelist);


          if(c != 1){
            //Err mesaj yok
            sprintf(value, "-ERR no such message, only %d messages in maildrop.\n", a);
            len = strlen(value);
            bytes_sent = send(cfd, value, len, 0);

            if(DEBUG == 1){
              printf("Value:%s\n",value);
            }
          }

        }//parametre olarak integer değer varsa çalışır yoksa aşağısı
        else{
          continue;
        }

      }//LIST 2 gibi durumlarda çalışacak
    }
    //LIST Command END


    //RET Command BEGIN +OK
    //Buraya sadece "RET" şeklinde kontrol de eklencek
    else if((strcmp(command,"RET ")) == 0){
      if(auth == 0){
        AuthMsg(cfd);
        continue;
      }

      b = 0;
      sscanf(recvbuf, "%s %d%s", temp2, &b, temp);
      //buraya daha detaylı kontrol eklenebilir.

      if((strcmp(temp, "")) == 0){
        DEBUG == 0 ? : printf("Dir:%s\n",gDir);

        pDIR = opendir(gDir);
        pDirEnt = readdir( pDIR );

        i = c = a = 0;
        n = scandir(gDir, &namelist, 0, alphasort);
        if (n < 0)
            perror("Error(3): scandir\n");
        else {
            for (i = 0; i < n; i++) {
              if(i > 1){
                a++;
                if (a == b){//b is option which user selected
                  sscanf(namelist[i]->d_name,"%[^_]_%[^_]_%[^.]",timestamp, tmpuser, size);
                  sprintf(value, "+OK %s %s %s Octets\n", tmpuser, timestamp, size);
                  free(namelist[i]);

                  char ch;
                  fp = NULL;
                  sprintf(filename, "%s/%s", gDir, namelist[i]->d_name);
                  fp = fopen(filename, "r");
                  if(fp == NULL){
                    perror("Error while reading the message\n");
                  }
                  else{
                    while((ch = fgetc(fp)) != EOF){
                      sprintf(value,"%s%c",value,ch);
                    }
                  }
                  sprintf(value,"%s\n.\n",value);

                  len = strlen(value);
                  bytes_sent = send(cfd, value, len, 0);
                  DEBUG == 0 ? :printf("%s%s\n", "Value:",value);
                  c = 1;
                }//if
              }//if
            }//for
        }//else
        free(namelist);
        fclose(fp);


        if(c != 1){
          //Err mesaj yok
          sprintf(value, "-ERR no such message available.\n");
          len = strlen(value);
          bytes_sent = send(cfd, value, len, 0);
        }

      }
      else{
        continue;
      }
    }
    //RET Command END


    //DEL Command BEGIN
    //Buraya sadece "DEL" şeklinde kontrolde eklencek
    else if((strcmp(command,"DEL ")) == 0){
      if(auth == 0){
        AuthMsg(cfd);
        continue;
      }

      //Eğer list comutu çalıştırıldıysa çalışsın.
      if(msgList == 0){
        strcpy(value, "-ERR First send LIST command.\n");
        len = strlen(value);
        bytes_sent = send(cfd, value, len, 0);
        continue;
      }
      //msgList 1 ise listteleme için dizini tekrar tarama


      DEBUG == 0 ? : printf("Dir:%s\n",gDir);

      memset( &filename, '\0', sizeof(filename) );
      memset( &temp, '\0', sizeof(temp) );
      b = 0;
      sscanf(recvbuf, "%s %d%s", temp2, &b, temp);
      //buraya daha detaylı kontrol eklenebilir.
      //tempdeki her karakter integer mı kontrolu ve sonra

      //Girilen değer kontrolu konulacak

      if((strcmp(temp, "")) == 0){
        i = c = a = 0;
        n = scandir(gDir, &namelist, 0, alphasort);

        int t = 0;

        if (n < 0)
            perror("Error(4): scandir\n");
        else {
            for (i = 0; i < n; i++) {
              if(i > 1){
                a++;

                if (a == b){//b is option which user selected

                  //Mesajın daha önceden silinip silinmediği
                  int set = 0;
                  int dongu;
                  for(dongu = 0; dongu < msgListSize; dongu++){
                    if(b != msgArray[dongu]){
                      set = 1;

                    }else{
                      set = 0;
                      break;
                    }
                  }

                  //mesaj ilk kez siliniyorsa
                  if(set == 1){

                    sscanf(namelist[i]->d_name,"%s",filename);
                    free(namelist[i]);
                    sprintf(temp, "%s/%s",gDir, filename);

                    DEBUG == 0 ? :  printf("Silinecek dosya:%s\n",temp);

                    if(remove(temp)){
                      DEBUG == 0 ? : perror("Dosya silinmedi");
                      sprintf(value, "-ERR message %d not deleted.\n", b);
                    }
                    else{
                      DEBUG == 0 ? : perror("Dosya silindi");
                      sprintf(value, "+OK message %d deleted.\n", b);
                      //already Deleted için aşağısı
                      msgArray[msgIndex] = b;
                      msgIndex++;
                    }

                  }
                  else{
                    sprintf(value, "-ERR message %d already deleted.\n", b);
                  }
                  c = 1;
                }
              }
            }
        }//else
        len = strlen(value);
        bytes_sent = send(cfd, value, len, 0);
      }
      else{
        continue;
      }

      if(c != 1){
        //Err mesaj yok
        sprintf(value, "-ERR no such message available.\n");
        len = strlen(value);
        bytes_sent = send(cfd, value, len, 0);
      }
    }
    //DEL Command END


    //QUIT Command BEGIN +OK
    else if((strcmp(command,"QUIT")) == 0){
      sprintf(value, "+OK Good Bye %s.\n",user);
      len = strlen(value);
      bytes_sent = send(cfd, value, len, 0);

      if(DEBUG == 1){
        printf("Value:%s\n",value);
      }

      close(cfd);
      pthread_exit(NULL);
    }
    //QUIT Command END

    if(DEBUG == 1){
      printf("UserID:%s\n",user);
    }

//COMMANDS END
  }//While END
}//TcpClient END


void AuthMsg(int fd){
  char value[50] = "-ERR Authentication Required.\n";
  send(fd, value, strlen(value), 0);
}
