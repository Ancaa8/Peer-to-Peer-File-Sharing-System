#ifndef TRACKER_H
#define TRACKER_H

void initialize_files_from_clients(int numtasks, unordered_map<string, File_Swarm>& files);
void process_client_requests(int numtasks, int& active_clients, unordered_map<string, File_Swarm>& files);
void tracker(int numtasks, int rank);

#endif // TRACKER_H
