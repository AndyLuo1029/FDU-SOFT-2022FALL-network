#include <stdio.h>
#include <stdlib.h>       
#include "ftpclient.h"   
 
char g_fileName[256];     // ������������͹������ļ���
char* g_fileBuf;          // ���ܴ洢�ļ�����
char g_recvBuf[1024];     // ������Ϣ������
int g_fileSize;           // �ļ��ܴ�С
 
int main(void)
{
    initSocket();
 
    connectToHost();
 
    closeSocket();
 
    return 0;
}
 
// ��ʼ��socket��
bool initSocket()
{
    WSADATA wsadata;
 
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))        // ����Э��,�ɹ�����0
    {
        printf("WSAStartup faild: %d\n", WSAGetLastError());
        return false;
    }
 
    return true;
}
 
// �ر�socket��
bool closeSocket()
{
    if (0 != WSACleanup())
    {
        printf("WSACleanup faild: %d\n", WSAGetLastError());
        return false;
    }
 
    return true;
}
 
// �����ͻ�������
void connectToHost()
{
    // ����server socket�׽��� ��ַ���˿ں�,AF_INET��IPV4
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (INVALID_SOCKET == serfd)
    {
        printf("socket faild:%d", WSAGetLastError());
        return;
    }
 
    // ��socket��IP��ַ�Ͷ˿ں�
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(SPORT);                       // htons�ѱ����ֽ���תΪ�����ֽ���
    serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // ��������IP��ַ
 
    // ���ӵ�������
    if (0 != connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
    {
        printf("connect faild:%d", WSAGetLastError());
        return;
    }
    printf("���ӳɹ���\n");
    if(!login(serfd)) {
        printf("���Դ����ﵽ���ޣ���������Ͽ�����");
        closesocket(serfd);
        return;
    }


    printf("��¼�ɹ���\n");
    printf("***************************************\n");
    printf("1.�����ļ��������\n");
    printf("2.�ӷ����ȡ�ļ�\n");
    printf("3.�˳�����\n");
    printf("4.pwd\n");
    printf("***************************************\n");
    while (1)
    {    
        int flag;
        do {
            printf("ftp >>");
            scanf("%d", &flag);
        } while (!(flag == 1 || flag == 2 || flag == 3 || flag == 4 || flag == 5));

        if (flag == 1)
        {
            printf("���ڿ�ʼ�����˴����ļ�");
            clientReadySend(serfd);
            while(processMag(serfd))
            {}
        }
        else if(flag == 2)
        {
            printf("���ڿͻ��˿�ʼ�����ļ�\n");
            downloadFileName(serfd);// ��ʼ������Ϣ,100Ϊ������Ϣ���
            while (processMag(serfd))
            {}
        }
        else if(flag == 4) {
            requestPwd(serfd);
            while (processMag(serfd))
            {}
        }
        else if(flag == 5) {
            requestLs(serfd);
            while (processMag(serfd))
            {}
        }
        else
        {
            printf("ϵͳҪ�˳���...\n");
            closesocket(serfd);
            return;
        }
//        printf("\nPress Any Key To Continue:\n");
//        getchar();
    }
}
 
// ������Ϣ
bool processMag(SOCKET serfd)
{
 
    recv(serfd, g_recvBuf, 1024, 0);                     // �յ���Ϣ   
    struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
 
    /*
    *MSG_FILENAME       = 1,       // �ļ�����                ������ʹ��
    *MSG_FILESIZE       = 2,       // �ļ���С                �ͻ���ʹ��
    *MSG_READY_READ     = 3,       // ׼������                �ͻ���ʹ��
    *MSG_SENDFILE       = 4,       // ����                    ������ʹ��
    *MSG_SUCCESSED      = 5,       // �������                ���߶�ʹ��
    *MSG_OPENFILE_FAILD = 6        // ���߿ͻ����ļ��Ҳ���    �ͻ���ʹ��
    */
 
    switch (msg->msgID)
    {
    case MSG_OPENFILE_FAILD:         // 6
        downloadFileName(serfd);
        break;
    case MSG_FILESIZE:               // 2  ��һ�ν���
        readyread(serfd, msg);
        break;
    case MSG_READY_READ:             // 3
        writeFile(serfd, msg);
        break;
    case MSG_SUCCESSED:              // 5
        printf("������ɣ�\n");
        return false;
        break;
    case MSG_SERVERREAD:
        printf("׼���������");
        sendFile(serfd, msg);
        break;

    case MSG_RECV:                  //added by yxy
        readMessage(msg);
        return false;
    }

    return true;
}
bool login(SOCKET serfd) {
    char username[30]; //TODO ���ȼ���
    char password[30];
    struct MsgHeader send_msg;
    struct MsgHeader* rec_msg;
    send_msg.msgID = MSG_LOGIN;

    while(true) {
        //�����û�������
        printf("username >>");
        scanf("%s", username);
        printf("password >>");
        scanf("%s", password);
        strcpy(send_msg.myUnion.fileInfo.fileName, username);
        strcat(send_msg.myUnion.fileInfo.fileName, " ");
        strcat(send_msg.myUnion.fileInfo.fileName, password);
        send(serfd, (char*)&send_msg, sizeof(struct MsgHeader), 0);
        //���ռ����

        recv(serfd, g_recvBuf, 1024, 0);
        rec_msg = (struct MsgHeader*)g_recvBuf;
//        printf("%s\n",rec_msg->myUnion.fileInfo.fileName);
        if(!strcmp(rec_msg->myUnion.fileInfo.fileName, "Success")) return true;
        else if(!strcmp(rec_msg->myUnion.fileInfo.fileName, "ReachMax")) return false;
        else {
            printf("�˺Ż��������������\n");
        }
    }
}
void readMessage(struct MsgHeader* pmsg) {

    char *message = pmsg->myUnion.fileInfo.fileName;
    printf("%s\n", message);

}
void requestPwd(SOCKET serfd) {
    struct MsgHeader msg;
    msg.msgID = MSG_PWD;
    send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
}
void requestLs(SOCKET serfd) {
    struct MsgHeader msg;
    msg.msgID = MSG_LS;
    send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
}
void downloadFileName(SOCKET serfd)
{
    char fileName[1024];
    struct MsgHeader file;
 
    printf("�������ص��ļ�����");
 
    scanf("%s", fileName);                              // �����ļ�·��               
    file.msgID = MSG_FILENAME;                               // MSG_FILENAME = 1
    strcpy(file.myUnion.fileInfo.fileName, fileName);
    send(serfd, (char*)&file, sizeof(struct MsgHeader), 0);  // ���͡�IP��ַ�����ݡ�����    ��һ�η��͸�������
}
 
void readyread(SOCKET serfd, struct MsgHeader* pmsg)
{
    // ׼���ڴ� pmsg->fileInfo.fileSize
    g_fileSize = pmsg->myUnion.fileInfo.fileSize;
    strcpy(g_fileName, pmsg->myUnion.fileInfo.fileName);
 
    g_fileBuf = calloc(g_fileSize + 1, sizeof(char));         // ����ռ�
 
    if (g_fileBuf == NULL)
    {
        printf("�����ڴ�ʧ��\n");
    }
    else
    {
        struct MsgHeader msg;  // MSG_SENDFILE = 4
        msg.msgID = MSG_SENDFILE;
 
        if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))   // �ڶ��η���
        {
            printf("�ͻ��� send error: %d\n", WSAGetLastError());
            return;
        }
    }
 
    printf("size:%d  filename:%s\n", pmsg->myUnion.fileInfo.fileSize, pmsg->myUnion.fileInfo.fileName);
}
 
bool writeFile(SOCKET serfd, struct MsgHeader* pmsg)
{
    if (g_fileBuf == NULL)
    {
        return false;
    }
 
    int nStart = pmsg->myUnion.packet.nStart;
    int nsize = pmsg->myUnion.packet.nsize;
 
    memcpy(g_fileBuf + nStart, pmsg->myUnion.packet.buf, nsize);    // strncmpyһ��
    printf("packet size:%d %d\n", nStart + nsize, g_fileSize);
 
    if (nStart + nsize >= g_fileSize)                       // �ж������Ƿ�������
    {
        FILE* pwrite;
        struct MsgHeader msg;
 
        pwrite = fopen(g_fileName, "wb");
        msg.msgID = MSG_SUCCESSED;
 
        if (pwrite == NULL)
        {
            printf("write file error...\n");
            return false;
        }
 
        fwrite(g_fileBuf, sizeof(char), g_fileSize, pwrite);
        fclose(pwrite);
 
        free(g_fileBuf);
        g_fileBuf = NULL;
 
        send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
 
        return false;
    }
 
    return true;
}
 
void clientReadySend(SOCKET serfd)
{
    struct MsgHeader msg;
    msg.msgID = MSG_CLIENTREADSENT;
    char fileName[1024] = { 0 };
    printf("������Ҫ�ϴ����ļ�����");
    scanf("%s", fileName);
    FILE* pread = fopen(fileName, "rb");
 
    fseek(pread, 0, SEEK_END);
    g_fileSize = ftell(pread);
    fseek(pread, 0, SEEK_SET);
 
    strcpy(msg.myUnion.fileInfo.fileName, fileName);
    msg.myUnion.fileInfo.fileSize = g_fileSize;
 
    send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
    g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
 
    if (g_fileBuf == NULL)
    {
        printf("�ڴ治�㣬����\n");
    }
 
    fread(g_fileBuf, sizeof(char), g_fileSize, pread);
    g_fileBuf[g_fileSize] = '\0';
 
    fclose(pread);
}
 
 
bool sendFile(SOCKET serfd, struct MsgHeader* pms)
{
    struct MsgHeader msg;                                                     // ���߿ͻ���׼�������ļ�
    msg.msgID = MSG_CLIENTSENT;
 
    // ����ļ��ĳ��ȴ���ÿ�����ݰ��ܴ��͵Ĵ�С��1012������ô�÷ֿ�
    for (size_t i = 0; i < g_fileSize; i += PACKET_SIZE)                       // PACKET_SIZE = 1012
    {
        msg.myUnion.packet.nStart = i;
 
        // ���Ĵ�С���������ݵĴ�С
        if (i + PACKET_SIZE + 1 > g_fileSize)
        {
            msg.myUnion.packet.nsize = g_fileSize - i;
        }
        else
        {
            msg.myUnion.packet.nsize = PACKET_SIZE;
        }
 
        memcpy(msg.myUnion.packet.buf, g_fileBuf + msg.myUnion.packet.nStart, msg.myUnion.packet.nsize);
 
        if (SOCKET_ERROR == send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0))  // ���߿ͻ��˿��Է���
        {
            printf("�ļ�����ʧ�ܣ�%d\n", WSAGetLastError());
        }
    }
 
    return true;
}