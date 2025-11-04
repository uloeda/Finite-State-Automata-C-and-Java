#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_STATES 100
#define MAX_TRANSITIONS 500
#define EPSILON '\0'

// Structure to represent a transition
typedef struct {
    int from_state;
    int to_state;
    char symbol;
} Transition;

// Structure to represent the FSA
typedef struct {
    int states[MAX_STATES];
    int num_states;
    bool is_start[MAX_STATES];
    bool is_accepting[MAX_STATES];
    Transition transitions[MAX_TRANSITIONS];
    int num_transitions;
} FSA;

// Structure for state set (used in closure, next, and DFA conversion)
typedef struct {
    int states[MAX_STATES];
    int size;
} StateSet;

// Function prototypes
void initFSA(FSA *fsa);
void addState(FSA *fsa, int state, bool is_start, bool is_accepting);
void addTransition(FSA *fsa, int from, int to, char symbol);
bool accepts(FSA *fsa, const char *input);
StateSet closure(FSA *fsa, int state);
StateSet closureSet(FSA *fsa, StateSet *states);
StateSet next(FSA *fsa, int state, char symbol);
StateSet nextSet(FSA *fsa, StateSet *states, char symbol);
bool deterministic(FSA *fsa);
FSA* toDFA(FSA *fsa);
void printStateSet(StateSet *set);
bool stateSetContains(StateSet *set, int state);
void addToStateSet(StateSet *set, int state);
bool stateSetEqual(StateSet *s1, StateSet *s2);
void copyStateSet(StateSet *dest, StateSet *src);

// Initialize FSA
void initFSA(FSA *fsa) {
    fsa->num_states = 0;
    fsa->num_transitions = 0;
    for (int i = 0; i < MAX_STATES; i++) {
        fsa->is_start[i] = false;
        fsa->is_accepting[i] = false;
    }
}

// Add a state to the FSA
void addState(FSA *fsa, int state, bool is_start, bool is_accepting) {
    // Check if state already exists
    bool exists = false;
    for (int i = 0; i < fsa->num_states; i++) {
        if (fsa->states[i] == state) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        fsa->states[fsa->num_states++] = state;
    }

    fsa->is_start[state] = is_start;
    fsa->is_accepting[state] = is_accepting;
}

// Add a transition to the FSA
void addTransition(FSA *fsa, int from, int to, char symbol) {
    if (fsa->num_transitions < MAX_TRANSITIONS) {
        fsa->transitions[fsa->num_transitions].from_state = from;
        fsa->transitions[fsa->num_transitions].to_state = to;
        fsa->transitions[fsa->num_transitions].symbol = symbol;
        fsa->num_transitions++;
    }
}

// Check if state is in set
bool stateSetContains(StateSet *set, int state) {
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) {
            return true;
        }
    }
    return false;
}

// Add state to set if not already present
void addToStateSet(StateSet *set, int state) {
    if (!stateSetContains(set, state) && set->size < MAX_STATES) {
        set->states[set->size++] = state;
    }
}

// Compute epsilon closure of a single state
StateSet closure(FSA *fsa, int state) {
    StateSet result = {.size = 0};
    StateSet stack = {.size = 0};

    addToStateSet(&result, state);
    addToStateSet(&stack, state);

    while (stack.size > 0) {
        int current = stack.states[--stack.size];

        for (int i = 0; i < fsa->num_transitions; i++) {
            if (fsa->transitions[i].from_state == current &&
                fsa->transitions[i].symbol == EPSILON) {
                int next_state = fsa->transitions[i].to_state;
                if (!stateSetContains(&result, next_state)) {
                    addToStateSet(&result, next_state);
                    addToStateSet(&stack, next_state);
                }
            }
        }
    }

    return result;
}

// Compute epsilon closure of a set of states
StateSet closureSet(FSA *fsa, StateSet *states) {
    StateSet result = {.size = 0};

    for (int i = 0; i < states->size; i++) {
        StateSet single_closure = closure(fsa, states->states[i]);
        for (int j = 0; j < single_closure.size; j++) {
            addToStateSet(&result, single_closure.states[j]);
        }
     }

    return result;
}

// Get states reachable from a state with a given symbol
StateSet next(FSA *fsa, int state, char symbol) {
    StateSet result = {.size = 0};

    // First compute epsilon closure of the state
    StateSet start_closure = closure(fsa, state);

    // Find all transitions with the given symbol
    for (int i = 0; i < start_closure.size; i++) {
        int current = start_closure.states[i];
        for (int j = 0; j < fsa->num_transitions; j++) {
            if (fsa->transitions[j].from_state == current &&
                fsa->transitions[j].symbol == symbol) {
                addToStateSet(&result, fsa->transitions[j].to_state);
            }
        }
    }

    // Compute epsilon closure of result
    result = closureSet(fsa, &result);

    return result;
}

// Get states reachable from a set of states with a given symbol
StateSet nextSet(FSA *fsa, StateSet *states, char symbol) {
    StateSet result = {.size = 0};

    for (int i = 0; i < states->size; i++) {
        StateSet single_next = next(fsa, states->states[i], symbol);
        for (int j = 0; j < single_next.size; j++) {
            addToStateSet(&result, single_next.states[j]);
        }
    }

    return result;
}

// Check if the FSA accepts a given string
bool accepts(FSA *fsa, const char *input) {
    // Find start state
    int start_state = -1;
    for (int i = 0; i < fsa->num_states; i++) {
        if (fsa->is_start[fsa->states[i]]) {
            start_state = fsa->states[i];
            break;
        }
    }

    if (start_state == -1) {
        return false;
    }

    // Compute epsilon closure of start state
    StateSet current_states = closure(fsa, start_state);

    // Process each character in input
    for (int i = 0; input[i] != '\0'; i++) {
        current_states = nextSet(fsa, &current_states, input[i]);
        if (current_states.size == 0) {
            return false;
        }
    }

    // Check if any current state is accepting
    for (int i = 0; i < current_states.size; i++) {
        if (fsa->is_accepting[current_states.states[i]]) {
            return true;
        }
    }

    return false;
}

// Check if FSA is deterministic
bool deterministic(FSA *fsa) {
    // Check for epsilon transitions
    for (int i = 0; i < fsa->num_transitions; i++) {
        if (fsa->transitions[i].symbol == EPSILON) {
            return false;
        }
    }

    // Check for multiple transitions with same symbol from same state
    for (int i = 0; i < fsa->num_transitions; i++) {
        for (int j = i + 1; j < fsa->num_transitions; j++) {
            if (fsa->transitions[i].from_state == fsa->transitions[j].from_state &&
                fsa->transitions[i].symbol == fsa->transitions[j].symbol) {
                return false;
            }
        }
    }

    return true;
}

// Helper functions for state set comparison
bool stateSetEqual(StateSet *s1, StateSet *s2) {
    if (s1->size != s2->size) return false;

    for (int i = 0; i < s1->size; i++) {
        if (!stateSetContains(s2, s1->states[i])) {
            return false;
        }
    }
    return true;
}

void copyStateSet(StateSet *dest, StateSet *src) {
    dest->size = src->size;
    for (int i = 0; i < src->size; i++) {
        dest->states[i] = src->states[i];
    }
}

// Convert NFA to DFA using subset construction
FSA* toDFA(FSA *fsa) {
    FSA *dfa = (FSA *)malloc(sizeof(FSA));
    initFSA(dfa);

    // Find start state and compute its closure
    int start_state = -1;
    for (int i = 0; i < fsa->num_states; i++) {
        if (fsa->is_start[fsa->states[i]]) {
            start_state = fsa->states[i];
            break;
        }
    }

    StateSet dfa_states[MAX_STATES];
    int num_dfa_states = 0;
    StateSet unmarked[MAX_STATES];
    int num_unmarked = 0;

    // Start state of DFA is epsilon closure of NFA start state
    StateSet start_closure = closure(fsa, start_state);
    copyStateSet(&dfa_states[num_dfa_states++], &start_closure);
    copyStateSet(&unmarked[num_unmarked++], &start_closure);

    // Get alphabet (collect all non-epsilon symbols)
    char alphabet[256];
    int alphabet_size = 0;
    for (int i = 0; i < fsa->num_transitions; i++) {
        if (fsa->transitions[i].symbol != EPSILON) {
            bool found = false;
            for (int j = 0; j < alphabet_size; j++) {
                if (alphabet[j] == fsa->transitions[i].symbol) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                alphabet[alphabet_size++] = fsa->transitions[i].symbol;
            }
        }
    }

    // Process unmarked states
    while (num_unmarked > 0) {
        StateSet current = unmarked[--num_unmarked];

        for (int a = 0; a < alphabet_size; a++) {
            StateSet next_states = nextSet(fsa, &current, alphabet[a]);

            if (next_states.size > 0) {
                // Check if this state set already exists
                int existing_state = -1;
                for (int i = 0; i < num_dfa_states; i++) {
                    if (stateSetEqual(&dfa_states[i], &next_states)) {
                        existing_state = i;
                        break;
                    }
                }

                if (existing_state == -1) {
                    // New state
                    copyStateSet(&dfa_states[num_dfa_states], &next_states);
                    copyStateSet(&unmarked[num_unmarked++], &next_states);
                    existing_state = num_dfa_states++;
                }

                // Add transition in DFA
                int from_index = -1;
                for (int i = 0; i < num_dfa_states; i++) {
                    if (stateSetEqual(&dfa_states[i], &current)) {
                        from_index = i;
                        break;
                    }
                }

                addTransition(dfa, from_index, existing_state, alphabet[a]);
            }
        }
    }

    // Add states to DFA and mark accepting states
    for (int i = 0; i < num_dfa_states; i++) {
        bool is_accepting = false;
        for (int j = 0; j < dfa_states[i].size; j++) {
            if (fsa->is_accepting[dfa_states[i].states[j]]) {
                is_accepting = true;
                break;
            }
        }
        addState(dfa, i, i == 0, is_accepting);
    }

    return dfa;
}

// Print state set
void printStateSet(StateSet *set) {
    printf("{");
    for (int i = 0; i < set->size; i++) {
        printf("%d", set->states[i]);
        if (i < set->size - 1) printf(",");
    }
    printf("}");
}

// Main function with example usage
int main() {
    FSA fsa;
    initFSA(&fsa);

    // Build the example FSA from the assignment
    addState(&fsa, 0, true, false);
    addState(&fsa, 1, false, false);
    addState(&fsa, 2, false, false);
    addState(&fsa, 3, false, false);
    addState(&fsa, 4, false, false);
    addState(&fsa, 5, false, false);
    addState(&fsa, 6, false, false);
    addState(&fsa, 7, false, false);
    addState(&fsa, 8, false, false);
    addState(&fsa, 9, false, false);
    addState(&fsa, 10, false, true);

    // Add transitions (epsilon is '\0')
    addTransition(&fsa, 0, 1, EPSILON);
    addTransition(&fsa, 0, 7, EPSILON);
    addTransition(&fsa, 1, 2, EPSILON);
    addTransition(&fsa, 1, 4, EPSILON);
    addTransition(&fsa, 2, 3, 'a');
    addTransition(&fsa, 3, 6, EPSILON);
    addTransition(&fsa, 4, 5, 'b');
    addTransition(&fsa, 5, 6, EPSILON);
    addTransition(&fsa, 6, 1, EPSILON);
    addTransition(&fsa, 6, 7, EPSILON);
    addTransition(&fsa, 7, 8, 'a');
    addTransition(&fsa, 8, 9, 'b');
    addTransition(&fsa, 9, 10, 'b');

    // Test operations
    printf("Testing FSA Operations:\n\n");

    // Test closure
    printf("Closure of state 3: ");
    StateSet c = closure(&fsa, 3);
    printStateSet(&c);
    printf("\n\n");

    // Test next
    printf("Next from state 4 with 'b': ");
    StateSet n = next(&fsa, 4, 'b');
    printStateSet(&n);
    printf("\n\n");

    // Test deterministic
    printf("Is deterministic: %s\n\n", deterministic(&fsa) ? "true" : "false");

    // Test accepts
    printf("Accepts 'abb': %s\n", accepts(&fsa, "abb") ? "true" : "false");
    printf("Accepts 'aabb': %s\n", accepts(&fsa, "aabb") ? "true" : "false");
    printf("Accepts 'babb': %s\n", accepts(&fsa, "babb") ? "true" : "false");
    printf("Accepts 'ab': %s\n\n", accepts(&fsa, "ab") ? "true" : "false");

    // Convert to DFA
    printf("Converting to DFA...\n");
    FSA *dfa = toDFA(&fsa);
    printf("DFA has %d states\n", dfa->num_states);
    printf("DFA is deterministic: %s\n\n", deterministic(dfa) ? "true" : "false");

    // Test DFA accepts same strings
    printf("DFA accepts 'abb': %s\n", accepts(dfa, "abb") ? "true" : "false");
    printf("DFA accepts 'aabb': %s\n", accepts(dfa, "aabb") ? "true" : "false");

    free(dfa);

    return 0;
}
