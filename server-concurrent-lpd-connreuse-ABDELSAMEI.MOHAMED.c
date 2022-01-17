//ABDELSAMEI.MOHAMED 133844
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"

/* Massima lunghezza stringhe: 4 KiB */
#define MAX_REQUEST_SIZE (4 * 1024)

void riporta_imp(int fd_in, int fd_out) /* non completo (la mia idea e quella di fare un fork ,
                                         * il figlio fa una execlp (cut) e farei il controllo sul orario se minore di un ora aumento il tot di 1)*/
{
        FILE *stream;
        char str[MAX_REQUEST_SIZE],char str_fin[MAX_REQUEST_SIZE];
        int tot=0;

        stream = fdopen(fd_in, "r");
        if (stream == NULL){
                printf("Could not open file %d", fd_in);
                exit(EXIT_FAILURE);
        }

        while (fgets(str, MAX_REQUEST_SIZE, stream) != NULL){
                

        }

        snprintf(str_fin, sizeof(str_fin), "\n numero di fermi macchina importanti è: %f\n", tot);
        if (write_all(fd_out, str_fin, strlen(str_fin)) < 0){
                perror("write finale tot");
                exit(EXIT_FAILURE);
        }
}


/* Gestore del segnale SIGCHLD */
void handler(int signo)
{
        int status;

        (void)signo; /* per evitare warning */

        /* eseguo wait non bloccanti finchÃ© ho dei figli terminati */
        while (waitpid(-1, &status, WNOHANG) > 0)
                continue;
}


int main(int argc, char **argv)
{
        int sd, err, on;
        struct addrinfo hints, *res;
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        /* uso SA_RESTART per evitare di dover controllare esplicitamente se
         * accept Ã¨ stata interrotta da un segnale e in tal caso rilanciarla */
        sa.sa_flags   = SA_RESTART;
        sa.sa_handler = handler;

        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        memset(&hints, 0, sizeof(hints));
        /* Usa AF_INET per forzare solo IPv4, AF_INET6 per forzare solo IPv6 */
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags    = AI_PASSIVE;

        if ((err = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
                fprintf(stderr, "Errore setup indirizzo bind: %s\n", gai_strerror(err));
                exit(EXIT_FAILURE);
        }

        if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
                perror("Errore in socket");
                exit(EXIT_FAILURE);
        }

        on = 1;
        if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }

        if (bind(sd, res->ai_addr, res->ai_addrlen) < 0) {
                perror("Errore in bind");
                exit(EXIT_FAILURE);
        }

        /* rilascio memoria allocata da getaddrinfo */
        freeaddrinfo(res);

        /* trasforma in socket passiva d'ascolto */
        if (listen(sd, SOMAXCONN) < 0) {
                perror("listen");
                exit(EXIT_FAILURE);
        }

        for(;;) {
                int ns, pid;

                /* Mi metto in attesa di richieste di connessione */
                if ((ns = accept(sd, NULL, NULL)) < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                /* Creo un processo figlio per gestire la richiesta */
                if ((pid = fork()) < 0) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                } else if (pid == 0) { 
                        /* FIGLIO */
                        int fd;
                        uint8_t len[2];
                        char template[256];
                        char nomelinea[MAX_REQUEST_SIZE];
                        char giorno[MAX_REQUEST_SIZE];
                        char N[MAX_REQUEST_SIZE];
                        size_t nomelinea_len, giorno_len, N_len;

                        /* Chiudo la socket passiva */
                        close(sd);

                        /* Disabilito gestore SIGCHLD */
                        signal(SIGCHLD, SIG_DFL);
                                
                        for (;;) {
                                off_t off;
                                int pipe_n1n2[2], pipe_n2n3[2];
                                int pid_n1, pid_n2;

                                /* Leggo lunghezza nomelinea */
                                if (read(ns, len, 2) < 0) {
                                        perror("read 1");
                                        exit(EXIT_FAILURE);
                                }

                                /* Decodifico lunghezza nomelinea come intero unsigned a
                                * 16 bit in formato big endian (AKA network byte
                                * order) */
                                nomelinea_len = (size_t)len[1] | ((size_t)len[0] << 8);

                                /* Inizializzo il buffer nomelinea a zero e so che
                                * nomelinea_len < sizeof(nomelinea), quindi sono sicuro
                                * che il buffer sarÃ  sempre null-terminated. In questo
                                * modo, posso interpretarlo come una stringa C e
                                * passarlo direttamente alla funzione strcmp. */
                                memset(nomelinea, 0, sizeof(nomelinea));
                                if (read_all(ns, nomelinea, nomelinea_len) < 0) {
                                        perror("read 2");
                                        exit(EXIT_FAILURE);
                                }

                                /* Leggo lunghezza giorno */
                                if (read_all(ns, len, 2) < 0) {
                                        perror("read 3");
                                        exit(EXIT_FAILURE);
                                }

                                /* Decodifico lunghezza giorno come intero unsigned a
                                * 16 bit in formato big endian (AKA network byte
                                * order) */
                                giorno_len = (size_t)len[1] | ((size_t)len[0] << 8);

                                /* Inizializzo il buffer giorno a zero e so che
                                * giorno_len < sizeof(giorno), quindi sono sicuro
                                * che il buffer sarÃ  sempre null-terminated. In questo
                                * modo, posso interpretarlo come una stringa C e
                                * passarlo direttamente alla funzione strcmp. */
                                memset(giorno, 0, sizeof(giorno));
                                if (read_all(ns, giorno, giorno_len) < 0) {
                                        perror("read 4");
                                        exit(EXIT_FAILURE);
                                }

                                /* Leggo lunghezza N */
                                if (read_all(ns, len, 2) < 0) {
                                        perror("read 5");
                                        exit(EXIT_FAILURE);
                                }

                                /* Decodifico lunghezza N come intero unsigned a
                                 * 16 bit in formato big endian (AKA network byte
                                 * order) */
                                N_len = (size_t)len[1] | ((size_t)len[0] << 8);

                                /* Inizializzo il buffer N a zero e so che
                                 * N_len < sizeof(N), quindi sono sicuro
                                 * che il buffer sarÃ  sempre null-terminated. In questo
                                 * modo, posso interpretarlo come una stringa C e
                                 * passarlo direttamente alla funzione strcmp. */
                                memset(N, 0, sizeof(N));
                                if (read_all(ns, N, N_len) < 0) {
                                        perror("read 6");
                                        exit(EXIT_FAILURE);
                                }

                                if (pipe(pipe_n1n2) < 0) {
                                        perror("pipe");
                                        exit(EXIT_FAILURE);
                                }

                                pid_n1 = fork();
                                if (pid_n1 < 0) {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                } else if (pid_n1 == 0) {
                                        /* NIPOTE 1 */
                                        char filename[2 * MAX_REQUEST_SIZE + 256];

                                        /* snprintf(filename, sizeof(filename), "/var/local/downtimes/%s.txt", giorno); */
                                        snprintf(filename, sizeof(filename), "%s.txt", giorno);

                                        /* Chiudo file descriptor non usati */
                                        close(ns);
                                        close(pipe_n1n2[0]);

                                        /* Redirezione output su pipe */
                                        close(1);
                                        if (dup(pipe_n1n2[1]) < 0) {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n1n2[1]);
                                        
                                        execlp("sort", "sort", "-n" , "-r", filename , (char *)NULL);
                                        perror("exec sort");
                                        exit(EXIT_FAILURE);
                                }

                                if (pipe(pipe_n2n3) < 0) {
                                        perror("pipe");
                                        exit(EXIT_FAILURE);
                                }

                                pid_n2 = fork();
                                if (pid_n2 < 0) {
                                        perror("fork");
                                        exit(EXIT_FAILURE);
                                } else if (pid_n2 == 0) {
                                        /* NIPOTE 2 */

                                        /* Chiudo file descriptor non usati */
                                        close(ns);
                                        close(pipe_n1n2[1]);
                                        close(pipe_n2n3[0]);

                                        /* Redirezione input da pipe */
                                        close(0);
                                        if (dup(pipe_n1n2[0]) < 0) {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n1n2[0]);

                                        /* Redirezione output su pipe */
                                        close(1);
                                        if (dup(pipe_n2n3[1]) < 0) {
                                                perror("dup");
                                                exit(EXIT_FAILURE);
                                        }
                                        close(pipe_n2n3[1]);
                                     
                                        execlp("head", "head", "-n", N , (char *)NULL);
                                        perror("head");
                                        exit(EXIT_FAILURE);
                                }

                                /* FIGLIO */

                                /* Chiudo file descriptor non usati */
                                close(pipe_n1n2[0]);
                                close(pipe_n1n2[1]);
                                close(pipe_n2n3[1]);

                                /* Creo file temporaneo */
                                strcpy(template, "manufacturing_downtimes-XXXXXX");
                                fd = mkstemp(template);
                                if (fd < 0) {
                                        perror("MKSTEMP");
                                        exit(EXIT_FAILURE);
                                }

                                  //riporto anche il n numero di fermi macchina“importanti"
                                riporta_imp(pipe_n2n3[0], fd);
                                

                                /* Calcolo dimensione file */
                                off_t response_len = lseek(fd, 0, SEEK_END);
                                if (response_len < 0) {
                                        perror("lseek");
                                        exit(EXIT_FAILURE);
                                }

                                if (response_len > UINT16_MAX) {
                                        fprintf(stderr, "Risposta troppo grande!\n");
                                        exit(EXIT_FAILURE);
                                }

                                len[0] = (uint8_t)((response_len & 0xFF00) >> 8);
                                len[1] = (uint8_t)(response_len & 0x00FF);

                                if (write_all(ns, len, 2) < 0) {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                }

                                off = 0;
                                if (sendfile(ns, fd, &off, response_len) < 0) {
                                        perror("sendfile");
                                        exit(EXIT_FAILURE);
                                }

                                /* Rimuovo il file temporaneo */
                                unlink(template);
                                close(fd);
                        }

                        close(ns);
                    }

                /* PADRE */ 

                /* Chiudo la socket attiva */
                close(ns);
        }

        close(sd);

        return 0;
}
