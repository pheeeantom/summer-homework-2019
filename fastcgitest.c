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
    FCGX_PutS("<table border=\"1\"><tr>", request.out);
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
    FCGX_PutS("</table>\r\n", request.out);
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
		else if (buf[0] == 'i' && buf[1] == 'n' && buf[2] == 's') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request);
		}
		else if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'l') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request);
		}
		else {
	        FCGX_PutS("\
Content-type: text/html\r\n\
\r\n\
<html>\r\n\
<head>\r\n\
<meta charset=\"utf-8\">\r\n\
<title>База данных гостиницы</title>\r\n\
</head>\r\n\
<body>\r\n\
<button id=\"del\">Удалить</button>\r\n\
<button id=\"add\">Добавить</button>\r\n\
<button id=\"file\">Добавить из файла</button>\r\n\
<button id=\"pass\">Сменить пароль</button>\r\n\
<br>\r\n\
<input>\r\n\
<br>\r\n\
<div>\
", request.out); 
	    	printTable(conn, request);
	    	//FCGX_PutS("<br>\r\n", request.out);
	    	FCGX_PutS("\
</div>\
<button id=\"prev\"><</button>\r\n\
<button id=\"next\">></button>\r\n\
<script>\r\n\
	function transformNumberToName(i) {\r\n\
		if (i == 0) { return \"ФИО\"; }\r\n\
		if (i == 1) { return \"Паспорт\"; }\r\n\
		if (i == 2) { return \"\\\"Дата заезда\\\"\"; }\r\n\
		if (i == 3) { return \"\\\"Дата отъезда\\\"\"; }\r\n\
		if (i == 4) { return \"Номер\"; }\r\n\
	}\r\n\
	var blockEditInput = false;\r\n\
	var del = false;\r\n\
	var inputs = document.getElementsByTagName(\"td\");\r\n\
	document.getElementById(\'add\').addEventListener(\"click\", addButtonListener);\r\n\
	document.getElementById(\'del\').addEventListener(\"click\", deleteButtonListener);\r\n\
	for (var i = 0; i < inputs.length; i++) {\r\n\
		inputs[i].addEventListener(\"click\", tdClickListener);\r\n\
		inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n\
	}\r\n\
	function enterKeyListener(e) {\r\n\
		if (e.keyCode == 13) {\r\n\
			var r = /\\d+/g;\r\n\
			var m;\r\n\
			m = this.id.match(r);\r\n\
			var xhr = new XMLHttpRequest();\r\n\
			xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
			var val = document.getElementsByTagName(\'input\')[1].value;\r\n\
			if (m[1] != 4) { val = \"\\\'\" + val + \"\\\'\"; }\r\n\
			xhr.send(\"update Гостиница set \" + transformNumberToName(m[1]) + \" = \" + val + \" where id = \" + m[0] + \";\");\0\
			", request.out);
	    	//FCGX_PutS("if (xhr.status != 200) {\r\n", request.out);
  			//FCGX_PutS("alert( xhr.status + \': \' + xhr.statusText );\r\n", request.out);
			//FCGX_PutS("} else {\r\n", request.out);
			FCGX_PutS("\
			xhr.onreadystatechange = function() {\r\n\
				if (xhr.readyState == XMLHttpRequest.DONE) {\r\n\
					document.body.getElementsByTagName(\'div\')[0].remove();\r\n\
					var div = document.createElement(\'div\');\r\n\
					div.innerHTML = xhr.responseText;\r\n\
					document.body.insertBefore(div, document.body.getElementsByTagName(\'button\')[4]);\r\n\
					var inputs = document.getElementsByTagName(\"td\");\r\n\
					for (var i = 0; i < inputs.length; i++) {\r\n\
						inputs[i].addEventListener(\"click\", tdClickListener);\r\n\
						inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n\
					}\r\n\
					blockEditInput = false;}}}}\r\n\
	function tdClickListener() {\r\n\
		if (!blockEditInput) {\r\n\
			var len = this.offsetWidth;\r\n\
			document.getElementById(this.id).innerHTML = \"\";\r\n\
			var newInput = document.createElement(\'input\');\r\n\
			newInput.style = \"padding: 0; border: 0; margin: 0; width: \" + len + \"px\";\r\n\
			document.getElementById(this.id).appendChild(newInput);\r\n\
			blockEditInput = true;\r\n\
		}\r\n\
		if (del == true) {\r\n\
			var r = /\\d+/g;\r\n\
			var m;\r\n\
			m = this.id.match(r);\r\n\
			var xhr = new XMLHttpRequest();\r\n\
			xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
			xhr.send(\"delete from Гостиница where id = \" + m[0] + \";\");\r\n\
			xhr.onreadystatechange = function() {\r\n\
				if (xhr.readyState == XMLHttpRequest.DONE) {\r\n\
					document.body.getElementsByTagName(\'div\')[0].remove();\r\n\
					var div = document.createElement(\'div\');\r\n\
					div.innerHTML = xhr.responseText;\r\n\
					document.body.insertBefore(div, document.body.getElementsByTagName(\'button\')[4]);\r\n\
					var inputs = document.getElementsByTagName(\"td\");\r\n\
					for (var i = 0; i < inputs.length; i++) {\r\n\
						inputs[i].addEventListener(\"click\", tdClickListener);\r\n\
						inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n\
					}\r\n\
					blockEditInput = false;\r\n\
					del = false;\r\n\
				}\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
	function enterKeyListenerAdd(e) {\r\n\
		if (e.keyCode == 13) {\r\n\
			var inputs = document.getElementsByTagName(\"input\");\r\n\
			var xhr = new XMLHttpRequest();\r\n\
			xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
			xhr.send(\"insert into Гостиница (ФИО, Паспорт, \\\"Дата заезда\\\", \\\"Дата отъезда\\\", Номер) values (\\\'\" + inputs[1].value + \"\\\', \\\'\" + inputs[2].value + \"\\\', \\\'\" + inputs[3].value + \"\\\', \\\'\" + inputs[4].value + \"\\\', \" + inputs[5].value + \");\");\r\n\
			xhr.onreadystatechange = function() {\r\n\
				if (xhr.readyState == XMLHttpRequest.DONE) {\r\n\
					document.body.getElementsByTagName(\'div\')[0].remove();\r\n\
					var div = document.createElement(\'div\');\r\n\
					div.innerHTML = xhr.responseText;\r\n\
					document.body.insertBefore(div, document.body.getElementsByTagName(\'button\')[4]);\r\n\
					var inputs = document.getElementsByTagName(\"td\");\r\n\
					for (var i = 0; i < inputs.length; i++) {\r\n\
						inputs[i].addEventListener(\"click\", tdClickListener);\r\n\
						inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n\
					}\r\n\
					blockEditInput = false;}}\r\n\
		}\r\n\
	}\r\n\
	function addButtonListener() {\r\n\
		if (blockEditInput == false) {\r\n\
			var ths = document.getElementsByTagName(\"th\");\r\n\
			var newTr = document.createElement(\'tr\');\r\n\
			newTr.innerHTML = \"<td><input style=\\\"width: \" + ths[0].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[1].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[2].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[3].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[4].offsetWidth + \";\\\"></td>\";\r\n\
			document.getElementsByTagName(\'table\')[0].appendChild(newTr);\r\n\
			var inputs = document.getElementsByTagName(\"input\");\r\n\
			for (var i = 1; i < inputs.length; i++) {\r\n\
				inputs[i].addEventListener(\"keypress\", enterKeyListenerAdd);\r\n\
				inputs[i].style.padding = 0;\r\n\
				inputs[i].style.border = 0;\r\n\
				inputs[i].style.margin = 0;\r\n\
			}\r\n\
			blockEditInput = true;\r\n\
		}\r\n\
	}\r\n\
	function deleteButtonListener() {\r\n\
		if (blockEditInput == false) {\r\n\
			del = true;\r\n\
			blockEditInput = true;\r\n\
		}\r\n\
	}\r\n\
</script>\r\n\
</body>\r\n\
</html>\r\n\
", request.out);

	        //закрыть текущее соединение 
	        //FCGX_Finish_r(&request); 
	        
	        //завершающие действия - запись статистики, логгирование ошибок и т.п.
        } 
	}
	PQfinish(conn);
	return 0;
}