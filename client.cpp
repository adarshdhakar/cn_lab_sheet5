#include <bits/stdc++.h>
using namespace std;
#include <netinet/in.h>
#include <netdb.h>
#define pb push_back
#define f(n) for(int i=0;i<(n);i++)

void error(char *msg){
    perror(msg);
    exit(0);
}

const int N = 256;
int sockfd;

void *Write(void *useless){
    while(true){
        // cout<<"Enter Reciever's Name: ";
        string reciever;
        getline(cin, reciever);cout<<endl;

        // cout<<"Message to be sent: ";
        string mess;
        getline(cin,mess); cout<<endl;

        string msg = reciever + "#" + mess;

        int len=msg.size();
        char message[len];
        f(len) message[i]=msg[i];
        ssize_t n = write(sockfd,message,len);
        if(n < 0) error("ERROR writing to socket");
        bool exit_=(mess=="exit");
        if(exit_)
        {
            cout<<"Bye!!"<<endl; cout<<endl;
            exit(0);
        }
    }
}
void *Read(void *useless) {
    char buffer[N];
    while(true){
        bzero(buffer,N);
        ssize_t n = read(sockfd,buffer,N-1);
        if(n < 0) error("ERROR reading from socket");
        int len = strlen(buffer);
        string msg;
        f(len) msg.pb(buffer[i]);
        cout<<endl; cout<< msg << endl; cout<<endl;
    }
}

int main(int argc, char *argv[]){
    if (argc < 3){
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    int portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    struct hostent *server;
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0) error("ERROR connecting");
    
    string name;
    cout<<"Enter your name: ";
    getline(cin,name); cout<<endl;
    int name_len=name.size();
    char nm[name_len];
    f(name_len) nm[i]=name[i];
    ssize_t n = write(sockfd,nm,name_len);
    if(n < 0) error("ERROR writing to socket");

    pthread_t t1,t2;
    pthread_create(&t1,NULL,Write,NULL);
    pthread_create(&t2,NULL,Read,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

    close(sockfd);
    return 0;
}