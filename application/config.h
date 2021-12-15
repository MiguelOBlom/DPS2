// Socket family
#define DOMAIN AF_INET
// CRC Polynomial
#define POLY_TYPE uint16_t
#define POLY 0b1011101011010101
// Peer configuration
#define HEARTBEAT_PERIOD 5
// Tracker configuration
#define TIMEOUT_THRESHOLD 60
#define TRACKER_QUEUE_SIZE 20
// Application configuration
#define DIFFICULTY 24
#define MAX_TRANSACTIONS 5
#define ID_TYPE char
// Chance data gets mutated (on receive)
#define BITFLIP_CHANCE 10