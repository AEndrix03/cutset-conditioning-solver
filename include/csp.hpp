#include <vector>
#include <string>

// alias per descrivere meglio i problemi
using Var = int;
using Value = int;
using Domain = std::vector<int>;
using Domains = std::vector<Domain>;
using Assignment = std::vector<int>; // con -1 = non assegnato

struct BinaryConstraint {
	Var a, b;
	std::function<bool(Value, Value)> ok;
	std::string name;
}

struct CSP {
	std::string name;
	int nvars;
	Domains domains;
	std::vector<BinaryConstraint> constraints>;
}

struct SolverStats {
	long long nodes = 0;
	long long constraints_check = 0;
	long long cutset_assignments = 0;
	int cutset_size = 0;
	double milliseconds = 0.0;
	bool solved = false;
	bool timed_out = false;
}
