#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>

//#include "../fdblib.h"
#define FDBInterface

FDBInterface void socket_init();

FDBInterface SOCKET socket_tcp_connect(const char* host,int port);

FDBInterface int socket_tcp_senddata(SOCKET s,const char* data,int nsize,int count);

FDBInterface void socket_close(SOCKET s);

FDBInterface int smtp_session(SOCKET s,const char**send_list,const char**request_list);

FDBInterface int smtp_login(SOCKET s,const char* account,const char*password);

FDBInterface int smtp_data_start(SOCKET s);

FDBInterface int smtp_data_end(SOCKET s);

FDBInterface int smtp_quit(SOCKET s);

FDBInterface int smtp_mail_from_to_setting(SOCKET s,const char* src_email_address,const char* dst_email_address);

FDBInterface int smtp_set_content(SOCKET s,const char* Title,const char* Text);

FDBInterface int smtp_add_attachment(SOCKET s,const char* FilePath,const char* FileName, unsigned long long *currentSize);

#if defined(__cplusplus)
}
#endif
