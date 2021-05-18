#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include "iniparser.h"

#define DEBUG 0

/*

Write Mesage OK
QUIT OK
RET Message OK
DEL OK
ChangeCfg OK

*/

int ReadDelete();
void WriteMsg();
int ChangeCfg(int, int);
void Quit();

int sfd;
struct sockaddr_in adres; //Hedef adres için
struct hostent *he;
char recvbuf[1024], msg[100], errOk[10], recvtmp[1024];
char targetServer[20], userID[20], password[30];
int targetPort;

int main(){

  char *menu = "\
Please choose your option:\n\
1) Read/Delete Messages\n\
2) Write Message to User\n\
3) Change config parameters\n\
4) Quit\n\
Option-> ";

  dictionary *dict;
  int option;
  char temp[100];
  int connControl = 1;

  dict = iniparser_load("client.ini");
  strcpy(targetServer, iniparser_getstring(dict, "client:TargetServer", "NULL"));
  targetPort = iniparser_getint(dict, "client:TargetPort", 0);
  strcpy(userID, iniparser_getstring(dict, "client:UserID", "NULL"));
  strcpy(password, iniparser_getstring(dict, "client:Passwd", "NULL"));
  iniparser_freedict(dict);

  //.ini file control
  if(((strcmp(targetServer, "NULL")) == 0) || (targetPort == 0) || ((strcmp(userID, "NULL")) == 0) || ((strcmp(password, "NULL")) == 0)){
    perror("Error reading .ini file\n");
    exit(EXIT_FAILURE);
  }

  memset( &he, '\0', sizeof(he) );
  he = gethostbyname(targetServer);
  if(he == (struct hostent *) 0){
    perror("Error: gethostbyname");
  }

  memset( &adres, '\0', sizeof(adres) );
  memset(adres.sin_zero, '\0', sizeof adres.sin_zero);
  adres.sin_family = AF_INET;
  adres.sin_port = htons(targetPort);
  adres.sin_addr.s_addr = *((unsigned long *)he->h_addr);

  //Socket Oluşturma
  if((sfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    perror("Error: Socket creation");
    exit(EXIT_FAILURE);
  }

  if((connect(sfd, (struct sockaddr *)&adres, sizeof adres)) == -1){
    perror("Error(1): Connect");
    connControl = 0;
  }

  while(connControl == 0){
    printf("%s\n","While connecting to server!");
    connControl = ChangeCfg(sfd, connControl);
  }


  //Server Ready mesajı
  memset( &recvbuf, '\0', sizeof(recvbuf) );
  recv(sfd, recvbuf, 1024, 0);
  memset( &errOk, '\0', sizeof(errOk) );
  sscanf(recvbuf, "%s", errOk);
  if(DEBUG == 1){
    printf("\nRECV:%s",recvbuf);
    printf("\nMesaj türü:%s",errOk);
  }

  if((strcmp(errOk, "-ERR")) == 0){
      perror("\nError(3): Server has a problem.");
  }
  else{//+OK döndüğü durumda
    memset( &msg, '\0', sizeof(msg));
    sprintf(msg, "USER %s %s\n", userID, password);
    send(sfd, msg, strlen(msg), 0);
    memset( &recvbuf, '\0', sizeof(recvbuf) );
    recv(sfd, recvbuf, 1024, 0);
    memset( &errOk, '\0', sizeof(errOk) );
    sscanf(recvbuf, "%s", errOk);
    if(DEBUG == 1){
      printf("\nmsg:%s",msg);
      printf("\nRECV:%s",recvbuf);
      printf("\nMesaj türü:%s",errOk);
    }
    if((strcmp(errOk, "-ERR")) == 0){
      connControl = 0;
      while(connControl == 0){
        perror("\nError: Authentication.");
        connControl = ChangeCfg(sfd, 1);
      }
    }

    system("clear");
    int dongu = 1;
    while(dongu){
      printf("\n%s %s,\n","Hello",userID);
      printf("%s",menu);
      scanf("%d",&option);
      if((option > 4) || (option < 1)){
        system("clear");
        printf("\n%s\n","[-] Please, enter between 1 and 4");
      }
      else{
        switch (option) {
          case 1: ReadDelete(); break;
          case 2: WriteMsg(); break;
          case 3: ChangeCfg(sfd,1); break;
          case 4:
            dongu = 0;
            Quit();
          break;
        }//switch
      }//else
    }//while
    printf("\nYour option:%d\n",option);

  }//return OK else
  return 0;
}



int ReadDelete(){

  char *ReadDelMenu = "Your Messages listed below:\n";
  char option[10];
  char line[100];
  char user[30];
  int timestamp;
  time_t time;
  struct tm ts;
  char timebuf[20];
  int bytes, i, a, c, d, newline;
  printf("%s",ReadDelMenu);

  //Mesajları oku
  memset( &line, '\0', sizeof(line) );
  memset( &msg, '\0', sizeof(msg));
  strcpy(msg,"LIST"); //copy LIST to msg variable
  send(sfd, msg, strlen(msg), 0);
  memset( &recvbuf, '\0', sizeof(recvbuf) );
  recv(sfd, recvbuf, 1024, 0);

  DEBUG == 0 ? : printf("RecvBuffer:%s\n",recvbuf);

  //Hata mesajı olup olmadığını oku
  memset( &errOk, '\0', sizeof(errOk) );
  sscanf(recvbuf, "%s", errOk); //ilk string okunuyor

  if((strcmp(errOk, "-ERR")) == 0){
    printf("[-] You do not have any messages.\n");
    //Return döndürülcek
  }
  else{//+OK döndüğü durumda

    //Aslında böyle satır sayma falan gereksiz olmuş
    //dönen değerdeki +OK'den sonra kaç satır olduğunu alsak daha mantıklı
    newline = i = 0;
    //Count Newline characters
    while(recvbuf[i] != '\0'){
      if(recvbuf[i] == '\n'){
          newline++;
      }
      i++;
    }
    //Count Newline characters

    //Aşağıda aslında

    i = a = c = 0;
    //printf("Newline:%d\n",newline);
    while(recvbuf[i] != '\0'){
      if(recvbuf[i] == '\n'){
          c++;
          if(c != 1 && c != newline){
            sscanf(line, "%d %s %d %d", &d, user, &timestamp, &bytes);

            if(DEBUG == 1){
              printf("%s\n",line);
              printf("%d\n",d);
              printf("%s\n",user);
              printf("%d\n",timestamp);
              printf("%d\n",bytes);
            }

            memset( &timebuf, '\0', sizeof(timebuf) );
            time = timestamp;
            ts = *localtime(&time);
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", &ts);
            printf("%d) From: %s, Date: %s, %d Bytes\n", d, user, timebuf, bytes);
            memset( &line, '\0', sizeof(line) );

            //Burda sondakini birdaha okuyor muhtemelen server sonuna . eklerken oluşan değer yüzünden
          }
          a = 0;
      }
      else{
        line[a] = recvbuf[i];
        a++;
      }
      i++;
    }//While

    char opt[10], karakter;
    int no, set = 0;
    char sub[9];
    memset(&sub, '\0', sizeof(sub));
    printf("Type R<id> for Reading, D<id> for Deleting -> ");
    scanf("%s",opt);
    sscanf(opt, "%*c%s", sub);
    i = 0;
    while(sub[i] != '\0'){
      if(sub[i] >= '0' && sub[i] <= '9'){
        set = 1;
      }
      else{
        set = 0;
        break;
      }
      i++;
    }
    no = 0;
    if(set == 1){
      sscanf(opt, "%*c%d", &no);
      if(opt[0] == 'R'){
        memset( &msg, '\0', sizeof(msg));
        sprintf(msg, "RET %d", no);
        send(sfd, msg, strlen(msg), 0);

        memset( &recvbuf, '\0', sizeof(recvbuf) );
        recv(sfd, recvbuf, 1024, 0);

        char msgBuf[1024];
        memset( &msgBuf, '\0', sizeof(msgBuf) );
        newline = 0;
        i = 0;
        while(recvbuf[i] != '\n'){
          i++;
        }
        newline = i;
        i = 0;
        //while(recvbuf[(newline+1)] != '.' && recvbuf[(newline+2)] != '\n' && recvbuf[(newline+3)] != '\0'){
        while(recvbuf[(newline)] != '.' && recvbuf[(newline+1)] != '\n' && recvbuf[(newline+2)] != '\0'){
          newline++;
          msgBuf[i] = recvbuf[(newline)];
          i++;
        }
        printf("Your Message: \"%s\"\n",msgBuf);

      }
      else if(opt[0] == 'D'){

        char yn[5];
        printf("Do you want to delete message <%d>?\n",no);
        scanf("%s",yn);

        if((strcmp(yn, "y")) == 0 || (strcmp(yn, "Y")) == 0){

          memset( &msg, '\0', sizeof(msg));
          sprintf(msg, "DEL %d", no);
          send(sfd, msg, strlen(msg), 0);

          memset( &recvbuf, '\0', sizeof(recvbuf) );
          recv(sfd, recvbuf, 1024, 0);

        }
        else if((strcmp(yn, "n")) == 0 || (strcmp(yn, "N")) == 0){
            //ana menüye dönülcek
            return 0;
        }

      }//D seçildiyse


    }//if(set == 1)
    else{
      printf("%s\n", "Please enter a message number!");
    }

  }//else +OK döndüğü durumda
}


void WriteMsg(){

  char targetUserID[20];
  char opt[5];
  char line[256];
  int set, i, size;
  char c;

  memset( &msg, '\0', sizeof(msg));
  memset( &line, '\0', sizeof(line));

  printf("Give target UserID-> ");
  scanf("%s", &targetUserID);
  DEBUG == 0 ? : printf("Target UserID:%s\n",targetUserID);
  printf("Please type text-> \n");
  read(1, line, 256);
  DEBUG == 0 ? : printf("Line:%s\n",line);
  size = set = i = 0;
  DEBUG == 0 ? : printf("UserID:%s\n",targetUserID);

  printf("Would you like to Send message to %s?(Y/N):\n",targetUserID);
  scanf("%s", &opt); //burada &opt olabilir
  DEBUG == 0 ? : printf("Option:%s\n",opt);

  line[(strlen(line)-1)] = '\0';
  DEBUG == 0 ? : printf("Line:%s1\n",line);

  if((strcmp(opt, "Y") == 0) || (strcmp(opt, "y") == 0)){
    sprintf(msg, "SEND %s \"%s\"\n", targetUserID, line);
    DEBUG == 0 ? : printf("msg:%s\n", msg);
    send(sfd, msg, strlen(msg), 0);
    //Burada OK ya da -ERR döndügüna dair kontrol eklenebilir.

    memset( &recvbuf, '\0', sizeof(recvbuf) );
    recv(sfd, recvbuf, 1024, 0);
    memset( &errOk, '\0', sizeof(errOk) );
    sscanf(recvbuf, "%s", errOk);
    if((strcmp(errOk, "-ERR")) == 0){
      printf("User:%s not available.\n",targetUserID);
    }
    else{
      printf("Message sended to %s\n",targetUserID);
    }

    memset( &recvbuf, '\0', sizeof(recvbuf) );
  }
  else if((strcmp(opt, "N") == 0) || (strcmp(opt, "n") == 0)){
    memset( &msg, '\0', sizeof(msg));
    memset( &line, '\0', sizeof(line));
  }
}

int ChangeCfg(int fd, int conn){

  char tmpServer[20], tmpuserID[20], tmppassword[30];
  int tmpPort;
  char yn[5], yn2[5];
  int opt;


  memset( &tmpServer, '\0', sizeof(tmpServer));
  memset( &tmpuserID, '\0', sizeof(tmpuserID));
  memset( &tmppassword, '\0', sizeof(tmppassword));
  strcpy(tmpServer, targetServer);
  strcpy(tmpuserID, userID);
  strcpy(tmppassword, password);
  tmpPort = targetPort;

  if(DEBUG == 1){
    printf("Server:%s\n",targetServer);
    printf("Port:%d\n",targetPort);
    printf("UserID:%s\n",userID);
    printf("Passwd:%s\n",password);
  }

  printf("\n%s\n", "Would you change any following value?");
  printf("  1) TargetServer: %s\n",targetServer);
  printf("  2) TargetPort: %d\n",targetPort);
  printf("  3) UserID: %s\n", userID);
  printf("  4) Passwd: %s\n", password);
  printf("  Option-> ");
  scanf("%d",&opt);
  DEBUG == 0 ? : printf("Option:%d\n",opt);

  switch (opt) {
    case 1:
      printf("Please give new TargetServer-> ");
      scanf("%s",tmpServer);
      DEBUG == 0 ? : printf("targetServer:%s\n",tmpServer);
    break;

    case 2:
      printf("Please give new TargetPort-> ");
      scanf("%d",&tmpPort);
      DEBUG == 0 ? : printf("targetPort:%d\n",tmpPort);
    break;

    case 3:
      printf("Please give new UserID-> ");
      scanf("%s",tmpuserID);
      DEBUG == 0 ? : printf("tmpuserID:%s\n",tmpuserID);
      printf("%s","Do you want to change password also? (Y/N): ");
      scanf("%s",yn2);
      if((strcmp(yn2, "y")) == 0 || (strcmp(yn2, "Y")) == 0){
        printf("Please give new Passwd-> ");
        scanf("%s",tmppassword);
      }
      else if((strcmp(yn2, "n")) == 0 || (strcmp(yn2, "N")) == 0){
        break;
      }
    break;

    case 4:
      printf("Please give new Passwd-> ");
      scanf("%s",tmppassword);
      DEBUG == 0 ? : printf("tmppassword:%s\n",tmppassword);
      printf("%s","Do you want to change UserID also? (Y/N): ");
      scanf("%s",yn2);
      if((strcmp(yn2, "y")) == 0 || (strcmp(yn2, "Y")) == 0){
        printf("Please give new Passwd-> ");
        scanf("%s",tmppassword);
      }
      else if((strcmp(yn2, "n")) == 0 || (strcmp(yn2, "N")) == 0){
        break;
      }
    break;

    default:
      system("clear");
      printf("[-] Please, enter between 1 and 4\n");
      //Bu kontrole bakılacak!!!!
    break;
  }

  printf("Do you want to use this new value?(Y/N): ");
  scanf("%s",yn);

  if((strcmp(yn, "y")) == 0 || (strcmp(yn, "Y")) == 0){
    memset( &targetServer, '\0', sizeof(targetServer));
    strcpy(targetServer, tmpServer);
    targetPort = tmpPort;


    if(conn == 0){ //eğer önceden bağlanılamadıysa çalışacak
      memset( &he, '\0', sizeof(he) );
      he = gethostbyname(targetServer);
      if(he == (struct hostent *) 0){
        perror("Error: gethostbyname");
      }

      memset( &adres, '\0', sizeof(adres) );
      memset(adres.sin_zero, '\0', sizeof adres.sin_zero);
      adres.sin_family = AF_INET;
      adres.sin_port = htons(targetPort);
      adres.sin_addr.s_addr = *((unsigned long *)he->h_addr);

      if((connect(fd, (struct sockaddr *)&adres, sizeof adres)) == -1){
        perror("Error(2): Connect");
        return 0;
      }
      else{
        return 1;
      }
    }
    else{
      memset( &msg, '\0', sizeof(msg));
      sprintf(msg, "USER %s %s\n", tmpuserID, tmppassword);
      send(sfd, msg, strlen(msg), 0);
      memset( &recvbuf, '\0', sizeof(recvbuf) );
      recv(sfd, recvbuf, 1024, 0);
      memset( &errOk, '\0', sizeof(errOk) );
      sscanf(recvbuf, "%s", errOk);

      if((strcmp(errOk, "-ERR")) == 0){
        system("clear");
        printf("%s\n","Error: Authentication");
        return 0;

      }
      else{
        memset( &userID, '\0', sizeof(userID));
        memset( &password, '\0', sizeof(password));
        strcpy(userID, tmpuserID);
        strcpy(password, tmppassword);
        return 1;
      }

    }
  }
  else if((strcmp(yn, "n")) == 0 || (strcmp(yn, "N")) == 0){
    return 0;
  }
}

void Quit(){
  memset( &msg, '\0', sizeof(msg));
  strcpy(msg,"QUIT");
  send(sfd, msg, strlen(msg), 0);
  memset( &recvbuf, '\0', sizeof(recvbuf) );
  recv(sfd, recvbuf, 1024, 0);
  printf("%s\n",recvbuf);
  close(sfd);
}
