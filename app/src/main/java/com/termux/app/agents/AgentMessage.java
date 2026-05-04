package com.termux.app.agents;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/** Immutable envelope used for inter-agent communication with audit metadata. */
public final class AgentMessage {
    private final String sourceAgentId;
    private final String targetAgentId;
    private final String payload;
    private final String previousStateHash;
    private final Map<String, String> headers;

    public AgentMessage(String sourceAgentId, String targetAgentId, String payload,
                        String previousStateHash, Map<String, String> headers) {
        this.sourceAgentId = sourceAgentId;
        this.targetAgentId = targetAgentId;
        this.payload = payload;
        this.previousStateHash = previousStateHash;
        this.headers = headers == null ? Collections.emptyMap() : Collections.unmodifiableMap(new HashMap<>(headers));
    }

    public String getSourceAgentId() { return sourceAgentId; }
    public String getTargetAgentId() { return targetAgentId; }
    public String getPayload() { return payload; }
    public String getPreviousStateHash() { return previousStateHash; }
    public Map<String, String> getHeaders() { return headers; }
}
