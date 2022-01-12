#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define CMD_SIZE 128
#define SID_SIZE 11
#define NAME_SIZE 128
#define PSW_SIZE 128
#define LINE_SIZE 256
#define ARG_SIZE 128
#define NUM_POOL_SIZE 90
#define CHAR_POOL_SIZE 36
#define INTERVAL_DEFAULT 5
#define EXTRACTION_LINES 12
#define FILENAME_SIZE 256

char *strptime(const char *buf, const char *format, struct tm *tm);

int vedi_giocate(char* cmd[], char* buffer, int new_sd, char* logged_user){
	char temp[LINE_SIZE];
	FILE* f;
	int fd, ret, len;
	uint16_t lmsg;
	
	//scelgo il file da aprire
	if(!strcmp(cmd[1],"0")){
		sprintf(temp, "%s_giocate_passate.txt", logged_user);
		printf("Mostro le giocate passate di %s\n", logged_user);
	}
	else if(!strcmp(cmd[1],"1")){
		sprintf(temp, "%s_giocate_attive.txt", logged_user);
		printf("Mostro le giocate attive di %s\n", logged_user);
	}
	else{
		return -2;
	}
	
	f = fopen(temp, "r");
	
	if (!f){
		return -1;
	}
	
	fd = fileno(f);
	
	strcpy(buffer, "msg_start");
	len = strlen(buffer)+1;
	lmsg = htons(len);
	// invio la dimensione del messaggio
	ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
	//invio il messaggio
	ret = send(new_sd, (void*)buffer, len, 0);
	
	//assicuro la mutua esclusione
	flock(fd, LOCK_EX);
	
	while (fgets(buffer, LINE_SIZE, f)){
		len = strlen(buffer)+1;
		lmsg = htons(len);
		// invio la dimensione del messaggio
		ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
		//invio il messaggio
		ret = send(new_sd, (void*)buffer, len, 0);
		
		if (ret < 0){
			perror("Errore in fase di invio");
			exit(-1);
		}
	}
	
	fclose(f);
	//rilascio il lock sul file
	flock(fd, LOCK_UN);
	
	strcpy(buffer, "msg_end");
	
	return 1;
}

void vedi_vincite(char* buffer, int new_sd, char* logged_user){
	char temp[LINE_SIZE];
	FILE* f;
	int fd, ret, len, i;
	uint16_t lmsg;
	float vincite[5];
	char* token;
	int write = 0;
	char delim[] = "\n, ";
	
	for (i = 0; i < 5; i++){
		vincite[i] = 0;
	}
	
	//scelgo il file da aprire
	sprintf(temp, "%s_vincite.txt", logged_user);
	f = fopen(temp, "r");
	
	if (!f){
		return;
	}
	
	fd = fileno(f);
	
	printf("Mostro le vincite di %s\n", logged_user);
	
	strcpy(buffer, "msg_start");
	len = strlen(buffer)+1;
	lmsg = htons(len);
	// invio la dimensione del messaggio
	ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
	//invio il messaggio
	ret = send(new_sd, (void*)buffer, len, 0);
	
	flock(fd, LOCK_EX);
	
	while (fgets(buffer, LINE_SIZE, f)){
		len = strlen(buffer)+1;
		lmsg = htons(len);
		// invio la dimensione del messaggio
		ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
		//invio il messaggio
		ret = send(new_sd, (void*)buffer, len, 0);
		
		if (ret < 0){
			perror("Errore in fase di invio");
			exit(-1);
		}
		//parsing della riga
		token = strtok(buffer, delim);
		
		write = 0;
		do{
			//parsing della riga
			token = strtok(NULL, delim);
			
			if (write == 1 && i <= 4 && i >= 0){
				vincite[i] += atof(token);
				write = 0;
			}
			
			if (!token)
				break;
			
			if (!strcasecmp(token, "Estratto")){
				i = 0;
				write = 1;
			}
			else if (!strcasecmp(token, "Ambo")){
				i = 1;
				write = 1;
			}
			else if (!strcasecmp(token, "Terno")){
				i = 2;
				write = 1;
			}
			else if (!strcasecmp(token, "Quaterna")){
				i = 3;
				write = 1;
			}
			else if (!strcasecmp(token, "Cinquina")){
				i = 4;
				write = 1;
			}
			else{
				write = 0;
			}
			
		}while(token != NULL);
		
	}
	
	sprintf(buffer, "\nVincite su ESTRATTO: %.2f\nVincite su AMBO: %.2f\nVincite su TERNO: %.2f\nVincite su QUATERNA: %.2f\nVincite su CINQUINA: %.2f\n", 
			vincite[0],
			vincite[1],
			vincite[2],
			vincite[3],
			vincite[4]);
	
	len = strlen(buffer)+1;
	lmsg = htons(len);
	// invio la dimensione del messaggio
	ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
	//invio il messaggio
	ret = send(new_sd, (void*)buffer, len, 0);
	
	if (ret < 0){
		perror("Errore in fase di invio");
		exit(-1);
	}
	
	fclose(f);
	flock(fd, LOCK_UN);
	
	strcpy(buffer, "msg_end");
}

int fattoriale(int n){
	if (n == 0)
		return 1;
	return n*fattoriale(n-1);
}

int combinazioni_semplici(int n, int k){
	if (k <= n)
		return fattoriale(n)/(fattoriale(k)*fattoriale(n-k));
	else
		return 0;
}

void imposta_vincite(int* ruote_g, int* numeri_g, float* importi, int* ultima_estrazione, char* nome_utente, struct tm tm){
	int i, j, k, n_ruote = 0, giocati = 0, usciti_ruota = 0, estratti = 0, ambi = 0, terne = 0, quaterne = 0, cinquine = 0;
	int e_possibili = 0, a_possibili = 0, t_possibili = 0, q_possibili = 0, c_possibili = 0; 
	double vin_e = 0, vin_a = 0, vin_t = 0, vin_q = 0, vin_c = 0;
	FILE* f;
	int fd;
	char nome_file[LINE_SIZE] = "";
	char buffer[LINE_SIZE] = "";
	char temp[LINE_SIZE] = "";
	char* ruote[11] = {
						"Bari",
						"Cagliari", 
						"Firenze", 
						"Genova", 
						"Milano",
						"Napoli",
						"Palermo",
						"Roma",
						"Torino",
						"Venezia",
						"Nazionale"
						};
						
	for (i = 0; i < 11; i++){
		if (ruote_g[i] != 0){
			n_ruote++;
			strcat(buffer, ruote[i]);
			strcat(buffer, " ");
		}
	}
	
	if (n_ruote == 11){
		strcpy(buffer, "Tutte ");
	}
	
	for (i = 0; i < 10; i++){
		if (numeri_g[i] != 0){
			giocati++;
			sprintf(temp, "%d", numeri_g[i]);
			strcat(buffer, temp);
			strcat(buffer, " ");
		}
	}
	
	for (i = 0; i < 11; i++){
		if (ruote_g[i]){
			for (j = 0; j < 10; j++){
				for (k = 0; k < 5; k++){
					if (numeri_g[j] == ultima_estrazione[i*5+k]){
						usciti_ruota++;
					}
				}
			}
		}
		estratti += usciti_ruota;
		ambi += combinazioni_semplici(usciti_ruota, 2);
		terne += combinazioni_semplici(usciti_ruota, 3);
		quaterne += combinazioni_semplici(usciti_ruota, 4);
		cinquine += (usciti_ruota == 5);
		usciti_ruota = 0;
	}
	e_possibili = combinazioni_semplici(giocati, 1);
	a_possibili = combinazioni_semplici(giocati, 2);
	t_possibili = combinazioni_semplici(giocati, 3);
	q_possibili = combinazioni_semplici(giocati, 4);
	c_possibili = combinazioni_semplici(giocati, 5);
	
	if (e_possibili)
		vin_e = estratti*importi[0]*11.23/(n_ruote*e_possibili);
	if (a_possibili)
		vin_a = ambi*importi[1]*250/(n_ruote*a_possibili);
	if (t_possibili)
		vin_t = terne*importi[2]*4500/(n_ruote*t_possibili);
	if (q_possibili)
		vin_q = quaterne*importi[3]*120000/(n_ruote*q_possibili);
	if (c_possibili)
		vin_c = cinquine*importi[4]*6000000/(n_ruote*c_possibili);
	
	strcpy(nome_file, nome_utente);
	strcat(nome_file, "_vincite.txt");
	
	f = fopen(nome_file, "a+");
	fd = fileno(f);
	flock(fd, LOCK_EX);
	
	fprintf(f, "Estrazione del %02d-%02d-%d ore %02d:%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
	
	strcat(buffer, ">> ");
	
	if (importi[0] != 0){
		sprintf(temp, "Estratto %.2f ", vin_e);
		strcat(buffer, temp);
	}

	if (importi[1] != 0){
		sprintf(temp, "Ambo %.2f ", vin_a);
		strcat(buffer, temp);
	}

	if (importi[2] != 0){
		sprintf(temp, "Terno %.2f ", vin_t);
		strcat(buffer, temp);
	}

	if (importi[3] != 0){
		sprintf(temp, "Quaterna %.2f ", vin_q);
		strcat(buffer, temp);
	}

	if (importi[4] != 0){
		sprintf(temp, "Cinquina %.2f ", vin_c);
		strcat(buffer, temp);
	}
	
	strcat(buffer, "\n");
	fprintf(f, "%s", buffer);

	flock(fd, LOCK_UN);
	fclose(f);
}

int ruota_valida(char* ruota){
	if (
		!strcasecmp(ruota,"bari") ||
		!strcasecmp(ruota,"cagliari") ||
		!strcasecmp(ruota,"firenze") ||
		!strcasecmp(ruota,"genova") ||
		!strcasecmp(ruota,"milano") ||
		!strcasecmp(ruota,"napoli") ||
		!strcasecmp(ruota,"palermo") ||
		!strcasecmp(ruota,"roma") ||
		!strcasecmp(ruota,"torino") ||
		!strcasecmp(ruota,"venezia") ||
		!strcasecmp(ruota,"nazionale")
		)
		return 1;
	else
		return 0;
}

int vedi_estrazione(char* cmd[], char* buffer, int new_sd){
	FILE* f;
	int fd, i, ret = 0, lines = atoi(cmd[1]), len;
	uint16_t lmsg;
	char** extraction = (char**)malloc(lines*sizeof(char*));
	char line_w[LINE_SIZE];
	char line_t[LINE_SIZE];
	char* token;
	char delim[] = "\n, ";
	
	//controllo il formato del comando
	if (cmd[3] && !ruota_valida(cmd[2])){
		ret = -2;
		free(extraction);
		return ret;
	}else if (cmd[4] != NULL || cmd[2] == NULL){
		ret = -3;		
		free(extraction);
		return ret;
	}else if (lines < 1){
		ret = -4;
		free(extraction);
		return ret;
	}
	
	if (cmd[3]){ 
		for (i = 0; i < lines; i++){
			extraction[i] = (char*)malloc(LINE_SIZE*sizeof(char));	//alloco i buffer su cui salvare le righe
			strcpy(extraction[i], "");
			
		}
		printf("Mostro le %d estrazioni più recenti su %s\n", atoi(cmd[1]), cmd[2]);
	}else{
		free(extraction);
		extraction = (char**)malloc(lines*EXTRACTION_LINES*sizeof(char*));
		for (i = 0; i < lines*EXTRACTION_LINES; i++){
			extraction[i] = (char*)malloc(LINE_SIZE*sizeof(char));	//alloco i buffer su cui salvare le righe
			strcpy(extraction[i], "");
		}
		printf("Mostro le %d estrazioni più recenti su tutte le ruote\n", atoi(cmd[1]));
	}
	
	
	
	f = fopen("estrazioni.txt", "r");
	if (f){
		fd = fileno(f);
		flock(fd, LOCK_EX);
		
		while(fgets(line_w, LINE_SIZE, f)){
			strcpy(line_t, line_w);
			token = strtok(line_w, delim);
			if (!cmd[3] || !strcasecmp(token, cmd[2])){
				for (i = 0; i < lines*(1 + (cmd[3] == NULL)*(EXTRACTION_LINES-1))-1; i++){
					strcpy(extraction[i], extraction[i+1]);
				}
				strcpy(extraction[lines*(1 + (cmd[3] == NULL)*(EXTRACTION_LINES-1))-1], line_t);
			}
		}
		
		flock(fd, LOCK_UN);
		fclose(f);
		
		strcpy(buffer, "msg_start");
		len = strlen(buffer)+1;
		lmsg = htons(len);
		// invio la dimensione del messaggio
		ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
		//invio il messaggio
		ret = send(new_sd, (void*)buffer, len, 0);
		
		if (ret < 0){
			perror("Errore in fase di invio");
			exit(-1);
		}

		for (i = 0; i < lines*(1 + (cmd[3] == NULL)*(EXTRACTION_LINES-1)); i++){
			strcpy(buffer, extraction[i]);
			if (!strcmp(buffer,""))
				continue;
			len = strlen(buffer)+1;
			lmsg = htons(len);
			// invio la dimensione del messaggio
			ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
			//invio il messaggio
			ret = send(new_sd, (void*)buffer, len, 0);
			
			if (ret < 0){
				perror("Errore in fase di invio");
				exit(-1);
			}
		}
	}else{
		ret = -1;
	}
	
	for (i = 0; i < lines*(1 + (cmd[3] == NULL)*(EXTRACTION_LINES-1)); i++){
		free(extraction[i]);
	}
	
	strcpy(buffer, "msg_end");
	
	free(extraction);
	return ret;
}

int registra_giocata(char* cmd[], char* logged_user){
	FILE* f;
	int fd, i, j, val_start;
	char str[LINE_SIZE] = "";
	char filename[FILENAME_SIZE] = "default.txt";
	char line[LINE_SIZE] = "";
	
	strcpy(filename, logged_user);
	strcat(filename, "_giocate_attive.txt");
	
	if (strcmp(cmd[1], "-r")){
		return -1;
	}
		
	for (i = 2; cmd[i] && strcmp(cmd[i], "-n"); i++){
		if (!(ruota_valida(cmd[i]) || !strcasecmp(cmd[i], "tutte"))){
			return -1;
		}
		strcpy(str,cmd[i]);
		str[0] = toupper(str[0]);
		for (j = 1; str[j] && str[j] != '\0'; j++){
			str[j] = tolower(str[j]);
		}
		if (!strcasecmp(cmd[i], "tutte") && (i != 2 || (cmd[i+1] && strcmp(cmd[i+1], "-n")))){
			return -1;
		}
		
		strcat(line, str);
		strcat(line, " ");
	}
	
	val_start = i;
	
	for (i++; cmd[i] && strcmp(cmd[i], "-i"); i++){
		if (1 <= atoi(cmd[i]) && atoi(cmd[i]) <= 90){
			strcat(line, cmd[i]);
			strcat(line, " ");
		}else{
			return -1;
		}
	}
	
	if (i-val_start > 11){
		return -1;
	}
	
	val_start = i;
	
	for (i++; cmd[i] && cmd[i+1] && (i-val_start <= 5); i++){
		if (!atof(cmd[i])){
			continue;
		}
		strcat(line, "* ");
		strcat(line, cmd[i]);
		strcat(line, " ");
		strcat(line, 
			   (i == val_start+1) ? "estratto" :
			   (i == val_start+2) ? "ambo" :
			   (i == val_start+3) ? "terno" :
			   (i == val_start+4) ? "quaterna" :
			   (i == val_start+5) ? "cinquina" : ""
			  );
		strcat(line, " ");
	}
	
	if (val_start-i > 5){
		return -1;
	}
	
	f = fopen(filename, "a+");
	fd = fileno(f);
	flock(fd, LOCK_EX);
		
	printf("Registrazione giocata di %s: %s\n", logged_user, line);
	fprintf(f, "%s\n", line);
		
	flock(fd,LOCK_UN);
	fclose(f);
	
	return 1;
}

void shuffle(int* array, int dim){
	int k, i, temp;
	for (i = dim-1; i > 0; i--){
		k = rand() % (i+1);
		temp = array[i];
		array[i] = array[k];
		array[k] = temp;
	}
}

void estrazione(int* num_array){
	FILE* f, *f2, *f3, *f4;
	int fd, i, j, k, fd2, fd3, fd4;
	int random_num_ord[NUM_POOL_SIZE];
	char line[LINE_SIZE];
	char* token;
	char delim[] = "\n, ";
	char nome_file[LINE_SIZE] = "";
	char nome_utente[NAME_SIZE] = "";
	char file_g_passate[LINE_SIZE] = "";
	char temp_buffer[LINE_SIZE] = "";
	int extr_num[55];
	int ruote_giocate[11] = {0,0,0,0,0,0,0,0,0,0,0};
	int numeri_giocati[10] = {0,0,0,0,0,0,0,0,0,0};
	float importi[5] = {0,0,0,0,0};
	float temp = -1;
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char* ruote[11] = {
						"Bari",
						"Cagliari", 
						"Firenze", 
						"Genova", 
						"Milano",
						"Napoli",
						"Palermo",
						"Roma",
						"Torino",
						"Venezia",
						"Nazionale"
						};
	
	for (i = 0; i < NUM_POOL_SIZE; i++){
		random_num_ord[i] = num_array[i];
	}
	
	printf("Nuova estrazione\n");
	
	f = fopen("estrazioni.txt", "a+");
	fd = fileno(f);
	flock(fd, LOCK_EX);

	fprintf(f, "Estrazione del %02d-%02d-%d ore %02d:%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
	
	for (i = 0; i < 11; i++){
		shuffle(random_num_ord, NUM_POOL_SIZE);
		fprintf(
				f,
				"%-9s %2d   %2d   %2d   %2d   %2d\n", 
				ruote[i],
				random_num_ord[0],
				random_num_ord[1],
				random_num_ord[2],
				random_num_ord[3],
				random_num_ord[4]
		);
		extr_num[i*5] = random_num_ord[0];
		extr_num[i*5+1] = random_num_ord[1];
		extr_num[i*5+2] = random_num_ord[2];
		extr_num[i*5+3] = random_num_ord[3];
		extr_num[i*5+4] = random_num_ord[4];
	}
		
	flock(fd,LOCK_UN);
	fclose(f);
					
	f2 = fopen("utenti.txt", "r");
	if (!f2)
		return;
	fd2 = fileno(f2);
	flock(fd2, LOCK_EX);
	
	while (fgets(line, LINE_SIZE, f2)){
		token = strtok(line, delim);
		
		strcpy(nome_file,token);
		strcpy(nome_utente,token);
		strcat(nome_file, "_giocate_attive.txt");
		
		f3 = fopen(nome_file, "r");
		if (!f3)
			continue;
		fd3 = fileno(f3);
		flock(fd3, LOCK_EX);

		while (fgets(line, LINE_SIZE, f3)){
			strcpy(temp_buffer, line);
			temp = -1;
			k = 0;
			token = strtok(line, delim);
			do {
				if (ruota_valida(token) || !strcasecmp(token, "tutte")){
					for (j = 0; j < 11; j++){
						if (!strcasecmp(token, ruote[j]) || !strcasecmp(token, "tutte")){
							ruote_giocate[j] = 1;
						}
					}
				}else{
					if (k < 10 && strcmp(token,"*")){
						numeri_giocati[k] = atoi(token);
						k++;
					}else{
						k = 10;
						if (temp != -1){
							if (!strcasecmp(token, "estratto")){
								importi[0] = temp;
								temp = -1;
							}else if (!strcasecmp(token, "ambo")){
								importi[1] = temp;
								temp = -1;
							}else if (!strcasecmp(token, "terno")){
								importi[2] = temp;
								temp = -1;
							}else if (!strcasecmp(token, "quaterna")){
								importi[3] = temp;
								temp = -1;
							}else if (!strcasecmp(token, "cinquina")){
								importi[4] = temp;
								temp = -1;
							}
						}else{
							if (!strcmp(token,"*") ||
								!strcasecmp(token, "estratto") ||
								!strcasecmp(token, "ambo") ||
								!strcasecmp(token, "terno") ||
								!strcasecmp(token, "quaterna") ||
								!strcasecmp(token, "cinquina")
							){
								;
							}else{
								temp = atof(token);
							}
						}
					}
				}
				token = strtok(NULL, delim);
			}while (token != NULL);
			
			imposta_vincite(ruote_giocate,numeri_giocati,importi,extr_num,nome_utente, tm);
			
			for (i = 0; i < 11; i++){
				ruote_giocate[i] = 0;
			}
			
			for (i = 0; i < 10; i++){
				numeri_giocati[i] = 0;
			}
			
			for (i = 0; i < 5; i++){
				importi[i] = 0;
			}
			
			strcpy(file_g_passate,nome_utente);
			strcat(file_g_passate, "_giocate_passate.txt");
			
			f4 = fopen(file_g_passate, "a+");
			fd4 = fileno(f4);
			flock(fd4, LOCK_EX);
			
			fprintf(f4, "%s\n", temp_buffer);
			
			fclose(f4);
			flock(fd4, LOCK_UN);
		}
		
		fclose(f3);
		f3 = fopen(nome_file, "w");
		fclose(f3);
		flock(fd3, LOCK_UN);

		}
	fclose(f2);
	flock(fd2, LOCK_UN);	
}

int signup(char* cmd[]){
	FILE* f;
	int fd, ret = 0;
	char str[LINE_SIZE];
	char* token;
	char delim[] = "\n, ";
	
	if (cmd[1] != NULL && cmd[2] != NULL && cmd[3] == NULL){
		f = fopen("utenti.txt", "r");
		if (f){
			fd = fileno(f);
			flock(fd, LOCK_EX);
			
			while(fgets(str, LINE_SIZE, f)){
				token = strtok(str, delim);
				if (!strcmp(token, cmd[1])){
					ret = -1;
				}
			}
			
			flock(fd,LOCK_UN);
			fclose(f);
			if (ret < 0){
				return ret;
			}
		}
		f = fopen("utenti.txt", "a+");
		flock(fd, LOCK_EX);
		
		fprintf(f, "%s %s\n", cmd[1], cmd[2]);
		
		flock(fd,LOCK_UN);
		fclose(f);
		
		printf("Registrazione di %s effettuata con successo!\n", cmd[1]);
		
		return 0;
	}else{
		return -2;
	}
}

int login(char* cmd[]){
	FILE* f;
	int fd, ret = -1;
	char str[LINE_SIZE];
	char* token;
	char delim[] = "\n, ";
	
	if (cmd[1] != NULL && cmd[2] != NULL && cmd[3] == NULL){ // controllo il formato dell'istruzione
		f = fopen("utenti.txt", "r");
		if (f){
			fd = fileno(f);
			flock(fd, LOCK_EX);
			
			while(fgets(str, LINE_SIZE, f)){
				token = strtok(str, delim);
				if (!strcmp(token, cmd[1])){
					break;
				}
			}
			
			if (!strcmp(token, cmd[1])){
				token = strtok(NULL, delim);
				if (!strcmp(token, cmd[2])){
					ret = 0;
				}
			}
			
			flock(fd,LOCK_UN);
			fclose(f);

		}
		
	}else{
		ret = -2;
	}
	return ret;
}

void esegui_comando(char* cmd[], char* session_id, char* buffer, int new_sd, char* logged_user, int* tentativi){
	int ret, i, k;
	char temp[ARG_SIZE];
	
	for (k = 0; cmd[k] && cmd[k+1]; k++){
		;
	}
	
	if (!strcmp(cmd[0], "!signup")){ 
		// registro l'utente
		ret = signup(cmd); 
		if (ret < 0){
			if (ret == -1){
				strcpy(buffer, "Nome utente già registrato: inserire un nuovo nome utente e una password");
				return;
			}else if (ret == -2){
				strcpy(buffer, "Numero di argomenti errato");
				return;
			}
		}else{
			strcpy(buffer, "msg_signup_success");
			return;
		}
	}
	else if (!strcmp(cmd[0],"!login")){
		// effettuo l'accesso
		ret = login(cmd);
		if (ret < 0){
			if (ret == -1){
				(*tentativi)--;
				if(*tentativi)
					sprintf(buffer, "Nome utente o password errati: tentativi rimanenti: %d", *tentativi);
				else
					strcpy(buffer, "msg_blocked");
				return;
			}else if (ret == -2){
				strcpy(buffer, "Numero di argomenti errato");
				return;
			}
		}else{
			(*tentativi) = 3;
			char alphanum[CHAR_POOL_SIZE];
			
			strcpy(temp, cmd[1]);
			strcpy(buffer, "Accesso effettuato come ");
			strcat(buffer, temp);
			strcat(buffer, ", session ID: &");
			
			for (i = 0; i < CHAR_POOL_SIZE; i++){
				alphanum[i] = (i < 10) ? 48+i : 55+i;
			}
			
			for (i = 0; i < SID_SIZE-1; i++){
				session_id[i] = alphanum[rand() % 36];
			}
			
			session_id[SID_SIZE] = '\0';
			
			strcat(buffer, session_id);
			strcpy(logged_user, temp);
			
			printf("Inizio sessione-> utente: %s, ID: %s\n", temp, session_id);
			return;
		}
	}
	else if (!strcmp(cmd[0],"!esci")){
		if ((cmd[1] && !cmd[2] && strcmp(session_id,"NONE")) || (!strcmp(session_id,"NONE") && !cmd[1])){
			strcpy(buffer, "msg_exit");
			printf("Invio terminazione sessione %s \n", session_id);
			return;
		}
	}
	else if (!strcmp(session_id,"NONE")){
		strcpy(buffer, "Operazione non valida: effettuare l'accesso");
		return;
	}
	else if (strcmp(session_id, cmd[k])){
		strcpy(buffer, "Errore: l'ID di sessione non corrisponde"); 
	}
	
	if (!strcmp(cmd[0], "!invia_giocata")){
		// invio la schedina
		ret = registra_giocata(cmd, logged_user);
		if (ret < 0){
			strcpy(buffer, "Schedina non valida");
		}else{
			strcpy(buffer, "Schedina registrata con successo");
		}
		return;
	}
	else if (!strcmp(cmd[0],"!vedi_giocate")){
		// mostro le giocate
		ret = vedi_giocate(cmd, buffer, new_sd, logged_user);
		if (ret == -1){
			strcpy(buffer, "Nessuna giocata da visualizzare");
		}
		else if (ret == -2){
			strcpy(buffer, "Argomento errato");
		}
		return;
	}
	else if (!strcmp(cmd[0],"!vedi_estrazione")){
		// mostro le estrazioni
		ret = vedi_estrazione(cmd, buffer, new_sd);
		if (ret == -1){
			strcpy(buffer, "Errore nell'apertura del file: Impossibile visualizzare le estrazioni");
		}
		else if (ret == -2){
			strcpy(buffer, "Ruota non valida");
		}
		else if (ret == -3){
			strcpy(buffer, "Numero di argomenti errato");
		}
		else if (ret == -4){
			strcpy(buffer, "Errore: Selezionare un numero di estrazioni maggiore o uguale a 1");
		}
		return;
	}
	else if (!strcmp(cmd[0],"!vedi_vincite")){
		// mostro le vincite
		vedi_vincite(buffer, new_sd, logged_user);
		return;
	}
	
	strcpy(buffer,"Comando non riconosciuto");
}

int main(int argc, char* argv[]){
	int ret, sd, new_sd, len, i, interval_mins;
	pid_t pid, pid_ext;
	uint16_t lmsg;
	struct sockaddr_in my_addr, cl_addr;
	char buffer[BUFFER_SIZE];
	char* token;
	char* cmd[CMD_SIZE];
	char delim[] = "\n, ";
	char session_id[SID_SIZE] = "NONE";
	int numbers[NUM_POOL_SIZE];
	char logged_user[NAME_SIZE] = "";
	int tentativi = 3;
	char ipClient[20];
	FILE* f;
	int fd;
	time_t t;
	struct tm tm;
	struct tm tm2;
	char temp_buffer[BUFFER_SIZE];
	
	
	if (argc != 2 && argc != 3){
		printf("Errore: argomenti richiesti: 1, opzionali: 1, forniti: %d\n", argc-1);
		exit(-1);
	}
	
	interval_mins = (argc == 3) ? atoi(argv[2]) : INTERVAL_DEFAULT;
	
	printf("\nAvvio server-> Porta: %s, Intervallo estrazioni: %d minuti\n", argv[1], interval_mins);
	
	srand(time(0));
	
	for (i = 0; i < NUM_POOL_SIZE; i++){
		numbers[i] = i+1;
	}
	
	pid_ext = fork();
	
	if (pid_ext < 0){
		perror("Fork fallita");
		exit(-1);
	}
	
	if (pid_ext == 0){
		// sono nel processo figlio, che ha il compito di estrarre i numeri periodicamente
		while(1){
			sleep(interval_mins*60);
			estrazione(numbers);
		}
	}
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	printf("\nSocket di ascolto creato\n");
	
	memset(&my_addr, 0, sizeof(my_addr)); // pulizia
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(argv[1]));
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
	ret = bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr));
	
	if (ret < 0){
		perror("Errore in fase di bind");
		exit(-1);
	}

	ret = listen(sd,10);
	if (ret < 0){
		perror("Errore in fase di listen");
		exit(-1);
	}
	
	printf("Socket in attesa di richieste di connessione \n");
	
	while(1){
		len = sizeof(cl_addr);
		new_sd = accept(sd, (struct sockaddr*)&cl_addr, (socklen_t*)&len);
		inet_ntop(AF_INET,&cl_addr.sin_addr.s_addr,ipClient,sizeof(ipClient));
		
		pid = fork();
		
		if (pid < 0){
			perror("Fork fallita");
			continue;
		}
		
		if (pid == 0){
			// sono nel processo figlio
			close(sd);
			
			srand(time(0));
			
			while(1){
				printf("In attesa di un comando dal client\n");
				
				ret = recv(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
				len = ntohs(lmsg);
				ret = recv(new_sd,(void*)buffer, len, 0);
				
				if (ret < 0){
					perror("Errore in fase di ricezione");
					continue;
				}
				
				printf("Comando ricevuto dal client\n");
				
				for (i = 0; i < CMD_SIZE; i++){
					cmd[i] = NULL;
				}
				
				token = strtok(buffer, delim);
				
				i = 0;
				
				do{
					cmd[i] = token;
					token = strtok(NULL, delim);
					i++;
				}while(token != NULL && i < CMD_SIZE);
				
				t = time(NULL);
				tm = *localtime(&t);
				sprintf(temp_buffer, "%s_ipBloccati.txt", ipClient);
				f = fopen(temp_buffer, "r");
				
				if (!f){
					f = fopen(temp_buffer, "w+");
				}
				
				fd = fileno(f);
				flock(fd, LOCK_EX);

				if (fgets(temp_buffer, LINE_SIZE, f)){
					strptime(temp_buffer, "%d-%m-%Y %H:%M:%S", &tm2);
					tm2.tm_isdst = -1;
					tm.tm_isdst = -1;
					if (difftime(mktime(&tm), mktime(&tm2)) < 1800){
						printf("Questo ip è stato bloccato per 30 minuti\n");
						strcpy(buffer, "msg_blocked");
						len = strlen(buffer)+1;
						lmsg = htons(len);
						// invio la dimensione del messaggio
						ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
						//invio il messaggio
						ret = send(new_sd, (void*)buffer, len, 0);
						
						if (ret < 0){
							perror("Errore in fase di invio");
							exit(0);
						}
						fclose(f);
						flock(fd, LOCK_UN);
						close(new_sd);
						exit(0);
					}
					
					
				}
				fclose(f);
				flock(fd, LOCK_UN);
				
				esegui_comando(cmd, session_id, buffer, new_sd, logged_user, &tentativi);
				
				len = strlen(buffer)+1;
				lmsg = htons(len);
				// invio la dimensione del messaggio
				ret = send(new_sd, (void*)&lmsg, sizeof(uint16_t), 0);
				//invio il messaggio
				ret = send(new_sd, (void*)buffer, len, 0);
				
				if (ret < 0){
					perror("Errore in fase di invio");
					continue;
				}
				
				if (!tentativi){
					t = time(NULL);
					tm = *localtime(&t);
					sprintf(buffer, "%s_ipBloccati.txt", ipClient);
					f = fopen(buffer, "w");
					fd = fileno(f);
					flock(fd, LOCK_EX);
					
					fprintf(f, "%02d-%02d-%d %02d:%02d:%02d\n",
							tm.tm_mday,
							tm.tm_mon + 1,
							tm.tm_year + 1900,
							tm.tm_hour,
							tm.tm_min,
							tm.tm_sec);
					
					fclose(f);
					flock(fd, LOCK_UN);
					
					close(new_sd);
					printf("Troppi tentativi di accesso: chiusura forzata sessione\n");
					printf("Sessione %s terminata\n", session_id);
					exit(0);
				}
				
				if (!strcmp(buffer,"msg_exit")){
					close(new_sd);
					printf("Sessione %s terminata\n", session_id);
					exit(0);
				}
			}
		}else{
			// sono nel processo padre
			close(new_sd);
		}
	}
}