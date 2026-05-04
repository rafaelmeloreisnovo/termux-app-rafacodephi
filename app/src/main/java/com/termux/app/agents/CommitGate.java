package com.termux.app.agents;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/** Basic integrity gate for agent transitions. */
public final class CommitGate {
    private CommitGate() {}

    public static boolean verifyTransition(AgentMessage message, String expectedPrevHash) {
        return expectedPrevHash != null
            && expectedPrevHash.equals(message.getPreviousStateHash())
            && message.getPayload() != null
            && !message.getPayload().isEmpty();
    }

    public static String sha256Hex(String input) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] digest = md.digest(input.getBytes(StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder(digest.length * 2);
            for (byte b : digest) sb.append(String.format("%02x", b));
            return sb.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 not available", e);
        }
    }
}
