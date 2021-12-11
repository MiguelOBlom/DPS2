#ifndef _IPROOFOFWORK_
#define _IPROOFOFWORK_

template <typename T, typename S>
class IProofOfWork {
	public:
		IProofOfWork() {};
		virtual ~IProofOfWork() {};
		virtual S SolveProblem(T* seed) = 0;
		virtual bool CheckSolution(T* seed, S* solution) = 0;
};

#endif