#include "utils.h"
#include "tracker.h"

// Funcția principală a tracker-ului
void tracker(int numtasks, int rank) {
    int active_clients = numtasks - 1;
    unordered_map<string, File_Swarm> files;

    initialize_files_from_clients(numtasks, files);
    process_client_requests(numtasks, active_clients, files);
}

// Inițializează fișierele pe baza datelor primite de la clienți
void initialize_files_from_clients(int numtasks, unordered_map<string, File_Swarm>& files) {
    for (int i = 1; i < numtasks; i++) {
        json j = recv_json(i, GET_FILES);

        vector<File> owned_files;
        for (auto& file : j["owned_files"]) {
            File f = file.get<File>();
            owned_files.push_back(f);
        }

        // Configurează swarm-ul pentru fiecare fișier
        for (auto& file : owned_files) {
            if (files.find(file.file_name) == files.end()) {
                File_Swarm fs;
                fs.file = file;
                fs.chunk_seeders = unordered_map<string, vector<int>>();
                for (int j = 0; j < file.num_chunks; j++) {
                    fs.chunk_seeders[file.ordered_chunks[j]] = vector<int>();
                }
                files[file.file_name] = fs;
            }

            for (int j = 0; j < file.num_chunks; j++) {
                files[file.file_name].chunk_seeders[file.ordered_chunks[j]].push_back(i);
            }
        }
    }

    // Trimite confirmare către clienți
    for (int i = 1; i < numtasks; i++) {
        int ack = 0;
        MPI_Send(&ack, 1, MPI_INT, i, ACK, MPI_COMM_WORLD);
    }
}

// Procesează cererile primite de la clienți
void process_client_requests(int numtasks, int& active_clients, unordered_map<string, File_Swarm>& files) {
    while (true) {
        MPI_Status status;
        int count;

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &count);
        int source = status.MPI_SOURCE;

        json j = recv_json(source, status.MPI_TAG);

        switch (status.MPI_TAG) {
            case GET_SWARM: { // Trimite informațiile despre swarm-ul fișierului
                string file_name = j["file_name"];
                File_Swarm fs = files[file_name];

                json response;
                response["swarm"] = fs;
                send_json(source, GET_SWARM, response);
                break;
            }
            case UPDATE_SWARM: { // Actualizează seederii unui chunk
                string chunk = j["chunk"];
                int seeder = j["seeder"];

                for (auto& file : files) {
                    if (file.second.chunk_seeders.find(chunk) != file.second.chunk_seeders.end()) {
                        vector<int>& seeders = file.second.chunk_seeders[chunk];
                        seeders.push_back(seeder);
                    }
                }
                break;
            }
            case STOP: { // Reduce numărul de clienți activi
                active_clients--;
                if (active_clients == 0) { // Dacă nu mai sunt clienți activi, trimite semnalul de oprire
                    for (int i = 1; i < numtasks; i++) {
                        int stop = 0;
                        MPI_Send(&stop, 1, MPI_INT, i, STOP, MPI_COMM_WORLD);
                    }
                    return;
                }
                break;
            }
        }
    }
}
