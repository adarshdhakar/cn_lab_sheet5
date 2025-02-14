#include <bits/stdc++.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
using namespace std;

#define pb push_back
#define f(n) for (int i = 0; i < (n); i++)

void error(const char *msg) {
    perror(msg);
    exit(0);
}

const int N = 256;
int sockfd;

//
// Write thread: reads user input and sends messages.
// Use these formats:
//   - To broadcast:          Receiver = "All"
//   - For private chat:       Receiver = "<username>"
//   - To join a room:         Receiver = "join:room_name"
//   - To leave a room:        Receiver = "leave:room_name"
//   - To send room message:   Receiver = "room:room_name"
// Then enter your message and it is sent as "receiver#message".
//
void *Write(void *arg) {
    while (true) {
        cout << "Enter receiver (All, username, join:room, leave:room, room:room): ";
        string receiver;
        getline(cin, receiver);

        cout << "Enter message: ";
        string mess;
        getline(cin, mess);

        string msg = receiver + "#" + mess;
        int len = msg.size();
        char message[len + 1];
        memset(message, 0, sizeof(message));
        for (int i = 0; i < len; i++)
            message[i] = msg[i];

        ssize_t n = write(sockfd, message, len);
        if (n < 0)
            error("ERROR writing to socket");

        if (mess == "exit") {
            cout << "Bye!!" << endl;
            exit(0);
        }
    }
}

//
// Read thread: continuously reads and displays messages from the server.
//
void *Read(void *arg) {
    char buffer[N];
    while (true) {
        memset(buffer, 0, N);
        ssize_t n = read(sockfd, buffer, N - 1);
        if (n < 0)
            error("ERROR reading from socket");
        int len = strlen(buffer);
        string msg;
        for (int i = 0; i < len; i++)
            msg.push_back(buffer[i]);
        cout << "\n" << msg << "\n";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    int portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    cout << "Enter your name: ";
    string name;
    getline(cin, name);
    int name_len = name.size();
    char nm[name_len + 1];
    memset(nm, 0, sizeof(nm));
    for (int i = 0; i < name_len; i++)
        nm[i] = name[i];
    ssize_t n = write(sockfd, nm, name_len);
    if (n < 0)
        error("ERROR writing to socket");

    pthread_t t1, t2;
    pthread_create(&t1, NULL, Write, NULL);
    pthread_create(&t2, NULL, Read, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    close(sockfd);
    return 0;
}
