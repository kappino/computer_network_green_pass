/*Un ClientS, per verificare se un green pass `e valido, 
invia il codice di una tessera sanitaria al ServerG il 
quale richiede al ServerV il controllo della validit`a. 
Un ClientT, inoltre, pu`o invalidare o ripristinare la 
validit`a di un green pass comunicando al ServerG il contagio
o la guarigione di una persona attraverso il codice della 
tessera sanitaria.*/

/* Client S --> invia codice tessera sanitaria
 Client T ----> invia codice tessera sanitaria piu valido o no

 Server G ---> riceve tessera sanitaria

 se inoltrata da ClientS, la inoltra a sua volta al ServerV
 che risponde con valido o meno, la risposta viene inoltrata
 al Client S

 se inoltrata da ClientT, riceve un comando sul ripristino
 o l'annullamento di un greenpass.

*/


#include "grnpss.h"
struct tm getCurrentTime() {
            //Ritorna una struct di tipo tm che rappresenta la data corrente
            time_t rawtime;
            struct tm * timeinfo;
            time ( &rawtime );
            timeinfo = localtime ( &rawtime );
            return *timeinfo;
}

Data getInizio(struct tm *timeinfo) {
            //A partire dalla struct tm identifica le varie variabili interessate e le salva come interi
            Data inizio;
            inizio.giorno = timeinfo->tm_mday;
            inizio.mese = timeinfo->tm_mon+1; 
            //Aggiungiamo 1900, poichè il conteggio degli anni inizia dal 1900 (2023->123)
            inizio.anno = timeinfo->tm_year+1900; 
            return inizio;
}

void cambiaStatoCertificato(int connfd) { 

    char bitComServer = '0';
    char bitComClient = '1';
    Notifica n;
    printf("Leggo codice tessera e nuovo valore da ClientT\n");
    full_read(connfd, &n, sizeof(Notifica));
    printf("Ricerco green pass associato alla tessera: %s\n\n",n.codice_tessera);
     //Fase di connessione con ServerV per il salvataggio della Certificazione
    int socket_fd;
            /*Creazione certificato da mandare con campi:
            {
                char codice_tessera[ID_SIZE];    tessera a cui è associato il certificato
                Data inizio;                data di inizio certificato
                Data scadenza;              data di fine certificato
                char valido;                identificatore 0/1 per riconoscere stato certificato
            }*/
    struct sockaddr_in server_addr;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM,0))<0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1026);

    if (inet_pton(AF_INET, "127.0.0.1",&server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }
    if (connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(server_addr))< 0) {
        perror("connect()");
        exit(1);
    }
    char valido;
    //Invio un carattere che funzionerà da identificatore per distinguere ServerG e CentroVaccinale
    full_write(socket_fd,&bitComServer,sizeof(char));
    full_write(socket_fd,&bitComClient,sizeof(char));

    //Compilo i campi del certificato appena creato, la validità verrà assegnata dal ServerV
    full_write(socket_fd,&n,sizeof(Notifica));
    close(socket_fd);
    printf("\nRichiesta di cambio stato inoltrata, controllare tramite ClientG il cambiamento.. resto in attesa");


}

void verificaCertificato(int connfd) {
    char bitComServer = '0';
    char bitComClient = '0';
    char codice_tessera[ID_SIZE];
    Certificato p;
    printf("Leggo codice tessera da ClientS\n");
    full_read(connfd, codice_tessera, sizeof(codice_tessera));
    printf("Ricerco green pass associato alla tessera: %s\n\n",codice_tessera);
    int socket_fd;
    struct sockaddr_in server_addr;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM,0))<0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1026);

    if (inet_pton(AF_INET, "127.0.0.1",&server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }
    if (connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(server_addr))< 0) {
        perror("connect()");
        exit(1);
    }
    char valido;
    //Invio un carattere che funzionerà da identificatore per distinguere ServerG e CentroVaccinale
    full_write(socket_fd,&bitComServer,sizeof(char));
    full_write(socket_fd,&bitComClient,sizeof(char));

    full_write(socket_fd,codice_tessera,sizeof(codice_tessera));
    full_read(socket_fd,&p,sizeof(Certificato));
    close(socket_fd);

    Data attuale;
    struct tm timeinfo = getCurrentTime();
    attuale = getInizio(&timeinfo);
    printf("Verifico la data di scadenza del Certificato.\n");
    printf("Data Scadenza Certificazione: [%02d-%02d-%02d]\n",p.scadenza.giorno, p.scadenza.mese, p.scadenza.anno);
    printf("Data Attuale: [%02d-%02d-%02d]\n", attuale.giorno, attuale.mese,attuale.anno);
    if (p.valido == '2') valido = '2';
    else if (p.valido == '0') valido = '0';
    //Se l'anno di scadenza è minore dell'anno attuale il certificato non vale!
    else if (p.scadenza.anno < attuale.anno ) valido = '0';
    //Se l'anno attuale e quello di scadenza sono uguali, verifichiamo il mese.
    else if (p.scadenza.anno == attuale.anno && p.scadenza.mese < attuale.mese ) valido = '0';
    //Se sia l'anno che il mese sono uguali, verifichiamo il giorno. 
    else if (p.scadenza.anno == attuale.anno && p.scadenza.mese == attuale.mese && p.scadenza.giorno < attuale.giorno) valido = '0';
    else valido = '1';
    full_write(connfd,&valido,sizeof(char));



    printf("\nFine ricerca.. resto in attesa");

}

 


int main(int argc, char *argv[]) {
    char bitComServer = '0';
    char bitComClient;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int connfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_SIZE];
    pid_t pid;

    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1027);

    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, 1024) < 0) {
        perror("listen");
        return 1;
    }
    while (1) {
        printf("Attendo codice tessera dai client..\n");
        if ((connfd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept");
            exit(1);
        }

        if ( (pid = fork () ) < 0 ) {
            perror("fork error");
            exit(1);
        }
 
        if (pid == 0) {
            close(listen_fd); //non è più in ascolto e interagisce col client tramite connsd

            full_read(connfd, &bitComClient, sizeof(char));
            if (bitComClient == '0') verificaCertificato(connfd);
            else cambiaStatoCertificato(connfd);
            
            close(connfd);

            exit(0); //Terminazione processo figlio senza errori
        }//il processo padre chiude connsd e ripete il ciclo
        close(connfd);

    }

    exit(0);
}




