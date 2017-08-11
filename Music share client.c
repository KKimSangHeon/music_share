#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 30
#define NAME_SIZE 20
#define INIT 0
#define REGISTRATION 1
#define LOGIN 2
#define FIND 3
#define QUIT 4
#define MUSICLIST 11
#define DOWNMUSIC 12
#define UPLOADMUSIC 13
#define QUIT2 14

void printInit();
void printAfterLogin();
void error_handling(char * msg);
void readData(void * arg, char *str);
void processRegistration(void * arg);
void processLogin(void * arg);
void processFind(void * arg);
void processLogin2(void * arg);
void processDownMusic(void * arg);
void processUploadMusic(void * arg);
long GetFileSize(char * FileName);
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
	int sock, select = INIT, recv_len = 0, recv_cnt = 0;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	char opcode[30], ID[50], E_MAIL[100], PW[50], state[30];

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");


	while (1)
	{
		if (select == INIT)
		{
			printInit();
			printf("please select : ");
			scanf("%d", &select);
			printf("\n");
			fflush(stdin);
		}
		else if (select == REGISTRATION)	//Registration
		{
			processRegistration((void*)&sock);
			select = INIT;	//sucess
		}
		else if (select == LOGIN)
		{
			processLogin((void*)&sock);
			select = INIT;	//sucess
		}
		else if (select == FIND)	//find
		{
			processFind((void*)&sock);
			select = INIT; //process sucess

		}
		else if (select == QUIT)
		{
			sprintf(opcode, "%s", "quit!");
			write(sock, opcode, strlen(opcode));
			printf("Thank you for using Music Share!\n");
			break;
		}
		else
		{
			printf("Please Input 1~4 !\n");
			select = INIT;	//process sucess

		}

	}

	close(sock);
	return 0;
}
/*
* name : processFind(void * arg)
* function : Process find
*/
void processFind(void * arg)
{
	int sock = *((int*)arg);
	char opcode[30], ID[50], E_MAIL[100], PW[50], state[30];

	printf("Find your password\n");
	sprintf(opcode, "%s", "find!");
	write(sock, opcode, strlen(opcode));

	printf("please Input ID : ");

	scanf("%s", ID);
	fflush(stdin);
	strcat(ID, "!");	// attach '!'
	write(sock, ID, strlen(ID));

	readData((void*)&sock, state);
	if (!strcmp(state, "findfail"))
	{
		printf("not exist ID!\n");

	}
	else if (!strcmp(state, "find"))
	{
		printf("Please Input E-MAIL : ");
		fflush(stdin);
		scanf("%s", E_MAIL);
		strcat(E_MAIL, "!");	// attach '!'
		write(sock, E_MAIL, strlen(E_MAIL));
		readData((void*)&sock, state);
		if (!strcmp(state, "correct"))
		{
			readData((void*)&sock, PW);
			printf("your PassWord is : %s\n", PW);
		}
		else if (!strcmp(state, "notcorrect"))
		{
			printf("E-Mail is not correct\n");

		}
		else
		{
			printf("error occurred\n");
		}
	}
	else
	{

		printf("Find error occurred");
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
* name :  processUploadMusic(void * arg)
* function : Process Upload mp3 file
*/
void processUploadMusic(void * arg)
{
	int sock = *((int*)arg), read_cnt, fileLength;
	char state[30], FileName[100], buf[BUF_SIZE];
	FILE * fp = NULL;
	printf("please Input mp3 file : ");
	fflush(stdin);
	scanf("%s", FileName);

	write(sock, FileName, sizeof(FileName));

	fp = fopen(FileName, "rb");
	if (fp == NULL)
	{
		printf("not exist file\n");
		return;

	}
	fileLength = (int)(GetFileSize(FileName));
	write(sock, &fileLength, sizeof(int));
	while (1)
	{
		read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
		if (read_cnt < BUF_SIZE)
		{
			write(sock, buf, read_cnt);
			break;
		}
		write(sock, buf, BUF_SIZE);

	}
	fclose(fp);
	printf("Fnish sending mp3 %s file\n", FileName);
}

/*
* name :  processDownMusic(void * arg)
* function : Process Download mp3 file
*/
void processDownMusic(void * arg)
{
	int sock = *((int*)arg), read_cnt, fileLength, readLength;
	char state[30], FileName[100], buf[BUF_SIZE];
	FILE * fp = NULL;

	printf("please Input mp3 file : ");
	fflush(stdin);
	scanf("%s", FileName);
	strcat(FileName, "!");

	write(sock, FileName, strlen(FileName));

	readData((void*)&sock, state);

	if (!strcmp(state, "nofile"))
	{
		printf("not exist file or file error \n");
	}
	else if (!strcmp(state, "fileSend"))
	{


		FileName[strlen(FileName) - 1] = 0;
		fp = fopen(FileName, "wb");

		read(sock, &fileLength, sizeof(int));
		readLength = 0;
		while (1)
		{
			read_cnt = read(sock, buf, BUF_SIZE);
			fwrite((void*)buf, 1, read_cnt, fp);
			readLength += read_cnt;
			if (readLength == fileLength)
				break;

		}
		fclose(fp);
		printf("complete receiving %s file !\n", FileName);
	}
	else
		printf("download error occured\n");

}
/*
* name : processLogin2(void * arg)
* function : Process After Login
*/
void processLogin2(void * arg)
{
	int select, numOfMusicFile, i;
	int sock = *((int*)arg);
	char opcode[30], state[30], FileName[100];


	printAfterLogin();
	printf("What is your select : ");
	fflush(stdin);
	scanf("%d", &select);
	select = select + 10;
	if (select == MUSICLIST)
	{
		sprintf(opcode, "%s", "list!");
		write(sock, opcode, strlen(opcode));

		read(sock, &numOfMusicFile, sizeof(int));
		if (numOfMusicFile == 0)
			printf("Server doesn't have mp3 file\n");
		for (i = 0; i < numOfMusicFile; i++)
		{
			readData((void*)&sock, FileName);
			printf("%d : %s\n", i + 1, FileName);
		}

	}
	else if (select == DOWNMUSIC)
	{
		sprintf(opcode, "%s", "down!");
		write(sock, opcode, strlen(opcode));
		processDownMusic((void*)&sock);
	}
	else if (select == UPLOADMUSIC)
	{
		sprintf(opcode, "%s", "upload!");
		write(sock, opcode, strlen(opcode));
		processUploadMusic((void*)&sock);
	}
	else if (select == QUIT2)
	{
		sprintf(opcode, "%s", "quit!");
		write(sock, opcode, strlen(opcode));
	}
	else
	{
		printf("Please Input 1~4 !\n");

	}

}
/*
* name : processLogin(void * arg)
* function : Process Login
*/
void processLogin(void * arg)
{

	int sock = *((int*)arg);
	char opcode[30], ID[50], E_MAIL[100], PW[50], state[30];

	printf("Start Login\n");
	sprintf(opcode, "%s", "log!");
	write(sock, opcode, strlen(opcode));

	printf("please Input ID : ");
	fflush(stdin);
	scanf("%s", ID);
	strcat(ID, "!");	// attach '!'
	write(sock, ID, strlen(ID));
	fflush(stdin);

	readData((void*)&sock, state);

	if (!strcmp(state, "logok1"))
	{
		printf("please Input PassWord : ");
		fflush(stdin);
		scanf("%s", PW);
		fflush(stdin);
		strcat(PW, "!");	// attach '!'
		write(sock, PW, strlen(PW));

		readData((void*)&sock, state);
		if (!strcmp(state, "logok2"))
		{
			ID[strlen(ID) - 1] = 0;
			printf("\nsucess login!!\n");
			printf("--------------------\n");
			printf("|    Hello %s.. Select to do\n", ID);
			processLogin2((void*)&sock);
		}
		else if (!strcmp(state, "logfail2"))
		{
			printf("PassWord is not correct!\n");
		}
		else
		{

			printf("Login error occurred");
		}

	}
	else if (!strcmp(state, "logfail"))
	{
		ID[strlen(ID) - 1] = 0;
		printf("%s is not exist ID\n", ID);
	}
	else
	{
		printf("error get %s\n", ID);

	}

}
/*
* name : processRegistration(void * arg)
* function : Process Registration
*/
void processRegistration(void * arg)
{
	int sock = *((int*)arg);
	char opcode[30], ID[50], E_MAIL[100], PW[50], state[30];
	printf("Start Registration\n");
	sprintf(opcode, "%s", "reg!");
	write(sock, opcode, strlen(opcode));

	printf("Input ID : ");
	fflush(stdin);
	scanf("%s", ID);
	strcat(ID, "!");	// attach '!'
	write(sock, ID, strlen(ID));
	ID[strlen(ID) - 1] = 0;	//recover ID	


	readData((void*)&sock, state);

	if (strcmp(state, "dup") == 0)	//ID is Duplication 
	{
		printf("ID %s is already registered! \n", ID);

	}
	else if (strcmp(state, "notdup") == 0)
	{
		fflush(stdin);
		printf("Please Input E-mail : ");
		scanf("%s", E_MAIL);
		strcat(E_MAIL, "!");	// attach '!'
		write(sock, E_MAIL, strlen(E_MAIL));

		fflush(stdin);
		printf("Please Input PassWord : ");
		scanf("%s", PW);
		strcat(PW, "!");	// attach '!'
		write(sock, PW, strlen(PW));
		fflush(stdin);
		printf("thank you for registration!\n");

	}
	else
		printf("error occurred");
}

void printInit()
{
	printf("\nWelcome MusicShare!\n");
	printf("--------------------\n");
	printf("| 1. Registration\n");
	printf("| 2. Log in\n");
	printf("| 3. Find PassWord\n");
	printf("| 4. Quit\n");
	printf("--------------------\n");

}
void printAfterLogin()
{

	printf("| 1. Show Server MusicList\n");
	printf("| 2. Download Music\n");
	printf("| 3. Upload Music\n");
	printf("| 4. Quit\n");
	printf("--------------------\n");
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

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
