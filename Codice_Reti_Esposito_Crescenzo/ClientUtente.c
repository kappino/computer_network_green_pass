#include "grnpss.h"


int main(int argc, char *argv[]) {
    int sockfd, n ;
    char buffer[MAX_SIZE];
    struct sockaddr_in servaddr;


    if (argc != 2) {
        fprintf(stderr,"usage: %s localhost\n",argv[0]);
        exit(1);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1025);

    //Struttura per utilizzare la gethostbyname
	struct hostent *data;
    char **alias;
    char *address;

    if ((data = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname() error");
        exit(1);
    }
    alias = data -> h_addr_list;

    if ((address = (char *)inet_ntop(data -> h_addrtype, *alias, buffer, sizeof(buffer))) < 0) {
        perror("inet_ntop() error");
        exit(1);
    }
    //Si converte l’indirizzo IP, preso in input come stringa in formato dotted, in un indirizzo di rete in network order
    if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }
 
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }


    char codice_tessera[ID_SIZE];
    printf("Inserisci il codice della tua tessera sanitaria: ");
    int corretto = 0;
    while (corretto == 0) {
        scanf("%s", codice_tessera);
        if (strlen(codice_tessera)!=ID_SIZE-1) {
            printf("\nErrore! L'identificativo della tessera sanitaria ha 8 cifre! Reinserisci il codice:");
        } else corretto = 1;
    }
    
    //Si invia il pacchetto richiesto al CentroVaccinale
    full_write(sockfd, codice_tessera , sizeof(codice_tessera));

    //Ricezione del messaggio
    full_read(sockfd, buffer, MEX_SIZE);
    printf("Vaccinazione Effettuata! \n%s\n\n", buffer);

    exit(0);
}
