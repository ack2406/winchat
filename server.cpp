#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <algorithm>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

struct Data {
    std::vector<SOCKET>* clients;
    SOCKET ClientSocket;
};


DWORD WINAPI handleClient(void* args) {
    Data* pData = (Data *) args;
    SOCKET ClientSocket = pData->ClientSocket;
    int iResult;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    char name[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    printf("%d joined the chat.\n", ClientSocket);

    // Receive until the peer shuts down the connection
    do {
        strcpy(recvbuf, "");
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            if (strcmp(recvbuf, "/join") != 0) {
                printf("%d: %s\n", ClientSocket, recvbuf);

            }
            for (unsigned long long s : *pData->clients) {
                strcpy(name, "");
                if (s != ClientSocket && s != 0) {
                    // Echo the buffer back to the sender
                    sprintf(name, "%d", ClientSocket);
                    //printf("co wysylam: %s, %d\n",name, (int) strlen(name));

                    send(s, name, (int) strlen(name), 0);
                    iSendResult = send(s, recvbuf, iResult, 0);

                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(ClientSocket);
                        WSACleanup();
                        return 1;
                    }
                }
            }

        } else if (iResult == 0) {
            printf("%d left the chat.\n", ClientSocket);
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    for (unsigned long long & client : *pData->clients) {
        if (client == ClientSocket) {
            client = 0;
        }
    }

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    return 0;
}

int main() {
    WSADATA wsaData;
    int iResult;

    auto ListenSocket = INVALID_SOCKET;


    struct addrinfo *result = nullptr;
    struct addrinfo hints{};



    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    DWORD id;

    std::vector<SOCKET> clients;

    while (true) {
        auto ClientSocket = INVALID_SOCKET;

        // Accept a client socket
        ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        clients.emplace_back(ClientSocket);


        Data* data = new Data;
        data->clients = &clients;
        data->ClientSocket = ClientSocket;
        CreateThread(nullptr, 0, handleClient, (void *) data, 0, &id);


    }



    // No longer need server socket
    closesocket(ListenSocket);

    WSACleanup();

    return 0;
}