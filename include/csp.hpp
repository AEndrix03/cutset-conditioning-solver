// Questa classe contiene i tipi base per un CSP, con tipi come alias e concetti vari

#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>

using Var = int; // Alias per rappresentare una variabile
using Value = int; // Alias per rappresentare un valore assunto da una variabile
using Domain = std::vector<Value>; // Alias che rappresenta il dominio di una variabile, ossia la lista dei valori che la var può assumere
using Domains = std::vector<Domain>; // Domini
using Assignment = std::vector<Value>; // Rappresenta un'assegnazione parziale o completa
using Graph = std::vector<std::vector<Var>>;

constexpr Value UNASSIGNED = -1; // CONVENZIONE DI PROGETTO

/**
 * Statistiche Solver, misura ciò che succede durante la risoluzione del CSP con il Solver
 */
struct SolverStats {
    long long nodes = 0;
    long long constraint_checks = 0;
    long long cutset_assignments = 0;
    int cutset_size = 0;
    double milliseconds = 0.0;
    bool solved = false;
};

/*
 * Concetti teorici da ricordare:
 * - Assegnamento:
 *      - Parziale: valori assegnati a una variabile, ma ancora non tutti;
 *      - Completo: valori assegnati a una variabile, tutti.
 * - Consistenza:
 *      - Un'assegnamento è consistente se per ogni x assegnato si rispettano i vincoli (constraints)
 *
 * Un assegnamento è detto Soluzione se Completo e Consistente!
 */

/**
 * Rappresenta un qualsiasi vincolo (constraint) binario, ossia che relaziona su due variabili.
 * Implementazione generalizzata per poter costruire vincoli specifici con dei nomi.
 *
 * Un vincolo binario collega due variabili del CSP, quindi richiede le due variabili
 */
class BinaryConstraint {
public:
    BinaryConstraint(Var first, Var second, std::string name);

    virtual ~BinaryConstraint() = default;

    Var first() const;
    Var second() const;
    const std::string& name() const;

    /**
     * @param x Variabile
     * @return Ritorna true se la variabile passata costituisce il vincolo, false altrimenti
     */
    bool involves(Var x) const;

    /**
     * @param x Variabile X
     * @param y Variabile Y
     * @return Ritorna true se il vincolo collega X e Y, false altrimenti
     */
    bool connects(Var x, Var y) const;

    /**
     * @param x Variabile
     * @return Data una delle due variabili, ritorna l'altra. Utile al backtracing
     */
    Var other(Var x) const;


    /**
     * @param first_value Valore assegnato alla prima variabile
     * @param second_value Valore assegnato alla seconda variabile
     * @return Se soddisfa il vincolo, true
     */
    virtual bool is_satisfied(Value first_value, Value second_value) const = 0;

private:
    Var first_;
    Var second_;
    std::string name_;
};

/**
 * Sottoclasse di vincolo, su uguaglianze
 */
class NotEqualConstraint : public BinaryConstraint {
public:
    NotEqualConstraint(Var first, Var second, std::string name = "");

    bool is_satisfied(Value a, Value b) const override;
};

/**
 * Sottoclasse di vincolo, su comparazioni
 */
class LessThanConstraint : public BinaryConstraint {
public:
    LessThanConstraint(Var first, Var second, std::string name = "");

    bool is_satisfied(Value a, Value b) const override;
};

/**
 * Sottoclasse di vincolo, su funzioni predicate qualsiasi (default)
 */
class PredicateConstraint : public BinaryConstraint {
public:
    PredicateConstraint(Var first, Var second, std::function<bool(Value, Value)> predicate, std::string name = "");

    bool is_satisfied(Value a, Value b) const override;

private:
    std::function<bool(Value, Value)> predicate_;
};


/**
* Classe principale del progetto, rappresenta un CSP, ossia un
 * Constraint Satisfaction Problem
*/
class CSP {
public:
    CSP(std::string name, Domains domains);

    const std::string& name() const; // Nome del problema
    int nvars() const; // Numero di variabili

    const Domains& domains() const; // Domini delle variabili
    const std::vector<std::unique_ptr<BinaryConstraint>>& constraints() const; // Vincoli del problema

    Assignment empty_assignment() const; // Crea un assegnamento vuoto, CONVENZIONE DI PROGETTO: Valore -1 se non assegnato

    void add_constraint(std::unique_ptr<BinaryConstraint> constraint); // Aggiunge un vincolo al CSP

    /**
     * Controlla tutti i vincoli tra due variabili assegnate, serve perchè tra la stessa coppia possono esistere più
     * vincoli, anche se il primal graph mostrerà solo un arco.
     * @param x Variabile X
     * @param vx Valore associato alla Variabile X
     * @param y Variabile Y
     * @param vy Valore associato alla Variabile Y
     * @param stats Statistica Solver
     */
    bool constraints_ok_pair(Var x, Value vx, Var y, Value vy, SolverStats* stats = nullptr) const;

    /**
     * Controlla se l'assegnamento parziale è consistente rispetto la sola Variabile X passata.
     * @param x Variabile X
     * @param assignment Assegnamento parziale (o completo)
     */
    bool is_consistent_var(Var x, const Assignment& assignment, SolverStats* stats = nullptr) const;

    /**
     * Controlla se l'assegnamento è consistente considerando tutti i vincoli
     * @param assignment Assegnamento
     * @return
     */
    bool is_consistent(const Assignment& assignment, SolverStats* stats = nullptr) const;

    /**
     * @param assignment Assegnamento
     * @return True se tutte le variabili sono != -1 (CONVENZIONE DI PROGETTO)
     */
    bool is_complete(const Assignment& assignment) const;

    /**
     * Costruisce il grafo primale
     *
     * Nodo = Variabile
     * Arco = Vincolo tra due nodi
     * @return
     */
    Graph primal_graph() const; // Grafo primale del CSP

private:
    std::string name_;
    Domains domains_;
    std::vector<std::unique_ptr<BinaryConstraint>> constraints_;
};