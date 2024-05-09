#ifndef HOMFA_GRAPH_HPP
#define HOMFA_GRAPH_HPP

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <spot/twa/twagraph.hh>

class Graph {
public:
    using State = int;
    // index, next when 0, next when 1
    using DFADelta = std::vector<std::tuple<State, State, State>>;
    using NFADelta =
        std::vector<std::tuple<State, std::vector<State>, std::vector<State>>>;

private:
    DFADelta delta_;
    std::vector<std::vector<State>> parents0_, parents1_;
    std::vector<std::vector<State>> states_at_depth_;
    std::set<State> final_state_;
    std::vector<bool> final_state_vec_;
    State init_state_;

public:
    Graph();
    Graph(State init_st, const std::set<State>& final_sts,
          const DFADelta& delta);

    static Graph from_istream(std::istream& is);
    static Graph from_file(const std::string& filename);
    static Graph from_att_istream(std::istream& is);
    static Graph from_att_file(const std::string& filename);
    static Graph from_nfa(const std::set<State>& init_sts,
                          const std::set<State>& final_sts,
                          const NFADelta& delta);
    static Graph from_ltl_formula(const std::string& formula, size_t var_size,
                                  bool make_all_live_states_final);
    static Graph from_ltl_formula_reversed(const std::string& formula,
                                           size_t var_size,
                                           bool make_all_live_states_final);

    size_t size() const;
    bool is_final_state(State state) const;
    State next_state(State state, bool input) const;
    const std::vector<State>& prev_states(State state, bool input) const;
    State transition64(State src, uint64_t input, int length) const;
    State initial_state() const;
    void reserve_states_at_depth(size_t depth);
    std::vector<State> states_at_depth(size_t depth) const;
    std::vector<State> all_states() const;
    std::vector<std::vector<State>> track_live_states(
        const std::vector<State>& init_live_states, size_t max_depth);
    Graph reversed() const;
    Graph minimized() const;
    Graph removed_unreachable() const;
    Graph grouped_nondistinguishable() const;
    Graph negated() const;
    void dump(std::ostream& os) const;
    void dump_dot(std::ostream& os) const;
    void dump_att(std::ostream& os) const;

private:
    static std::tuple<std::set<State>, std::set<State>, NFADelta>
    ltl_to_nfa_tuple(const std::string& formula, size_t var_size,
                     bool make_all_live_states_final);
    static NFADelta reversed_nfa_delta(const NFADelta& src);
};

spot::twa_graph_ptr ltl_to_monitor(const std::string& formula, size_t var_size,
                                   bool deterministic);
#endif
