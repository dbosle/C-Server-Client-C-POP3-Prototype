# C SMS Server-Client (C POP3 Prototype)

## Compiling server.c and client.c
```
gcc server.c -lpthread -I. libiniparser.a -o server
gcc client.c -I. libiniparser.a -o client
```
* You will get the `server` and `client` executable elf files

## Configuration
* Create file named `server.ini` in the same folder as server(executable elf file)
* You can add/del users from server.ini
* server.ini:
```
[Server]
ServerMsg = My Chat Server v0.1
ListenPort=1891
ListenIp=All
ServerRoot=./home


[users]
#UserID = Password
User1=1234
User2=1234
User3=1234
```

* Create file named `client.ini` in the same folder as client(executable elf file)
* Set the ip and port address
* Set the username and password
* client.ini:
```
[client]
TargetServer=127.0.0.1
TargetPort=1891
UserID=User1
Passwd=1234
```

## Run
* Server:
```
./server
```

* Client:
```
./server
```


# To Do
* Swap all `strcat` with `sprintf`
