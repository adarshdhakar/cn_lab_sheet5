#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

const int N = 256;
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <port>\n";
        exit(EXIT_FAILURE);
    }

    int portno = atoi(argv[1]);
    int sockfd, newsockfd, maxfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    fd_set master_set, read_fds;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    maxfd = sockfd;

    map<int, string> client_names;

    cout << "Server is running on port " << portno << endl;

    while (true) {
        read_fds = master_set;

        int activity = select(maxfd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) error("ERROR on select");

        for (int i = 0; i <= maxfd; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                    clilen = sizeof(cli_addr);
                    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                    if (newsockfd < 0) error("ERROR on accept");

                    FD_SET(newsockfd, &master_set);
                    maxfd = max(maxfd, newsockfd);

                    char name_buffer[N];
                    bzero(name_buffer, N);
                    ssize_t name_len = read(newsockfd, name_buffer, N - 1);
                    if (name_len <= 0) {
                        close(newsockfd);
                        FD_CLR(newsockfd, &master_set);
                    } else {
                        client_names[newsockfd] = string(name_buffer);
                        cout << client_names[newsockfd] << " joined the chat." << endl;

                        string join_msg = client_names[newsockfd] + " joined the chat.\n";
                        for (int j = 0; j <= maxfd; ++j) {
                            if (j != sockfd && j != newsockfd && FD_ISSET(j, &master_set)) {
                                write(j, join_msg.c_str(), join_msg.size());
                            }
                        }
                    }
                } else {
                    char buffer[N];
                    bzero(buffer, N);
                    ssize_t n = read(i, buffer, N - 1);

                    if (n <= 0) {
                        cout << client_names[i] << " left the chat." << endl;
                        string exit_msg = client_names[i] + " left the chat.\n";

                        for (int j = 0; j <= maxfd; ++j) {
                            if (j != sockfd && j != i && FD_ISSET(j, &master_set)) {
                                write(j, exit_msg.c_str(), exit_msg.size());
                            }
                        }

                        close(i);
                        FD_CLR(i, &master_set);
                        client_names.erase(i);
                    } else {
                        string msg = client_names[i] + ": " + string(buffer) + "\n";
                        cout << msg;

                        for (int j = 0; j <= maxfd; ++j) {
                            if (j != sockfd && j != i && FD_ISSET(j, &master_set)) {
                                write(j, msg.c_str(), msg.size());
                            }
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
