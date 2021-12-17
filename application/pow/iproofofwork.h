/*
	Interface class for the proof-of-work mechanism.
	It describes the requirement of a method that solves 
	a problem (which is a function of the input data "seed").
	It also describes the requirement of a method that 
	checks the solution to the problem (given the same data)
	and the solution that is to be verified.

	The data is of a generic type and so is the solution 
	the the problem. This is considered for modularity.

	Author: Miguel Blom
*/


#ifndef _IPROOFOFWORK_
#define _IPROOFOFWORK_

template <typename T, typename S>
class IProofOfWork {
	public:
		IProofOfWork() {};
		virtual ~IProofOfWork() {};
		// Returns a solution of type S to the problem that is subject of a seed of type T
		virtual S SolveProblem(T* seed) = 0;
		// Verifies a solution of type S to the problem that is subject of a seed of type T
		virtual bool CheckSolution(T* seed, S* solution) = 0;
};

#endif
