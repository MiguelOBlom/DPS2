#include "db.h"

// Convert bytes to a short unsigned integer
static short unsigned int bytosui (char * bytes) {
	short unsigned int r = 0;
	r += bytes[0] & 0xFF;
	r += (bytes[1] & 0xFF) << 8;
	return r;
}

// Convert bytes to an unsigned integer
static unsigned int bytoui (char * bytes) {
	unsigned int r = 0;
	r += bytosui(&bytes[0]) & 0xFFFF;
	r += (bytosui(&bytes[2]) & 0xFFFF) << 16;
	return r;
}

// Callback for returning a single integer value from the database by SELECT.
// ( e.g. when using COUNT. )
static int _get_single_integer (	void* retval, 
									int __attribute__((unused)) argc, 
									char** argv, 
									char __attribute__((unused)) ** azColName) {
	*(size_t*)retval = strtoul(argv[0], NULL, 10);
	return 0;
}

// Callback for returning a table of peer_address data
static int _fill_peeraddress_table(	void* retval, 
									int __attribute__((unused)) argc, 
									char** argv, 
									char __attribute__((unused)) ** azColName) {
	size_t num = strtoul(argv[0], NULL, 10) - 1;

	(*(struct peer_address**)retval)[num].family = bytosui(argv[1]);
	(*(struct peer_address**)retval)[num].port = bytosui(argv[2]);
	(*(struct peer_address**)retval)[num].addr = bytoui(argv[3]);

	return 0;
}

// Callback for returning a table of peer_info data
static int _fill_peerinfo_table(	void* retval, 
									int __attribute__((unused)) argc, 
									char** argv, 
									char __attribute__((unused)) ** azColName) {
	size_t num = strtoul(argv[0], NULL, 10) - 1;

	(*(struct peer_info**)retval)[num].sockaddr.family = bytosui(argv[1]);
	(*(struct peer_info**)retval)[num].sockaddr.port = bytosui(argv[2]);
	(*(struct peer_info**)retval)[num].sockaddr.addr = bytoui(argv[3]);
	(*(struct peer_info**)retval)[num].heartbeat = strtoul(argv[4], NULL, 10);

	return 0;
}

// Report what error occurred and quit the application
void _report_sqlite3_err (sqlite3 * db, char * msg) {
	int sqlite3_retval = sqlite3_errcode(db);
	const char * errmsg = sqlite3_errmsg(db);
	printf("%s (%d): %s\n", msg, sqlite3_retval, errmsg);
	sqlite3_close(db);
	exit(EXIT_FAILURE);
}


// Create the table of the database, check the .h file for the 
// more elaborate explanation.
void _create_sqlite3_table (sqlite3* db) {
	char * errmsg;

	perror("b4?");
	int sqlite3_retval = sqlite3_exec(db, 
		"CREATE TABLE IF NOT EXISTS PeerInfo ("
			"peer_family BLOB NOT NULL,"
			"peer_port BLOB NOT NULL,"
			"peer_addr BLOB NOT NULL,"
			"peer_heartbeat INT NOT NULL DEFAULT (strftime('%s', 'now')),"
			"PRIMARY KEY(peer_family, peer_port, peer_addr)"
		")", NULL, NULL, &errmsg);

	perror("exec?");

	if (sqlite3_retval != SQLITE_OK) {
		printf("Error creating table: %s", errmsg);
		sqlite3_close(db);
		free(errmsg);
		exit(EXIT_FAILURE);
	}

	errno = 0;
	free(errmsg);
}

// Bind the peer_address values to the parametrized partial_stmt 
// and put the result in stmt
void _bind_sqlite3_peeraddress(sqlite3* db, char * partial_stmt, const struct peer_address* pa, sqlite3_stmt ** stmt) {
	int sqlite3_retval = sqlite3_prepare_v2(db, partial_stmt, -1, stmt, NULL);
	
	// Bind the values of the parameters to the statement by position
	if (sqlite3_retval == SQLITE_OK) {
		sqlite3_retval = sqlite3_bind_blob(*stmt, 1, &(pa->family), sizeof(pa->family), SQLITE_STATIC);
		if (sqlite3_retval == SQLITE_OK) {
			sqlite3_retval = sqlite3_bind_blob(*stmt, 2, &(pa->port), sizeof(pa->port), SQLITE_STATIC);
			if (sqlite3_retval == SQLITE_OK) {
				sqlite3_retval = sqlite3_bind_blob(*stmt, 3, &(pa->addr), sizeof(pa->addr), SQLITE_STATIC);
				if (sqlite3_retval == SQLITE_OK) {
					errno = 0;
					return;
				}
			}
		}
	}

	sqlite3_finalize(*stmt);
	_report_sqlite3_err(db, "Error binding peer to partial statement");
}

// Return the number of peers that are present in the database
size_t _get_number_of_peers (sqlite3* db, int timeout_threshold) {
	size_t n_items;
	char * errmsg;
	char * stmt = "SELECT COUNT(*) FROM PeerInfo WHERE ((strftime('%s', 'now') - peer_heartbeat)) < ";

	size_t stmt_len = strlen(stmt);
	size_t var_len = snprintf(NULL, 0, "%d", timeout_threshold);
	size_t len = stmt_len + var_len + 2;
	char * str = malloc(len);
	strncpy(str, stmt, stmt_len);
	snprintf(str + stmt_len, var_len, "%d", timeout_threshold);
	str[stmt_len + var_len] = ';';
	str[stmt_len + var_len + 1] = '\0';
	printf("sanity check %lu %lu %lu %lu\n", stmt_len, var_len, len, stmt_len + var_len);

	int sqlite3_retval = sqlite3_exec(db, str, _get_single_integer, &n_items, &errmsg);
	//sqlite3_retval = sqlite3_exec(db, str, _fill_peerinfo_table, data, &errmsg);

	free(str);




	if (sqlite3_retval != SQLITE_OK) {
		printf("Error while counting rows: %s\n", errmsg);
		free(errmsg);
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}
	errno = 0;
	free(errmsg);
	return n_items;
}


// **************** //
// Global functions //
// **************** //

// Compares two peer_address objects
// Returns 1 if equal, otherwise 0;
int cmp_peer_address(const struct peer_address* pa1, const struct peer_address* pa2) {
	return pa1->family == pa2->family && pa1->port == pa2->port && pa1->addr == pa2->addr;
}

// Open the database
void db_open (sqlite3 ** db, char * dbname) {
	printf("SQLite version: %s\n", sqlite3_libversion());
	int sqlite3_retval = sqlite3_open(dbname, db);

	if (sqlite3_retval != SQLITE_OK) {
		printf("Error opening database.");
		sqlite3_close(*db);
		exit(EXIT_FAILURE);
	}

	_create_sqlite3_table(*db);
	errno = 0;
}

// Close the database
void db_close(sqlite3 * db) {
	sqlite3_close(db);
}

// Check in the database whether a peer with the specified peer address exists
int db_peer_exists(sqlite3 * db, const struct peer_address* pa) {
	int retval;
	sqlite3_stmt* stmt;
	int sqlite3_retval;

	char * partial_stmt = "SELECT COUNT(*) FROM PeerInfo WHERE peer_family = ? AND peer_port = ? AND peer_addr = ?;";
	_bind_sqlite3_peeraddress(db, partial_stmt, pa, &stmt);
	sqlite3_retval = sqlite3_step(stmt);

	if (sqlite3_retval == SQLITE_ROW) {
		retval = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);	
		errno = 0;
		return retval;
	}

	sqlite3_finalize(stmt);
	_report_sqlite3_err(db, "Error checking if peer exists");

	return 0;
}

// Insert a peer in the database, specified by the peer address
void db_insert_peer (sqlite3* db, const struct peer_address* pa) {
	sqlite3_stmt* stmt;
	int sqlite3_retval;

	char * partial_stmt = "INSERT INTO PeerInfo (peer_family, peer_port, peer_addr) VALUES (?, ?, ?)";
	_bind_sqlite3_peeraddress(db, partial_stmt, pa, &stmt);

	sqlite3_retval = sqlite3_step(stmt);
	if (sqlite3_retval == SQLITE_DONE) {
		sqlite3_finalize(stmt);	
		errno = 0;
		return;
	}

	sqlite3_finalize(stmt);
	_report_sqlite3_err(db, "Error inserting peer");
}

// Update the heartbeat of the peer specified by the peer address
void db_update_peer_heartbeat(sqlite3 * db, const struct peer_address* pa) {
	sqlite3_stmt* stmt;
	int sqlite3_retval;

	char * partial_stmt = "UPDATE PeerInfo SET peer_heartbeat=(strftime('%s', 'now')) WHERE peer_family = ? AND peer_port = ? AND peer_addr = ?";
	_bind_sqlite3_peeraddress(db, partial_stmt, pa, &stmt);

	sqlite3_retval = sqlite3_step(stmt);
	if (sqlite3_retval == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		errno = 0;
		return;
	}

	sqlite3_finalize(stmt);
	_report_sqlite3_err(db, "Error updating peer heartbeat");
}

// Remove all peers from the database of which the heartbeat has not been updated in 
// the last timeout_threshold seconds
void db_remove_outdated_peers (sqlite3* db, int timeout_threshold) {
	char * partial_stmt = "DELETE FROM PeerInfo WHERE ((strftime('%s', 'now') - peer_heartbeat)) > ?;";
	sqlite3_stmt* stmt;
	int sqlite3_retval = sqlite3_prepare_v2(db, partial_stmt, -1, &stmt, NULL);
	if (sqlite3_retval == SQLITE_OK) {
		sqlite3_retval = sqlite3_bind_int(stmt, 1, timeout_threshold);
		if (sqlite3_retval == SQLITE_OK) {
			sqlite3_retval = sqlite3_step(stmt); // Executes the statement
			if (sqlite3_retval == SQLITE_DONE) {
				sqlite3_finalize(stmt); // Cleans up the statement
				errno = 0;
				return;
			}
		}
	}

	sqlite3_finalize(stmt);
	_report_sqlite3_err(db, "Error removing outdated peers");
}

// Remove a specific peer from the database
void db_remove_peer (sqlite3 * db, const struct peer_address* pa) {
	char * partial_stmt = "DELETE FROM PeerInfo WHERE peer_family = ? AND peer_port = ? AND peer_addr = ?";
	sqlite3_stmt* stmt;

	_bind_sqlite3_peeraddress(db, partial_stmt, pa, &stmt);

	int sqlite3_retval = sqlite3_step(stmt);
	if (sqlite3_retval == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		errno = 0;
		return;
	}

	sqlite3_finalize(stmt);
	_report_sqlite3_err(db, "Error removing peer");
}

// Provide a list of all peer addresses in data, the size is provided in n_items
void db_get_all_peer_addresses (sqlite3* db, struct peer_address** data, size_t* n_items, int timeout_threshold) {
	char * errmsg;
	char * stmt;
	int sqlite3_retval;

	*n_items = _get_number_of_peers(db, timeout_threshold);
	*data = malloc(sizeof(struct peer_address) * *n_items);

	stmt = "SELECT ROW_NUMBER() OVER() AS num_row, peer_family, peer_port, peer_addr FROM PeerInfo WHERE ((strftime('%s', 'now') - peer_heartbeat)) < ";

	size_t stmt_len = strlen(stmt);
	size_t var_len = snprintf(NULL, 0, "%d", timeout_threshold);
	size_t len = stmt_len + var_len + 2;
	char * str = malloc(len);
	strncpy(str, stmt, stmt_len);
	snprintf(str + stmt_len, var_len, "%d", timeout_threshold);
	str[stmt_len + var_len] = ';';
	str[stmt_len + var_len + 1] = '\0';

	sqlite3_retval = sqlite3_exec(db, str, _fill_peeraddress_table, data, &errmsg);

	free(str);

	if (sqlite3_retval != SQLITE_OK) {
		printf("Error while fetching peer info: %s\n", errmsg);
		free(errmsg);
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}

	errno = 0;
	free(errmsg);
}

// Provide a list of all peer info in data, the size is provided in n_items
void db_get_all_peer_info (sqlite3* db, struct peer_info** data, size_t* n_items, int timeout_threshold) {
	char * errmsg;
	char * stmt;
	int sqlite3_retval;

	*n_items = _get_number_of_peers(db, timeout_threshold);
	*data = malloc(sizeof(struct peer_info) * *n_items);

	stmt = "SELECT ROW_NUMBER() OVER() AS num_row, * FROM PeerInfo WHERE ((strftime('%s', 'now') - peer_heartbeat)) < ";

	size_t stmt_len = strlen(stmt);
	size_t var_len = snprintf(NULL, 0, "%d", timeout_threshold);
	size_t len = stmt_len + var_len + 2;
	char * str = malloc(len);
	strncpy(str, stmt, stmt_len);
	snprintf(str + stmt_len, var_len, "%d", timeout_threshold);
	str[stmt_len + var_len] = ';';
	str[stmt_len + var_len + 1] = '\0';

	sqlite3_retval = sqlite3_exec(db, str, _fill_peerinfo_table, data, &errmsg);

	free(str);

	if (sqlite3_retval != SQLITE_OK) {
		printf("Error while fetching peer address: %s\n", errmsg);
		free(errmsg);
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}
	
	errno = 0;
	free(errmsg);
}

// ************ //
// Example code //
// ************ //

/***************************** /

#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

int main (int argc, char ** argv) {
	char* dbname;
	sqlite3* db;

	struct peer_address pa;

	if (argc != 2) {
		printf("Usage: %s <db_file>\n", argv[0]);
	}

	dbname = argv[1];

	// Get the SQLite version
	printf("SQLite version: %s\n", sqlite3_libversion());

	// Open the database and create a table if not exists
	db_open(&db, dbname);

	// Define peer address information
	pa.family = AF_INET;
	pa.port = htons(8080);
	pa.addr = inet_addr("192.168.2.123");

	// The values that we expect from the database
	printf("What it should be -- family: %d, port: %d, addr: %u\n", pa.family, pa.port, pa.addr);

	// Inserting a new peer in the database
	if(!db_peer_exists(db, &pa)) {
		db_insert_peer(db, &pa);
	} else {
		printf("Peer already exists!\n");
	}
	
	// Get all information (including heartbeat) of a peer
	struct peer_info * data;
	size_t n_items;
	db_get_all_peer_info(db, &data, &n_items);

	for (unsigned int i = 0; i < n_items; i++) {
		printf("%d, %d, %d, %lu\n", data[i].sockaddr.family, data[i].sockaddr.port, data[i].sockaddr.addr, data[i].heartbeat);
	}

	free(data);

	// Updating the last heartbeat of a peer
	db_update_peer_heartbeat(db, &pa);

	// Get the peer address information
	struct peer_address * pa_data;
	db_get_all_peer_addresses(db, &pa_data, &n_items);

	for (unsigned int i = 0; i < n_items; i++) {
		printf("%d, %d, %d\n", pa_data[i].family, pa_data[i].port, pa_data[i].addr);
	}

	free(pa_data);

	// Remove peers of which the heartbeat has outdated
	db_remove_outdated_peers(db, 60);

	// Close the database
	db_close(db);

	return 0;
}

/ *****************************/