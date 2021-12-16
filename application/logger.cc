#include "logger.h"

Logger::Logger(){
	first_write = true;
	LogStartTime();
}

Logger::~Logger(){

}

// Get the current time as string
std::string Logger::get_time() {
	// Create time object
	std::time_t time = std::time(nullptr);
	std::tm tm = *std::localtime(&time);
	// Convert to string
	std::ostringstream stream;
	stream << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
	return stream.str();
}

void Logger::LogStartTime() {
	log.push_back("START_TIME\t" + get_time());
}

void Logger::LogSendingBlock(size_t block_id) {
	log.push_back("SENDING_BLOCK\t" + get_time() + "\t" + std::to_string(block_id));
}

void Logger::LogRequestBlock(size_t block_id) {
	log.push_back("REQUEST_BLOCK\t" + get_time() + "\t" + std::to_string(block_id));
}

void Logger::LogReceivedBlock(size_t block_id, bool success) {
	if (success) {
		log.push_back("RECEIVED_BLOCK\t" + get_time() + "\t" + std::to_string(block_id) + "\tSUCCESS");
	} else {
		log.push_back("RECEIVED_BLOCK\t" + get_time() + "\t" + std::to_string(block_id) + "\tFAIL");
	}
}

void Logger::LogBlockAddedStart(size_t block_id) {
	log.push_back("BLOCK_ADDED_START\t" + get_time() + "\t" + std::to_string(block_id));
}

void Logger::LogBlockAddedStop(size_t block_id, bool success) {
	if (success) {
		log.push_back("BLOCK_ADDED_STOP\t" + get_time() + "\t" + std::to_string(block_id) + "\tSUCCESS");
	} else {
		log.push_back("BLOCK_ADDED_STOP\t" + get_time() + "\t" + std::to_string(block_id) + "\tFAIL");
	}
}

void Logger::WriteBack(char* filename) {
	std::cout << "Logging to " << filename << std::endl;
	std::ofstream outfile;

	if (first_write) {
		outfile.open(filename);
		first_write = false;
	} else {
		outfile.open(filename, std::ios_base::app);
	}

	for (std::string line : log) {
		outfile << line << std::endl;
	}

  	outfile.close();
  	log.clear();
}