// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public final class RemoteServerInfo {
    private String address = "";
    private String apiKey = "";
    private String host = "";
    private int port;

    public RemoteServerInfo address(String address) {
        final String prefix = "s5run://";
        if (!address.startsWith(prefix)) {
            throw new IllegalArgumentException("expected format s5run://host:port");
        }
        String rest = address.substring(prefix.length());
        int colon = rest.lastIndexOf(':');
        if (colon <= 0 || colon + 1 >= rest.length()) {
            throw new IllegalArgumentException("expected format s5run://host:port");
        }
        this.address = address;
        this.host = rest.substring(0, colon);
        this.port = Integer.parseInt(rest.substring(colon + 1));
        return this;
    }

    public RemoteServerInfo apiKey(String apiKey) {
        this.apiKey = apiKey;
        return this;
    }

    public String getAddress() { return address; }
    public String getApiKey() { return apiKey; }
    public String getHost() { return host; }
    public int getPort() { return port; }
}
