#include <bits/stdc++.h>
using namespace std;
#include <netinet/in.h>
#define pb push_back
#define f(n) for(int i=0;i<(n);i++)

const int N = 256;

void error(char *msg){
    perror(msg);
    exit(1);
}

set<int> active_clients;

void *Clients(void *ns){
    int newsockfd = *(int *)ns;
    active_clients.insert(newsockfd);
    char buffer[N];
    bzero(buffer,N);
    int n = read(newsockfd,buffer,N-1);
    if(n < 0) error("ERROR reading from socket");
    int len_name=strlen(buffer);
    string name;
    f(len_name) name.pb(buffer[i]);

    string message = "\n" + name + " joined the chat.\n";
    message += "\nActive Clients: " + to_string(active_clients.size()) + "\n";
    cout << message << endl;

    int len=message.size();
    char msg[len];
    f(len) msg[i]=message[i];
    for(int sockfd:active_clients){
        if(sockfd!=newsockfd){
            ssize_t n = write(sockfd,msg,len);
            if (n < 0) error("ERROR writing to socket");
        }
    }

    while(true){
        bzero(buffer,N);
        int n = read(newsockfd,buffer,N-1);
        if(n < 0) error("ERROR reading from socket");
        int len=strlen(buffer);
        string msg;
        f(len) msg.pb(buffer[i]);

        bool exit_=(msg=="exit");
        if(exit_){
            msg = "\n" + name + " left the chat.\n";
            msg += "\nActive Clients: " + to_string(active_clients.size()-1) + "\n";
        }
        else msg = "\n" + name + ": " + msg + "\n";

        cout << msg << endl;

        len=msg.size();
        char message[len];
        f(len) message[i]=msg[i];
        for(int sockfd:active_clients){
            if(sockfd!=newsockfd){
                ssize_t n = write(sockfd,message,len);
                if (n < 0) error("ERROR writing to socket");
            }
        }

        if(exit_){
            active_clients.erase(newsockfd); cout<<endl;
            pthread_exit(NULL);
        }
    }
}

int main(int argc, char *argv[]){ 
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int portno = atoi(argv[1]);
    if(sockfd < 0) error("ERROR opening socket");
    struct sockaddr_in serv_addr, cli_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0) error("ERROR on binding");
    pthread_t thread_id;

    cout<<"Active Clients = 0"<<endl;
    while(true){
        listen(sockfd,5);
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd < 0) error("ERROR on accept");
        
        if(active_clients.find(newsockfd)!=active_clients.end()) continue;
        int x = newsockfd;
        pthread_create(&thread_id,NULL,Clients,&x);
        if(active_clients.find(newsockfd)==active_clients.end()) pthread_detach(thread_id);
    }
    return 0;
}