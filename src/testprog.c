// vim: ts=4 expandtab ai
 
/*
 * Copyright (c) 2008, Henrik Torstensson <laban@kryo.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "sl500.h"

#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <time.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#define BUZZ_SIZE 1024
#define FAST 10
#define SLOW 50

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}


void shutdownn(int fd, int errorcode)
{
    printf("\nResetting communication speed to 19200 baud...\n");
    rf_init_com(fd, BAUD_19200);
    exit(errorcode);
}

void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

char * getLicense(const char* fromFile)
{
    char buff[BUZZ_SIZE];
    FILE *f = fopen(fromFile, "r");
    fgets(buff, BUZZ_SIZE, f);
}

int sendPostRequest(const char* url, int type, const char *token, unsigned int card_number)
{
    CURL *curl;
    int codeRet = -1;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL) {
        return 128;
    }
    struct string s;
    init_string(&s);

    char* jsonObj = "{ \"type\" : \"%d\" , \"cardNumber\" : \"%u\", \"token\" : \"%s\" }";
    cJSON *req = cJSON_CreateObject();
    cJSON *typeJson = cJSON_CreateNumber(type);
    cJSON_AddItemToObject(req, "type", typeJson);
    cJSON *cardNumberJson = cJSON_CreateNumber(card_number);
    cJSON_AddItemToObject(req, "cardNumber", cardNumberJson);
    cJSON *tokenJson = cJSON_CreateString(token);
    cJSON_AddItemToObject(req, "token", tokenJson);

    char *strJson = cJSON_Print(req);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strJson);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcrp/0.1");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    curl_easy_perform(curl);

    printf("data: %s\n", s.ptr);
    cJSON *res = cJSON_Parse(s.ptr);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(res, "error");
    if (cJSON_IsString(error) && (error->valuestring != NULL))
    {
        // cJSON *message = cJSON_GetObjectItemCaseSensitive(res, "message");
        printf("Error request: %s\n", error->valuestring);
        goto end;
    }

    cJSON *code = cJSON_GetObjectItemCaseSensitive(res, "code");
    if (!cJSON_IsNumber(code))
        {
            goto end;
        }
	else
		{
			codeRet = code->valueint;

		}

    end:
    free(s.ptr);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    cJSON_Delete(req);
    return codeRet;
}
void inputSuccess(int fd)
{
	rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(100);
        rf_light(fd, LED_OFF);
        delay(100);
        rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(100);
        rf_light(fd, LED_OFF);
        delay(1000);

}

void serverFail(int fd)
{
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
}

void checkinoutSuccess(int fd)
{
	rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(300);
        rf_light(fd, LED_OFF);
}

void checkinoutFail(int fd)
{
	rf_beep(fd, SLOW);
	rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
}

void registedSuccess(int fd)
{
	rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(300);
        rf_light(fd, LED_OFF);
	delay(300);
        rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(300);
        rf_light(fd, LED_OFF);
        delay(300);
        rf_beep(fd, FAST);
        rf_light(fd, LED_GREEN);
        delay(300);
        rf_light(fd, LED_OFF);
}

void nocard(int fd)
{
	rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
        delay(300);
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
        delay(300);
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
}

void deviceInvalid(int fd)
{ 
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
        delay(300);
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
        delay(300);
        rf_beep(fd, SLOW);
        rf_light(fd, LED_RED);
        delay(300);
        rf_light(fd, LED_OFF);
}
int main()
{
    int fd;
    uint8_t dev_id[2], buf[100];
    uint8_t new_dev_id[] = {0x13, 0x1a};
    uint8_t key[6];
    uint8_t status;
    char printbuf[100];
    int block, i, pos, authed;
    unsigned int card_no;

    /* Set up serial port */
    fd = open_port();

    printf("\nSpeeding up communication to 115200 baud...\n");
    rf_init_com(fd, BAUD_115200);

    rf_get_model(fd, sizeof(buf), buf);
    printf("Model: %s\n", buf);

    /* START, MIFARE COMMANDS */
    while (1)
    {
	rf_light(fd, LED_OFF);
        status = rf_request(fd);
        status = rf_anticoll(fd, &card_no);
        if (status != 0)
        {
//            printf("ERROR %d\n", status);
            // shutdown(fd, status);
        }
        else{
	    	char *license = getLicense("license/RFIDLicense.txt");
		char *tok = strtok(license, ";");
		char url[BUZZ_SIZE];
		strcpy(url, tok);
//		fgets(url, BUZZ_SIZE, tok);
		printf("\nurl is: %s\n", url);
		tok = strtok(NULL, ";");
                char token[BUZZ_SIZE];
		strcpy(token, tok);
  //              fgets(token, BUZZ_SIZE, tok);
		printf("token is: %s\n", token);
		printf("Card number: %u (0x%08x)\n", card_no, card_no);

 		inputSuccess(fd);

		int code = sendPostRequest(url, 0, token, card_no);
		printf("code response is: %d\n", code);

		switch(code){
			case -1:
				serverFail(fd);break;
			case 0:
				checkinoutSuccess(fd);break;
			case 2:
				checkinoutFail(fd);break;
			case 10:
				registedSuccess(fd);break;
			case 90:
				nocard(fd);break;
			case 99:
				deviceInvalid(fd);break;
		}
	}
    }

    shutdownn(fd, 0);
}
