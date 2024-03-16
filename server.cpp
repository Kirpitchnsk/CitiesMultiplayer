#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <locale>
#include <limits> // Для использования std::numeric_limits
#include <ios>    // Для использования std::streamsize
#include <map>    // Для использования std::map

constexpr int BUFFER_SIZE = 1024;

// Function to load cities from file
std::vector<std::string> loadCitiesFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::string> cities;
    std::string city;
    while (file >> city) {
        cities.push_back(city);
    }

    file.close();
    return cities;
}

// Function to load cities from file
std::map<std::string, int> getDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::map<std::string, int> scores;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return scores;
    }

    std::string name;
    int score;
    while (file >> name >> score) {
        scores[name] = score;
    }

    file.close();
    return scores;
}

// Function to find a free port
int findFreePort() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    socklen_t len = sizeof(addr);
    getsockname(sockfd, reinterpret_cast<sockaddr*>(&addr), &len);

    int port = ntohs(addr.sin_port);

    close(sockfd);

    return port;
}

std::map<std::string, int> sortMap(const std::map<std::string, int>& inputMap) {
    // Создаем временный вектор пар ключ-значение из исходной map
    std::vector<std::pair<std::string, int>> tempVec(inputMap.begin(), inputMap.end());

    // Сортируем вектор по значению (int)
    std::sort(tempVec.begin(), tempVec.end(),
              [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                  return a.second <= b.second;
              });

    // Создаем новую map и заполняем ее отсортированными данными из временного вектора
    std::map<std::string, int> sortedMap;
    for (const auto& pair : tempVec) {
        sortedMap[pair.first] = pair.second;
    }

    return sortedMap;
}

bool hasKey(const std::map<std::string, int>& myMap, const std::string& key) {
    return myMap.find(key) != myMap.end();
}


// Function to write player scores to file
void writePlayerScoresToFile(const std::string& filename, const std::map<std::string, int>& playerScores) {
    std::map<std::string, int> previousScores = getDataFromFile(filename);

    std::ofstream scoreFile(filename, std::ios::trunc); // Режим добавления в конец файла
    if (!scoreFile.is_open()) {
        std::cerr << "Error: Could not open file for writing.\n";
        return;
    }

    for (const auto& pair : playerScores) {
        const std::string& nickname = pair.first;
        int score = pair.second;

        if (hasKey(previousScores, nickname)) {
            if (score > previousScores.at(nickname)) {
                scoreFile << nickname << " " << score << "\n";
            } else {
                // Записываем предыдущий результат, если текущий не лучше
                scoreFile << nickname << " " << previousScores.at(nickname) << "\n";
            }
        } else {
            scoreFile << nickname << " " << score << "\n";
        }
    }

    scoreFile.close();
}

int main() {
    // Load cities from file
    std::vector<std::string> cities = loadCitiesFromFile("files/citiesen.txt");

    // Initialize server socket
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }

    // Find a free port
    int port = findFreePort();
    std::cout << "Server started on port: " << port << std::endl;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    // Bind socket to port
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "Error: Could not bind socket\n";
        close(serverSocket);
        return 1;
    }

    // Wait for two players to connect
    std::cout << "Waiting for players to connect...\n";
    std::vector<sockaddr_in> clientAddrs(2);
    socklen_t clientLen = sizeof(clientAddrs[0]);

    char buffer[BUFFER_SIZE];
    std::string lastCity;
    int currentPlayer = 0; // Индекс текущего игрока

    std::vector<std::string> nicknames; // Список ников подключившихся клиентов
    std::map<std::string, int> playerScores; // Карта для отслеживания количества правильно отвеченных городов каждым игроком

    std::string prevousResults;
    playerScores = sortMap(getDataFromFile("files/scores.txt"));

    // Receive data from clients
    for (int i = 0; i < 2; ++i) {
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&clientAddrs[i]), &clientLen);
        std::string clientNickname(buffer);
        nicknames.push_back(clientNickname);

        int count = 0;
        for (const auto& pair : playerScores) {
            if (count >= 10) {
                break;
            }
            prevousResults += pair.first + " " + std::to_string(pair.second) + "\n";
            count++;
        }

        if(hasKey(playerScores,clientNickname)) {
            prevousResults += "Your best record is " + std::to_string(playerScores[clientNickname]) + " cities\n";
        }
        
        if (i == 0) {
            std::cout << "Player 1 (" << clientNickname << ") connected\n";
        } else {
            std::cout << "Player 2 (" << clientNickname << ") connected\n";
        }

        sendto(serverSocket, prevousResults.c_str(), prevousResults.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[i]), clientLen);

        prevousResults = "";
    }

    for (const auto& nickname : nicknames) {
        playerScores[nickname] = 0; // Инициализируем счет игрока
    }

    std::vector<std::string> usedCities;
    std::string receivedCity; // Объявляем переменную вне блоков if-else

    // Main game loop
    while (true) {
        if (currentPlayer == 0) {
            // Ожидание хода игрока 1
            std::string response;
            if (!lastCity.empty()) response = "Player 1's turn, your opponent named: " + lastCity;
            else response = "Player 1's turn, next city must start with different letter";
            sendto(serverSocket, response.c_str(), response.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);

            // Получение города от игрока 1
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t recv_len = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), &clientLen);
            buffer[recv_len] = '\0';
            std::cout << "Received from client: " << buffer << std::endl;
            receivedCity = std::string(buffer);
            receivedCity[recv_len] = '\0';

            // Проверка корректности города игрока 1
            if (receivedCity == "exit") {
                // Обработка выхода из игры
                std::cout << "Player 1 (" << nicknames[0] << ") has left the game.\n";

                std::string answer;
                // Вывод результатов игры
                for (const auto& nickname : nicknames) {
                    std::cout << "Player " << nickname << " score: " << playerScores[nickname] << " cities\n";
                    answer += "Player " + nickname + " score: " + std::to_string(playerScores[nickname]) + " cities\n";
                }

                sendto(serverSocket, "exit", 4, 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
                sendto(serverSocket, "exit", 4, 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
                sendto(serverSocket, answer.c_str(), answer.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
                sendto(serverSocket, answer.c_str(), answer.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
                break;
            } else if (std::find(cities.begin(), cities.end(), receivedCity) != cities.end() &&
                std::find(usedCities.begin(), usedCities.end(), receivedCity) == usedCities.end() &&
                (lastCity.empty() || std::tolower(receivedCity.front(),std::locale()) == std::tolower(lastCity.back(),std::locale()))) {
                lastCity = receivedCity;
                usedCities.push_back(receivedCity);
                currentPlayer = 1;
                ++playerScores[nicknames[0]]; // Увеличиваем счет игрока 1
                sendto(serverSocket, "Wait for second player turn", 27, 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
            } else {
                sendto(serverSocket, "Invalid city! Try again.", 24, 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
            }
        } else {
            // Ожидание хода игрока 2
            std::string response = "Player 2's turn, your opponent named: " + lastCity;
            sendto(serverSocket, response.c_str(), response.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);

            // Получение города от игрока 2
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t recv_len = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), &clientLen);
            buffer[recv_len] = '\0';
            std::cout << "Received from client: " << buffer << std::endl;
            receivedCity = std::string(buffer);
            receivedCity[recv_len] = '\0';

            // Проверка корректности города игрока 2
            if (receivedCity == "exit") {
                // Обработка выхода из игры
                std::cout << "Player 2 (" << nicknames[1] << ") has left the game.\n";

                std::string answer;
                // Вывод результатов игры
                for (const auto& nickname : nicknames) {
                    std::cout << "Player " << nickname << " score: " << playerScores[nickname] << " cities\n";
                    answer += "Player " + nickname + " score: " + std::to_string(playerScores[nickname]) + " cities\n";
                }

                sendto(serverSocket, "exit", 4, 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
                sendto(serverSocket, "exit", 4, 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
                sendto(serverSocket, answer.c_str(), answer.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[0]), clientLen);
                sendto(serverSocket, answer.c_str(), answer.size(), 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
                break;
            } else if (std::find(cities.begin(), cities.end(), receivedCity) != cities.end() &&
                std::find(usedCities.begin(), usedCities.end(), receivedCity) == usedCities.end() &&
                (lastCity.empty() || std::tolower(receivedCity.front(),std::locale()) == std::tolower(lastCity.back(),std::locale()))) {
                lastCity = receivedCity;
                usedCities.push_back(receivedCity);
                currentPlayer = 0;
                ++playerScores[nicknames[1]]; // Увеличиваем счет игрока 2
                sendto(serverSocket, "Wait for second player turn", 27, 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
            } else {
                sendto(serverSocket, "Invalid city! Try again.", 24, 0, reinterpret_cast<sockaddr*>(&clientAddrs[1]), clientLen);
            }
        }
    }

    // Запись результатов в файл
    writePlayerScoresToFile("files/scores.txt", playerScores);

    // Close socket
    close(serverSocket);

    return 0;
}
