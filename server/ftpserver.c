#include <stdio.h>
#include "ftpserver.h"
 
char g_recvBuf[1024] = { 0 };      // �������տͻ�����Ϣ
int g_fileSize;                    // �ļ���С
char* g_fileBuf;                   // �����ļ�
char g_fileName[256];
int main(void)
{
    initSocket();
 
    listenToClient();
 
    closeSocket();
 
    return 0;
}
 
// ��ʼ��socket��
bool initSocket()
{
    WSADATA wsadata;
 
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)  // ����Э��,�ɹ�����0
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
void listenToClient()
{

    // ����server socket�׽��� ��ַ���˿ں�,AF_INET��IPV4
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (serfd == INVALID_SOCKET)
    {
        printf("socket faild:%d", WSAGetLastError());
        WSACleanup();
        return;
    }
 
    // ��socket��IP��ַ�Ͷ˿ں�
    struct sockaddr_in serAddr;
 
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(SPORT);             // htons�ѱ����ֽ���תΪ�����ֽ���
    serAddr.sin_addr.S_un.S_addr = ADDR_ANY;     // ����������������
 
    if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))
    {
        printf("bind faild:%d", WSAGetLastError());
        return;
    }
 
    // �����ͻ�������
    if (0 != listen(serfd, 10))                  // 10Ϊ���������
    {
        printf("listen faild:%d", WSAGetLastError());
        return;
    }
 
    // �пͻ������ӣ���������
    struct sockaddr_in cliAddr;
    int len = sizeof(cliAddr);
 
    SOCKET clifd = accept(serfd, (struct sockaddr*)&cliAddr, &len);
 
    if (INVALID_SOCKET == clifd)
    {
        printf("accept faild:%d", WSAGetLastError());
        return;
    }
 
    printf("���ܳɹ���\n");
 
        // ��ʼ������Ϣ
 
 
        while (processMag(clifd))
        {
            Sleep(200);
        }
 
}
 
// ������Ϣ
bool processMag(SOCKET clifd)
{
    // �ɹ�������Ϣ�����յ����ֽ��������򷵻�0
    int nRes = recv(clifd, g_recvBuf, 1024, 0);         // ����

    if (nRes <= 0)
    {
        printf("�ͻ�������...%d", WSAGetLastError());
        return false;
    }
 
    // ��ȡ���ܵĵ���Ϣ
    struct MsgHeader* msg = (struct MsgHeader*)g_recvBuf;
    struct MsgHeader exitmsg;
 
    /*
    *MSG_FILENAME       = 1,    // �ļ�����                ������ʹ��
    *MSG_FILESIZE       = 2,    // �ļ���С                �ͻ���ʹ��
    *MSG_READY_READ     = 3,    // ׼������                �ͻ���ʹ��
    *MSG_SENDFILE       = 4,    // ����                    ������ʹ��
    *MSG_SUCCESSED      = 5,    // �������                ���߶�ʹ��
    *MSG_OPENFILE_FAILD = 6     // ���߿ͻ����ļ��Ҳ���    �ͻ���ʹ��
     *
    */
    char inf[505]; //��Ż�ȡ�ı�����Ϣ
    memset(inf, 0, sizeof(inf));
    switch (msg->msgID)
    {
        case MSG_FILENAME:          // 1  ��һ�ν���
            printf("%s\n", msg->myUnion.fileInfo.fileName);
            readFile(clifd, msg);
            break;
        case MSG_SENDFILE:          // 4
            sendFile(clifd, msg);
            break;
        case MSG_SUCCESSED:         // 5

            exitmsg.msgID = MSG_SUCCESSED;

            if (SOCKET_ERROR == send(clifd, (char*)&exitmsg, sizeof(struct MsgHeader), 0))   //ʧ�ܷ��͸��ͻ���
            {
                printf("send faild: %d\n", WSAGetLastError());
                return false;
            }
            printf("��ɣ�\n");
            break;
        case MSG_CLIENTREADSENT: //7
            serverReady(clifd, msg);
            break;
        case MSG_CLIENTSENT:
            writeFile(clifd, msg);
            break;
        case MSG_PWD: //added by yxy
            getMessage(MSG_PWD, inf);
            sendMessage(clifd, inf);
            break;
        case MSG_LS: //added by yxy
            getMessage(MSG_LS, inf);
            sendMessage(clifd, inf);
            break;

    }
    return true;
}
 
/*
*1.�ͻ������������ļ� �����ļ������͸�������
*2.���������տͻ��˷��͵��ļ��� �������ļ����ҵ��ļ������ļ���С���͸��ͻ���
*3.�ͻ��˽��յ��ļ���С��׼����ʼ���ܣ������ڴ�  ׼�����Ҫ���߷��������Է�����
*4.���������ܵĿ�ʼ���͵�ָ�ʼ����
*5.��ʼ�������ݣ�������     ������ɣ����߷������������
*6.�ر�����
*/
void getMessage(int type, char inf[505]) {
    char path[105];
    DIR *dp;
    struct dirent *entry;

    switch(type)
    {
        case MSG_PWD:
            getcwd(path, sizeof(path));
            strcpy(inf, path);
            return;
        case MSG_LS:
            getcwd(path, sizeof(path));
            dp = opendir(path);
            while((entry = readdir(dp)) != NULL)
            {
                strcat(inf, entry->d_name);
                strcat(inf, "  ");
            }
            return;
    }
}
bool readFile(SOCKET clifd, struct MsgHeader* pmsg)
{
    FILE* pread = fopen(pmsg->myUnion.fileInfo.fileName, "rb");
 
    if (pread == NULL)
    {
        printf("�Ҳ���[%s]�ļ�...\n", pmsg->myUnion.fileInfo.fileName);
 
        struct MsgHeader msg;
        msg.msgID = MSG_OPENFILE_FAILD;                                             // MSG_OPENFILE_FAILD = 6
 
        if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))   // ʧ�ܷ��͸��ͻ���
        {
            printf("send faild: %d\n", WSAGetLastError());
        }
 
        return false;
    }
 
    // ��ȡ�ļ���С
    fseek(pread, 0, SEEK_END);
    g_fileSize = ftell(pread);
    fseek(pread, 0, SEEK_SET);
 
    // ���ļ���С�����ͻ���
    char text[100];
    char tfname[200] = { 0 };
    struct MsgHeader msg;
 
    msg.msgID = MSG_FILESIZE;                                       // MSG_FILESIZE = 2
    msg.myUnion.fileInfo.fileSize = g_fileSize;
 
    _splitpath(pmsg->myUnion.fileInfo.fileName, NULL, NULL, tfname, text);  //ֻ��Ҫ�������ּӺ�׺
 
    strcat(tfname, text);
    strcpy(msg.myUnion.fileInfo.fileName, tfname);
 
    send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);            // �ļ����ͺ�׺���ļ���С���ؿͻ���  ��һ�η��͸��ͻ���
 
    //��д�ļ�����
    g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
 
    if (g_fileBuf == NULL)
    {
        printf("�ڴ治�㣬����\n");
        return false;
    }
 
    fread(g_fileBuf, sizeof(char), g_fileSize, pread);
    g_fileBuf[g_fileSize] = '\0';
 
    fclose(pread);
    return true;
}

void sendMessage(SOCKET clifd, char* message) {
    struct MsgHeader msg;

    msg.msgID = MSG_RECV;
    strcpy(msg.myUnion.fileInfo.fileName, message);

    if (SOCKET_ERROR == send(clifd, (const char *)&msg, sizeof(struct MsgHeader), 0))
    {
        printf("message send error: %d\n", WSAGetLastError());
        return;
    }
}
bool sendFile(SOCKET clifd, struct MsgHeader* pms)
{
    struct MsgHeader msg;                                                     // ���߿ͻ���׼�������ļ�
    msg.msgID = MSG_READY_READ;
 
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
 
        if (SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0))  // ���߿ͻ��˿��Է���
        {
            printf("�ļ�����ʧ�ܣ�%d\n", WSAGetLastError());
        }
    }
 
    return true;
}
 
void serverReady(SOCKET clifd, struct MsgHeader* pmsg)
{
    g_fileSize = pmsg->myUnion.fileInfo.fileSize;
    char text[100];
    char tfname[200] = { 0 };
 
    _splitpath(pmsg->myUnion.fileInfo.fileName, NULL, NULL, tfname, text);  //ֻ��Ҫ�������ּӺ�׺
 
    strcat(tfname, text);
    strcpy(g_fileName, tfname);
    g_fileBuf = calloc(g_fileSize + 1, sizeof(char));         // ����ռ�
 
    if (g_fileBuf == NULL)
    {
        printf("�����ڴ�ʧ��\n");
    }
    else
    {
        struct MsgHeader msg;  
        msg.msgID = MSG_SERVERREAD;
 
        if (SOCKET_ERROR == send(clifd, (const char *)&msg, sizeof(struct MsgHeader), 0))   // �ڶ��η���
        {
            printf("�ͻ��� send error: %d\n", WSAGetLastError());
            return;
        }
    }
 
    printf("filename:%s    size:%d  \n", pmsg->myUnion.fileInfo.fileName, pmsg->myUnion.fileInfo.fileSize);
}
 
bool writeFile(SOCKET clifd, struct MsgHeader* pmsg)
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
 
        send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0);
 
        return false;
    }
 
    return true;
}