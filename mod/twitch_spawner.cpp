#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <winsock2.h> // Or standard BSD sockets

#pragma comment(lib, "ws2_32.lib")

std::queue<std::string> spawnQueue;
std::mutex queueMutex;

// Forward declaration of Twilight Princess actor creation function
// (Exported or mapped via Dusklight's internal actor system)
extern "C" int fopAcM_create(s16 procName, u32 parameters, void* pos, int roomNo, void* rot, void* scale, s8 parentPrm);

void ListenForTwitchCommands() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Local port for Twitch bot
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 1);

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        char buffer[256] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            std::lock_guard<std::mutex> lock(queueMutex);
            spawnQueue.push(std::string(buffer));
        }
        closesocket(clientSocket);
    }
}

// Called once per frame inside Dusklight's main game loop update
void OnGameUpdate(void* playerPosition) {
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!spawnQueue.empty()) {
        std::string command = spawnQueue.front();
        spawnQueue.pop();

        if (command == "spawn_moblin") {
            // ID for Moblin/Bokoblin in TP decomp actor registry
            s16 moblinId = 0x0123; 
            fopAcM_create(moblinId, 0, playerPosition, -1, nullptr, nullptr, 0);
        } else if (command == "spawn_heart") {
            s16 heartId = 0x0001;
            fopAcM_create(heartId, 0, playerPosition, -1, nullptr, nullptr, 0);
        }
    }
}

void InitTwitchMod() {
    std::thread(ListenForTwitchCommands).detach();
}