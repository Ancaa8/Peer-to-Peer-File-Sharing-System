#include "utils.h"


void send_json(int dest, int tag, json j) {
	string payload = j.dump();
	int payload_size = payload.size();
	MPI_Send(&payload_size, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
	MPI_Send(payload.c_str(), payload_size, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
}

json recv_json(int source, int tag) {
	int payload_size;
	MPI_Recv(&payload_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	char *payload = new char[payload_size + 1];
	MPI_Recv(payload, payload_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	payload[payload_size] = '\0';
	json j = json::parse(payload);
	delete[] payload;
	return j;
}

void send_ack(int dest, int tag, int ack) {
	MPI_Send(&ack, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

int recv_ack(int source, int tag) {
	int ack;
	MPI_Recv(&ack, 1, MPI_INT, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	return ack;
}

void to_json(json &j, const Wanted_File &wf) {
	j = json{
		{"file_name", wf.file_name}, 
		{"num_chunks", wf.num_chunks}, 
		{"missing_chunks", wf.missing_chunks}, 
		{"available_chunks", wf.available_chunks}
	};
}

void from_json(const json &j, Wanted_File &wf) {
	j.at("file_name").get_to(wf.file_name);
	j.at("num_chunks").get_to(wf.num_chunks);
	j.at("missing_chunks").get_to(wf.missing_chunks);
	j.at("available_chunks").get_to(wf.available_chunks);
}

void to_json(json &j, const File &f) {
	j = json{
		{"file_name", f.file_name}, 
		{"num_chunks", f.num_chunks}, 
		{"ordered_chunks", f.ordered_chunks}
	};
}

void from_json(const json &j, File &f) {
	j.at("file_name").get_to(f.file_name);
	j.at("num_chunks").get_to(f.num_chunks);
	j.at("ordered_chunks").get_to(f.ordered_chunks);
}

void to_json(json &j, const File_Swarm &fs) {
	j = json{
		{"file", fs.file}, 
		{"chunk_seeders", fs.chunk_seeders}
	};
}

void from_json(const json &j, File_Swarm &fs) {
	j.at("file").get_to(fs.file);
	j.at("chunk_seeders").get_to(fs.chunk_seeders);
}