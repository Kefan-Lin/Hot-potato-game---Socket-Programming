#include<cstdio>
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
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

//ringmaster becomes a server
void Ringmaster::initServer(const char* master_port){
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(NULL,master_port,&host_info,&host_info_list);
  if(status!=0){
    cerr << "Error: cannot get address info for host" << endl;
    exit(EXIT_FAILURE);
  }
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
  freeaddrinfo(host_info_list);
}
// accept all player's connect requests
void Ringmaster::acceptAll(){
  for(int i=0;i<num_players;i++){
    struct sockaddr_storage new_addr;
    socklen_t new_addr_len = sizeof(new_addr);
    if((fd_arr[i] = accept(listen_fd,(struct sockaddr*)&new_addr,&new_addr_len))==-1){
      cerr<<"Error: cannot accept connection on socket" << endl;
      exit(EXIT_FAILURE);
    }
    struct sockaddr_in* new_addr_ptr = (struct sockaddr_in*)(&new_addr);
    ip_arr[i]=inet_ntoa(new_addr_ptr->sin_addr);
    //send player id
    send(fd_arr[i],&i,sizeof(i),0);
    //send num_players
    send(fd_arr[i],&num_players,sizeof(num_players),0);
    //receive player's listen port number
    recv(fd_arr[i],&port_arr[i],sizeof(port_arr[i]),0);
    cout<<"Player "<<i<<" is ready to play"<<endl;
  }
}

// send each player's left neighbor's info to everyone
void Ringmaster::sendLeftInfoToAll(){
  for(int i=0;i<num_players;i++){
    int left_id = (i+num_players-1)%num_players;
    int ip_len = ip_arr[left_id].length();
    //send the ip addr size
    send(fd_arr[i],&ip_len,sizeof(ip_len),0);
    //send the ip addr 
    const char* ip = ip_arr[left_id].c_str();
    send(fd_arr[i],ip,ip_len,0);
    //send the port number
    send(fd_arr[i],&port_arr[left_id],sizeof(port_arr[left_id]),0);
//    sleep(1);
  }
}

 void Ringmaster::play(){
  Potato potato;
  potato.hops=hops;
  //hops == 0, end the game
  if(hops==0){
    for(int i=0;i<num_players;i++){
      send(fd_arr[i],&potato,sizeof(potato),0);
      return;
    }
  }
  //hops != 0,start the game
  srand((unsigned int)time(NULL));
  int random_id = rand()%num_players;
  cout<<"Ready to start the game, sending potato to player "<<random_id<<endl;
  send(fd_arr[random_id],&potato,sizeof(potato),0);
  //wait for potato return
  fd_set readfds;
  FD_ZERO(&readfds);
  for(int i=0;i<num_players;i++){
    FD_SET(fd_arr[i],&readfds);
  }
  int numfd = fd_arr[fd_arr.size()-1]+1;
  int status = select(numfd,&readfds,NULL,NULL,NULL);
  for(int i=0;i<num_players;i++){
    if(FD_ISSET(fd_arr[i],&readfds)){
      recv(fd_arr[i],&potato,sizeof(potato),MSG_WAITALL);
      assert(potato.hops==0);
      Potato empty_potato; //send empty potato, which indicates game over
      for(int j=0;j<num_players;j++){
        send(fd_arr[j],&empty_potato,sizeof(empty_potato),0);
      }
      break;
    }
  }
  potato.printTrace();
 }

int main(int argc,char *argv[]){

  if(argc!=4){
    cout<<"Usage: ./ringmaster <port_num> <num_players> <num_hops>"<<endl;
    exit(EXIT_FAILURE);
  }
  if(atoi(argv[1])>65535||atoi(argv[1])<1024||atoi(argv[2])<2||atoi(argv[3])<0||atoi(argv[3])>512){
    cout<<"Please enter legal values for the arguments!"<<endl;
    exit(EXIT_FAILURE);
  }
  Ringmaster ringmaster(argv);
  ringmaster.initServer(argv[1]);
  ringmaster.acceptAll();
  ringmaster.sendLeftInfoToAll();
  ringmaster.play();
  exit(EXIT_SUCCESS);
}









