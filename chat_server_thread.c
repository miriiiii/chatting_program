#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define PORT 3500

void * handle_clnt(void *arg);//클라이언트를 다루는 함수
void send_msg(char * msg, int len);//메세지를 보내는 함수

void error_handling(char * msg);//에러체크 함수

char user_id[MAX_CLNT][20];//사용자 아이디 저장하는 배열(@show에서 사용)
int clnt_cnt = 0;//접속클라이언트 개수
int clnt_socks[MAX_CLNT];//접속한 클라이언트 소켓들
pthread_mutex_t mutx;//임계영역에 필요한 뮤텍스 선언
char buffer[256] = "";//메세지 받을때 사용되는 버퍼
char buffer2[256] = "";//환영메세지 받을때 사용되는 버퍼


int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;//스레드id
	char *ptr;
	char id[20];
	int i=0;//user_id index
	pthread_mutex_init(&mutx, NULL);//뮤텍스 초기화
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);//소켓생성

	memset(&serv_adr, 0, sizeof(serv_adr));//소켓초기화
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(PORT);

	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)//바인드
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)//리슨
		error_handling("listen() error");

	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		
		pthread_mutex_lock(&mutx);//임계영역의 시작부분
		clnt_socks[clnt_cnt++] = clnt_sock;//연결된 소켓을 클라이언트소켓배열에 넣음
		pthread_mutex_unlock(&mutx);//임계영역 끝부분

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);//연결된 클라이언트 소켓에 read/write전용 스레드 생성
		read(clnt_sock, buffer, sizeof(buffer));//클라이언트 소켓을 내용을 읽어온다.
		ptr=strtok(buffer, " ");//이중에서 소켓의 아이디만을 토큰으로 배열에 저장해둔다
		strcpy(user_id[i],ptr);//임의의 배열에 아이디값 저장
		sprintf(buffer,	"%s님이 접속하셨습니다.\n",user_id[i]);//접속한 클라이언트를 다른클라이언트에 알리는 메세지
		sprintf(buffer2,"%s님 채팅창에 들어오신 것을 환영합니다!\n", user_id[i]);//채팅창 환영인사
		pthread_detach(t_id);//스레드 없애기
		printf("Connected client IP & Name: %s %s\n", inet_ntoa(clnt_adr.sin_addr),user_id[i]);//연결된 ip주소+name
		i++;//user_id index++;	
		for(int i=0; i<clnt_cnt; i++){//전체 클라이언트에 지금접속한 아이디를 전송해준다.
			if(clnt_sock != clnt_socks[i]){	//클라이언트 자기자신 제외한 애들에게 접속정보를 보낸다.
			write(clnt_socks[i], buffer, sizeof(buffer));
			}
			else {//접속한 클라이언트에게는 환영메세지를 보낸다.
				write(clnt_socks[i], buffer2, sizeof(buffer2));
			}
		}
		
	}
	close(serv_sock);
	return 0;
}

void * handle_clnt(void * arg)
{
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	char exit_msg[100];
	char *ptr, *ptr2;
	char exitid[10];
	char show_msg[1024];
	char show_name_list[256];
	char sys_msg[20], cpy_msg[100];

	
	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0){
		//if(clnt_sock!=clnt_socks[i]){//자기자신 제외한 애들에게 문자내용 전송한다.
			strcpy(cpy_msg,msg);	//혹시나 원래 받은 문자열인msg이게 영향을 받을까싶어..cpy_msg여기에 복사해서 사용		
			ptr=strtok(cpy_msg," ");
			strcpy(exitid,ptr);//ptr은 클라이언트 아이디가 저장됨
			ptr2=strtok(NULL," ");//이때 ptr2는 채팅창에 친 대화내용임
			strcpy(sys_msg,ptr2);//@exit만 잘라오기 위한 과정!
			//printf("-------------잘라온system Message:%s--------------------\n",sys_msg);
			if (!strcmp(sys_msg, "@exit\n") || !strcmp(sys_msg, "@EXIT\n"))
			{
			//close(clnt_sock);
			//ptr=strtok(cpy_msg," ");
			//printf("-----------위에서 아이디 잘라온거 확인:%s----------\n",exitid);
			//strcpy(exitid,ptr2);
			sprintf(exit_msg,"%s님이 나가셨습니다.\n",exitid); //sprintf문 사용해서 다른 클라이언트들에게 어떤 클라이언트가 나갔는지알려줌
			//printf("-----------exit_msg구성 확인:%s------------\n",exit_msg);//보내려는 메세지 다시확인...
			send_msg(exit_msg, sizeof(exit_msg));//메세지 전송
			//printf("----------------next send-------------\n");
			close(clnt_socks[i]);//소켓닫기
			clnt_cnt--;
			return 0;
			}
			else if(!strcmp(sys_msg, "@show\n") || !strcmp(sys_msg,"@SHOW\n"))//이건 현재 접속자수와 아이디 명을 출력하는역할..
			{
				for(int j=0; j<clnt_cnt; j++){
					if(clnt_socks[j]!=-1){
					strcat(show_name_list,user_id[j]);	//show_name_list에 현재 접속한 유저아이디 전부를 넣음
					printf("-----user_id[i]: %s------\n",user_id[j]);
					}
					//else{strcat(show_name_list,NULL);}
				
				}
				//printf("---------show_name_list: %s---------------\n",show_name_list);
				sprintf(show_msg,"현재 접속자 수는 %d명이고, 접속자 이름은 %s입니다.\n",clnt_cnt,show_name_list);
				//printf("--------show_msg: %s-------------------------\n",show_msg);
				//printf("--------------------show_msg확인:%s--------------\n",show_msg);
				send_msg(show_msg, sizeof(show_msg));
			}
			
			else{send_msg(msg, str_len);}}
		//}
	pthread_mutex_lock(&mutx);
	for (i = 0; i < clnt_cnt; i++)
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
	close(clnt_sock);
	return NULL;
}

void send_msg(char * msg, int len)
{
	int i;
	pthread_mutex_lock(&mutx);
	for (i = 0; i < clnt_cnt; i++){
		
		write(clnt_socks[i], msg, len);
	}
	pthread_mutex_unlock(&mutx);
}



void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
