import java.util.*;

// Abstract base class for states
abstract class State {
    protected String name;
    protected boolean isAccepting;

    public State(String name, boolean isAccepting) {
        this.name = name;
        this.isAccepting = isAccepting;
    }

    public String getName() {
        return name;
    }

    public boolean isAccepting() {
        return isAccepting;
    }

    public void setAccepting(boolean accepting) {
        this.isAccepting = accepting;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        State state = (State) o;
        return Objects.equals(name, state.name);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name);
    }

    @Override
    public String toString() {
        return name + (isAccepting ? " (accepting)" : "");
    }
}

// Concrete implementation of a standard state
class FSAState extends State {
    public FSAState(String name, boolean isAccepting) {
        super(name, isAccepting);
    }
}

// Transition class encapsulating state transitions
class Transition {
    private State fromState;
    private Character symbol; // null represents epsilon
    private State toState;

    public Transition(State fromState, Character symbol, State toState) {
        this.fromState = fromState;
        this.symbol = symbol;
        this.toState = toState;
    }

    public State getFromState() {
        return fromState;
    }

    public Character getSymbol() {
        return symbol;
    }

    public State getToState() {
        return toState;
    }

    public boolean isEpsilon() {
        return symbol == null;
    }

    @Override
    public String toString() {
        String sym = symbol == null ? "ε" : symbol.toString();
        return fromState.getName() + " --" + sym + "--> " + toState.getName();
    }
}

// Main FSA class
public class FiniteStateAutomaton {
    private Set<State> states;
    private Set<Character> alphabet;
    private Set<Transition> transitions;
    private State startState;
    private Map<State, Map<Character, Set<State>>> transitionMap;

    public FiniteStateAutomaton() {
        this.states = new HashSet<>();
        this.alphabet = new HashSet<>();
        this.transitions = new HashSet<>();
        this.transitionMap = new HashMap<>();
        this.startState = null;
    }

    /**
     * Add a new state to the FSA
     * @param name Name of the state
     * @param isStart Whether this is the start state
     * @param isAccepting Whether this is an accepting state
     */
    public void addState(String name, boolean isStart, boolean isAccepting) {
        State state = new FSAState(name, isAccepting);
        states.add(state);
        transitionMap.put(state, new HashMap<>());

        if (isStart) {
            startState = state;
        }
    }

    /**
     * Add a transition between two states
     * @param fromStateName Source state name
     * @param symbol Transition symbol (null for epsilon)
     * @param toStateName Destination state name
     */
    public void addTransition(String fromStateName, Character symbol, String toStateName) {
        State fromState = findState(fromStateName);
        State toState = findState(toStateName);

        if (fromState == null || toState == null) {
            throw new IllegalArgumentException("State not found");
        }

        Transition transition = new Transition(fromState, symbol, toState);
        transitions.add(transition);

        // Update transition map
        transitionMap.get(fromState)
            .computeIfAbsent(symbol, k -> new HashSet<>())
            .add(toState);

        // Add symbol to alphabet (if not epsilon)
        if (symbol != null) {
            alphabet.add(symbol);
        }
    }

    /**
     * Check if the FSA accepts the given string
     * @param input Input string to test
     * @return true if accepted, false otherwise
     */
    public boolean accepts(String input) {
        if (startState == null) {
            return false;
        }

        Set<State> currentStates = closure(startState);
        for (char c : input.toCharArray()) {
            Set<State> nextStates = new HashSet<>();
            for (State state : currentStates) {
                nextStates.addAll(next(state, c));
            }
            currentStates = nextStates;

            if (currentStates.isEmpty()) {
                return false;
            }
        }

        // Check if any current state is accepting
        for (State state : currentStates) {
            if (state.isAccepting()) {
                return true;
            }
        }

        return false;
    }

    /**
     * Compute epsilon closure of a state
     * @param state The state
     * @return Set of states reachable via epsilon transitions
     */
    public Set<State> closure(State state) {
        Set<State> closureSet = new HashSet<>();
        Queue<State> queue = new LinkedList<>();

        closureSet.add(state);
        queue.add(state);

        while (!queue.isEmpty()) {
            State current = queue.poll();
            Map<Character, Set<State>> transitions = transitionMap.get(current);

            if (transitions != null && transitions.containsKey(null)) {
                for (State epsilonState : transitions.get(null)) {
                    if (!closureSet.contains(epsilonState)) {
                        closureSet.add(epsilonState);
                        queue.add(epsilonState);
                    }
                }
            }
        }

        return closureSet;
    }

    /**
     * Compute epsilon closure for a set of states
     */
    private Set<State> closure(Set<State> stateSet) {
        Set<State> result = new HashSet<>();
        for (State state : stateSet) {
            result.addAll(closure(state));
        }
        return result;
    }

    /**
     * Get states reachable from given state on input symbol
     * @param state Source state
     * @param symbol Input symbol
     * @return Set of reachable states (including epsilon closure)
     */
    public Set<State> next(State state, char symbol) {
        Set<State> nextStates = new HashSet<>();
        Set<State> closureStates = closure(state);

        for (State s : closureStates) {
            Map<Character, Set<State>> transitions = transitionMap.get(s);
            if (transitions != null && transitions.containsKey(symbol)) {
                nextStates.addAll(transitions.get(symbol));
            }
        }

        return closure(nextStates);
    }

    /**
     * Check if the FSA is deterministic
     * @return true if DFA, false if NFA
     */
    public boolean deterministic() {
        // Check for epsilon transitions
        for (Map<Character, Set<State>> transitions : transitionMap.values()) {
            if (transitions.containsKey(null)) {
                return false;
            }
        }

        // Check for multiple transitions on same symbol
        for (State state : states) {
            Map<Character, Set<State>> transitions = transitionMap.get(state);
            for (Set<State> targetStates : transitions.values()) {
                if (targetStates.size() > 1) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * Convert NFA to equivalent DFA using subset construction
     * @return A new DFA equivalent to this FSA
     */
    public FiniteStateAutomaton toDFA() {
        FiniteStateAutomaton dfa = new FiniteStateAutomaton();

        // Map from set of NFA states to DFA state name
        Map<Set<State>, String> stateSetToName = new HashMap<>();
        Queue<Set<State>> unprocessed = new LinkedList<>();
        Set<Set<State>> processed = new HashSet<>();

        // Start with epsilon closure of start state
        Set<State> startClosure = closure(startState);
        String startName = setToString(startClosure);
        stateSetToName.put(startClosure, startName);

        boolean isAccepting = startClosure.stream().anyMatch(State::isAccepting);
        dfa.addState(startName, true, isAccepting);

        unprocessed.add(startClosure);

        while (!unprocessed.isEmpty()) {
            Set<State> currentSet = unprocessed.poll();
            if (processed.contains(currentSet)) {
                continue;
            }
            processed.add(currentSet);

            String currentName = stateSetToName.get(currentSet);

            // For each symbol in alphabet
            for (char symbol : alphabet) {
                Set<State> nextSet = new HashSet<>();

                // Compute next states for all states in current set
                for (State state : currentSet) {
                    nextSet.addAll(next(state, symbol));
                }

                if (!nextSet.isEmpty()) {
                    String nextName = stateSetToName.get(nextSet);

                    if (nextName == null) {
                        nextName = setToString(nextSet);
                        stateSetToName.put(nextSet, nextName);

                        boolean accepting = nextSet.stream().anyMatch(State::isAccepting);
                        dfa.addState(nextName, false, accepting);

                        unprocessed.add(nextSet);
                    }

                    dfa.addTransition(currentName, symbol, nextName);
                }
            }
        }

        return dfa;
    }

    // Helper method to convert set of states to string representation
    private String setToString(Set<State> stateSet) {
        List<String> names = new ArrayList<>();
        for (State s : stateSet) {
            names.add(s.getName());
        }
        Collections.sort(names);
        return "{" + String.join(",", names) + "}";
    }

    // Helper method to find state by name
    private State findState(String name) {
        for (State state : states) {
            if (state.getName().equals(name)) {
                return state;
            }
        }
        return null;
    }

    public void printFSA() {
        System.out.println("States: " + states);
        System.out.println("Alphabet: " + alphabet);
        System.out.println("Start State: " + startState);
        System.out.println("Transitions:");
        for (Transition t : transitions) {
            System.out.println("  " + t);
        }
    }

    // Test method with the example from the assignment
    public static void main(String[] args) {
        FiniteStateAutomaton fsa = new FiniteStateAutomaton();

        // Build the FSA from the assignment (accepts (a|b)*abb)
        fsa.addState("0", true, false);
        fsa.addState("1", false, false);
        fsa.addState("2", false, false);
        fsa.addState("3", false, false);
        fsa.addState("4", false, false);
        fsa.addState("5", false, false);
        fsa.addState("6", false, false);
        fsa.addState("7", false, false);
        fsa.addState("8", false, false);
        fsa.addState("9", false, false);
        fsa.addState("10", false, true);

        // Add transitions (null for epsilon)
        fsa.addTransition("0", null, "1");
        fsa.addTransition("1", null, "2");
        fsa.addTransition("1", null, "4");
        fsa.addTransition("2", 'a', "3");
        fsa.addTransition("3", null, "6");
        fsa.addTransition("4", 'b', "5");
        fsa.addTransition("5", null, "6");
        fsa.addTransition("6", null, "7");
        fsa.addTransition("6", null, "1");
        fsa.addTransition("7", 'a', "8");
        fsa.addTransition("8", 'b', "9");
        fsa.addTransition("9", 'b', "10");
        fsa.addTransition("0", null, "7");

        System.out.println("Original FSA:");
        fsa.printFSA();

        System.out.println("\nIs deterministic? " + fsa.deterministic());

        // Test accepts
        System.out.println("\nTesting accepts:");
        String[] testStrings = {"abb", "aabb", "babb", "ababb", "ab", "aa"};
        for (String test : testStrings) {
            System.out.println("  '" + test + "': " + fsa.accepts(test));
        }

        // Test closure
        System.out.println("\nClosure of state 3:");
        State state3 = fsa.findState("3");
        Set<State> closure3 = fsa.closure(state3);
        System.out.println("  " + closure3);

        // Test next
        System.out.println("\nNext states from state 4 on 'b':");
        State state4 = fsa.findState("4");
        Set<State> next4b = fsa.next(state4, 'b');
        System.out.println("  " + next4b);

        // Convert to DFA
        System.out.println("\n\nConverting to DFA...");
        FiniteStateAutomaton dfa = fsa.toDFA();
        dfa.printFSA();
        System.out.println("\nIs DFA deterministic? " + dfa.deterministic());

        // Test DFA accepts same strings
        System.out.println("\nTesting DFA accepts:");
        for (String test : testStrings) {
            System.out.println("  '" + test + "': " + dfa.accepts(test));
        }
    }
}
