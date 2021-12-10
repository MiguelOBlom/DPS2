template <typename T, typename S>
class IProofOfWork {
	public:
		virtual IProofOfWork();
		virtual ~IProofOfWork();
		virtual void CreateProblem(T* seed);
		virtual S SolveProblem();
		virtual CheckProblem(T* seed, S* solution);
};