#include<cstdio>
#include<iostream>
#include<sstream>
#include<stdio.h>
#include<stdlib.h>
#include<cstdlib>
#include<string.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/types.h> 
#include<sys/socket.h> 
#include<netdb.h>
#include<unistd.h>
#include<assert.h>
#include"potato.h"

using namespace std;

//player becomes a server
void Player::initServer(){
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE; 
  char hostname[100];
  if (gethostname(hostname, sizeof(hostname)) < 0) { //get the player's hostname
    cerr<<"Error: cannot get hostname"<<endl;
    exit(EXIT_FAILURE);
  }
  status = getaddrinfo(hostname,NULL,&host_info,&host_info_list);
  if(status!=0){
    cerr << "Error: cannot get address info for host" << endl;
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in *temp = (struct sockaddr_in *)(host_info_list->ai_addr);
  temp->sin_port = 0; // let the OS assign the listen port
  listen_fd = socket(host_info_list->ai_family, 
                     host_info_list->ai_socktype, 
                     host_info_list->ai_protocol);
  if(listen_fd==-1){
    cerr << "Error: cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }
  int yes = 1;
  status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(listen_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    exit(EXIT_FAILURE);
  } 
  status = listen(listen_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    exit(EXIT_FAILURE);
  } 
  struct sockaddr_in addr_in;
  socklen_t len = sizeof(addr_in);
  //get information from the listen_fd
  status = getsockname(listen_fd,(struct sockaddr*)&addr_in,&len);
  if(status==-1){
    cerr << "Error: cannot get the player's listen socket name" << endl; 
    exit(EXIT_FAILURE);
  }
  //store the player's listen port
  listen_port = ntohs(addr_in.sin_port);
  freeaddrinfo(host_info_list);
}

//connect to ringmaster
void Player::connectMaster(const char* machine_name,const char* port){
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(machine_name, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    exit(EXIT_FAILURE);
  }
  master_fd = socket(host_info_list->ai_family, 
         host_info_list->ai_socktype, 
         host_info_list->ai_protocol);
  if (master_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }
  status = connect(master_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    exit(EXIT_FAILURE);
  }
  // after connected, recv the player id and num_players
  recv(master_fd,&id,sizeof(id),0);
  recv(master_fd,&num_players,sizeof(num_players),0);
  //the player then becomes a server
  initServer();
  //send the player's listen port to ringmaster
  send(master_fd,&listen_port,sizeof(listen_port),0);
  cout << "Connected as player " << id << " out of "
       << num_players << " total players"<<endl;
  freeaddrinfo(host_info_list);
}

//accept left neighbor's info from ringmaster
void Player::acceptLeftInfo(){
  char temp_ip[100];
  memset(temp_ip,0,100);
  int ip_size;
  recv(master_fd,&ip_size,sizeof(ip_size),MSG_WAITALL);
  recv(master_fd,temp_ip,ip_size,MSG_WAITALL);
  recv(master_fd,&left_port,sizeof(left_port),MSG_WAITALL);
  temp_ip[ip_size]='\0';
  string temp_str(temp_ip);
  left_ip=temp_str;
  // connect to left neighbor 
  connectLeft();
  // accept right neighbor
  acceptRight(); 
}

//accept right neighbor's connect request
void Player::acceptRight(){
  struct sockaddr_storage right_addr;
  socklen_t right_addr_len = sizeof(right_addr);
  if((right_fd = accept(listen_fd,(struct sockaddr*)&right_addr,&right_addr_len))==-1){
    cerr<<"Error: cannot accept connection on socket" << endl;
    exit(EXIT_FAILURE);
  }
}

//connect to left neighbor
void Player::connectLeft(){
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  const char* ip = left_ip.c_str();
  stringstream ss;
  ss<<left_port;
  string temp_port;
  ss>>temp_port;
  const char* port = temp_port.c_str();
  status = getaddrinfo(ip, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    exit(EXIT_FAILURE);
  }
  left_fd = socket(host_info_list->ai_family, 
         host_info_list->ai_socktype, 
         host_info_list->ai_protocol);
  if (left_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    exit(EXIT_FAILURE);
  }
  status = connect(left_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr<<"Error: cannot connect to socket"<<endl;;
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(host_info_list);
}

//play the game
void Player::play(){
  Potato potato;
  srand((unsigned int)time(NULL)+id);
  fd_set readfds;
  int fds[3]={master_fd,left_fd,right_fd};
  int numfd=getMaxFD()+1;
  while(true){
    FD_ZERO(&readfds);
    FD_SET(fds[0],&readfds);
    FD_SET(fds[1],&readfds);
    FD_SET(fds[2],&readfds);
    int status = select(numfd,&readfds,NULL,NULL,NULL);
    if(status!=1){
      return;
    }
    for(int i=0;i<3;i++){
      if(FD_ISSET(fds[i],&readfds)){//recv the potato
        recv(fds[i],&potato,sizeof(potato),MSG_WAITALL);
        break;
      }
    }
    //if recv an empty potato, game over
    if(potato.hops==0&&potato.count==0){
      break;
    }
    //normal potato
    potato.hops--;
    potato.trace[potato.count]=id;
    potato.count++;
    if(potato.hops==0){//game over
      //send the potato back to ringmaster
      send(master_fd,&potato,sizeof(potato),0);
      cout<<"I'm it"<<endl;
      continue;
    }
    // send potato
    int randNeigh = rand()%2;
    if(randNeigh==0){//left
      cout<<"Sending potato to "<<(id - 1 + num_players) % num_players<<endl;
      send(left_fd,&potato,sizeof(potato),0);
    }
    else{//right
      cout<<"Sending potato to "<<(id + 1) % num_players<<endl;
      send(right_fd,&potato,sizeof(potato),0);
    }
  }
}

int Player::getMaxFD(){
  return max(master_fd,max(left_fd,right_fd));
}

int main(int argc, char* argv[]){
  if(argc!=3){
    cout<<"Usage: ./player <machine_name> <port_num>"<<endl;
    exit(EXIT_FAILURE);
  }
  Player player;
  player.connectMaster(argv[1],argv[2]);
  player.acceptLeftInfo();
  player.play();
  exit(EXIT_SUCCESS);
}
