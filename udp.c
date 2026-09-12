
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
#define DEFAULT_DSPORT "59000"
#define DEFAULT_DSIP "tejo.tecnico.ulisboa.pt"
#define DEFAULT_DSPORT "59000"

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char line[256];
char buffer[128];
char message[256];
char uid[7], 
password[9], 
extra[16];
int is_logged_in = 0;

int send_message(const char *message) {
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit (1);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) /* error */ exit(1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

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
        if (peerport == NULL) { //adicionei isto pq é origatiorio ter peerport
            fprintf(stderr, "Error: -m peerport is mandatory.\n");
            exit(1);
}
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

    errcode = getaddrinfo(DSIP, DSport, &hints, &res);
    if (errcode != 0) /* error */ exit(1);

    while(1){
        if (fgets(line, sizeof(line), stdin) == NULL) {
            // erro ou EOF (ex: Ctrl+D)
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        char command[16];
        // extrai só a primeira palavra para saber que comando é
        if (sscanf(line, "%15s", command) != 1) {
            continue; // linha vazia
        }

        if (strcmp(command, "login") == 0) { // comando login
            if (is_logged_in == 1 ) {//para evitar dar login quando ja estás logged in (se calhar foi isso que te aconteceu quando te deu aquele erro da password dar errada mesmo que nunca tivvesses feito login com esse UID)
                printf("Já existe um utilizador com sessão iniciada. Faz logout primeiro.\n");
                continue;
            }
            if (sscanf(line, "login %s %s %s", uid, password, extra) != 2) {
                printf("Uso: login UID password\n");
                continue;
            }
            
            if (strlen(uid) != 6 || strlen(password) != 8) {
                printf("UID deve ter 6 caracteres e password deve ter 8 caracteres.\n");
                continue;
            }

            if (strspn(uid, "0123456789") != 6) {
                printf("UID inválido: deve ter exatamente 6 dígitos\n");
                continue;
            }
            
            if (strspn(password, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != 8) {
                printf("Password inválida: deve ter exatamente 8 caracteres alfanuméricos\n");
                continue;
            }
            

            // Se chegou aqui, UID e password são válidos
            sprintf(message, "LIN %s %s %s\n", uid, password, peerport); // Tá no enunciado que a mensagem de login para o DS tem de ir com LIN
            send_message(message);
            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0,
                    (struct sockaddr *)&addr, &addrlen);
            
            //o exit, freeaddinfo e close são só final com a flag exit para fechar o socket
            if (n == -1){
                perror("Error receiving message from DS");
                continue;
            }

            //As messagens pedidas no enunciado
            write(1, buffer, n);
            if (strncmp(buffer, "OK", 2) == 0) {
                printf("Successfull login.\n");
                is_logged_in = 1;
            } 
            else if (strncmp(buffer, "NOK", 3) == 0) {
                printf("Incorrect login attempt.\n");
            } 
            else if (strncmp(buffer, "REG", 3) == 0) {
                printf("New user registered.\n");
            }
            
        }
        if (strcmp(command, "logout") == 0) { 
            if (is_logged_in !=1 ) { //se o user não estiver logged in verifico primeiro localmente só no caso
                printf("User not logged in.\n"); continue; 
            }
            sprintf(message, "LOU %s %s %s\n", uid, password, peerport); // Tá no enunciado que a mensagem de logout para o DS tem de ir com LOU
            send_message(message);

            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0,
                    (struct sockaddr *)&addr, &addrlen);
            if (n == -1){
                perror("Error receiving message from DS");
                continue;
            }

            //mensagems pedidas no enunciado
            write(1, buffer, n);
            if (strncmp(buffer, "OK", 2) == 0) {
                printf("Successfull logout.\n");
                is_logged_in = 0;
            } else if (strncmp(buffer, "NLG", 3) == 0) {
                printf("User not logged in.\n");
            } else if (strncmp(buffer, "UNR", 3) == 0) {
                printf("Unknown user.\n");
            }
            // comando logout
            freeaddrinfo(res);
            close(fd);
            return 0;
        }

        if (strcmp(command, "unregister") == 0) {
             // comando unregister
            send_message(line);
            freeaddrinfo(res);
            close(fd);
            return 0;
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
