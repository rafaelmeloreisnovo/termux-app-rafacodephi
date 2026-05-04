package com.termux.app.agents;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Registry for agent dependencies and execution order.
 * Keeps deterministic ordering for reproducible workflows.
 */
public final class DependencyRobot {
    private final Map<String, List<String>> dependencies = new LinkedHashMap<>();

    public void registerAgent(String agentId) {
        dependencies.putIfAbsent(agentId, new ArrayList<String>());
    }

    public void addDependency(String agentId, String dependsOn) {
        registerAgent(agentId);
        registerAgent(dependsOn);
        dependencies.get(agentId).add(dependsOn);
    }

    public List<String> getDependencies(String agentId) {
        return dependencies.getOrDefault(agentId, new ArrayList<String>());
    }

    public boolean hasCircularDependency() {
        Map<String, Integer> state = new LinkedHashMap<>();
        for (String node : dependencies.keySet()) {
            state.put(node, 0);
        }
        for (String node : dependencies.keySet()) {
            if (state.get(node) == 0 && dfsCycle(node, state)) {
                return true;
            }
        }
        return false;
    }

    private boolean dfsCycle(String node, Map<String, Integer> state) {
        state.put(node, 1);
        for (String next : dependencies.getOrDefault(node, new ArrayList<String>())) {
            int nextState = state.getOrDefault(next, 0);
            if (nextState == 1) return true;
            if (nextState == 0 && dfsCycle(next, state)) return true;
        }
        state.put(node, 2);
        return false;
    }
}
