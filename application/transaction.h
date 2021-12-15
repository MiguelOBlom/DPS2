
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

