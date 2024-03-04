#include "grnpss.h"


void salvaCertificato(int connfd) {
    int fd;
    Certificato p;

    //Leggo certificato inviato dal Centro Vaccinale 
    full_read(connfd, &p, sizeof(Certificato));
    Certificato p2;
    //Apro il file
    fd = open("file.dat", O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    char codice[strlen(p.codice_tessera)];
    strcpy(codice,p.codice_tessera);
    printf("\nCodice: %s ", codice);
    flock(fd, LOCK_EX);
    int pos = 0;


    while (read(fd, &p2, sizeof(Certificato)) > 0) {
        pos++;
        if (strcmp(p2.codice_tessera, codice) == 0) {
            printf("GreenPass Già Esistente\n");
            //Controllo se sono passati almeno 4 mesi dall'ultima vaccinazione/guarigione
            int diffAnno = 12 * (p.inizio.anno - p2.inizio.anno); 
            int diffMesi = abs((p2.inizio.mese+diffAnno) - p.inizio.mese);
            if (diffMesi > 4) {
                printf("Sono passati piu' di 4 mesi, salvo la nuova vaccinazione\n");
            } else {
                printf("Non sono passati ancora 4 mesi, non posso salvare la nuova vaccinazione\n");
                pos=-1;
                } 
            break;
            }
    }
    if (pos != -1) {
        p.valido = '1';
        lseek(fd, 0, SEEK_END);
        write(fd, &p, sizeof(Certificato));
    }

    stampaCertificato(p);
    flock(fd, LOCK_UN);

    close(fd);
} 

void verificaCertificato(int connfd) {
    char codice_tessera[ID_SIZE];
    full_read(connfd, codice_tessera, sizeof(codice_tessera));
    Certificato p;
    printf("Connesso al server G\n");
    int fd;
    printf("Effettuo verifica su codice: %s",codice_tessera);   
    fd = open("file.dat", O_RDONLY, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    flock(fd, LOCK_EX);
    int pos = 0;

    while (read(fd, &p, sizeof(Certificato)) > 0) {
        pos++;
        if (strcmp(p.codice_tessera, codice_tessera) == 0) {
            printf("\nGreenPass Trovato, mando esito a ServerG con codice tessera: %s",p.codice_tessera);
            full_write(connfd,&p,sizeof(Certificato));
            pos=-1;        // Abbiamo trovato il certificato
            break;
        }
    
    }
    if (pos != -1) {
        printf("\nGreenPass non trovato, mando esito a ServerG");
        p.valido = 2;
        full_write(connfd,&p,sizeof(Certificato));
    }
    flock(fd, LOCK_UN);
    close(fd);    
}

void aggiornaStato(int connfd) {
    Notifica n;
    full_read(connfd, &n, sizeof(Notifica));
    int fd;
    Certificato p2;
    //Apro il file
    fd = open("file.dat", O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    flock(fd, LOCK_EX);
    int pos = -1;
    int trovato = 0;

    while (read(fd, &p2, sizeof(Certificato)) > 0) {
        pos++;
        if (strcmp(p2.codice_tessera, n.codice_tessera) == 0) {
            printf("GreenPass Da Modificare Trovato!\nStato attuale: %c\n",p2.valido);
            trovato = 1;
            break;
        }
    }
    if (trovato == 0) {
        printf("Certificazione non trovata!\n");
        flock(fd, LOCK_UN);
        close(fd);
        exit(1);
    } else {
    p2.valido = n.valido;
    lseek(fd, pos * sizeof(Certificato),SEEK_SET);
    //Aggiorno lo stato del certificato con quello richiesto da ClientT
    write(fd, &p2, sizeof(Certificato));
    printf("Certificato modificato:\n");
    stampaCertificato(p2);
    }
    flock(fd, LOCK_UN);

    close(fd);


}



int main(int argc, char *argv[]) {

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int connfd;
    char buffer[MAX_SIZE];
    pid_t pid;
    char bitComServer;
    char bitComClient;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1026);

    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listenfd, 1024) < 0) {
        perror("listen");
        return 1;
    }

    while (1) {
        printf("In attesa di connessioni... \n\n");
        
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept");
            exit(1);
        }
        if ((pid = fork()) < 0) {
            perror("fork()");
            exit(1);
        }

        if (pid == 0 ) {
            close(listenfd);

            if (full_read(connfd, &bitComServer, sizeof(char)) < 0) {
                perror("full_read() error");
                exit(1);
            }            
            //printf("BIT SERVER: %c\n",bitComServer);

            if (bitComServer == '1') {
            printf("Connesso al Centro Vaccinale, attendo dati green pass\n\n");
            salvaCertificato(connfd);
            //Chiudo la connessione col centro vaccinale
            close(connfd);}

        
            else if (bitComServer == '0') {

                printf("ServerG Collegato\n");
                //Si connette al serverG e verifica se la richiesta arriva da ClientT o ClientS
                full_read(connfd,&bitComClient,sizeof(char));

                if (bitComClient == '0') {
                    verificaCertificato(connfd);
                } else {
                    aggiornaStato(connfd);
                }
            }
            exit(0);

        }
        close(connfd);        
    }
    exit(0);
}
