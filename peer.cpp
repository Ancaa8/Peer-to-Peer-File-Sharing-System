#include "utils.h"
#include "peer.h"


// Thread pentru descărcarea fișierelor
void* download_thread_func(void* arg) {
    Client* client = (Client*)arg;

    while (true) {
        pthread_mutex_lock(&client->mutex);
        if (client->wanted_files.empty()) {
            pthread_mutex_unlock(&client->mutex);
            break;
        }

        string wanted_file_name = client->wanted_files.begin()->first;
        Wanted_File* wanted_file = &client->wanted_files.begin()->second;
        pthread_mutex_unlock(&client->mutex);

        File_Swarm fs = get_swarm(wanted_file_name);

        wanted_file->num_chunks = fs.file.num_chunks;
        wanted_file->missing_chunks = set<string>(fs.file.ordered_chunks.begin(), fs.file.ordered_chunks.end());

        int counter = 0;

        while (!wanted_file->missing_chunks.empty()) {
            string chunk = *wanted_file->missing_chunks.begin();
            int random_seeder = fs.chunk_seeders[chunk][rand() % fs.chunk_seeders[chunk].size()];

            json j;
            j["file_name"] = wanted_file_name;
            j["chunk"] = chunk;
            send_json(random_seeder, DOWNLOAD, j);

            int ack = recv_ack(random_seeder, ACK);

            if (ack == 0) {
                pthread_mutex_lock(&client->mutex);
                wanted_file->missing_chunks.erase(chunk);
                wanted_file->available_chunks.insert(chunk);
                pthread_mutex_unlock(&client->mutex);

                counter++;

                json update;
                update["chunk"] = chunk;
                update["seeder"] = client->rank;
                send_json(TRACKER_RANK, UPDATE_SWARM, update);

                if (counter > 9) {
                    counter = 0;
                    fs = get_swarm(wanted_file_name);
                }

                pthread_mutex_lock(&client->mutex);
                if (wanted_file->missing_chunks.empty()) {
                    pthread_mutex_unlock(&client->mutex);
                    write_file(client, wanted_file_name, fs);
                    break;
                }
                pthread_mutex_unlock(&client->mutex);
            } else {
                cout << "Chunk " << chunk << " download failed" << endl;
            }
        }
    }

    json j;
    j["rank"] = client->rank;
    send_json(TRACKER_RANK, STOP, j);
    return NULL;
}

// Thread pentru încărcarea fișierelor
void* upload_thread_func(void* arg) {
    Client* client = (Client*)arg;

    while (true) {
        MPI_Status status_stop;
        int flag_stop;
        MPI_Iprobe(TRACKER_RANK, STOP, MPI_COMM_WORLD, &flag_stop, &status_stop);
        if (flag_stop) {
            MPI_Recv(&flag_stop, 1, MPI_INT, TRACKER_RANK, STOP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            return NULL;
        }

        MPI_Status status;
        int count;
        MPI_Iprobe(MPI_ANY_SOURCE, DOWNLOAD, MPI_COMM_WORLD, &count, &status);
        if (count == 0) {
            continue;
        }

        json j = recv_json(status.MPI_SOURCE, DOWNLOAD);
        string file_name = j["file_name"];
        string chunk = j["chunk"];

        int ack = 1;

        pthread_mutex_lock(&client->mutex);
        if (client->owned_files.find(file_name) != client->owned_files.end()) {
            File file = client->owned_files[file_name];
            if (find(file.ordered_chunks.begin(), file.ordered_chunks.end(), chunk) != file.ordered_chunks.end()) {
                ack = 0;
            }
        }

        if (client->wanted_files.find(file_name) != client->wanted_files.end()) {
            Wanted_File wanted_file = client->wanted_files[file_name];
            if (wanted_file.available_chunks.find(chunk) != wanted_file.available_chunks.end()) {
                ack = 0;
            }
        }
        pthread_mutex_unlock(&client->mutex);

        send_ack(status.MPI_SOURCE, ACK, ack);
    }
}

// Funcția principală a unui peer
void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void* status;
    int r;

    Client* client = initialize_client(rank);

    json j;
    j["owned_files"] = client->owned_files;
    send_json(TRACKER_RANK, GET_FILES, j);

    recv_ack(TRACKER_RANK, ACK);

    r = pthread_create(&download_thread, NULL, download_thread_func, (void*)client);
    if (r) {
        printf("Error creating download thread\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void*)client);
    if (r) {
        printf("Error creating upload thread\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Error joining download thread\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Error joining upload thread\n");
        exit(-1);
    }

    pthread_mutex_destroy(&client->mutex);
    delete client;
}

// Obține detalii despre swarm-ul fișierului
File_Swarm get_swarm(string file_name) {
    json j;
    j["file_name"] = file_name;
    send_json(TRACKER_RANK, GET_SWARM, j);
    json response = recv_json(TRACKER_RANK, GET_SWARM);
    File_Swarm fs = response["swarm"].get<File_Swarm>();
    return fs;
}

// Inițializează un client
Client* initialize_client(int rank) {
    Client* client = new Client();
    client->rank = rank;
    pthread_mutex_init(&client->mutex, NULL);

    char filename[MAX_FILENAME];
    sprintf(filename, "in%d.txt", rank);

    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("Error opening file %s\n", filename);
        exit(-1);
    }

    int num_owned_files;
    fscanf(f, "%d", &num_owned_files);

    for (int i = 0; i < num_owned_files; i++) {
        File file;
        char file_name[MAX_FILENAME];
        int num_chunks;
        fscanf(f, "%s %d", file_name, &num_chunks);
        file.file_name = file_name;
        file.num_chunks = num_chunks;

        for (int j = 0; j < num_chunks; j++) {
            char chunk_hash[HASH_SIZE];
            fscanf(f, "%s", chunk_hash);
            file.ordered_chunks.push_back(chunk_hash);
        }

        client->owned_files[file_name] = file;
    }

    int num_wanted_files;
    fscanf(f, "%d", &num_wanted_files);

    for (int i = 0; i < num_wanted_files; i++) {
        Wanted_File wanted_file;
        char file_name[MAX_FILENAME];
        fscanf(f, "%s", file_name);
        wanted_file.file_name = file_name;
        client->wanted_files[file_name] = wanted_file;
    }

    fclose(f);
    return client;
}

// Scrie un fișier după descărcarea completă
void write_file(Client* client, const string& wanted_file_name, File_Swarm& fs) {
    string filename = "client" + to_string(client->rank) + "_" + wanted_file_name;
    FILE* f = fopen(filename.c_str(), "w");
    if (f == NULL) {
        cout << "Error opening file " << filename << endl;
        exit(-1);
    }

    File file;
    file.file_name = wanted_file_name;
    file.num_chunks = fs.file.num_chunks;

    for (int i = 0; i < file.num_chunks; i++) {
        string chunk = fs.file.ordered_chunks[i];
        fprintf(f, "%s\n", chunk.c_str());
        file.ordered_chunks.push_back(chunk);
    }

    fclose(f);

    pthread_mutex_lock(&client->mutex);
    client->owned_files[wanted_file_name] = file;
    client->wanted_files.erase(wanted_file_name);
    pthread_mutex_unlock(&client->mutex);
}
