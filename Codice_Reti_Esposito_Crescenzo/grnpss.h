#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>


#define MAX_SIZE 1024
#define ID_SIZE 9
#define MEX_SIZE 64




typedef struct {
    int giorno;
    int mese;
    int anno;
 } Data;
 


typedef struct {
    char codice_tessera[ID_SIZE];
    Data inizio;
    Data scadenza;
    char valido;
} Certificato;

typedef struct {
    char codice_tessera[ID_SIZE];
    char valido;
} Notifica;




 //Legge esattamente count byte s iterando opportunamente le letture. Legge anche se viene interrotta da una System Call
ssize_t full_read(int fd, void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_read;
    n_left = count;
    while (n_left > 0) {  //repeat finchè non ci sono left
        if ((n_read = read(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; // Se si verifica una System Call che interrompe ripete il ciclo
            else exit(n_read);
        } else if (n_read == 0) break; // Se sono finiti esce
        n_left -= n_read;
        buffer += n_read;
    }
    buffer = 0;
    return n_left;
}


//Scrive esattamente count byte s iterando opportunamente le scritture. Scrive anche se viene interrotta da una System Call
ssize_t full_write(int fd, const void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_written;
    n_left = count;
    while (n_left > 0) {  //repeat finchè non ci sono left
        if ((n_written = write(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; //Se si verifica una System Call che interrompe ripete il ciclo
            else exit(n_written); //Se non è una System Call,esce con un errore
        }
        n_left -= n_written;
        buffer += n_written;
    }
    buffer = 0;
    return n_left;
}


void stampaCertificato(Certificato p) {
    printf("Codice Tessera: %s\n", p.codice_tessera);
    printf("Scadenza: [%02d-%02d-%02d]  \n", p.scadenza.giorno, p.scadenza.mese,p.scadenza.anno);
    printf("Valido: %c", p.valido);
}
