#ifndef PEER_H
#define PEER_H

using namespace std;

File_Swarm get_swarm(string file_name);
Client* initialize_client(int rank);
void write_file(Client* client, const string& wanted_file_name, File_Swarm& fs);
void peer(int numtasks, int rank);

#endif // PEER_H
