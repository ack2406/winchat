#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <algorithm>

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

struct Data {
    std::vector<SOCKET> *clients;
    SOCKET ClientSocket;
};


DWORD WINAPI handleClient(void *args) {
    Data *pData = (Data *) args;
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

            if (strncmp(recvbuf, "/tell", 5) == 0) {
                strncpy(name, recvbuf + 6, 4);
                name[3] = '\0';
                SOCKET sName = atoi(name);

                sprintf(name, "%d", ClientSocket);
                iSendResult = send(sName, name, (int) strlen(name), 0);
                if (iSendResult == SOCKET_ERROR) {
                    send(ClientSocket, "e01", 4, 0); // error for no user available
                    send(ClientSocket, "", 1, 0);
                } else {
                    send(sName, recvbuf, iResult, 0);
                }
            } else {
                for (unsigned long long s: *pData->clients) {
                    strcpy(name, "");
                    if (s != ClientSocket && s != 0) {
                        // Echo the buffer back to the sender
                        sprintf(name, "%d", ClientSocket);
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
            }
        } else if (iResult == 0) {
            printf("%d left the chat.\n", ClientSocket);
        }

    } while (iResult > 0);

    for (unsigned long long &client: *pData->clients) {
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

    for (int i = 0; i < 10000; i++) {
        auto ClientSocket = INVALID_SOCKET;

        // Accept a client socket
        ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket == INVALID_SOCKET) {
            continue;
        }
        clients.emplace_back(ClientSocket);

        Data *data = new Data;
        data->clients = &clients;
        data->ClientSocket = ClientSocket;
        CreateThread(nullptr, 0, handleClient, (void *) data, 0, &id);
    }

    return 0;
}