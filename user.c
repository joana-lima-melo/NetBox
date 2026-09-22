
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#define DEFAULT_DSPORT "59000"
#define DEFAULT_DSIP "tejo.tecnico.ulisboa.pt"
#define DEFAULT_DSPORT "59000"

typedef struct user {
    char uid[7];
    char password[9];
    int is_logged_in;
} User;

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char line[256];
char buffer[128];
char message[256];
char extra[16];
User user = {0};

int send_message(const char *message) {
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit (1);
    }
    return 0;
}

void sigint_handler(int sig) {
    (void)sig; // evita warning de parâmetro não usado

    if (user.is_logged_in == 1) {
        printf("\nJá existe um utilizador com sessão iniciada. Faz logout primeiro.\n");
        fflush(stdout);
        return; // não sai, volta ao que estava a fazer
    }

    freeaddrinfo(res);
    close(fd);
    exit(0);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler); 
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) /* error */ exit(1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct timeval timeout;
    timeout.tv_sec = 5;   // 5 segundos
    timeout.tv_usec = 0;  // 0 microssegundos (podes combinar os dois, ex: 500000 = meio segundo)

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        fprintf(stderr, "setsockopt failed");
        exit(1);
    }

    //OK calma, basicamente o programa é iniciado com ./projeto -m peerport [-n DSIP] [-p DSport],
    //o peerport é obrigatorio e é o PORT do TCP que vakos fazer mais pra frente, o DSIP e o DSport sao opcionais e por isso se nao sao mencinados ficam como os dados pelo prof
    //o getopt le os valores à frente do m, n e p. O opt é a cena da qual esta a ler o valor (om, n ou o p)
    //o optarg é o valor que esta a frente do opt, ou seja, o valor que o user escreveu no terminal
    //ou seja, estou so a dizer por exemplo que o peerport é o valor que o user escreveu a frente do -m.
    //ou seja, se eu escrever no terminal ./projeto -m 58002, o peerport vai ser 58002
    //Espero que tenha explicado bem e sorry se nao expliquei T_T
    char *DSIP = DEFAULT_DSIP;
    char *DSport = DEFAULT_DSPORT;
    char *peerport = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "m:n:p:")) != -1) {
        switch (opt) {
            case 'm':
                peerport = optarg;
                break;
            case 'n':
                DSIP = optarg;
                break;
            case 'p':
                DSport = optarg;
                break;
            default:
                fprintf(stderr, "Uso: %s -m peerport [-n DSIP] [-p DSport]\n", argv[0]);
                exit(1);
        }
    }
    
    if (peerport == NULL) { //adicionei isto pq é origatiorio ter peerport (tirei fora do while em cima)
            fprintf(stderr, "Error: -m peerport is mandatory.\n");
            exit(1);
        }

    errcode = getaddrinfo(DSIP, DSport, &hints, &res);
    if (errcode != 0) /* error */ exit(1);

    while(1){
        char command[16],reply_cmd[16], status[16]; //falta-me declarar uma variaveis então meti tudo no inicio do loop

        if (fgets(line, sizeof(line), stdin) == NULL) {
            // erro ou EOF (ex: Ctrl+D)
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        // extrai só a primeira palavra para saber que comando é
        if (sscanf(line, "%15s", command) != 1) {
            continue; // linha vazia
        }

        if(strcmp(command, "publish") == 0) {
            printf("Comando publish não implementado neste código.\n");
            continue;
        }

        if (strcmp(command, "login") == 0) { // comando login
            if (user.is_logged_in == 1 ) {//para evitar dar login quando ja estás logged in (se calhar foi isso que te aconteceu quando te deu aquele erro da password dar errada mesmo que nunca tivvesses feito login com esse UID)
                printf("Já existe um utilizador com sessão iniciada. Faz logout primeiro.\n");
                continue;
            }
            if (sscanf(line, "login %s %s %s", user.uid, user.password, extra) != 2) {
                printf("Uso: login UID password\n");
                continue;
            }

            if (strlen(user.uid) != 6 || strlen(user.password) != 8) {
                printf("UID deve ter 6 caracteres e password deve ter 8 caracteres.\n");
                continue;
            }

            if (strspn(user.uid, "0123456789") != 6) {
                printf("UID inválido: deve ter exatamente 6 dígitos\n");
                continue;
            }
            
            if (strspn(user.password, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != 8) {
                printf("Password inválida: deve ter exatamente 8 caracteres alfanuméricos\n");
                continue;
            }
            

            // Se chegou aqui, UID e password são válidos
            sprintf(message, "LIN %s %s %s\n", user.uid, user.password, peerport); // Tá no enunciado que a mensagem de login para o DS tem de ir com LIN
            send_message(message);
            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0,
                    (struct sockaddr *)&addr, &addrlen);
            
            //o exit, freeaddinfo e close são só final com a flag exit para fechar o socket
            if (n == -1){
                perror("Error receiving message from DS");
                continue;
            }

            //As messagens pedidas no enunciado - corrigia-as faltava ler o RLI
            buffer[n] = '\0';
            if (sscanf(buffer, "%s %s", reply_cmd, status) == 2 && strcmp(reply_cmd, "RLI") == 0) {
                if (strncmp(status, "OK", 2) == 0) {
                    printf("Successfull login.\n");
                    user.is_logged_in = 1;
                } 
                else if (strncmp(status, "NOK", 3) == 0) {
                    printf("Incorrect login attempt.\n");
                } 
                else if (strncmp(status, "REG", 3) == 0) {
                    printf("New user registered.\n");
                    user.is_logged_in = 1; 
                } 
                else if (strcmp(status, "ERR") == 0) {
                    printf("Syntax or parameter error.\n");
                } 
            }   
        
        }

        if (strcmp(command, "logout") == 0) { // comando logout
            if (user.is_logged_in !=1 ) { //se o user não estiver logged in verifico primeiro localmente só no caso(tenho de perguntar a stora se faz senitdio tho)
                printf("User not logged in.\n"); continue; 
            }

            sprintf(message, "LOU %s %s\n", user.uid, user.password); // Tá no enunciado que a mensagem de logout para o DS tem de ir com LOU
            send_message(message);

            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0,
                    (struct sockaddr *)&addr, &addrlen);

            if (n == -1){
                perror("Error receiving message from DS");
                continue;
            }

            //As messagens pedidas no enunciado - corrigia-as faltava ler o RLO
            buffer[n] = '\0';
            if (sscanf(buffer, "%s %s", reply_cmd, status) == 2 && strcmp(reply_cmd, "RLO") == 0) {
                if (strncmp(status, "OK", 2) == 0) {
                    printf("Successfull logout.\n");
                    user.is_logged_in = 0;
                } 
                else if (strncmp(status, "NLG", 3) == 0) {
                    printf("User not logged in.\n");
                } 
                else if (strncmp(status, "UNR", 3) == 0) {
                    printf("Unknown user.\n");
                }
                else if (strncmp(status, "WRP", 3) == 0) {
                    printf("Wrong password.\n");
                } 
                else if (strcmp(status, "ERR") == 0) {
                    printf("Syntax or parameter error.\n");
                }
            }
        }

        if (strcmp(command, "unregister") == 0) { // comando unregister
            if (user.is_logged_in != 1) { //se o user não estiver logged in verifico primeiro localmente só no caso(tenho de perguntar a stora se faz senitdio tho)
                printf("User not logged in.\n"); continue;
            }

            sprintf(message, "UNR %s %s\n", user.uid, user.password); // Tá no enunciado que a mensagem de unregister para o DS tem de ir com UNR
            send_message(message);

            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0,
                    (struct sockaddr *)&addr, &addrlen);

            if (n == -1){
                perror("Error receiving message from DS");
                continue;
            }
            
            //mensagens pedidas no enunciado
            buffer[n] = '\0';
            //As messagens pedidas no enunciado - corrigia-as faltava ler o RUR
            if (sscanf(buffer, "%s %s", reply_cmd, status) == 2 && strcmp(reply_cmd, "RUR") == 0) {
                if (strncmp(status, "OK", 2) == 0) {
                    printf("Successfull unregister.\n");
                    user.is_logged_in = 0;
                } 
                else if (strncmp(status, "NOK", 3) == 0) {
                    printf("User not logged in.\n");
                } 
                else if (strncmp(status, "UNR", 3) == 0) {
                    printf("Unknown user.\n");
                } 
                else if (strncmp(status, "WRP", 3) == 0) {
                    printf("Wrong password.\n");
                } 
                else if (strcmp(status, "ERR") == 0) {
                    printf("Syntax or parameter error.\n");
                }
            }
        }

        if (strcmp(command, "exit") == 0) { //comanado de exit
            if (user.is_logged_in == 1 ) { //está na seccao 3.1 do enunciado isto é só um commando local com verificaçoes locais
                printf("Please logout first before exiting.\n"); 
                continue;
            }

            freeaddrinfo(res);
            close(fd);
            exit(0);
        }

        if (strcmp(command, "remove") == 0){
            if (user.is_logged_in == 1 ) {//para evitar dar login quando ja estás logged in (se calhar foi isso que te aconteceu quando te deu aquele erro da password dar errada mesmo que nunca tivvesses feito login com esse UID)
                printf("Já existe um utilizador com sessão iniciada. Faz logout primeiro.\n");
                continue;
            }

            if (sscanf(line, "remove %s %s %s", user.uid, user.password, extra) != 2) {
                printf("Uso: login UID password\n");
                continue;
            }

        }

    }

    /*n = sendto(fd, "Hello", 7, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0,
                 (struct sockaddr *)&addr, &addrlen);
    if (n == -1)  exit(1);

    write(1, "echo: ", 6);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
*/
    return 0;
}
