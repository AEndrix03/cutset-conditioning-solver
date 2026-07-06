#include "csp.hpp"

#include <iostream>
#include <memory>

int main() {
    Domains domains = {
            {1, 2, 3},
            {1, 2, 3},
            {1, 2, 3}
    };

    CSP csp("toy_chain_lt", domains);

    csp.add_constraint(std::make_unique<LessThanConstraint>(0, 1, "X0 < X1"));
    csp.add_constraint(std::make_unique<LessThanConstraint>(1, 2, "X1 < X2"));

    Assignment a = csp.empty_assignment();

    // prova assegnamento parziale
    a[0] = 1;
    a[1] = 2;

    std::cout << csp.name() << "\n";
    std::cout << "Consistente parziale? " << csp.is_consistent(a) << "\n";

    // completa: X0=1, X1=2, X2=3
    a[2] = 3;

    std::cout << "Completo? " << csp.is_complete(a) << "\n";
    std::cout << "Consistente completo? " << csp.is_consistent(a) << "\n";

    auto graph = csp.primal_graph();

    for (Var x = 0; x < static_cast<Var>(graph.size()); ++x) {
        std::cout << "X" << x << ": ";
        for (Var y : graph[x]) {
            std::cout << "X" << y << " ";
        }
        std::cout << "\n";
    }

    return 0;
}