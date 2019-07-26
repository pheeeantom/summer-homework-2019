#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcgi_config.h"
#include "fcgiapp.h"
#include <libpq-fe.h>

#define SOCKET_PATH "127.0.0.1:9000"

int socketId;

void do_exit(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

/*char * transformNumberToName(int i) {
	if (i == 0) return "ФИО";
	if (i == 1) return "Паспорт";
	if (i == 2) return "\"Дата заезда\"";
	if (i == 3) return "\"Дата отъезда\"";
	if (i == 4) return "Номер";
}*/

void printTable(PGconn *conn, FCGX_Request request) {
	PGresult *res = PQexec(conn, "select*from Гостиница");    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data retrieved\n");        
        PQclear(res);
        do_exit(conn);
    }
    int ncols = PQnfields(res);
    FCGX_PutS("<div><table border=\"1\"><tr>", request.out);
    for (int i = 0; i < ncols - 1; i++) {
    	FCGX_PutS("<th>", request.out);
    	FCGX_PutS(PQfname(res, i), request.out);
    	FCGX_PutS("</th>", request.out);
    }
    FCGX_PutS("</tr>", request.out);
    int nrows = PQntuples(res);
    char arr[10];
    for (int i = 0; i < nrows; i++) {
    	FCGX_PutS("<tr>", request.out);
    	for (int j = 0; j < ncols - 1; j++) {
    		FCGX_PutS("<td id=\"row", request.out);
    		FCGX_PutS(PQgetvalue(res, i, 5), request.out);
    		FCGX_PutS("col", request.out);
    		sprintf(arr,"%d",j);
    		FCGX_PutS(arr, request.out);
    		FCGX_PutS("\">", request.out);
    		FCGX_PutS(PQgetvalue(res, i, j), request.out);
    		FCGX_PutS("</td>", request.out);
    	}
    	FCGX_PutS("</tr>", request.out);
    }
    FCGX_PutS("</table></div>", request.out);
    PQclear(res);
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
        //server_name = FCGX_GetParam("SERVER_NAME", request.envp); 
		//char * clStr = FCGX_GetParam("CONTENT_LENGTH", request.envp);
		//int cl = atoi(clStr);
		char * buf = malloc(500);
		FCGX_GetStr(buf, 500, request.in);
		char * ach = strchr(buf, '\x3B');
		printf("%ld", ach - buf + 1);
		if (ach - buf + 1 < 0) {

		}
		else {
			buf[(int)(ach - buf + 1)] = '\0';
		}

		//buf[strlen(buf)] = '\0';
		//printf("%s", buf);

		if (buf[0] == 'u' && buf[1] == 'p' && buf[2] == 'd') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request);
			//FCGX_Finish_r(&request); 
			//free(buf);
		}
		else {
	        //вывести все HTTP-заголовки (каждый заголовок с новой строки) 
	        FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        //между заголовками и телом ответа нужно вывести пустую строку 
	        FCGX_PutS("\r\n", request.out); 
	        //вывести тело ответа (например - html-код веб-страницы) 
	        FCGX_PutS("<html>\r\n", request.out); 
	        FCGX_PutS("<head>\r\n", request.out); 
	        FCGX_PutS("<meta charset=\"utf-8\">\r\n", request.out); 
	        FCGX_PutS("<title>База данных гостиницы</title>\r\n", request.out);
	        FCGX_PutS("</head>\r\n", request.out); 
	        FCGX_PutS("<body>\r\n", request.out); 
	    	printTable(conn, request);
		    FCGX_PutS("<script>\r\n", request.out);
		    FCGX_PutS("function transformNumberToName(i) {\r\n", request.out);
		    FCGX_PutS("if (i == 0) { return \"ФИО\"; }\r\n", request.out);
		    FCGX_PutS("if (i == 1) { return \"Паспорт\"; }\r\n", request.out);
		    FCGX_PutS("if (i == 2) { return \"\\\"Дата заезда\\\"\"; }\r\n", request.out);
		    FCGX_PutS("if (i == 3) { return \"\\\"Дата отъезда\\\"\"; }\r\n", request.out);
		    FCGX_PutS("if (i == 4) { return \"Номер\"; }\r\n", request.out);
		    FCGX_PutS("}\r\n", request.out);
		    FCGX_PutS("var blockEditInput = false;\r\n", request.out);
	        FCGX_PutS("var inputs = document.getElementsByTagName(\"td\");\r\n", request.out);
			FCGX_PutS("for (var i = 0; i < inputs.length; i++) {\r\n", request.out);
	  		FCGX_PutS("inputs[i].addEventListener(\"click\", tdClickListener);\r\n", request.out);
	  		FCGX_PutS("inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n", request.out);
			FCGX_PutS("}\r\n", request.out);
			FCGX_PutS("function enterKeyListener(e) {\r\n", request.out);
			FCGX_PutS("if (e.keyCode == 13) {\r\n", request.out);
			FCGX_PutS("var r = /\\d+/g;\r\n", request.out);
			FCGX_PutS("var m;\r\n", request.out);
			FCGX_PutS("m = this.id.match(r);\r\n", request.out);
			FCGX_PutS("var xhr = new XMLHttpRequest();\r\n", request.out);
	    	FCGX_PutS("xhr.open(\'POST\', \'http://localhost/\', true);\r\n", request.out);
	    	FCGX_PutS("var val = document.getElementsByTagName(\'input\')[0].value;\r\n", request.out);
	    	FCGX_PutS("if (m[1] != 4) { val = \"\\\'\" + val + \"\\\'\"; }\r\n", request.out);
	    	FCGX_PutS("xhr.send(\"update Гостиница set \" + transformNumberToName(m[1]) + \" = \" + val + \" where id = \" + m[0] + \";\");\0", request.out);
	    	//FCGX_PutS("if (xhr.status != 200) {\r\n", request.out);
  			//FCGX_PutS("alert( xhr.status + \': \' + xhr.statusText );\r\n", request.out);
			//FCGX_PutS("} else {\r\n", request.out);
			FCGX_PutS("xhr.onreadystatechange = function() {\r\n", request.out);
    		FCGX_PutS("if (xhr.readyState == XMLHttpRequest.DONE) {\r\n", request.out);
    		FCGX_PutS("document.body.getElementsByTagName(\'div\')[0].remove();\r\n", request.out);
  			FCGX_PutS("var div = document.createElement(\'div\');\r\n", request.out);
  			FCGX_PutS("div.innerHTML = xhr.responseText;\r\n", request.out);
  			FCGX_PutS("document.body.insertBefore(div, document.body.firstChild);\r\n", request.out);
  			FCGX_PutS("var inputs = document.getElementsByTagName(\"td\");\r\n", request.out);
			FCGX_PutS("for (var i = 0; i < inputs.length; i++) {\r\n", request.out);
	  		FCGX_PutS("inputs[i].addEventListener(\"click\", tdClickListener);\r\n", request.out);
	  		FCGX_PutS("inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n", request.out);
			FCGX_PutS("}\r\n", request.out);
			//FCGX_PutS("}\r\n", request.out);
	    	FCGX_PutS("blockEditInput = false;}}}}\r\n", request.out);
			FCGX_PutS("function tdClickListener() {\r\n", request.out);
			FCGX_PutS("if (!blockEditInput) {", request.out);
			//FCGX_PutS("var len = document.getElementById(this.id).innerText.length;\r\n", request.out);
	    	FCGX_PutS("document.getElementById(this.id).innerHTML = \"\";\r\n", request.out);
	    	FCGX_PutS("var newInput = document.createElement('input');\r\n", request.out);
	    	//FCGX_PutS("newInput.size = len;\r\n", request.out);
	    	FCGX_PutS("var len = this.offsetWidth;\r\n", request.out);
	    	FCGX_PutS("newInput.style = \"padding: 0; border: 0; margin: 0; width: \" + len + \"px\";\r\n", request.out);
	    	FCGX_PutS("document.getElementById(this.id).appendChild(newInput);\r\n", request.out);
	    	FCGX_PutS("blockEditInput = true;\r\n", request.out);
	    	//FCGX_PutS("document.getElementById(this.id).onkeyup = enterKeyListener;\r\n", request.out);
	    	//FCGX_PutS("\r\n", request.out);
			FCGX_PutS("}}\r\n", request.out);
			FCGX_PutS("</script>\r\n", request.out);
	        FCGX_PutS("</body>\r\n", request.out); 
	        FCGX_PutS("</html>\r\n", request.out); 

	        //закрыть текущее соединение 
	        //FCGX_Finish_r(&request); 
	        
	        //завершающие действия - запись статистики, логгирование ошибок и т.п.
        } 
	}
	PQfinish(conn);
	return 0;
}