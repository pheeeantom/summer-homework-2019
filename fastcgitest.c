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

void parseRequest(char * ach, char * buf, char * offset, char * limit, char * sort, char * order, char * search, char * value) {
	strncpy(offset, ach + 1, (strchr(buf, '\x2F') - buf) - (ach - buf + 1));
	offset[(strchr(buf, '\x2F') - buf) - (ach - buf + 1)] = '\0';
	strncpy(limit, strchr(buf, '\x2F') + 1, strchr(buf, '\x24') - strchr(buf, '\x2F') - 1);
	limit[strchr(buf, '\x24') - strchr(buf, '\x2F') - 1] = '\0';
	strncpy(sort, strchr(buf, '\x24') + 1, strchr(buf, '~') - strchr(buf, '\x24') - 1);
	sort[strchr(buf, '~') - strchr(buf, '\x24') - 1] = '\0';
	strncpy(order, strchr(buf, '~') + 1, strchr(buf, '.') - strchr(buf, '~') - 1);
	order[strchr(buf, '.') - strchr(buf, '~') - 1] = '\0';
	strncpy(search, strchr(buf, '.') + 1, strchr(buf, '!') - strchr(buf, '.') - 1);
	search[strchr(buf, '!') - strchr(buf, '.') - 1] = '\0';
	strncpy(value, strchr(buf, '!') + 1, strchr(buf, '?') - strchr(buf, '!') - 1);
	value[strchr(buf, '?') - strchr(buf, '!') - 1] = '\0';
}

void printTable(PGconn *conn, FCGX_Request request, char * offset, char * limit, char * sort, char * order, char * search, char * value) {
	char * query = malloc(300);
	strcpy(query, "select*from Гостиница");
	if (strcmp(search, "null") != 0 && strcmp(value, "null") != 0) {
		strcat(query, " where ");
		strcat(query, search);
		if (strcmp(search, "ФИО") != 0 && strcmp(search, "Паспорт") != 0) {
			strcat(query, "::varchar(50)");
		}
		strcat(query, " like \'%");
		strcat(query, value);
		strcat(query, "%\'");
	}
	if (strcmp(sort, "null") != 0 && strcmp(order, "null") != 0) {
		strcat(query, " order by ");
		strcat(query, sort);
		strcat(query, " ");
		strcat(query, order);
	}
	strcat(query, " offset ");
	strcat(query, offset);
	strcat(query, " limit ");
	strcat(query, limit);
	strcat(query, ";");
	printf("%s", query);
	PGresult *res = PQexec(conn, query);   
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data retrieved\n");        
        PQclear(res);
        do_exit(conn);
    }
    int ncols = PQnfields(res);
    FCGX_PutS("<table border=\"1\"><tr>", request.out);
    for (int i = 0; i < ncols - 1; i++) {
    	FCGX_PutS("<th>", request.out);
    	char * name = malloc(50);
    	strcpy(name, PQfname(res, i));
    	//printf("%ld\n", strlen(name));
    	/*char * q = malloc(10);
    	strcpy(q, "test");*/
    	char * name1 = realloc(name, strlen(name) + 5);
    	if (strcmp(sort, "null") != 0 && strcmp(order, "null") != 0) {
    		char * sort1 = malloc(25);
	    	if (strchr(sort, '\"') != NULL) {
	    		strncpy(sort1, sort + 1, strlen(sort) - 1);
	    		sort1[strlen(sort) - 2] = '\0';
	    	}
	    	else {
	    		sort1 = sort;
	    	}
	    	if (strcmp(name, sort1) == 0) {
	    		int len = strlen(name);
	    		printf("~%d~", len);
		    	if (strcmp(order, "asc") == 0) {
		    		name1[len] = '/';
		    		name1[len + 1] = '\\';
		    		name1[len + 2] = '\0';
		    	}
		    	else if (strcmp(order, "desc") == 0) {
		    		name1[len] = '\\';
		    		name1[len + 1] = '/';
		    		name1[len + 2] = '\0';
		    	}
	    	}
    	}
    	FCGX_PutS(name1, request.out);
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

	PGconn *conn;

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
		char * buf = malloc(1000);
		char * offset = malloc(10);
		char * limit = malloc(10);
		char * sort = malloc(50);
		char * order = malloc(15);
		char * search = malloc(50);
		char * value = malloc(100);
		FCGX_GetStr(buf, 1000, request.in);
		if (!(buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l')) {
			if (!(buf[0] == 'm' && buf[1] == 'a' && buf[2] == 'i')) {
				if (!(buf[0] == 'i' && buf[1] == 'n' && buf[2] == 'i')) {
					if (!(buf[0] == 'c' && buf[1] == 'h' && buf[2] == 'e')) {
						if (!(buf[0] == 'n' && buf[1] == 'e' && buf[2] == 'w')) {
							char * ach = strchr(buf, '\x3B');
							printf("%ld", ach - buf + 1);
							if (ach - buf + 1 < 0) {

							}
							else {
								parseRequest(ach, buf, offset, limit, sort, order, search, value);
								buf[(int)(ach - buf + 1)] = '\0';
							}
						}
					}
				}
			}
		}
		else {
			char * ach = strchr(buf, '@');
			printf("%ld", ach - buf);
			if (ach - buf < 0) {

			}
			else {
				parseRequest(ach, buf, offset, limit, sort, order, search, value);
				buf[(int)(ach - buf)] = '\0';
			}
		}

		//buf[strlen(buf)] = '\0';
		//printf("%s", buf);

		if (buf[0] == 'u' && buf[1] == 'p' && buf[2] == 'd') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request, offset, limit, sort, order, search, value);
			//FCGX_Finish_r(&request); 
			//free(buf);
		}
		else if (buf[0] == 'i' && buf[1] == 'n' && buf[2] == 's') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request, offset, limit, sort, order, search, value);
		}
		else if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'l') {
			PQexec(conn, buf);
			printf("%s", PQerrorMessage(conn));
			printf("%s", buf);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request, offset, limit, sort, order, search, value);
		}
		else if (buf[0] == 's' && buf[1] == 'e' && buf[2] == 'l') {
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request, offset, limit, sort, order, search, value);
		}
		else if (buf[0] == 'a' && buf[1] == 'm' && buf[2] == 'o') {
			char * str = malloc(10);
			char * query = malloc(300);
			char * search1 = malloc(50);
			char * value1 = malloc(100);
			strncpy(search1, strchr(buf, '$') + 1, strchr(buf, '!') - strchr(buf, '$') - 1);
			search1[strchr(buf, '!') - strchr(buf, '$') - 1] = '\0';
			strncpy(value1, strchr(buf, '!') + 1, strchr(buf, '?') - strchr(buf, '!') - 1);
			value1[strchr(buf, '?') - strchr(buf, '!') - 1] = '\0';
			strcpy(query, "select*from Гостиница");
			if (strcmp(search, "null") != 0 && strcmp(value1, "null") != 0) {
				strcat(query, " where ");
				strcat(query, search1);
				if (strcmp(search1, "ФИО") != 0 && strcmp(search1, "Паспорт") != 0) {
					strcat(query, "::varchar(50)");
				}
				strcat(query, " like \'%");
				strcat(query, value1);
				strcat(query, "%\'");
			}
			PGresult *res = PQexec(conn, query);
			int row_num = PQntuples(res);
			PQclear(res);
			sprintf(str, "%d", row_num);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			FCGX_PutS(str, request.out);
		}
		else if (buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l') {
			//printf("%s", buf + 4);
			char * buf1 = malloc(300);
			char * buf2 = malloc(200);
			char * p;
			char * pp = buf + 4;
			do {
				p = strchr(pp, ';');
				if (p != NULL) {
					strcpy(buf1, "insert into Гостиница(ФИО, Паспорт, \"Дата заезда\", \"Дата отъезда\", Номер) values ");
					strncpy(buf2, pp, p - pp + 1);
					strcat(buf1, buf2);
					PQexec(conn, buf1);
					printf("%s", PQerrorMessage(conn));
				}
				pp = p + 1;
			} while (p != NULL);
			free(buf1);
			free(buf2);
			FCGX_PutS("Content-type: text/html\r\n", request.out); 
	        FCGX_PutS("\r\n", request.out); 
			printTable(conn, request, offset, limit, sort, order, search, value);
		}
		else if (buf[0] == 'm' && buf[1] == 'a' && buf[2] == 'i') {
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
<input id=\"fileinput\" type=\"file\" style=\"display:none;\" />\r\n\
<button id=\"pass\">Сменить пароль</button>\r\n\
<br>\r\n\
<input id=\"search\">\r\n\
<select id=\"selectsearch\">\r\n\
	<option selected value=\"ФИО\">Поиск по ФИО</option>\r\n\
	<option value=\"Паспорт\">Поиск по паспорту</option>\r\n\
	<option value=\"&quot;Дата заезда&quot;\">Поиск по дате заезда</option>\r\n\
	<option value=\"&quot;Дата отъезда&quot;\">Поиск по дате отъезда</option>\r\n\
	<option value=\"Номер\">Поиск по номеру</option>\r\n\
</select>\r\n\
<br>\r\n\
<div>\
", request.out); 
	    	printTable(conn, request, "0", "5", "null", "null", "null", "null");
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
	function addQuotes(colName) {\r\n\
		if (colName === \"Дата заезда\") {\r\n\
			return \"\\\"\" + colName + \"\\\"\";\r\n\
		}\r\n\
		else if (colName === \"Дата отъезда\") {\r\n\
			return \"\\\"\" + colName + \"\\\"\";\r\n\
		}\r\n\
		else {\r\n\
			return colName;\r\n\
		}\r\n\
	}\r\n\
	function xhrSend (s) {\r\n\
		var xhr = new XMLHttpRequest();\r\n\
		xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
		xhr.send(s);\r\n\
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
				var ths = document.getElementsByTagName(\"th\");\r\n\
				for (var i = 0; i < ths.length; i++) {\r\n\
					ths[i].addEventListener(\"click\", thClickListener);\r\n\
				}\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
	function updatePass(str) {\r\n\
		var xhr = new XMLHttpRequest();\r\n\
		xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
		xhr.send(\"newpass\" + str + \"\\0\");\r\n\
	}\r\n\
	//function transformNameToNumber(i) {\r\n\
		//if (i == \"ФИО\") { return 0; }\r\n\
		//if (i == \"Паспорт\") { return 1; }\r\n\
		//if (i == \"\\\"Дата заезда\\\"\") { return 2; }\r\n\
		//if (i == \"\\\"Дата отъезда\\\"\") { return 3; }\r\n\
		//if (i == \"Номер\") { return 4; }\r\n\
	//}\r\n\
	function transformBoolToOrder(i) {\r\n\
		if (!i) {\r\n\
			return \"asc\";\r\n\
		}\r\n\
		else if (i) {\r\n\
			return \"desc\";\r\n\
		}\r\n\
		else {\r\n\
			return null;\r\n\
		}\r\n\
	}\r\n\
	var offset = 0;\r\n\
	var limit = 5;\r\n\
	var blockEditInput = false;\r\n\
	var del = false;\r\n\
	var amount = \
			", request.out);
			char * str = malloc(10);
			PGresult *res = PQexec(conn, "select * from Гостиница;");
			int row_num = PQntuples(res);
			PQclear(res);
			sprintf(str, "%d", row_num);
			FCGX_PutS(str, request.out);
	    	FCGX_PutS("\
	;\r\n\
	var inputs = document.getElementsByTagName(\"td\");\r\n\
	var ths = document.getElementsByTagName(\"th\");\r\n\
	var sort = \"null\";\r\n\
	var order = \"null\";\r\n\
	var searchVal = \"null\";\r\n\
	var selectsearchVal = \"ФИО\";\r\n\
	document.getElementById(\'add\').addEventListener(\"click\", addButtonListener);\r\n\
	document.getElementById(\'del\').addEventListener(\"click\", deleteButtonListener);\r\n\
	document.getElementById(\'prev\').addEventListener(\"click\", prevButtonListener);\r\n\
	document.getElementById(\'next\').addEventListener(\"click\", nextButtonListener);\r\n\
	document.getElementById(\'file\').addEventListener(\"click\", fileButtonListener);\r\n\
	document.getElementById(\'pass\').addEventListener(\"click\", changePass);\r\n\
	document.getElementById(\'fileinput\').addEventListener(\'change\', getFile);\r\n\
	document.getElementById(\'search\').addEventListener(\"keypress\", search);\r\n\
	document.getElementById(\'selectsearch\').addEventListener(\"change\", updateSearchValues);\r\n\
	for (var i = 0; i < inputs.length; i++) {\r\n\
		inputs[i].addEventListener(\"click\", tdClickListener);\r\n\
		inputs[i].addEventListener(\"keypress\", enterKeyListener);\r\n\
	}\r\n\
	for (var i = 0; i < ths.length; i++) {\r\n\
		ths[i].addEventListener(\"click\", thClickListener);\r\n\
	}\r\n\
	function enterKeyListener(e) {\r\n\
		if (e.keyCode == 13) {\r\n\
			var r = /\\d+/g;\r\n\
			var m;\r\n\
			m = this.id.match(r);\r\n\
			var val = document.getElementsByTagName(\'input\')[2].value;\r\n\
			if (m[1] != 4) { val = \"\\\'\" + val + \"\\\'\"; }\r\n\
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"update Гостиница set \" + transformNumberToName(m[1]) + \" = \" + val + \" where id = \" + m[0] + \";\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			blockEditInput = false;\r\n\
			", request.out);
	    	//FCGX_PutS("if (xhr.status != 200) {\r\n", request.out);
  			//FCGX_PutS("alert( xhr.status + \': \' + xhr.statusText );\r\n", request.out);
			//FCGX_PutS("} else {\r\n", request.out);
			FCGX_PutS("\
		}\r\n\
	}\r\n\
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
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"delete from Гостиница where id = \" + m[0] + \";\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			var xhr1 = new XMLHttpRequest();\r\n\
			xhr1.open(\'POST\', \'http://localhost/\', true);\r\n\
			xhr1.send(\"amount$\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			xhr1.onreadystatechange = function() {\r\n\
				if (xhr1.readyState == XMLHttpRequest.DONE) {\r\n\
					amount = +xhr1.responseText;\r\n\
				}\r\n\
			}\r\n\
			del = false;\r\n\
			blockEditInput = false;\r\n\
		}\r\n\
	}\r\n\
	function enterKeyListenerAdd(e) {\r\n\
		if (e.keyCode == 13) {\r\n\
			var inputs = document.getElementsByTagName(\"input\");\r\n\
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"insert into Гостиница (ФИО, Паспорт, \\\"Дата заезда\\\", \\\"Дата отъезда\\\", Номер) values (\\\'\" + inputs[2].value + \"\\\', \\\'\" + inputs[3].value + \"\\\', \\\'\" + inputs[4].value + \"\\\', \\\'\" + inputs[5].value + \"\\\', \" + inputs[6].value + \");\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			var xhr1 = new XMLHttpRequest();\r\n\
			xhr1.open(\'POST\', \'http://localhost/\', true);\r\n\
			xhr1.send(\"amount$\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			xhr1.onreadystatechange = function() {\r\n\
				if (xhr1.readyState == XMLHttpRequest.DONE) {\r\n\
					amount = +xhr1.responseText;\r\n\
				}\r\n\
			}\r\n\
		}\r\n\
		blockEditInput = false\r\n\
	}\r\n\
	function addButtonListener() {\r\n\
		if (blockEditInput == false) {\r\n\
			var ths = document.getElementsByTagName(\"th\");\r\n\
			var newTr = document.createElement(\'tr\');\r\n\
			newTr.innerHTML = \"<td><input style=\\\"width: \" + ths[0].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[1].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[2].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[3].offsetWidth + \";\\\"></td><td><input style=\\\"width: \" + ths[4].offsetWidth + \";\\\"></td>\";\r\n\
			document.getElementsByTagName(\'table\')[0].appendChild(newTr);\r\n\
			var inputs = document.getElementsByTagName(\"input\");\r\n\
			for (var i = 2; i < inputs.length; i++) {\r\n\
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
	function prevButtonListener() {\r\n\
		if (offset != 0 && blockEditInput == false) {\r\n\
			offset -= 5;\r\n\
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"select * from Гостиница;\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
		}\r\n\
	}\r\n\
	function nextButtonListener() {\r\n\
		if (offset + 5 < amount && blockEditInput == false) {\r\n\
			offset += 5;\r\n\
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"select * from Гостиница;\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
		}\r\n\
	}\r\n\
	function thClickListener() {\r\n\
		var colName = this.innerHTML.replace(/[/\\\\]/gi, '');\r\n\
		if (sort === \"null\") {\r\n\
			sort = colName;\r\n\
			order = false;\r\n\
		}\r\n\
		else if (sort == colName) {\r\n\
			order = ~order;\r\n\
		}\r\n\
		else {\r\n\
			sort = colName;\r\n\
			order = false;\r\n\
		}\r\n\
		if (colName === \"Дата заезда\") {\r\n\
			colName = \"\\\"\" + colName + \"\\\"\";\r\n\
		}\r\n\
		else if (colName === \"Дата отъезда\") {\r\n\
			colName = \"\\\"\" + colName + \"\\\"\";\r\n\
		}\r\n\
		xhrSend(\"select * from Гостиница;\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
	}\r\n\
	function search(e) {\r\n\
		if (e.keyCode == 13) {\r\n\
			searchVal = this.value;\r\n\
			if (searchVal == \"\") {\r\n\
				searchVal = \"null\";\r\n\
			}\r\n\
			offset = 0;\r\n\
			var colName = addQuotes(sort);\r\n\
			xhrSend(\"select * from Гостиница;\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			var xhr1 = new XMLHttpRequest();\r\n\
			xhr1.open(\'POST\', \'http://localhost/\', true);\r\n\
			xhr1.send(\"amount$\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
			xhr1.onreadystatechange = function() {\r\n\
				if (xhr1.readyState == XMLHttpRequest.DONE) {\r\n\
					amount = +xhr1.responseText;\r\n\
				}\r\n\
			}\r\n\
		}\r\n\
	}\r\n\
	function updateSearchValues() {\r\n\
		selectsearchVal = this.value;\r\n\
	}\r\n\
	function fileButtonListener() {\r\n\
		document.getElementById(\'fileinput\').click();\r\n\
	}\r\n\
	function getFile(e) {\r\n\
		var fileToLoad = document.getElementById(\"fileinput\").files[0];\r\n\
  		var fileReader = new FileReader();\r\n\
  		fileReader.onload = function(fileLoadedEvent){\r\n\
      		var textFromFileLoaded = fileLoadedEvent.target.result;\r\n\
      		var colName = addQuotes(sort);\r\n\
      		xhrSend(\"file\" + textFromFileLoaded + \"@\" + offset + \"/\" + limit + \"$\" + colName + \"~\" + transformBoolToOrder(order) + \".\" + selectsearchVal + \"!\" + searchVal + \"?\");\r\n\
  		};\r\n\
  		fileReader.readAsText(fileToLoad, \"UTF-8\");\r\n\
	}\r\n\
	function changePass() {\r\n\
		var str = prompt(\"Введите новый пароль\");\r\n\
		updatePass(str);\r\n\
	}\r\n\
</script>\r\n\
</body>\r\n\
</html>\r\n\
", request.out);

	        //закрыть текущее соединение 
	        //FCGX_Finish_r(&request); 
	        
	        //завершающие действия - запись статистики, логгирование ошибок и т.п.
		}
		else if (buf[0] == 'c' && buf[1] == 'h' && buf[2] == 'e') {
			char * query = malloc(300);
			strcpy(query, "user=oleg password=");
			strcat(query, buf + 5);
			strcat(query, " dbname=hw");
			conn = PQconnectdb(query);
			free(query);
			if (PQstatus(conn) == CONNECTION_BAD) {
    			//fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
    			printf("Connection to database failed: %s\n", PQerrorMessage(conn));
    			//do_exit(conn);
    			FCGX_PutS("\
Content-type: text/html\r\n\
\r\n\
no\r\n\
				", request.out);
			}
			else {
				FCGX_PutS("\
Content-type: text/html\r\n\
\r\n\
yes\r\n\
				", request.out);
			}
		}
		else if (buf[0] == 'n' && buf[1] == 'e' && buf[2] == 'w') {
			PGconn *postgres = PQconnectdb("user=postgres password=postgres dbname=hw");
			if (PQstatus(postgres) == CONNECTION_BAD) {
    			printf("Connection to database failed: %s\n", PQerrorMessage(postgres));
    			PQfinish(postgres);
    		}
    		else {
    			char * query = malloc(300);
    			strcpy(query, "alter user oleg with encrypted password \'");
    			strcat(query, buf + 7);
    			strcat(query, "\';");
    			PGresult *res = PQexec(postgres, query);
    			PQclear(res);
    			PQfinish(postgres);
    		}
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
Пароль:<input type=\"password\">\r\n\
<script>\r\n\
var answer;\r\n\
function xhrSend (s) {\r\n\
    var xhr = new XMLHttpRequest();\r\n\
    xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
    xhr.send(s);\r\n\
    xhr.onreadystatechange = function() {\r\n\
        if (xhr.readyState == XMLHttpRequest.DONE) {\r\n\
            document.open();\r\n\
            document.write(xhr.responseText);\r\n\
            document.close();\r\n\
        }\r\n\
    }\r\n\
}\r\n\
function getAnswer (s) {\r\n\
	var xhr = new XMLHttpRequest();\r\n\
    xhr.open(\'POST\', \'http://localhost/\', true);\r\n\
    xhr.send(s);\r\n\
    xhr.onreadystatechange = function() {\r\n\
        if (xhr.readyState == XMLHttpRequest.DONE) {\r\n\
            answer = xhr.responseText;\r\n\
            if (answer == \"no\\r\\n\\t\\t\\t\\t\") {\r\n\
				alert(\"Неверный пароль!\");\r\n\
			}\r\n\
			else if (answer == \"yes\\r\\n\\t\\t\\t\\t\") {\r\n\
				xhrSend(\"main\");\r\n\
			}\r\n\
        }\r\n\
    }\r\n\
}\r\n\
document.getElementsByTagName(\'input\')[0].addEventListener(\"keypress\", checkPass);\r\n\
function checkPass(e) {\r\n\
	if (e.keyCode == 13) {\r\n\
		getAnswer(\"check\" + document.getElementsByTagName(\"input\")[0].value + \"\\0\");\r\n\
	}\r\n\
}\r\n\
</script>\r\n\
</body>\r\n\
</html>\r\n\
			", request.out);
        } 
	}
	PQfinish(conn);
	return 0;
}