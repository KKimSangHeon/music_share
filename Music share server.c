#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 30
#define MAX_CLNT 256

#define REGISTRATION 1
#define LOGIN 2
#define FIND 3

#define QUIT 10

#define DUPLICATION 20
#define NOT_DUPLICATION 21

#define ERROR 99
int identifyOpcode(char * str);
void writeClientData();
void readClientData();
void writeMusiclistData();
void readMusiclistData();
void * handle_clnt(void * arg);
void error_handling(char * msg);
void readData(void * arg, char *str);
void processRegistration(void * arg);
void processLogin(void * arg);
void processLogin2(void * arg);
void processFind(void * arg);
void processDownMusic(void * arg);
void processUploadMusic(void * arg);
int checkIDDuplication(char * ID);	//ID Duplication Checking Function
long GetFileSize(char * FileName);
int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int registedCustomer = 0;
int numOfMusicFile = 0;
int duplicationIndex;

pthread_mutex_t mutx;

typedef struct customerInfo
{
	char ID[50];
	char E_MAIL[100];
	char PW[50];
}customer_Info;

typedef struct filename
{
	char mp3file[100];
}file_Name;
static customer_Info CUSTOMER_INFO[256];
static file_Name FILE_NAME[256];

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("[Noti] Connected client IP : %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}
/*
* name : identifyOpcode(char * str)
* function : analysis opcode
*/
int identifyOpcode(char * str)
{
	if (!(strcmp(str, "reg")))
		return REGISTRATION;
	else if (!(strcmp(str, "log")))
		return LOGIN;
	else if (!(strcmp(str, "find")))
		return FIND;
	else if (!(strcmp(str, "quit")))
		return QUIT;

	else
		return ERROR;

}
/*
* name : checkIDDuplication(char * ID)
* function : Check Duplicate input and CUSTOMER_INFO array
*/
int checkIDDuplication(char * ID)
{
	int i = 0;

	while (i < registedCustomer)
	{
		if (!strcmp(CUSTOMER_INFO[i].ID, ID))
		{
			duplicationIndex = i;
			return DUPLICATION;	//ID Duplication
		}
		i++;
	}

	return NOT_DUPLICATION;	//ID not Duplication

}
/*
* name : readData(void * arg,char *str)
* function : receive opcode from client, and remove '!'
*/
void readData(void * arg, char *str)
{
	int recv_len = 0, recv_cnt = 0;
	int clnt_sock = *((int*)arg);

	recv_len = 0;
	while (str[recv_len - 1] != '!')	// receive until get '!' 
	{
		recv_cnt = read(clnt_sock, &str[recv_len], sizeof(str));
		recv_len += recv_cnt;
	}
	str[recv_len - 1] = 0;	//remove '!' in received string


}
/*
* name : readMusiclistData()
* function : read client data from 'MusiclistDat' file
*/
void readMusiclistData()
{
	FILE * fp = fopen("MusiclistDat", "r");
	if (fp == NULL)
		return;

	fread(&numOfMusicFile, sizeof(int), 1, fp);
	fread(FILE_NAME, sizeof(file_Name), numOfMusicFile, fp);

	printf("[Noti] Finished loading %d Client Data from server\n", numOfMusicFile);

	fclose(fp);
}
/*
* name : writeMusiclistData()
* function : write client data from 'MusiclistDat' file
*/
void writeMusiclistData()
{
	FILE * fp = fopen("MusiclistDat", "w");
	int i;


	if (fp == NULL)
		printf("[Noti] Music list Data Write Error\n");
	else
	{

		fwrite(&numOfMusicFile, sizeof(int), 1, fp);

		fwrite(FILE_NAME, sizeof(file_Name), numOfMusicFile, fp);


		printf("[Noti] Finished Writing Client Data to server\n");
	}

	fclose(fp);
}
/*
* name : readClientData()
* function : read client data from 'ClientDat' file
*/
void readClientData()
{
	FILE * fp = fopen("ClientDat", "r");
	if (fp == NULL)
		return;

	fread(&registedCustomer, sizeof(int), 1, fp);
	fread(CUSTOMER_INFO, sizeof(customer_Info), registedCustomer, fp);
	printf("[Noti] Finished Reading %d Client Data from server\n", registedCustomer);
	fclose(fp);
}
/*
* name : writeClientData()
* function : write client data from 'ClientDat' file
*/
void writeClientData()
{
	FILE * fp = fopen("ClientDat", "w");
	int i;

	if (fp == NULL)
		printf("[Noti] Client Data Write Error\n");
	else
	{

		fwrite(&registedCustomer, sizeof(int), 1, fp);

		fwrite(CUSTOMER_INFO, sizeof(customer_Info), registedCustomer, fp);
		printf("[Noti] Finished Writing Client Data to server\n");
	}

	fclose(fp);
}
/*
* name : processRegistration(void * arg)
* function : Process Registration
*/
void processRegistration(void * arg)
{
	int clnt_sock = *((int*)arg), duplicationCheck;
	char ID[50], E_MAIL[100], PW[50], state[30];

	readData((void*)&clnt_sock, ID);
	duplicationCheck = checkIDDuplication(ID);
	if (duplicationCheck == DUPLICATION)	//ID is Duplication
	{
		strcpy(state, "dup!");
		write(clnt_sock, state, strlen(state));

	}
	else if (duplicationCheck == NOT_DUPLICATION)
	{
		strcpy(state, "notdup!");
		write(clnt_sock, state, strlen(state));


		readData((void*)&clnt_sock, E_MAIL);
		readData((void*)&clnt_sock, PW);



		strcpy(CUSTOMER_INFO[registedCustomer].ID, ID);
		strcpy(CUSTOMER_INFO[registedCustomer].E_MAIL, E_MAIL);
		strcpy(CUSTOMER_INFO[registedCustomer].PW, PW);

		registedCustomer++;
		printf("[Noti] ID : %s registered!\n", ID);

		writeClientData();	//write client data to server

	}
	else
	{
		printf("[Noti] server error occurred\n");
	}

}
/*
* name : processLogin(void * arg)
* function : Process Login
*/
void processLogin(void * arg)
{
	int clnt_sock = *((int*)arg), duplicationCheck;
	char ID[50], E_MAIL[100], PW[50], state[30];



	readData((void*)&clnt_sock, ID);
	duplicationCheck = checkIDDuplication(ID);
	if (duplicationCheck == NOT_DUPLICATION)
	{
		strcpy(state, "logfail!");
		write(clnt_sock, state, strlen(state));

	}
	else if (duplicationCheck == DUPLICATION)
	{
		strcpy(state, "logok1!");
		write(clnt_sock, state, strlen(state));

		readData((void*)&clnt_sock, PW);

		if (!strcmp(CUSTOMER_INFO[duplicationIndex].PW, PW))
		{
			strcpy(state, "logok2!");
			write(clnt_sock, state, strlen(state));
			printf("[Noti] ID : %s login!\n", ID);
			processLogin2((void*)&clnt_sock);
		}
		else
		{
			strcpy(state, "logfail2!");
			write(clnt_sock, state, strlen(state));
		}
	}
	else
	{

		printf("login error occured\n");
	}

}
/*
* name :  processUploadMusic(void * arg)
* function : Process Upload mp3 file(send to client)
*/
void processUploadMusic(void * arg)
{
	int clnt_sock = *((int*)arg), read_cnt, readLength, fileLength;
	char FileName[100];
	char buf[BUF_SIZE];

	read(clnt_sock, FileName, sizeof(FileName));


	FILE * fp = fopen(FileName, "wb");


	readLength = 0;
	read(clnt_sock, &fileLength, sizeof(int));
	while (1)
	{
		read_cnt = read(clnt_sock, buf, BUF_SIZE);
		fwrite((void*)buf, 1, read_cnt, fp);
		readLength += read_cnt;

		if (readLength == fileLength)
			break;
	}
	fclose(fp);
	printf("[Noti] complete receiving %s file !\n", FileName);

	strcpy(FILE_NAME[numOfMusicFile++].mp3file, FileName);

	writeMusiclistData();
}
/*
* name :  processDownMusic(void * arg)
* function : Process Download mp3 file(send to client)
*/
void processDownMusic(void * arg)
{
	int clnt_sock = *((int*)arg), duplicationCheck, i, read_cnt, fileLength;
	char state[30], FileName[100], buf[BUF_SIZE];
	FILE *fp = NULL;
	FILE *fp2 = NULL;

	readData((void*)&clnt_sock, FileName);

	fp = fopen(FileName, "rb");

	if (fp == NULL)
	{
		strcpy(state, "nofile!");
		write(clnt_sock, state, strlen(state));
		fclose(fp);
		return;
	}
	else
	{
		fclose(fp);
		strcpy(state, "fileSend!");

		write(clnt_sock, state, strlen(state));

		fileLength = (int)(GetFileSize(FileName));

		fp = fopen(FileName, "rb");
		write(clnt_sock, &fileLength, sizeof(int));
		while (1)
		{
			read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
			if (read_cnt < BUF_SIZE)
			{
				write(clnt_sock, buf, read_cnt);
				break;
			}
			write(clnt_sock, buf, BUF_SIZE);

		}
		fclose(fp);
	}
}
/*
* name :  GetFileSize(char * FileName)
* function : Get file size
*/
long GetFileSize(char * FileName)
{
	FILE *fp = NULL;
	long fpos;
	long fsize;
	fp = fopen(FileName, "rb");
	fpos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, fpos, SEEK_SET);
	fclose(fp);
	return fsize;
}


/*
* name : processLogin2(void * arg)
* function : Process After Login
*/
void processLogin2(void * arg)
{
	int clnt_sock = *((int*)arg), duplicationCheck, i;
	char opcode[30], state[30], FileName[100];

	
	readData((void*)&clnt_sock, opcode);

	if (!strcmp(opcode, "list"))
	{

		readMusiclistData();

		write(clnt_sock, &numOfMusicFile, sizeof(int));

		for (i = 0; i < numOfMusicFile; i++)
		{
			strcpy(FileName, FILE_NAME[i].mp3file);
			strcat(FileName, "!");
			write(clnt_sock, FileName, strlen(FileName));
		}

	}
	else if (!strcmp(opcode, "down"))
	{
		processDownMusic((void*)&clnt_sock);
	}
	else if (!strcmp(opcode, "upload"))
	{
		processUploadMusic((void*)&clnt_sock);
	}
	else if (!strcmp(opcode, "quit"))
	{

		printf("list\n");
	}
	else
		printf("[Noti] login2 error occurred\n");

}
/*
* name : processFine(void * arg)
* function : Process Find
*/
void processFind(void * arg)
{
	int clnt_sock = *((int*)arg), duplicationCheck;
	char ID[50], E_MAIL[100], PW[50], state[30];

	readData((void*)&clnt_sock, ID);
	duplicationCheck = checkIDDuplication(ID);
	if (duplicationCheck == NOT_DUPLICATION)
	{
		strcpy(state, "findfail!");
		write(clnt_sock, state, strlen(state));

	}
	else if (duplicationCheck == DUPLICATION)
	{
		strcpy(state, "find!");
		write(clnt_sock, state, strlen(state));
		readData((void*)&clnt_sock, E_MAIL);

		if (!strcmp(CUSTOMER_INFO[duplicationIndex].E_MAIL, E_MAIL))
		{
			strcpy(state, "correct!");
			write(clnt_sock, state, strlen(state));

			strcpy(PW, CUSTOMER_INFO[duplicationIndex].PW);
			strcat(PW, "!");
			write(clnt_sock, PW, strlen(PW));
		}
		else
		{
			strcpy(state, "notcorrect!");
			write(clnt_sock, state, strlen(state));
		}
	}
	else
	{

		printf("find error occured\n");
	}

}
void * handle_clnt(void * arg)
{
	int clnt_sock = *((int*)arg), select = 0;
	int recv_len = 0, recv_cnt = 0, i, duplicationCheck;
	char opcode[30], ID[50], E_MAIL[100], PW[50], state[30];
	int temp;

	readClientData();	//write client data from server
	while (select != QUIT)
	{
		readData((void*)&clnt_sock, opcode);

		select = identifyOpcode(opcode);

		if (select == REGISTRATION)	//Registraion
		{
			processRegistration((void*)&clnt_sock);
		}
		else if (select == LOGIN)	//Login
		{
			processLogin((void*)&clnt_sock);
		}
		else if (select == FIND)	//find
		{
			processFind((void*)&clnt_sock);
		}
		else if (select == QUIT)
		{
			printf("[Noti] one Client is Disconnected\n");
		}
		else
		{
			printf("Error occurred !\n");
		}
	}

	pthread_mutex_lock(&mutx);
	for (i = 0; i < clnt_cnt; i++)   // remove disconnected client
	{
		if (clnt_sock == clnt_socks[i])
		{
			while (i++ < clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);

	return NULL;
}


void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
