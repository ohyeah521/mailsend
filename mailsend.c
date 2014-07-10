#include "mailsend.h"


//***********************************************Base64 加解密模块***********************************************//

//将6位bit字节转换为Base64字符
static char base64_string[]= {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

//将Base64字符转换为6位bit字节,若非Base64字符,返回-1
static int base64_decode_char(char c)
{
    int i;
    if(c=='\0'||c=='=')return -1;
    for(i=0; i<sizeof(base64_string)/sizeof(*base64_string); ++i)
    {
        if(c==base64_string[i])
        {
            return i;
        }
    }
    return -1;
}

/*Base64编码:
    对in中数据进行base64编码,结果放入out中
    [out]参数out:指向存放编码后Base64编码的缓冲区
    [in] 参数in:指向存放待编码的数据缓冲区
    [in] 参数in_len:表示待编码数据的长度
    return: 返回Base64编码长度
*/
int base64_encode(char* out, const char* in,int in_len)
{
    int i_out = 0;
    int i_bit = 0;
    int i_in = 0;
    out[i_out] = 0 ;
    for (i_in = 0; i_in < in_len; ++i_in)//对输入buf进行遍历
    {
        char c = in[i_in];
        int char_bit = 8;
        while(--char_bit>=0)
        {
            out[i_out] |= ((c&0X80)!=0);//最高位放入
            ++i_bit;
            c <<= 1;

            if( (i_bit)%6==0 )//某个字节已经填满
            {
                out[i_out] = base64_string[out[i_out]];
                ++i_out;//跳转到下一个字节
                out[i_out] = 0 ;//清零
            }
            else //未被填满 继续左移
            {
                out[i_out] <<= 1;
            }
        }
    }

    //不足3n个字符,进行末尾处理
    if( ( (i_out)%4)!=0 )
    {
        //最末字符的处理
        while( (++i_bit)%6!=0 )
        {
            out[i_out] <<= 1;
        }

        out[i_out] = base64_string[out[i_out]];

        //填 '=' 符号
        while( ( (i_out)%4)!=0 )
        {
            ++i_out;
            out[i_out] = '=';
        }
    }
    out[i_out+1]='\0';
    return i_out;
}

/*Base64解码:
    对in中数据进行base64解码,结果放入out中
    [out]参数out:指向存放解码后数据的缓冲区
    [in] 参数in:指向存放Base64编码的数据缓冲区
    [in] 参数in_len:表示Base64编码数据的长度
    return: 返回解码后数据的长度
*/
int base64_decode(char* out, const char* in,int in_len)
{
    int i_out = 0;
    int i_bit = 0;
    int i_in = 0;
    out[i_out] = 0 ;
    for (i_in = 0; i_in < in_len; ++i_in)//对输入buf进行遍历
    {
        int char_bit = 6;
        char c = base64_decode_char(in[i_in]);
        if(c==-1)return i_out; //遇到结束符 '\0' 或者 '\='
        while(--char_bit>=0)
        {
            out[i_out] |= ((c&0X20)!=0);//最高位放入
            ++i_bit;
            c <<= 1;

            if( (i_bit)%8==0 )//某个字节已经填满
            {
                ++i_out;//跳转到下一个字节
                out[i_out] = 0 ;
                continue;
            }
            else //未被填满 继续左移
            {
                out[i_out] <<= 1;
            }
        }
    }
    return i_out;
}
//***********************************************Base64 加解密模块结束***********************************************//

/*
 * 初始化网络套接字
 */
void socket_init()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
}

/*通过网络地址字符串获得32位网络地址值
 * [in] name:网络地址字符串.
 * return: 32位网络地址值,如果错误,返回-1
 */
long socket_gethost(const char* name)
{
    long i = -1;
    if(name!=NULL)
    {
        struct hostent *host = gethostbyname(name);
        if(host&&host->h_addr)
        {
            i = *(long *)(host->h_addr);
            return i;
        }
    }
    return i;
}

/*建立TCP连接
 * [in] host:待连接的主机域名或者ip
 * [in] port:待连接端口
 * return: 返回套接字,错误返回-1
 */
SOCKET socket_tcp_connect(const char* host,int port)
{
    SOCKET s;
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = socket_gethost(host);

    if(sa.sin_addr.s_addr==-1)return -1;
    s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==-1)return -1;
    if(connect(s,(struct sockaddr*)&sa,sizeof(sa))==-1)
    {
        closesocket(s);
        return -1;
    }
    return s;
}

/*通过TCP套接字发送数据
 * [in] s:套接字
 * [in] data:数据缓冲区
 * [in] nsize:单个数据块大小,一次就发送一个数据块,
 * [in] count:发送数据块个数
 * return: 发送成功返回发送数据块个数,否则0
 */
int socket_tcp_senddata(SOCKET s,const char* data,int nsize,int count)
{
    int i;
    if(s==-1||data==NULL)return 0;
    for(i=0;i<count;++i)
    {
        int ret = send(s,data+i*nsize,nsize,0);
        if(ret<=0)return 0; //send failed
    }
    return i;//send successful
}

/*关闭套接字
 */
void socket_close(SOCKET s)
{
    closesocket(s);
}

/************************ SMTP **********************/

static const char *mime_header =
        "mime-version: 1.0\r\n"
        "content-type: multipart/mixed;\r\n"
        "\tboundary=\"----=_001_nextpart_=----\"\r\n\r\n"
        "this is a multi-part message in mime format.\r\n\r\n"
        "------=_001_nextpart_=----\r\n"
        "content-type: multipart/alternative;\r\n"
        "\tboundary=\"----=_002_nextpart_=----\"\r\n\r\n"
        "------=_002_nextpart_=----\r\n"
        "content-type: text/plain;\r\n"
        "\tcharset=\"utf-8\"\r\n"
        "content-transfer-encoding: base64\r\n\r\n";


static const char *boundary1 = "------=_001_nextpart_=------";
static const char *boundary2 = "------=_002_nextpart_=------";

static const char *subject = "subject: ";
static const char *Content_1 =
        "Content-Type: application/octet-stream;\r\n"
        "\tname=";

static const char *Content_2 =
        "Content-Transfer-Encoding: base64\r\n"
        "Content-Disposition: attachment;\r\n"
        "\tfilename=";

/*完成SMTP的会话
 * [in] s: 套接字
 * [in] send_list: 发送数据列表,指向一个以'\0'结尾的ASCII字符串列表,列表以NULL结尾
 * [in] request_list: 接收列表,指向一个以'\0'结尾的ASCII字符串列表,接收数据中若找到request_list中的标志字符串,表示会话成功
 * return: 返回执行成功的会话个数
 */
int smtp_session(SOCKET s,const char**send_list,const char**request_list)
{
    int i;
    if(send_list==NULL)return 0;
    for(i=0;send_list[i]!=NULL;++i)
    {
        char buf[8192]={0};
        const char *pStr = send_list[i];
        if(pStr!=NULL)
        {
            int ret = socket_tcp_senddata(s,pStr,strlen(pStr),1);//发送
            if(ret<=0)
            {
                break;
            }
            if(request_list!=NULL && request_list[i]!=NULL)
            {
                //接收
                int recv_size = recv(s,buf,sizeof(buf),0);
                if(recv_size<=0 || strstr(buf,request_list[i])==NULL)
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    return i;
}

/*SMTP登录
 * [in] s:套接字
 * [in] base64_account:base64编码过的账户名字符串
 * [in] base64_password:base64编码过的密码字符串
 * return: 成功返回1,否则0
 */
int smtp_login(SOCKET s,const char* account,const char* password)
{
    char buf[8192];

    char base64_account[512];
    char base64_password[512];
    int len_account = strlen(account);
    int len_password= strlen(password);

    const char* attrib_list[]=
    {
        "HELO HEHE\r\n",
        "AUTH LOGIN\r\n",
        base64_account,                         //base64_encode(账号)
        "\r\n",
        base64_password,                    //base64_encode(密码), 相当于明文
        "\r\n",
        NULL
    };
    const char* request_list[]=
    {
        "250",
        "334",
        NULL,
        "334",
        NULL,
        "235",
        NULL
    };

    int ret ;

    recv(s,buf,8192,0);//清除数据

    if(len_account>256)len_account = 256;
    if(len_password>256)len_password = 256;
    base64_encode(base64_account,account,len_account);
    base64_encode(base64_password,password,len_password);

    ret = smtp_session(s,attrib_list,request_list);
    if(ret==6)
    {
        return 1;
    }
    return 0;
}

/*SMTP邮件源电邮地址以及目的电邮地址设置
 * [in] s:套接字
 * [in] src_email_address:源电邮地址
 * [in] dst_email_address:目的电邮地址
 */
int smtp_mail_from_to_setting(SOCKET s,const char* src_email_address,const char* dst_email_address)
{
    char src_email[256]={0};
    char dst_email[256]={0};

    const char* attrib_list[]=
    {
        src_email,
        dst_email,
        NULL
    };
    const char* recv_list[]=
    {
        "250",
        "250",
    };

    if(src_email_address==NULL || dst_email_address==NULL)return 0;
    sprintf(src_email,"MAIL FROM: <%s>\r\n",src_email_address);
    sprintf(dst_email,"RCPT TO: <%s>\r\n",dst_email_address);

    if(smtp_session(s,attrib_list,recv_list)==2)
    {
        return 1;
    }
    return 0;
}

/*SMTP 邮件正文开始
 * [in] s:套接字
 * return: 1成功,0失败
 */
int smtp_data_start(SOCKET s)
{
    const char* attrib_list[]=
    {
        "DATA\r\n\r\n",
        NULL
    };
    const char* recv_list[]=
    {
        "354"
    };
    if(smtp_session(s,attrib_list,recv_list)==1)
    {
        return 1;
    }
    return 0;
}

/*SMTP 邮件正文结束
 * [in] s:套接字
 * return: 1成功,0失败
 */
int smtp_data_end(SOCKET s)
{
    const char* attrib_list[]=
    {
        "\r\n.\r\n",
        NULL
    };
    const char* recv_list[]=
    {
        "250"
    };
    socket_tcp_senddata(s,boundary1,strlen(boundary1),1);
    socket_tcp_senddata(s,"\r\n",2,1);
    if(smtp_session(s,attrib_list,recv_list)==1)
    {
        return 1;
    }
    return 0;
}

/*SMTP 退出登录
 * [in] s:套接字
 * return: 1成功,0失败
 */
int smtp_quit(SOCKET s)
{
    const char *data = "QUIT\r\n\r\n";
    int ret = socket_tcp_senddata(s,data,strlen(data),1);
    if(ret>0)ret = 1;
    return ret;
}

/*SMTP 正文开始
 * [in] s:套接字
 * [in] Title:标题
 * [in] Text:正文
 * return: 成功为1,失败为0
 */
int smtp_set_content(SOCKET s,const char* Title,const char* Text)
{
    int ret;
    if(s!=-1&&Title!=NULL&&Text!=NULL)
    {
        int title_length = strlen(Title);
        char *subject_buf = (char*)malloc(sizeof(char)*title_length*2);
        char *send_buf = (char*)malloc(sizeof(char)*title_length*2+100);
        base64_encode(subject_buf,Title,title_length);
        sprintf(send_buf,"%s=?utf-8?B?%s?=\r\n",subject,subject_buf);
        {
            int send_length = strlen(send_buf);
            int ret = socket_tcp_senddata(s,send_buf,send_length,1);
            int mime_header_length = strlen(mime_header);
            ret = socket_tcp_senddata(s,mime_header,mime_header_length,1);
            free(subject_buf);
            free(send_buf);
        }
    }
    else
        return 0;

    {
        int text_length = strlen(Text);
        char *base64_Text = (char*)malloc(sizeof(char)*text_length*2);
        int encode_length = base64_encode(base64_Text,Text,text_length);
        ret = socket_tcp_senddata(s,base64_Text,encode_length,1);
        ret = socket_tcp_senddata(s,"\r\n\r\n",4,1);
        ret = socket_tcp_senddata(s,boundary2,strlen(boundary2),1);
        ret = socket_tcp_senddata(s,"\r\n\r\n",4,1);
        free(base64_Text);
    }
    return ret;
}

/*SMTP 添加附件
 * [in] s:套接字
 * [in] FilePath: 附件路径
 * [in] FileName: 附件文件名
 * return: 成功返回1,失败返回0
 */
int smtp_add_attachment(SOCKET s,const char* FilePath,const char* FileName,unsigned long long *currentSize)
{
    FILE* readFile = NULL;
    if(s==-1 || FilePath==NULL || FileName==NULL)return 0;
    readFile = fopen(FilePath,"rb");
    if(readFile!=NULL)
    {
        //头部
        {
            int filename_length = strlen(FileName);
            char *FileName_buf = (char*)malloc(sizeof(char)*filename_length*2);
            char *Send_buf = (char*)malloc(sizeof(char)*(filename_length*4 + 500) );
            base64_encode(FileName_buf,FileName,filename_length);
            sprintf(Send_buf,
                    "%s\"=?utf-8?B?%s?=\"\r\n"
                    "%s\"=?utf-8?B?%s?=\"\r\n\r\n",
                    Content_1,FileName_buf,
                    Content_2,FileName_buf);
            socket_tcp_senddata(s,boundary1,strlen(boundary1)-2,1);
            socket_tcp_senddata(s,"\r\n",2,1);
            socket_tcp_senddata(s,Send_buf,strlen(Send_buf),1);
            free(Send_buf);
            free(FileName_buf);
        }

        //Base64数据开始
        {
            char *Send_buf = (char*)malloc(8192*2*sizeof(char));
            char *Read_buf = (char*)malloc(6144*2*sizeof(char));

            unsigned long long totalread= 0 ;
            int read_size = 0;
            while(read_size = fread(Read_buf,1,6144,readFile),
                  read_size>0)
            {
                int send_size =
                        base64_encode(Send_buf,Read_buf,read_size);
                int ret =
                        socket_tcp_senddata(s,Send_buf,send_size,1);
                totalread += read_size;
                if(currentSize!=NULL)
                {
                    *currentSize = totalread;
                }
                if(ret<=0 && read_size!=6144)
                {
                    break;
                }
            }

            socket_tcp_senddata(s,"\r\n\r\n",4,1);
            free(Send_buf);
            free(Read_buf);
        }
        fclose(readFile);

        return 1;
    }
    return 0;
}
