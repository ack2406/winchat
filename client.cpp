#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <string>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

DWORD WINAPI receiver(void* args) {
    auto ConnectSocket = (SOCKET) args;
    char recvbuf[DEFAULT_BUFLEN];
    char name[DEFAULT_BUFLEN];
    int buflen = DEFAULT_BUFLEN;
    int iResult;
    // Receive until the peer closes the connection
    do {
        recv(ConnectSocket, name, 4, 0);

        iResult = recv(ConnectSocket, recvbuf, buflen, 0);

        if (strcmp(recvbuf, "/exit") == 0) {
            printf("%s left the chat.\n", name);
        }
        else if (strcmp(recvbuf, "/join") == 0) {
            printf("%s joined the chat.\n", name);
        }
        else if (recvbuf[0] == '/') {
            continue;
        }
        else {
            if (iResult > 0)
                printf("%s: %s\n", name, recvbuf);
            else if (iResult == 0)
                printf("Connection closed\n");
            else
                continue;
            //printf("recv failed with error: %d\n", WSAGetLastError());
        }


    } while (iResult > 0);

    return 0;
}


int main(int argc, char **argv) {
    WSADATA wsaData;
    auto ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = nullptr,
            *ptr = nullptr,
            hints{};
    int iResult;


    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    int buflen = DEFAULT_BUFLEN;

    DWORD id;
    CreateThread(nullptr, 0, receiver, (void *) ConnectSocket, 0, &id);

    iResult = send(ConnectSocket, "/join", buflen, 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    do {
        strcpy(sendbuf,"");
        fgets(sendbuf,sizeof(sendbuf),stdin);
        sendbuf[strcspn(sendbuf, "\n")] = 0;

        //printf("sent: %s\n", sendbuf);
        iResult = send(ConnectSocket, sendbuf, buflen, 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        //printf("Bytes Sent: %ld\n", iResult);

    } while (strcmp(sendbuf, "/exit") != 0);


    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }






    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}