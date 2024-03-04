#include "grnpss.h"


int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[MAX_SIZE];
    char bitComClient = '1';
    char valido;
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
    fflush(stdin);

    printf("Ora inserisci 1 se vuoi attivare il certificato selezionato 0 se lo vuoi disattivare: ");
    corretto = 0;
    while (corretto == 0) {
        scanf("%c", &valido);
        if (valido == '0' || valido == '1') {
            corretto = 1;
        } else printf("\nErrore! Sono accettati solo i valori 1 (per attivare) o 0 (per disattivare)! \nRiprova:  ");
    }
    
    Notifica n;
    strcpy(n.codice_tessera,codice_tessera);
    n.valido = valido;
    //Si invia il codice_tessera da analizzare al ServerG
    full_write(sockfd, &n , sizeof(Notifica));
    printf("\nRichiesta di cambio stato inoltrata, controllare tramite ClientG il cambiamento..");

    
    close(sockfd);

    exit(0);
}
