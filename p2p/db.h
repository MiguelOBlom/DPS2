/*
	Author: Miguel Blom

	This adapter code was created for abstracting the interaction between 
	a peer to peer tracker with the sqlite3 library. The code could easily be
	switched out for any other adapter, as long as the global definitions
	present in this header (API) file persists (and ofcourse the functionality
	must be correct).

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <stdint.h>

// Structure for the peer address, consisting of the ip, port and connection family
struct peer_address{
	short unsigned int family; // sockaddr_in::sin_family is of type sa_family_t {aka short unsigned int}
	short unsigned int port; // sockaddr_in::sin_port is of type in_port_t {aka short unsigned int}
	uint32_t addr; // sockaddr_in::sin_addr.s_addr is of type uint32_t
};

// Additional information to the peer's address for the tracker can be specified in this struct
struct peer_info {
	struct peer_address sockaddr;
	uint64_t heartbeat; // Moment of last recorded heartbeat (long unsigned int)
};

int cmp_peer_address(const struct peer_address* pa1, const struct peer_address* pa2);

/*
	SQLite can only store one of the five classes:
		NULL		A Null value
		INTEGER		Signed Integer of 1, 2, 3 ,4 ,6 or 8 bytes
		REAL 		8-byte floating point
		TEXT		String with UTF-8, UTF-16BE or UTF-16LE encoding
		BLOB		A blob of data

	Since we are handling unsigned values for the peer_address family, port and address
	it is most naturaly to choose the BLOB type, since the INTEGER type is forced to be 
	signed.

	Furthermore, we should recognize workers by their full address, 
	so this would make up their Primary Key.

	We will store the datetime of the last heartbeat as unixepoch, since it describes 
	seconds, which is all we need to check for a heartbeat every 5 seconds and consider
	a node lost after 60 seconds. No milliseconds are involved. This would require a
	long unsigned int sized datatype.
	
	CREATE TABLE IF NOT EXISTS PeerInfo (
		peer_family BLOB NOT NULL,
		peer_port BLOB NOT NULL,
		peer_addr BLOB NOT NULL,
		peer_heartbeat INT NOT NULL DEFAULT (strftime('%s', 'now')),
		PRIMARY KEY(peer_family, peer_port, peer_addr)
	)
	
	SELECT datetime(d1,'unixepoch')
*/


// **************** //
// Global functions //
// **************** //
// Check the .c file for the more elaborate explanations
void db_open (sqlite3 ** db, char * dbname);
void db_close (sqlite3 * db);
int  db_peer_exists (sqlite3 * db, const struct peer_address* pa);
void db_insert_peer (sqlite3* db, const struct peer_address* pa);
void db_update_peer_heartbeat (sqlite3 * db, const struct peer_address* pa);
void db_remove_outdated_peers (sqlite3* db, int timeout_threshold);
void db_get_all_peer_addresses (sqlite3* db, struct peer_address** data, size_t* n_items);
void db_get_all_peer_info (sqlite3* db, struct peer_info** data, size_t* n_items);
int  is_data_available(const int* sockfd);