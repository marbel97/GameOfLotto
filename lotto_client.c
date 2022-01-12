#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define BUFFER_SIZE 1024
#define CMD_SIZE 100
#define SID_SIZE 11

void print_all_commands(){
	printf("Sono disponibili i seguenti comandi:\n\n");
	printf("1) !help <comando> --> mostra i dettagli di un comando\n");
	printf("2) !signup <username> <password> --> crea un nuovo utente\n");
	printf("3) !login <username> <password> --> autentica un utente\n");
	printf("4) !invia_giocata g --> invia una giocata g al server\n");
	printf("5) !vedi_giocate tipo --> visualizza le giocate precedenti dove tipo = {0,1}\n");
	printf("			  e permette di visualizzare le giocate passate ‘0’\n");
	printf("			  oppure le giocate attive ‘1’ (ancora non estratte)\n");
	printf("6) !vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime n estrazioni\n");
	printf("				    sulla ruota specificata\n");
	printf("7) !vedi_vincite --> mostra le vincite\n");
	printf("8) !esci --> termina il client\n");
}

void print_help(const char* cmd){
	if (!strcmp(cmd, "!help")){
		printf("Formato: !help comando(fac):\n\n");
		printf("Mostra i dettagli di un comando.\n");
		printf("Se non viene specificato nessun comando, mostra la lista dei comandi disponibili\n");
	}else if (!strcmp(cmd, "!signup")){
		printf("Formato: !signup username password:\n\n");
		printf("Invia al server una richiesta di registrazione usando username e password specificate come argomenti\n");
	}else if (!strcmp(cmd, "!login")){
		printf("Formato: !login username password:\n\n");
		printf("Accedi all'utente <username> utilizzando <password> come parola chiave\n");
	}else if (!strcmp(cmd, "!invia_giocata")){
		printf("Formato: !invia_giocata -r ruota1 ruota2 ... ruotaN -n num1 num2 ... numN -i imp1 imp2 ... :\n\n");
		printf("Invia al server una schedina compilata come segue:\n");
		printf("- -r precede un qualsiasi numero di ruote o l'opzione <tutte> per giocare su tutte le ruote\n");
		printf("- -n precede i numeri giocati (da 1 a 5 numeri)\n");
		printf("- -i precede gli importi giocati rispettivamente su <estratto>, <ambo>, <terno>, <quaterna> e <cinquina>:\n");
		printf("  devono essere specificati in ordine, anche quando nulli, fino all'ultimo importo non nullo");
	}else if (!strcmp(cmd, "!vedi_giocate")){
		printf("Formato: !vedi_giocate tipo:\n\n");
		printf("tipo = 0: visualizza le tue giocate relative a estrazioni già effettuate\n");
		printf("tipo = 1: visualizza le tue giocate ancora in attesa del risultato\n");
	}else if (!strcmp(cmd, "!vedi_estrazione")){
		printf("Formato: !vedi_estrazione n ruota(fac):\n\n");
		printf("Vedi le <n> estrazioni passate sulla ruota <ruota>.\n");
		printf("Se non viene specificata alcuna ruota, mostra tutte le ruote\n");
	}else if (!strcmp(cmd, "!vedi_vincite")){
		printf("Formato: !vedi_vincite:\n\n");
		printf("Visualizza tutte le tue precedenti vincite e le statistiche complessive\n");
	}else if (!strcmp(cmd, "!esci")){
		printf("Formato: !esci:\n\n");
		printf("Termina la sessione ed esci\n");
	}else{
		printf("%s: comando inesistente\n", cmd);
	}
}

int main(int argc, char* argv[]){
	int ret, sd, len, i, req_type; // req type 0 login 1 signup
	struct sockaddr_in srv_addr;
	uint16_t lmsg;
	char buffer[BUFFER_SIZE];
	char tok_buf[BUFFER_SIZE];
	char* token;
	char* cmd[CMD_SIZE];
	char delim[] = "\n, ";
	char session_id[SID_SIZE] = "";
	
	//controllo numero di argomenti
	if (argc != 3){
		printf("Errore: argomenti richiesti: 2, forniti: %d\n", argc-1);
		exit(-1);
	}
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &srv_addr.sin_addr);
	
	ret = connect(sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret < 0){
		perror("Errore in fase di connessione");
		exit(-1);
	}
	
	printf("\n*****************************GIOCO DEL LOTTO *****************************\n");
	print_all_commands();
	
	while(1){
		if (req_type != 1){
			printf("\n> ");
			fgets(buffer, BUFFER_SIZE, stdin);
			strcpy(tok_buf, buffer);
		}else{
			printf("Nome e password: ");
			strcpy(tok_buf, "!signup ");
			fgets(buffer, BUFFER_SIZE-8, stdin);
			strcat(tok_buf, buffer);
			strcpy(buffer, tok_buf);
			req_type = -1;
		}
		
		for (i = 0; i < CMD_SIZE; i++){
			cmd[i] = NULL;
		}
		
		token = strtok(tok_buf, delim);
		
		i = 0;
		
		do{
			cmd[i] = token;
			token = strtok(NULL, delim);
			i++;
		}while(token != NULL && i < CMD_SIZE);
		
		if (!cmd[0]){
			printf("Nessun comando inserito\n");
			continue;
		}
		
		if (!strcmp("!help", cmd[0])){
			if (cmd[1] == NULL){
				print_all_commands();
			}else{
				if (cmd[2] == NULL){
					print_help(cmd[1]);
				}else{
					printf("Errore: forniti troppi argomenti\n");
				}
			}
			continue;
		}else if (!strcmp("!login", cmd[0])){
			req_type = 0;
		}else{
			if (strcmp("!signup", cmd[0])){
				strcat(buffer," ");
				strcat(buffer,session_id);
			}else{
				req_type = 1;
			}
		}

		len = strlen(buffer)+1;
		lmsg = htons(len);
		// invio la dimensione del messaggio
		ret = send(sd, (void*)&lmsg, sizeof(uint16_t), 0);
		//invio il messaggio
		ret = send(sd, (void*)buffer, len, 0);
		
		if (ret < 0){
			perror("Errore in fase di invio");
			exit(-1);
		}
		
		strcpy(buffer,"");
		printf("%s\n",buffer);
		
		// ricevo la dimensione del messaggio
		ret = recv(sd, (void*)&lmsg, sizeof(uint16_t), 0);
		len = ntohs(lmsg);
		// ricevo il messaggio
		ret = recv(sd,(void*)buffer, len, 0);
		
		if (ret < 0){
			perror("Errore in fase di ricezione");
			exit(-1);
		}
		
		if (!strcmp(buffer, "msg_exit")){
			printf("Disconnessione completata\n");
			break;
		}
		
		if (!strcmp(buffer, "msg_blocked")){
			printf("Troppi tentativi di accesso. Riprovare più tardi\n");
			break;
		}
		
		if (!strcmp(buffer, "msg_start")){
			do {
				// ricevo la dimensione del messaggio
				ret = recv(sd, (void*)&lmsg, sizeof(uint16_t), 0);
				len = ntohs(lmsg);
				// ricevo il messaggio
				ret = recv(sd,(void*)buffer, len, 0);
				
				if (ret < 0){
					perror("Errore in fase di ricezione");
					exit(-1);
				}
				if (strcmp(buffer,"msg_end"))
					printf("%s", buffer);
			} while (strcmp(buffer, "msg_end"));
		}
		
		if (!req_type){
			token = strtok(buffer, "&");
			printf("%s\n", token);
			token = strtok(NULL,"&");
			if (token){	
				strcpy(session_id, token);
				printf("%s\n", session_id);
			}
			req_type = -1;
		}else{
			if (strcmp(buffer,"msg_end") && strcmp(buffer,"msg_signup_success"))
				printf("%s\n", buffer);
		}
		
		if (!strcmp(buffer,"msg_signup_success")){
			req_type = -1;
			printf("Registrazione avvenuta con successo!");
		}
		
	}
	close(sd);
	exit(0);
}