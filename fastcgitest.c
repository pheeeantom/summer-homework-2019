#include <stdio.h>

#include "fcgi_config.h"
#include "fcgiapp.h"

#define SOCKET_PATH "127.0.0.1:9000"

int socketId;

int main(void)
{
	FCGX_Init();
	printf("Lib is inited\n");

	socketId = FCGX_OpenSocket(SOCKET_PATH, 0);
	if (socketId < 0)
	{
		printf("Error with opening socket\n");
		return 1;
	}
	printf("Socket is opened\n");

	int rc;
	FCGX_Request request;
	char *server_name;

	if (FCGX_InitRequest(&request, socketId, 0) != 0)
	{
		printf("Can not init request\n");
		return 1;
	}
	printf("Request is inited\n");

	for(;;)
	{
		printf("Try to accept new request\n");
		rc = FCGX_Accept_r(&request);

		if(rc < 0)
		{
			printf("Can not accept new request\n");
			break;
		}
		printf("request is accepted\n");

		//получить значение переменной 
        server_name = FCGX_GetParam("SERVER_NAME", request.envp); 

        //вывести все HTTP-заголовки (каждый заголовок с новой строки) 
        FCGX_PutS("Content-type: text/html\r\n", request.out); 
        //между заголовками и телом ответа нужно вывести пустую строку 
        FCGX_PutS("\r\n", request.out); 
        //вывести тело ответа (например - html-код веб-страницы) 
        FCGX_PutS("<html>\r\n", request.out); 
        FCGX_PutS("<head>\r\n", request.out); 
        FCGX_PutS("<title>FastCGI Hello! (C, fcgiapp library)</title>\r\n", request.out); 
        FCGX_PutS("</head>\r\n", request.out); 
        FCGX_PutS("<body>\r\n", request.out); 
        FCGX_PutS("<h1>FastCGI Hello! (C, fcgiapp library)</h1>\r\n", request.out); 
        FCGX_PutS("<p>Request accepted from host <i>", request.out); 
        FCGX_PutS(server_name ? server_name : "?", request.out); 
        FCGX_PutS("</i></p>\r\n", request.out); 
        FCGX_PutS("</body>\r\n", request.out); 
        FCGX_PutS("</html>\r\n", request.out); 

        //закрыть текущее соединение 
        FCGX_Finish_r(&request); 
        
        //завершающие действия - запись статистики, логгирование ошибок и т.п. 
	}
	return 0;
}