#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")  /* WinSock使用的库函数 */

/* 定义常量 */
#define FTP_DEF_PORT        21     /* 连接的缺省端口 */
#define FTP_BUF_SIZE      1024     /* 缓冲区的大小 */
#define FTP_FILENAME_LEN   256     /* 文件名长度 */

void ftp_get(SOCKET soc, char* filename){

    FILE *file;
    file = fopen(filename, "rb+");
    if(file == NULL){
        printf("[FTP] The file [%s] is not existed\n", filename);
        exit(1);
    }

    int file_len;
    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    int read_len;
    char read_buf[FTP_BUF_SIZE];
    do /* 发送文件文件*/
    {
        read_len = fread(read_buf, sizeof(char), FTP_BUF_SIZE, file);

        if (read_len > 0)
        {
            send(soc, read_buf, read_len, 0);
            file_len -= read_len;
        }
    } while ((read_len > 0) && (file_len > 0));

    fclose(file);

}

void ftp_put(SOCKET soc, char *file_name){

    FILE *file_ftp;
    file_ftp = fopen(file_name, "w+");
    if(file_ftp == NULL){
        printf("[FTP] The file [%s] is not existed\n", file_name);
        exit(1);
    }

    int result = 0;
    char data_buf[FTP_BUF_SIZE];
    do /* 接收响应并保存到文件中 */
    {
        result = recv(soc, data_buf, FTP_BUF_SIZE, 0);
        if (result > 0)
        {
            fwrite(data_buf, sizeof(char), result, file_ftp);

            /* 在屏幕上输出 */
            data_buf[result] = 0;
            printf("%s", data_buf);
        }
    } while(result > 0);

    fclose(file_ftp);

}
/**

*/
int ftp_send_response(SOCKET soc, char *buf, int buf_len){

    buf[buf_len] = 0;
    printf("The Server received: '%s' cmd from client \n", buf);
    //get filename
    char file_name[FTP_FILENAME_LEN];
    strcpy(file_name, buf+4);
    //文件下载
    if (strncmp(buf,"get",3)==0)  ftp_get(soc, file_name);
    //文件上传
    else if (strncmp(buf,"put",3)==0)  ftp_put(soc, file_name);
    else printf("the commod is not found\n");
    return 0;
}




int main(int argc, char **argv){
    WSADATA wsa_data;

    SOCKET	srv_soc = 0, acpt_soc;  /* socket 句柄 */
    struct sockaddr_in serv_addr;   /* 服务器地址  */
    struct sockaddr_in from_addr;   /* 客户端地址  */
    char recv_buf[FTP_BUF_SIZE];
    unsigned short port = FTP_DEF_PORT;
    int from_len = sizeof(from_addr);
    int	result = 0, recv_len;

    if (argc == 2) /* 端口号 */
        port = atoi(argv[1]);


    WSAStartup(MAKEWORD(2,0), &wsa_data); /* 初始化 WinSock 资源 */

    srv_soc = socket(AF_INET, SOCK_STREAM, 0); /* 创建 socket */
    if (srv_soc == INVALID_SOCKET)
    {
        printf("[FTP] socket() Fails, error = %d\n", WSAGetLastError());
        return -1;
    }

    /* 服务器地址 */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    result = bind(srv_soc, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (result == SOCKET_ERROR) /* 绑定失败 */
    {
        closesocket(srv_soc);
        printf("[FTP Server] Fail to bind, error = %d\n", WSAGetLastError());
        return -1;
    }

    result = listen(srv_soc, SOMAXCONN);
    printf("[FTP] The server is running ... ...\n");
	//printf("%d\n",SOMAXCONN);
	while (1)
    {
        acpt_soc = accept(srv_soc, (struct sockaddr *) &from_addr, &from_len);
        if (acpt_soc == INVALID_SOCKET) /* 接受失败 */
        {
            printf("[FTP] Fail to accept, error = %d\n", WSAGetLastError());
            break;
        }

        printf("[FTP] Accepted address:[%s], port:[%d]\n",
            inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));

        recv_len = recv(acpt_soc, recv_buf, FTP_BUF_SIZE, 0);
        char temp[1] ={'a'};
        send(acpt_soc, temp, 1, 0);
        //getchar();
        if (recv_len == SOCKET_ERROR) /* 接收失败 */
        {
           // getchar();
            closesocket(acpt_soc);
            printf("[FTP] Fail to recv, error = %d\n", WSAGetLastError());
            break;
        }

        recv_buf[recv_len] = 0;
        /* 向客户端发送响应数据 */
        result = ftp_send_response(acpt_soc, recv_buf, recv_len);
        closesocket(acpt_soc);
    }

    closesocket(srv_soc);
    WSACleanup();
    printf("[FTP] The server is stopped.\n");
    return 0;
}

