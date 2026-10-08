# CR-005: Cycle Detection for `selectedBy` Declarations

Status: PROPOSED
Supersedes: -
Related: CR-001, CR-002, CR-004, [selectedByRuntimeImplementationPlan.md](../selectedByRuntimeImplementationPlan.md)

## Synopsis

This change introduces **compile-time cycle detection** for `selectedBy` declarations in the JSON map file parser. A cycle occurs when a register's selection condition depends—directly or indirectly—on itself, creating a logical paradox that prevents deterministic validity resolution. Currently, the parser accepts such configurations, leading to undefined runtime behavior (oscillating validity, non-determinism, or infinite recursion risks). This change rejects map files containing cyclic `selectedBy` dependencies with a descriptive error message.

## Background

The [`selectedBy`](../selectedByRuntimeImplementationPlan.md) feature ([CR-004](#)) enables conditional activation of registers based on the value of a selector register. For example, a muxed data register `DAQ.DATA` may be declared active only when `DAQ.MUX_SEL == 1`. The runtime enforcement ([selectedByRuntimeImplementationPlan.md](../selectedByRuntimeImplementationPlan.md)) evaluates these conditions dynamically during reads or interrupt processing.

However, the current implementation does **not prevent** circular dependencies, such as:

```yaml
addressSpace:
  REG_A:
    selectedBy: { register: "REG_B", value: 1 }
  REG_B:
    selectedBy: { register: "REG_A", value: 2 }
```

In this case, the validity of `REG_A` depends on `REG_B`, which in turn depends on `REG_A`, forming a cycle. Such configurations violate the fundamental assumption that `selectedBy` describes a **unidirectional conditional relationship** and lead to:

1. **Logical Paradox**: There is no stable state where both registers' validity can be determined consistently.
2. **Non-Determinism**: Validity may depend on the order of evaluation (e.g., which `SelectorGate` is checked first).
3. **Debugging Difficulties**: Users observe unpredictable behavior without clear causes.
4. **Future Deadlock Risks**: If thread synchronization is added around gate evaluations, cycles could cause deadlocks.

This change closes this gap by detecting and rejecting cyclic dependencies at **parse time**, ensuring that all `selectedBy` declarations form a **directed acyclic graph (DAG)**.

## Requirements

### Functional Requirements

- **FR1**: The parser **must reject** map files containing cyclic `selectedBy` dependencies.
- **FR2**: Upon rejection, the parser **must emit** a `ChimeraTK::logic_error` with a human-readable message listing all registers involved in the cycle.
- **FR3**: The cycle detection **must cover** both direct and indirect cycles (e.g., `A → B → C → A`).
- **FR4**: The cycle detection **must include** inherited `selectedBy` declarations (e.g., a parent module declaring `selectedBy` and a child overriding or extending it).
- **FR5**: The cycle detection **must handle** self-references (e.g., `A → A`).

### Non-Functional Requirements

- **NFR1**: Cycle detection **must not** impact the performance of parsing typical (acyclic) map files. Target overhead: <1 ms for catalogues with up to 1000 registers.
- **NFR2**: The implementation **must integrate** seamlessly into the existing `JsonMapFileParser` workflow without requiring changes to other components (except tests).
- **NFR3**: The solution **must use** standard graph algorithms (e.g., DFS) for clarity and maintainability.

### Out of Scope

- **OS1**: Runtime cycle detection or recovery. Cycles are a **modelling error** and must be caught at parse time.
- **OS2**: Automatic cycle breaking. Users must manually resolve cycles by redesigning their `selectedBy` hierarchy.
- **OS3**: Support for dynamic catalogues (adding/removing registers at runtime). Cycle detection is performed once during parsing.

## Specifications

### Data Structures

#### Dependency Graph

The `selectedBy` relationships are modelled as a **directed graph**:

- **Node**: A register path (e.g., `/DAQ/MUX_SEL`).
- **Edge**: A directed edge from register `A` to register `B` if `A` declares `selectedBy.register == B`.

The graph is represented as:

```cpp
using DependencyGraph = std::unordered_map<RegisterPath, std::vector<RegisterPath>>;
```

Where:
- Keys are register paths with `selectedBy` declarations.
- Values are lists of selector register paths referenced by those declarations.

#### Cycle Detection Algorithm

The algorithm uses **Depth-First Search (DFS)** to detect cycles in the directed graph. Key properties:

- **Time Complexity**: O(V + E), where V is the number of vertices (registers with `selectedBy`) and E is the number of edges (dependencies).
- **Space Complexity**: O(V) for the recursion stack and auxiliary sets.

The algorithm tracks:

1. `visited`: A set of permanently visited nodes (to avoid redundant checks).
2. `recursionStack`: A set of nodes in the current DFS recursion stack. If a node is encountered again while still in the stack, a cycle exists.

### Interface Changes

#### New Functions in `JsonMapFileParser.cc`

```cpp
// Build the dependency graph from the catalogue's selectedBy declarations.
// Includes all registers (even those without selectedBy) as nodes to simplify traversal.
DependencyGraph buildDependencyGraph(const NumericAddressedRegisterCatalogue& catalogue);

// Detect cycles in the dependency graph using DFS.
// Returns true if a cycle exists; optionally populates 'cycleNodes' with involved registers.
bool hasCycle(
    const DependencyGraph& graph,
    std::unordered_set<RegisterPath>* cycleNodes = nullptr);
```

#### Modified Function

Update `JsonMapFileParser::Imp::parse()` to include cycle detection:

```cpp
std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> JsonMapFileParser::Imp::parse(std::ifstream& stream) {
  // ... existing parsing logic ...

  // After building the catalogue, check for cyclic selectedBy dependencies
  auto graph = buildDependencyGraph(catalogue);
  std::unordered_set<RegisterPath> cycleNodes;
  if (hasCycle(graph, &cycleNodes)) {
    std::string msg = "Circular 'selectedBy' dependency detected involving registers: ";
    for (const auto& node : cycleNodes) {
      msg += node + ", ";
    }
    msg = msg.substr(0, msg.size() - 2); // Trim trailing ", "
    throw ChimeraTK::logic_error(msg);
  }

  return {catalogue, metadata};
}
```

### Error Messages

The error message for cyclic dependencies follows the convention:

```
Circular 'selectedBy' dependency detected involving registers: <comma-separated-list-of-registers>
```

Example:

```
Circular 'selectedBy' dependency detected involving registers: DAQ.SELECTOR_A, DAQ.DATA_B, DAQ.CONTROL_C
```

### Test Cases

Add the following test cases to `tests/executables_src/testJsonMapFileParser.cpp`:

| Test ID | Description | Input | Expected Result |
|---------|-------------|-------|-----------------|
| CD1 | No cycle (linear chain) | `A → B → C` | Pass |
| CD2 | Direct cycle | `A → B`, `B → A` | Fail, error lists `A, B` |
| CD3 | Indirect cycle | `A → B`, `B → C`, `C → A` | Fail, error lists `A, B, C` |
| CD4 | Self-reference | `A → A` | Fail, error lists `A` |
| CD5 | Inherited cycle | Parent `P → Q`, Child `Q → P` | Fail, error lists `P, Q` |
| CD6 | Multiple independent cycles | `A → B → A` and `X → Y → X` | Fail, error lists `A, B, X, Y` |
| CD7 | Acyclic diamond | `A → B`, `A → C`, `B → D`, `C → D` | Pass |
| CD8 | Large acyclic graph | 1000 registers, random acyclic dependencies | Pass |
| CD9 | Large cyclic graph | 1000 registers in a single cycle | Fail, error lists subset |

### Documentation Updates

Update `doc/jmapFormat.dox` to include:

```markdown

ote The  selectedBy declarations must form a directed acyclic graph (DAG). 
Circular dependencies (e.g.,  A depends on  B and  B depends on  A) are 
prohibited and will cause the map file parser to reject the input with an error.
```

## Implementation Notes

### File Locations

- **Primary Changes**: `src/JsonMapFileParser.cc`
- **Tests**: `tests/executables_src/testJsonMapFileParser.cpp`
- **Header**: No new headers required (implementation is self-contained in `.cc` file).

### Dependencies

- **Standard Library**: `<unordered_map>`, `<unordered_set>`, `<functional>` (for lambda recursion).
- **Existing ChimeraTK Headers**: `NumericAddressedRegisterCatalogue.h`, `RegisterPath.h`.

### Build System

No changes to `CMakeLists.txt` are required, as the implementation uses existing dependencies.

### Backward Compatibility

- **Breaking Change**: Map files with cyclic `selectedBy` dependencies that previously parsed successfully will now **fail to parse**.
- **Justification**: These configurations were **incorrect** and led to undefined behavior. Rejecting them improves correctness.
- **Migration Guide**: Users must revise their map files to eliminate cycles. Suggested approaches:
  - Redesign the `selectedBy` hierarchy to avoid mutual dependencies.
  - Introduce intermediate registers to break cycles (e.g., use a common selector register).

## Alternatives Considered

### Alternative 1: Runtime Cycle Detection

Detect cycles during the first access to a register with `selectedBy`.

- **Pros**: No upfront cost for acyclic configurations.
- **Cons**: 
  - Errors occur late (at runtime), making debugging harder.
  - Requires additional bookkeeping in `SelectedByDecorator` or accessors.
  - May miss cycles if the problematic registers are never accessed.
- **Decision**: Rejected in favor of **parse-time detection** for fail-fast behavior.

### Alternative 2: Allow Cycles with Stable Fixed Points

Permit cycles if a stable combination of register values exists where all `selectedBy` conditions are satisfied.

- **Pros**: More flexible for advanced use cases.
- **Cons**: 
  - Violates the principle of **mutual exclusivity** (e.g., mux alternatives should not overlap).
  - Introduces non-determinism (which stable state is chosen?).
  - Significantly increases complexity (requires solving constraint satisfaction problems).
- **Decision**: Rejected. `selectedBy` is designed for **conditional activation**, not bidirectional dependencies.

### Alternative 3: Union-Find (DSU) Algorithm

Use the Union-Find data structure to detect cycles.

- **Pros**: Efficient for undirected graphs.
- **Cons**: 
  - Does not naturally handle **directed graphs** (may miss directed cycles or falsely detect others).
  - Less intuitive for this use case.
- **Decision**: Rejected in favor of **DFS**, which is simpler and more appropriate for directed graphs.

### Alternative 4: Topological Sorting

Attempt to topologically sort the dependency graph. Failure indicates a cycle.

- **Pros**: Also provides a linear ordering of registers (useful for other purposes).
- **Cons**: More complex to implement than DFS for cycle detection alone.
- **Decision**: Rejected due to unnecessary complexity. DFS is sufficient for cycle detection.

## Open Questions

1. **Should we report the full cycle path or just the involved nodes?**
   - Current proposal: Report involved nodes (simpler).
   - Enhancement: Optionally show the cycle path (e.g., `A → B → C → A`).
   - **Resolution**: Start with involved nodes; enhance later if needed.

2. **How should we handle cycles spanning multiple map files (if supported)?**
   - Current assumption: Cycle detection is performed per-map-file.
   - **Resolution**: Cross-file cycles are out of scope for this change.

3. **Should we warn about long dependency chains (non-cyclic but deep)?**
   - **Resolution**: Out of scope. Long chains are valid but may indicate poor design.

## References

- [selectedByRuntimeImplementationPlan.md](../selectedByRuntimeImplementationPlan.md): Original design document for `selectedBy` runtime enforcement.
- [CR-004](#): Introduction of `selectedBy` parsing (without runtime enforcement).
- [CR-001](#)/[CR-002](#): Related changes for double-buffering and interrupt handling.

## Appendix: Pseudocode for Cycle Detection

```python
# Input: dependency_graph (dict mapping register -> list of selector registers)
# Output: True if cycle exists, along with the set of involved nodes

def has_cycle(dependency_graph):
    visited = set()          # Permanently visited nodes
    rec_stack = set()        # Nodes in current recursion stack
    cycle_nodes = set()     # Nodes involved in the first detected cycle

    def dfs(node):
        if node in rec_stack:
            # Cycle detected: add all nodes in the current stack + the repeating node
            cycle_nodes.update(rec_stack)
            cycle_nodes.add(node)
            return True
        if node in visited:
            return False

        visited.add(node)
        rec_stack.add(node)

        for neighbor in dependency_graph.get(node, []):
            if dfs(neighbor):
                return True

        rec_stack.remove(node)
        return False

    for node in dependency_graph:
        if dfs(node):
            return True, cycle_nodes
    return False, set()
```

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-09-XX | - | Initial draft |
