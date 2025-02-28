#pragma once

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <set>
#include "include/nlohmann/json.hpp"

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

enum Tags {
    ACK,
    GET_FILES,
    GET_SWARM,
    UPDATE_SWARM,
    DOWNLOAD,
    STOP
};

using json = nlohmann::json;
using namespace std;

typedef struct Wanted_File {
    string file_name;
    int num_chunks;
    set<string> missing_chunks;
    set<string> available_chunks;
} Wanted_File;

typedef struct File {
    string file_name;
    int num_chunks;
    vector<string> ordered_chunks;
} File;

typedef struct File_Swarm {
    File file;
    unordered_map<string, vector<int>> chunk_seeders;
} File_Swarm;

typedef struct Client {
    int rank;
    unordered_map<string, File> owned_files;
    unordered_map<string, Wanted_File> wanted_files;
    pthread_mutex_t mutex;
} Client;

void send_json(int dest, int tag, json j);

json recv_json(int source, int tag);

void send_ack(int dest, int tag, int ack);

int recv_ack(int source, int tag);

void to_json(json &j, const Wanted_File &wf);
void from_json(const json &j, Wanted_File &wf);

void to_json(json &j, const File &f);
void from_json(const json &j, File &f);

void to_json(json &j, const File_Swarm &fs);
void from_json(const json &j, File_Swarm &fs);

void to_json(json &j, const Client &c);
void from_json(const json &j, Client &c);
