#ifndef POTATO
#define POTATO

#include<iostream>
#include<vector>
#include<algorithm>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>

using namespace std;
class Potato{
public:
	int hops;
	int count;
	int trace[512];
	Potato():hops(0),count(0){
		memset(trace,0,sizeof(trace));
	}
	void printTrace(){
		cout<<"Trace of potato:"<<endl;
		for(int i=0;i<count-1;i++){
			cout<<trace[i]<<",";
		}
		cout<<trace[count-1]<<endl;
	}
};


class Player{
public:
	int left_fd;   // left socket fd (connect to it)
	int right_fd;  // right socket fd (accept it)
	int listen_fd; // for right neighbor
	int master_fd; // socket fd for master
	int listen_port; //The listen port as a server
	int id;			//player id
	int num_players; //number of players
	string left_ip;  //ip of left neighbor
	int left_port;	//listen port of left neighbor
	Player():left_fd(-1),right_fd(-1),listen_fd(-1),master_fd(-1),
			listen_port(-1),id(-1),num_players(-1),left_ip(""),left_port(-1){}
	~Player(){
		close(master_fd);
		close(left_fd);
		close(right_fd);
		close(listen_fd);
	}
	int getMaxFD();
	void initServer();
	void connectMaster(const char* machine_name,const char* port);
	void acceptLeftInfo();
	void connectLeft();
	void acceptRight();
	void play();
};

class Ringmaster{
public:
	int num_players;
	int hops;
	int listen_fd; 			//listen socket fd
	vector<int> fd_arr; 	//store all socket file descriptors for players
	vector<string> ip_arr; 	//store all ips for players
	vector<int> port_arr; 	//store all listen ports for players
	Ringmaster(char** args):num_players(atoi(args[2])),hops(atoi(args[3])){
		fd_arr.resize(num_players);
		ip_arr.resize(num_players);
		port_arr.resize(num_players);
		cout<<"Potato Ringmaster"<<endl;
		cout<<"Players = "<<num_players<<endl;
		cout<<"Hops = "<<hops<<endl;
	}
	~Ringmaster(){
	  close(listen_fd);
	  for(int i=0;i<fd_arr.size();i++){
	    close(fd_arr[i]);
	  }
	}
	void initServer(const char* master_port);
	void acceptAll();
	void sendLeftInfoToAll(); 
	void play();
};

#endif
