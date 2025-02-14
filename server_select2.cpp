#include <bits/stdc++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <chrono>
using namespace std;

// Constants
const int N = 256;
const int TIMEOUT_DURATION = 120; // seconds

// Global data structures (all used in the single main thread)
set<int> active_clients;                  // set of connected client sockets
map<int, string> sockToName;              // mapping from socket fd to client name
map<string, int> nameToSockfd;            // mapping from client name to socket fd
map<int, double> last_active;             // mapping from socket fd to last active time (in seconds since epoch)
map<string, set<int>> rooms;              // mapping from room name to set of member socket fds

// Utility: get current time in seconds (as double)
double getCurrentTime() {
    auto now = chrono::high_resolution_clock::now();
    return chrono::duration<double>(now.time_since_epoch()).count();
}

// Broadcast a message to all active clients except (optionally) one socket
void broadcastMessage(const string &msg, int exclude_sock = -1) {
    for (int sockfd : active_clients) {
        if (sockfd != exclude_sock) {
            if (write(sockfd, msg.c_str(), msg.size()) < 0) {
                perror("ERROR writing to socket");
            }
        }
    }
}

// Remove a client from all data structures and close its socket.
void removeClient(int sockfd, const string &reason = "") {
    string name = sockToName[sockfd];
    active_clients.erase(sockfd);
    sockToName.erase(sockfd);
    // Remove from name lookup as well.
    if (nameToSockfd.find(name) != nameToSockfd.end()) {
        nameToSockfd.erase(name);
    }
    last_active.erase(sockfd);
    // Remove from any rooms the client may be in.
    for (auto &room_pair : rooms) {
        room_pair.second.erase(sockfd);
    }
    
    // Optionally, broadcast a message if a reason is provided.
    if (!reason.empty()) {
        string exitMsg = "\n" + name + " " + reason + "\n";
        exitMsg += "\nActive Clients: " + to_string(active_clients.size()) + "\n";
        broadcastMessage(exitMsg, sockfd);
    }
    
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    cout << "Closed connection for " << name << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    
    // Create listener socket.
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    
    // Allow address reuse.
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    
    // Bind socket.
    int portno = atoi(argv[1]);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(listener, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    
    // Listen for connections.
    if (listen(listener, 10) < 0) {
        perror("ERROR on listen");
        exit(1);
    }
    
    cout << "Server started on port " << portno << ". Waiting for connections..." << endl;
    
    // Set up the master file descriptor set and add the listener.
    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    FD_SET(listener, &master);
    int fdmax = listener;
    
    // Main loop using select()
    while (true) {
        read_fds = master;  // copy master to read_fds
        
        // Set timeout for select: 1 second to check for client timeouts.
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(fdmax + 1, &read_fds, nullptr, nullptr, &tv);
        if (activity < 0) {
            perror("select");
            exit(1);
        }
        
        // Check if there's a new connection on the listener.
        if (FD_ISSET(listener, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int newfd = accept(listener, (struct sockaddr *)&cli_addr, &clilen);
            if (newfd < 0) {
                perror("ERROR on accept");
            } else {
                // Add the new socket to the master set and active_clients.
                FD_SET(newfd, &master);
                if (newfd > fdmax) {
                    fdmax = newfd;
                }
                active_clients.insert(newfd);
                last_active[newfd] = getCurrentTime();
                
                // Read the client's name immediately.
                char buffer[N];
                memset(buffer, 0, N);
                int n = read(newfd, buffer, N - 1);
                if (n <= 0) {
                    // If unable to read name, close the connection.
                    close(newfd);
                    FD_CLR(newfd, &master);
                    continue;
                }
                string name(buffer);
                // Save the client's name.
                sockToName[newfd] = name;
                nameToSockfd[name] = newfd;
                
                // Broadcast join message.
                string joinMsg = "\n" + name + " joined the chat.\n";
                joinMsg += "\nActive Clients: " + to_string(active_clients.size()) + "\n";
                broadcastMessage(joinMsg, newfd);
                cout << joinMsg;
            }
        }
        
        // Check all client sockets for incoming data.
        for (int sockfd = 0; sockfd <= fdmax; sockfd++) {
            if (sockfd == listener) continue;
            
            if (FD_ISSET(sockfd, &read_fds)) {
                char buffer[N];
                memset(buffer, 0, N);
                int n = read(sockfd, buffer, N - 1);
                if (n <= 0) {
                    // Client disconnected or error
                    string name = sockToName[sockfd];
                    cout << name << " disconnected.\n";
                    removeClient(sockfd, "left the chat.");
                    FD_CLR(sockfd, &master);
                } else {
                    // Process received message.
                    string msg(buffer);
                    // Message format: "receiver#content"
                    int hashInd = msg.find("#");
                    if (hashInd == string::npos)
                        continue;
                    
                    string receiver = msg.substr(0, hashInd);
                    string content = msg.substr(hashInd + 1);
                    string sender = sockToName[sockfd];
                    
                    // Update last active timestamp.
                    last_active[sockfd] = getCurrentTime();
                    
                    // Handle exit command.
                    if (content == "exit") {
                        string exitMsg = "\n" + sender + " left the chat.\n";
                        exitMsg += "\nActive Clients: " + to_string(active_clients.size() - 1) + "\n";
                        write(sockfd, "BYE\n", 4);
                        removeClient(sockfd, "left the chat.");
                        FD_CLR(sockfd, &master);
                        broadcastMessage(exitMsg, sockfd);
                        continue;
                    }
                    
                    // Check for room-related commands:
                    // "join:room_name" to join, "leave:room_name" to leave, "room:room_name" to send room message.
                    if (receiver.rfind("join:", 0) == 0) { // starts with "join:"
                        string roomName = receiver.substr(5);
                        rooms[roomName].insert(sockfd);
                        string joinRoomMsg = "\n" + sender + " joined room " + roomName + ".\n";
                        // Notify all members in the room.
                        for (int member : rooms[roomName]) {
                            if (member != sockfd)
                                write(member, joinRoomMsg.c_str(), joinRoomMsg.size());
                        }
                        continue;
                    }
                    else if (receiver.rfind("leave:", 0) == 0) { // starts with "leave:"
                        string roomName = receiver.substr(6);
                        if (rooms.find(roomName) != rooms.end())
                            rooms[roomName].erase(sockfd);
                        string leaveRoomMsg = "\n" + sender + " left room " + roomName + ".\n";
                        for (int member : rooms[roomName]) {
                            write(member, leaveRoomMsg.c_str(), leaveRoomMsg.size());
                        }
                        continue;
                    }
                    else if (receiver.rfind("room:", 0) == 0) { // starts with "room:"
                        string roomName = receiver.substr(5);
                        string roomMsg = "\n" + sender + " (room:" + roomName + "): " + content + "\n";
                        if (rooms.find(roomName) != rooms.end()) {
                            for (int member : rooms[roomName]) {
                                if (member != sockfd)
                                    write(member, roomMsg.c_str(), roomMsg.size());
                            }
                        }
                        continue;
                    }
                    // Broadcast to all if receiver is "All".
                    else if (receiver == "All") {
                        string broadcastMsg = "\n" + sender + ": " + content + "\n";
                        broadcastMessage(broadcastMsg, sockfd);
                        continue;
                    }
                    // Otherwise, treat as a private message (one-to-one).
                    else {
                        string privateMsg = "\n" + sender + ": " + content + "\n";
                        if (nameToSockfd.find(receiver) != nameToSockfd.end()) {
                            int targetSock = nameToSockfd[receiver];
                            write(targetSock, privateMsg.c_str(), privateMsg.size());
                        }
                        continue;
                    }
                }
            }
        }
        
        // After processing I/O, check for client timeouts.
        double now = getCurrentTime();
        vector<int> toRemove;
        for (auto &pair : last_active) {
            int sockfd = pair.first;
            double lastTime = pair.second;
            if (now - lastTime >= TIMEOUT_DURATION) {
                toRemove.push_back(sockfd);
            }
        }
        for (int sockfd : toRemove) {
            string name = sockToName[sockfd];
            cout << "Client " << name << " timed out.\n";
            string timeoutMsg = "\n" + name + " timed out and was disconnected.\n";
            removeClient(sockfd, "timed out.");
            FD_CLR(sockfd, &master);
            broadcastMessage(timeoutMsg, sockfd);
        }
    }
    
    // Should never reach here.
    close(listener);
    return 0;
}
