#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define PORT 3500

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if (argc != 3) {
		printf("Usage : %s <IP> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[2]);//입력한 아이디를 name에 저장
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);//수신 메세지 스레드 생성
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);//송신 메세지 스레드 생성
	//pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(rcv_thread, &thread_return);	
	pthread_join(snd_thread, &thread_return);
	//pthread_join(rcv_thread, &thread_return);
	
	close(sock);
	return 0;
}

void * send_msg(void * arg)
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	while (1)
	{
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "@exit\n") || !strcmp(msg, "@EXIT\n"))//@exit을 커멘트 창에 치면 서버에 보내고 나서 종료해야함 왜냐하면 서버에서 다른클라이언트에게 얘 나갔다고 전달해줘야해서
		{
			sprintf(name_msg, "%s %s", name, msg);//이름이랑 메세지 붙여서 name_msg이 변수에 저장
			//printf("------name_msg:%s---------\n",name_msg);
			write(sock, name_msg, strlen(name_msg));//보냄			
			close(sock);//소켓 닫고
			exit(0);//클라이언트 종료시킴
		}
		sprintf(name_msg, "%s %s", name, msg);//@exit가 아니면 그냥 원하는 메세지 적어서 보내기
		write(sock, name_msg, strlen(name_msg));
	}
	return NULL;
}

void * recv_msg(void * arg)
{
	int sock = *((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
		if (str_len == -1)
			return (void*)-1;
		name_msg[str_len] = 0;
		fputs(name_msg, stdout);//사람아이디+메세지같이출력
		
	}
	return NULL;
}

void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
