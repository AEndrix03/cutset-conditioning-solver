#include "csp.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {

    constexpr Value UNASSIGNED = -1; // CONVENZIONE DI PROGETTO

    // Utilities varie
    void check_var(Var x, int nvars) {
        if (x < 0 || x >= nvars) {
            throw std::out_of_range("Var index out of range!");
        }
    }

    void check_assignment_size(const Assignment& assignment, int nvars) {
        if (static_cast<int>(assignment.size()) != nvars) {
            throw std::invalid_argument("Invalid assignment size!");
        }
    }
}

BinaryConstraint::BinaryConstraint(Var first, Var second, std::string name): first_(first), second_(second), name_(std::move(name)) {
    if (first == second) {
        throw std::invalid_argument("Vars cannot be same!");
    }
}

Var BinaryConstraint::first() const {
    return first_;
}

Var BinaryConstraint::second() const {
    return second_;
}

const std::string &BinaryConstraint::name() const {
    return name_;
}

bool BinaryConstraint::involves(Var x) const {
    return first_ == x || second_ == x;
}

bool BinaryConstraint::connects(Var x, Var y) const {
    return (first_ == x && second_ == y)  || (first_ == y && second_ == x);
}

Var BinaryConstraint::other(Var x) const {
    if (first_ == x) {
        return second_;
    } else if (second_ == x) {
        return first_;
    } else {
        throw std::invalid_argument("Var is not involved in this constraint");
    }
}

NotEqualConstraint::NotEqualConstraint(Var first, Var second, std::string name)
        : BinaryConstraint(first, second, std::move(name)) {
}

bool NotEqualConstraint::is_satisfied(Value a, Value b) const {
    return a != b;
}

LessThanConstraint::LessThanConstraint(Var first, Var second, std::string name)
        : BinaryConstraint(first, second, std::move(name)) {
}

bool LessThanConstraint::is_satisfied(Value a, Value b) const {
    return a < b;
}

PredicateConstraint::PredicateConstraint(Var first, Var second, std::function<bool(Value, Value)> predicate,
                                         std::string name) : BinaryConstraint(first, second, std::move(name)),
                                                             predicate_(std::move(predicate)) {
    if (!predicate_) {
        throw std::invalid_argument("PredicateConstraint needs a valid predicate");
    }
}

bool PredicateConstraint::is_satisfied(Value a, Value b) const {
    return predicate_(a, b);
}

CSP::CSP(std::string name, Domains domains) : name_(name), domains_(domains) {
    if (domains_.empty()) {
        throw std::invalid_argument("A CSP must have at least one variable");
    }

    for (const auto& domain : domains_) {
        if (domain.empty()) {
            throw std::invalid_argument("Each variable must have a non-empty domain"); // Altrimenti sarebbe impossibile!
        }

        if (std::find(domain.begin(), domain.end(), UNASSIGNED) != domain.end()) {
            throw std::invalid_argument("Domain cannot contain UNASSIGNED value");
        }
    }
}

const std::string &CSP::name() const {
    return name_;
}

const Domains &CSP::domains() const {
    return domains_;
}

int CSP::nvars() const {
    return static_cast<int>(domains_.size());
}

const std::vector <std::unique_ptr<BinaryConstraint>> &CSP::constraints() const {
    return constraints_;
}

Assignment CSP::empty_assignment() const {
    return Assignment(nvars(), UNASSIGNED);
}

// Aggiunge un vincolo
void CSP::add_constraint(std::unique_ptr <BinaryConstraint> constraint) {
    if (!constraint) {
        throw std::invalid_argument("Cannot add null constraints");
    }

    check_var(constraint->first(), nvars());
    check_var(constraint->second(), nvars());

    constraints_.push_back(std::move(constraint)); // unique ptr non copia, va fatto move
}

// Funzione cuore
bool CSP::constraints_ok_pair(Var x, Value vx, Var y, Value vy, SolverStats *stats) const {
    check_var(x, nvars());
    check_var(y, nvars());

    for (const auto& constraint : constraints_) {
        // Skip archi non collegati tra loro
        if (!constraint->connects(x, y)) continue;

        // Conteggio controlli su vincoli
        if (stats != nullptr) stats->constraint_checks++;

        bool ok;

        // ATTENZIONE: controllare ordinatamente le variabili!
        if (constraint->first() == x && constraint->second() == y) ok = constraint->is_satisfied(vx, vy);
        else if (constraint->first() == y && constraint->second() == x) ok = constraint->is_satisfied(vy, vx);

        if (!ok) return false;
    }

    return true;
}

bool CSP::is_consistent_var(Var x, const Assignment &assignment, SolverStats * stats) const {
    check_var(x, nvars());
    check_assignment_size(assignment, nvars());

    // Si controlla solo i vincoli valutabili, ossia su variabili assegnate. True non degrada il check
    if (assignment[x] == UNASSIGNED) return true;

    std::vector<bool> checked(nvars(), false);

    for (const auto& constraint : constraints_) {
        if (!constraint->involves(x)) continue; // non riguarda X

        Var y = constraint->other(x);

        if (checked[y]) continue;

        checked[y] = true;

        if (assignment[y] == UNASSIGNED) continue;

        // Si controlla la consistenza di arco
        if (!constraints_ok_pair(x, assignment[x], y, assignment[y], stats)) {
            return false;
        }
    }

    return true;
}

bool CSP::is_consistent(const Assignment &assignment, SolverStats *stats) const {
    check_assignment_size(assignment, nvars());

    for (const auto& constraint: constraints_) {
        Var x = constraint->first();
        Var y = constraint->second();

        if (assignment[x] == UNASSIGNED || assignment[y] == UNASSIGNED) continue;

        if (stats != nullptr) stats->constraint_checks++;

        if (!constraint->is_satisfied(assignment[x], assignment[y])) return false;
    }

    return true;
}

bool CSP::is_complete(const Assignment &assignment) const {
    check_assignment_size(assignment, nvars());

    for (Value value : assignment) {
        if (value == UNASSIGNED) return false;
    }

    return true;
}

std::vector <std::vector<Var>> CSP::primal_graph() const {
    std::vector<std::vector<Var>> graph(nvars());
    // Lista di adiacenze, tipo graph[0] = {}, graph[1] = {} ...

    for (const auto& constraint : constraints_) {
        // Prendo le variabile del vincolo
        Var x = constraint->first();
        Var y = constraint->second();

        // Cerco i collegamenti per x e y, controllando per end() evito duplicati
        auto& gx = graph[x];
        if (std::find(gx.begin(), gx.end(), y) == gx.end()) gx.push_back(y);

        // Cerco i collegamenti per y e x
        auto& gy = graph[y];
        if (std::find(gy.begin(), gy.end(), x) == gy.end()) gy.push_back(x);
    }

    return graph;
}
