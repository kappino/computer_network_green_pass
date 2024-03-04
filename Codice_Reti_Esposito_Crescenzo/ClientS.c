#include "grnpss.h"


int main(int argc, char *argv[]) {
    int sockfd, n ;
    char buffer[MAX_SIZE];
    char bitComClient = '0';
    struct sockaddr_in servaddr;

    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1027);




    //Si converte l’indirizzo IP, preso in input come stringa in formato dotted, in un indirizzo di rete in network order
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }
 
 
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }

    full_write(sockfd,&bitComClient,sizeof(char) );

    char codice_tessera[ID_SIZE];
    printf("Inserisci il codice della tua tessera sanitaria: ");
    int corretto = 0;
    while (corretto == 0) {
        scanf("%s", codice_tessera);
        if (strlen(codice_tessera)!=ID_SIZE-1) {
            printf("\nErrore! L'identificativo della tessera sanitaria ha 8 cifre! Reinserisci il codice:");
        } else corretto = 1;
    }
    
    //Si invia il codice_tessera da analizzare al ServerG
    full_write(sockfd, codice_tessera , sizeof(codice_tessera));

    //Ricezione del messaggio
    char valido;
    full_read(sockfd, &valido, sizeof(char));
    if (valido == '1') printf ("Il Certificato associato alla tessera sanitaria %s è valido",codice_tessera);
    else if (valido == '0') printf ("Il Certificato associato alla tessera sanitaria %s non è valido",codice_tessera);
    else printf("Il Certificato associato alla tessera sanitaria %s non è esistente",codice_tessera);
    
    close(sockfd);

    exit(0);
}
