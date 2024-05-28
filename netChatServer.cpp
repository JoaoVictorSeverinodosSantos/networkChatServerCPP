#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

std::vector<int> clients;
std::mutex clientsMutex;

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            close(clientSocket);
            clientsMutex.lock();
            clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
            clientsMutex.unlock();
            break;
        }
        clientsMutex.lock();
        for (int client : clients) {
            if (client != clientSocket) {
                send(client, buffer, bytesReceived, 0);
            }
        }
        clientsMutex.unlock();
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed\n";
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Listening failed\n";
        return 1;
    }
    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
        if (clientSocket < 0) {
            std::cerr << "Acceptance failed\n";
            return 1;
        }

        clientsMutex.lock();
        clients.push_back(clientSocket);
        clientsMutex.unlock();

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
