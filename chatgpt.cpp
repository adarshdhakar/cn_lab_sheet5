#include <bits/stdc++.h>
#include <netinet/in.h>
#include <pthread.h>
using namespace std;

#define pb push_back
#define f(n) for(int i=0;i<(n);i++)

const int N = 256;
const int TIMEOUT_DURATION = 6000; // 6 seconds

void error(const char *msg) {
    perror(msg);
    exit(1);
}

atomic<int> client_index(-1);
vector<double> timeout(256);
set<int> active_clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread safety

struct ClientData {
    int idx;
    int socket_fd;
};

void *timeoutCheck(void *arg) {
    ClientData *temp = (ClientData *)arg;
    
    while (true) {
        auto now = chrono::high_resolution_clock::now();
        double current_time = chrono::duration<double>(now.time_since_epoch()).count();
        
        pthread_mutex_lock(&clients_mutex);
        if (current_time - timeout[temp->idx] >= TIMEOUT_DURATION) {
            active_clients.erase(temp->socket_fd);
            cout << "Client " << temp->socket_fd << " timed out.\n";
            close(temp->socket_fd);
            pthread_mutex_unlock(&clients_mutex);
            free(temp);  // Free allocated memory
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&clients_mutex);
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void *Clients(void *arg) {
    int newsockfd = *(int *)arg;
    
    pthread_mutex_lock(&clients_mutex);
    client_index++;
    int idx = client_index;
    active_clients.insert(newsockfd);
    pthread_mutex_unlock(&clients_mutex);

    char buffer[N];
    bzero(buffer, N);
    int n = read(newsockfd, buffer, N - 1);
    if (n < 0) error("ERROR reading from socket");

    string name(buffer);
    string message = "\n" + name + " joined the chat.\n";
    message += "\nActive Clients: " + to_string(active_clients.size()) + "\n";
    cout << message << endl;

    pthread_mutex_lock(&clients_mutex);
    for (int sockfd : active_clients) {
        if (sockfd != newsockfd) {
            write(sockfd, message.c_str(), message.size());
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    auto now = chrono::high_resolution_clock::now();
    timeout[idx] = chrono::duration<double>(now.time_since_epoch()).count();

    ClientData *client_data = (ClientData *)malloc(sizeof(ClientData));
    client_data->idx = idx;
    client_data->socket_fd = newsockfd;

    pthread_t timeCheck;
    pthread_create(&timeCheck, NULL, timeoutCheck, client_data);
    pthread_detach(timeCheck);

    while (true) {
        bzero(buffer, N);
        n = read(newsockfd, buffer, N - 1);
        if (n < 0) error("ERROR reading from socket");

        string msg(buffer);
        if (msg == "exit") {
            pthread_mutex_lock(&clients_mutex);
            active_clients.erase(newsockfd);
            pthread_mutex_unlock(&clients_mutex);

            msg = "\n" + name + " left the chat.\n";
            msg += "\nActive Clients: " + to_string(active_clients.size()) + "\n";
            cout << msg << endl;

            pthread_mutex_lock(&clients_mutex);
            for (int sockfd : active_clients) {
                write(sockfd, msg.c_str(), msg.size());
            }
            pthread_mutex_unlock(&clients_mutex);

            close(newsockfd);
            pthread_exit(NULL);
        }

        msg = "\n" + name + ": " + msg + "\n";
        cout << msg << endl;

        pthread_mutex_lock(&clients_mutex);
        for (int sockfd : active_clients) {
            if (sockfd != newsockfd) {
                write(sockfd, msg.c_str(), msg.size());
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        now = chrono::high_resolution_clock::now();
        timeout[idx] = chrono::duration<double>(now.time_since_epoch()).count();
    }
}

int main(int argc, char *argv[]) { 
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    int portno = atoi(argv[1]);
    struct sockaddr_in serv_addr, cli_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    cout << "Active Clients = 0" << endl;

    while (true) {
        listen(sockfd, 5);
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");

        pthread_mutex_lock(&clients_mutex);
        if (active_clients.find(newsockfd) != active_clients.end()) {
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, Clients, &newsockfd);
        pthread_detach(thread_id);
    }

    return 0;
}
