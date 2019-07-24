#include <stdio.h>
#include <stdlib.h>

#include "fcgi_config.h"
#include "fcgiapp.h"
#include <libpq-fe.h>

#define SOCKET_PATH "127.0.0.1:9000"

int socketId;

void do_exit(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

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

	PGconn *conn = PQconnectdb("user=postgres password=postgres dbname=hw");
	if (PQstatus(conn) == CONNECTION_BAD) {
    
    	fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
    	do_exit(conn);
	}

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
        FCGX_PutS("<meta charset=\"utf-8\">\r\n", request.out); 
        FCGX_PutS("</head>\r\n", request.out); 
        FCGX_PutS("<body>\r\n", request.out); 
        FCGX_PutS("<h1>FastCGI Hello! (C, fcgiapp library)</h1>\r\n", request.out); 
        FCGX_PutS("<p>Request accepted from host <i>", request.out); 
        FCGX_PutS(server_name ? server_name : "?", request.out); 
        FCGX_PutS("</i></p>\r\n", request.out); 
        char *user = PQuser(conn);
    	char *db_name = PQdb(conn);
    	//char *pswd = PQpass(conn);
    	FCGX_PutS("<p>User: ", request.out);
    	FCGX_PutS(user, request.out);
    	FCGX_PutS("</p>\r\n", request.out);
    	FCGX_PutS("<p>Db name: ", request.out);
    	FCGX_PutS(db_name, request.out);
    	FCGX_PutS("</p>\r\n", request.out);
    	PGresult *res = PQexec(conn, "select*from Гостиница");    
	    if (PQresultStatus(res) != PGRES_TUPLES_OK) {

	        printf("No data retrieved\n");        
	        PQclear(res);
	        do_exit(conn);
	    }
	    int ncols = PQnfields(res);
	    FCGX_PutS("<table border=\"1\"><tr>", request.out);
	    for (int i = 0; i < ncols; i++) {
	    	FCGX_PutS("<th>", request.out);
	    	FCGX_PutS(PQfname(res, i), request.out);
	    	FCGX_PutS("</th>", request.out);
	    }
	    FCGX_PutS("</tr>", request.out);
	    int nrows = PQntuples(res);
	    for (int i = 0; i < nrows; i++) {
	    	FCGX_PutS("<tr>", request.out);
	    	for (int j = 0; j < ncols; j++) {
	    		FCGX_PutS("<td>", request.out);
	    		FCGX_PutS(PQgetvalue(res, i, j), request.out);
	    		FCGX_PutS("</td>", request.out);
	    	}
	    	FCGX_PutS("</tr>", request.out);
	    }
	    FCGX_PutS("</table>", request.out);
	    PQclear(res);
        FCGX_PutS("</body>\r\n", request.out); 
        FCGX_PutS("</html>\r\n", request.out); 

        //закрыть текущее соединение 
        FCGX_Finish_r(&request); 
        
        //завершающие действия - запись статистики, логгирование ошибок и т.п. 
	}
	PQfinish(conn);
	return 0;
}