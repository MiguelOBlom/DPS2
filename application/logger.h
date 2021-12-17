/*
	Log for experiments when all transactions have been added

	Formatting:
	START_TIME\t<TIME>

	SENDING_BLOCK\t<TIME>\t<BLOCK_NUMBER>

	REQUEST_BLOCK\t<TIME>\t<BLOCK_NUMBER>
	RECEIVED_BLOCK\t<TIME>\t<BLOCK_NUMBER>\t[FAIL/SUCCESS]

	BLOCK_ADDED_START\t<TIME>\t<BLOCK_NUMBER>
	BLOCK_ADDED_STOP\t<TIME>\t<BLOCK_NUMBER>\t[FAIL/SUCCESS]

	Author: Miguel Blom
*/

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <iostream>

class Logger {
private:
	// Whether we are first writing to the file or should append
	bool first_write;
	// Store the log in a vector before writing our first write
	std::vector<std::string> log = std::vector<std::string>();
	// Gets the current date time as a string
	std::string get_time();


public:
	// Initializes the log with the start time of the logging
	Logger();
	~Logger();
	// Logs the start time to our log vector
	void LogStartTime();
	// Logs the time at which a block with ID block_id was sent
	void LogSendingBlock(size_t block_id);
	// Logs the time at which a block with ID block_id was requested
	void LogRequestBlock(size_t block_id);
	// Logs the time at which a block with ID block_id was received
	// and whether it was successful to do so (or timed out)
	void LogReceivedBlock(size_t block_id, bool success);
	// Logs the time at which an attempt to add block with ID block_id 
	// to the blockchain was started
	void LogBlockAddedStart(size_t block_id);
	// Logs the time at which an attempt to add block with ID block_id 
	// to the blockchain has finished and whether it was successful
	void LogBlockAddedStop(size_t block_id, bool success);
	// Writes the log to a file specified by filename, the log is then
	// cleared and we modify first_write for appending to the file again later
	void WriteBack(char * filename);
};