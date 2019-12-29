#include "stdafx.h"
using namespace std;
#define CURL_STATICLIB
#include "mongoose.h"
#include <curl/curl.h>
#include<string>

static const char *s_http_port = "2708";
string CurlResBuffer;

void getContentsFromBody(char *RequestData, string &LocalVarName, string ClientVarName) {
	string res;
	int i = 0, j;
	while (RequestData[i] != '$') {
		res = "";
		for (j = i; j < i + ClientVarName.length(); j++) {
			if (isalpha(RequestData[j]) != 0 || isdigit(RequestData[j]) != 0 || RequestData[j] == '_') {
				res += RequestData[j];
			}
		}
		if (ClientVarName == res) {
			res = "";
			j++;
			while (RequestData[j] != ';') {
				res += RequestData[j];
				j++;
			}
			LocalVarName = res;
			break;
		}
		i++;
	}
}

size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	CurlResBuffer.append((char*)ptr, size*nmemb);
	return size*nmemb;
}

static void processClientRequest(struct mg_connection *nc, struct http_message *hm, int ev, void *ev_data) {
	string MethodName,SearchWord,response, s_url;
	int IsValidRequest;
	/* Send headers */
	mg_printf
	(
		nc, "%s",
		"HTTP/1.1 200 OK\r\n"
		"Access-Control-Allow-Headers: *\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
	);

	char *RequestData = (char*)hm->body.p;
	IsValidRequest = 0;
	for (int i = 0; i < strlen(RequestData); i++) {
		if (RequestData[i] == '$')
			IsValidRequest = 1;
	}
	response="None";
	if (IsValidRequest == 1) {
		getContentsFromBody(RequestData, MethodName, "MethodName");
		if (MethodName == "search") {
			getContentsFromBody(RequestData, SearchWord, "SearchWord");
			s_url = "http://superheroapi.com/api.php/2407204939543779/" + MethodName + "/" + SearchWord;
		}
		else {
			s_url = "http://superheroapi.com/api.php/2407204939543779/" + MethodName;
		}
		CURL *curl = curl_easy_init();
		CurlResBuffer.clear();
		curl_easy_setopt(curl, CURLOPT_URL, &*s_url.begin());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (CurlResBuffer != "")
			response = CurlResBuffer;
	}
	mg_printf_http_chunk(nc, "%s", &*response.begin());
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	struct http_message *hm = (struct http_message *) ev_data;
	switch (ev) {
		case MG_EV_HTTP_REQUEST:
			if (mg_vcmp(&hm->uri, "/api/processClientRequest") == 0) {
				processClientRequest(nc, hm, ev, ev_data); /* Handle RESTful call */
			}
		break;
		default:
		break;
	}
}

void main() {
	struct mg_connection *nc;
	struct mg_mgr mgr;
	struct mg_bind_opts bind_opts;
	const char *err_str;

	mg_mgr_init(&mgr, NULL);

	/* Set HTTP server options */
	memset(&bind_opts, 0, sizeof(bind_opts));
	bind_opts.error_string = &err_str;
	#if MG_ENABLE_SSL
		if (ssl_cert != NULL) {
			bind_opts.ssl_cert = ssl_cert;
		}
	#endif
	nc = mg_bind_opt(&mgr, s_http_port, ev_handler, bind_opts);
	if (nc == NULL) {
		fprintf(stderr, "\nError Starting Server on port %s: %s\n", s_http_port,
			*bind_opts.error_string);
		exit(1);
	}
	mg_set_protocol_http_websocket(nc);
	printf("\n\t*******************************************************************");
	printf("\n\n\t\t C++ Web Server is Running Sucsessfully on port %s\n", s_http_port);
	printf("\n\t*******************************************************************");
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
}
