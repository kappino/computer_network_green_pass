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
            printf ( "Vaccinazione Effettuata il:  %s\n", asctime (timeinfo) );
            Data inizio;
            inizio.giorno = timeinfo->tm_mday;
            inizio.mese = timeinfo->tm_mon; 
            //Aggiungiamo 1900, poichè il conteggio degli anni inizia dal 1900 (2023->123)
            inizio.anno = timeinfo->tm_year+1900; 
            return inizio;
}

Data getScadenza(struct tm *timeinfo) {
            //Aggiungiamo 7 mesi invece che sei (durata green pass vaccinazione) poichè time conta i mesi da 0 a 11
            timeinfo->tm_mon += 7; 
            //Aggiungiamo 1900, poichè il conteggio degli anni inizia dal 1900 (2023->123)
            timeinfo->tm_year += 1900;
            //Convert broken-down time into time since the Epoch
            mktime(timeinfo);
            printf("Data Scadenza Certificazione: [%02d-%02d-%02d]\n", timeinfo->tm_mday,timeinfo->tm_mon,timeinfo->tm_year);
            Data scadenza;
            scadenza.giorno = timeinfo->tm_mday;
            scadenza.mese = timeinfo->tm_mon;   
            scadenza.anno = timeinfo->tm_year; 
            return scadenza;
}

 


int main(int argc, char *argv[]) {
    char bitComServer = '1';
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int connfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_SIZE];
    time_t ticks;
    pid_t pid;


    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1024);

    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, 1025) < 0) {
        perror("listen");
        return 1;
    }
    while (1) {
        printf("In attesa di nuovi vaccinati..\n");
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


            char codice_tessera[ID_SIZE];
            full_read(connfd, codice_tessera, sizeof(codice_tessera));

            // Qui si dovrebbe implementare il codice per recuperare la data di scadenza del green pass
            struct tm timeinfo = getCurrentTime();
            Data inizio = getInizio(&timeinfo);
            Data scadenza = getScadenza(&timeinfo);

            // Comunica all'Utente il codice della tessera sanitaria ed il periodo di validità del green pass
            sprintf(buffer, "%s Scadenza: [%02d-%02d-%02d]", codice_tessera, scadenza.giorno,scadenza.mese,scadenza.anno);
            full_write(connfd, buffer, strlen(buffer));
            
            close(connfd);

            //Fase di connessione con ServerV per il salvataggio della Certificazione
            int socket_fd;
            /*Creazione certificato da mandare con campi:
            {
                char codice_tessera[ID_SIZE];    tessera a cui è associato il certificato
                Data inizio;                data di inizio certificato
                Data scadenza;              data di fine certificato
                char valido;                identificatore 0/1 per riconoscere stato certificato
            }*/
            Certificato p;
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
            //Invio un carattere che funzionerà da identificatore per distinguere ServerG e CentroVaccinale
            full_write(socket_fd,&bitComServer,sizeof(char));
            //Compilo i campi del certificato appena creato, la validità verrà assegnata dal ServerV
            strcpy(p.codice_tessera, codice_tessera);
            p.scadenza = scadenza;
            p.inizio = inizio;
            full_write(socket_fd,&p,sizeof(Certificato));
            close(socket_fd);


            exit(0); //Terminazione processo figlio senza errori
        }//il processo padre chiude connsd e ripete il ciclo
        close(connfd);

    }

    exit(0);
}
