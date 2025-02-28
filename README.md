# Peer-to-Peer-File-Sharing-System
This project implements a peer-to-peer distributed file sharing system using MPI for inter-process communication and multithreading for parallel upload/download operations. It consists of a tracker that manages file availability and multiple peers that exchange file chunks. 

This project implements a peer-to-peer distributed file sharing system using two main technologies:

MPI for inter-process communication.
Threads for parallel execution of upload and download operations.
The system is divided into two main components:

Tracker - The central process that manages file-sharing information and active clients.
Peer - A client that can upload and download files within the system.

Project Files
1. tracker.cpp
Main function (tracker):

Initializes file states received from clients.
Manages client requests for swarm information or updates.
Monitors the number of active clients and shuts down when all clients become inactive.
Key functionalities:

initialize_files_from_clients: Receives file information from clients and sets up swarm structures.
process_client_requests: Handles client requests and updates chunk states accordingly.

2. peer.cpp
Main function (peer):

Initializes a client with its owned and requested files.
Runs two threads:
download_thread_func: Handles file downloads by requesting chunks randomly from seeders.
upload_thread_func: Responds to other clients’ requests for chunk downloads.
Key functionalities:

get_swarm: Retrieves swarm information for a file from the tracker.
write_file: Writes a fully downloaded file to disk.
initialize_client: Configures the client based on input files (e.g., in1.txt, in2.txt).

3. utils.h
Definitions and utility functions:
Structures for representing files (File), swarms (File_Swarm), and clients (Client).
Functions for serialization and deserialization of data using the nlohmann/json library.
MPI-based communication functions:
send_json: Sends JSON messages to another process.
recv_json: Receives JSON messages from another process.

