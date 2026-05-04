package com.termux.app.agents;

import java.util.Collections;

public class AgentCoreSelfTest {
    public static void main(String[] args) {
        DependencyRobot robot = new DependencyRobot();
        robot.addDependency("translator", "safety");
        robot.addDependency("safety", "integrity");
        if (robot.hasCircularDependency()) {
            throw new AssertionError("Should not have circular dependency");
        }

        AgentMessage ok = new AgentMessage("safety", "translator", "payload", "abc", Collections.emptyMap());
        if (!CommitGate.verifyTransition(ok, "abc")) {
            throw new AssertionError("CommitGate transition should be valid");
        }

        if (CommitGate.verifyTransition(ok, "def")) {
            throw new AssertionError("CommitGate should reject invalid previous hash");
        }

        String hash = CommitGate.sha256Hex("rafaelia");
        if (hash.length() != 64) {
            throw new AssertionError("SHA-256 hex should be 64 chars");
        }

        System.out.println("AgentCoreSelfTest OK");
    }
}
