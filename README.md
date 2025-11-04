# Finite-State-Automata-C-and-Java
C implementation 



What the language made easier 

● Predictable memory layout. Plain old data (Transition, FSA, StateSet) as fixed 
arrays gives tight control over memory and iteration. You know the cost of every 
operation and avoid GC churn. 

● Simple control flow. for loops over contiguous arrays align with the algorithms 
taught in class. Debugging with printf shows raw state without abstraction noise. 

● Low overhead. No virtual dispatch, boxing, or hashing. For very small automata the 
C version is fast by default due to cache locality. 



What the language made harder 

● Set algebra is manual. StateSet uses linear arrays. Membership, union, and 
equality are O(n) with explicit loops. This inflates the cost and code size of closure, 
nextSet, and toDFA. 

● Alphabet management is ad-hoc. The alphabet is collected by scanning transitions 
and deduplicating into a char array. This is fragile if ε is mishandled and adds 
O(|T|·|Σ|) overhead. 

● Subset construction bookkeeping. Mapping “set of NFA states → DFA state id” 
uses linear search over previously created StateSets with stateSetEqual. As the DFA 
grows, this turns into an O(k·|Q|) hotspot, where k is the number of DFA states 
discovered. 

● Safety and errors. You must enforce MAX_STATES and MAX_TRANSITIONS. 
There is no bounds checking or ownership model. A single off-by-one or missing 
guard can corrupt memory. Extending the model requires more hand edits. 

● Expressiveness. The type system does not encode invariants. For example, nothing 
prevents adding a transition involving a state that is not yet marked present unless 
you check manually. 



Net effect on each required operation 

● addState/addTransition: O(1) append into arrays. Easy to write, but relies on caller 
discipline and bounds guards. 

● closure: Stack-based DFS over ε-edges. Because set membership is linear and 
transitions are scanned linearly, worst-case cost is O(|Q|·|T|). Still fine for the given 
automaton. 

● next/nextSet: ε-close the source, scan transitions by symbol, ε-close the result. 
Same O(|Q|·|T|) amplification due to linear set ops. 

● accepts: Iterates the characters and calls nextSet. Correct, but runtime is tied to the 
previous costs. 

● deterministic: Detects ε-edges and duplicate (from,symbol) pairs via double loops. 
O(|T|^2) worst case. 

● toDFA: Correct subset construction, but uses linear search to decide whether a 
subset is new. Works for the assignment graph; scales poorly as DFA size increases. 
What I would change in C if I extended it 

● Replace StateSet arrays with a 64- or 128-bit bitset (if |Q|≤128) or an 
open-addressing hash set. This drops membership/equality to O(1) expected and 
turns subset interning into raw integer hash keys. 

● Index transitions by (state,symbol) into contiguous ranges to avoid scanning all 
transitions. 

● Centralize bounds checks and return error codes from mutators to fail fast under 
overflow.
