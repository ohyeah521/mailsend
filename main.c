#include "mailsend.h"
#include "stdio.h"

int main(int argc,char**argv)
{
	socket_init();
	SOCKET s = socket_tcp_connect("smtp.qq.com",25);
	printf("1 %d\n",s!=-1);
	int ret = smtp_login(s,"254203716@qq.com","");
	printf("2 %d\n",ret);
	while(1)
	{
		ret = smtp_mail_from_to_setting(s,"254203716@qq.com",
				"www.397379567@qq.com");
		printf("3 %d\n",ret);
		ret = smtp_data_start(s);
		ret = smtp_set_content(s,"呵呵","啦啦啦啦");
		printf("4 %d\n",ret);
		//ret = smtp_add_attachment(s,"2B5AB000.jpg","1.jpg",NULL);
		//ret = smtp_add_attachment(s,"2G__Disk.jpg","2.jpg",NULL);
		printf("5 %d\n",ret);
		ret = smtp_data_end(s);
		printf("6 %d\n",ret);
	}
	smtp_quit(s);
	socket_close(s);
	return 0;
}
