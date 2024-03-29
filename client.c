#include "client.h"

int main(int argc, char const *argv[]) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[1024];
    pthread_t thread_id;
    struct sockaddr_in servaddr;
    setvbuf(stdout, NULL, _IONBF, 0);
    if(sock == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        return errno;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(PORT);
    if(connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0){
        fprintf(stderr, "%s\n",strerror(errno));
        return errno;
    }
    pthread_create(&thread_id, NULL, afficheAll, &sock);
    while (true) {
        memset(buffer, 0x0 , 1024);
        fgets(buffer, 1024, stdin);
        send(sock, buffer, strlen(buffer),0);
    }
    return 0;
}

void *afficheAll(void *sock){
    int s=*(int *)sock;
    char buff[256] = {0};
    while (true){
        memset(buff, 0x0, 256);
        recv(s,buff, 256, 0);
        printf("%s",buff);
    }
}

