/*
	This code defines the datastructures used for the transactions
	that are stored in the blocks in our blockchain.
	
	Author: Miguel Blom
*/

#ifndef _TRANSACTION_
#define _TRANSACTION_

template <typename ID>
struct Transaction {
	ID sender;
	ID receiver;
	double amount;
};

template <typename ID, unsigned int N>
struct Transactions {
	struct Transaction<ID> transaction[N];
};

#endif

