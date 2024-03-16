#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <locale>
#include <limits> // Для использования std::numeric_limits
#include <ios>    // Для использования std::streamsize

constexpr int BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
        return 1;
    }

    const char* serverIp = argv[1];
    int serverPort = std::stoi(argv[2]);

    // Initialize client socket
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    char buffer[BUFFER_SIZE];

    std::string nickname;
    std::cout << "Enter your nickname: ";
    std::getline(std::cin, nickname);

    // Send nickname to server
    sendto(clientSocket, nickname.c_str(), nickname.size(), 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

    // Receive response from server
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t recv_len = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
    std::cout << buffer << std::endl;

    // Main game loop
    while (true) {
        // Receive response from server
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
        std::cout << "Server response: " << buffer << std::endl;

        if (std::string(buffer) == "exit") {
            std::cout << "Your opponent have surrendered...\n";

            // Receive response from server
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t recv_len = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
            std::cout << buffer << std::endl;

            break;
        }

        std::string city;
        std::cout << "Enter a city name: ";
        std::getline(std::cin, city);

        // Send city to server
        sendto(clientSocket, city.c_str(), city.size(), 0, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

        // Receive response from server
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t recv_len = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
        std::cout << "Server response: " << buffer << std::endl;
        buffer[recv_len] = '\0';

        if (std::string(buffer) == "exit") {
            std::cout << "Exiting the game...\n";

            // Receive response from server
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t recv_len = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, nullptr, nullptr);
            std::cout << buffer << std::endl;

            break;
        }
    }

    // Close socket
    close(clientSocket);

    return 0;
}
