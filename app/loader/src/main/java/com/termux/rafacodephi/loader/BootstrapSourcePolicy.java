package com.termux.rafacodephi.loader;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Locale;
import java.util.regex.Pattern;

/** Pure-Java fail-closed policy for bootstrap source URLs and identifiers. */
final class BootstrapSourcePolicy {

    static final long MAX_DOWNLOAD_BYTES = 128L * 1024L * 1024L;
    static final int MAX_REDIRECTS = 3;
    private static final Pattern SHA256 = Pattern.compile("^[0-9a-f]{64}$");
    private static final Pattern ABI = Pattern.compile("^(aarch64|arm|i686|x86_64)$");

    private BootstrapSourcePolicy() {}

    static String requireAbi(String abi) {
        if (abi == null || !ABI.matcher(abi).matches()) {
            throw new IllegalArgumentException("UNSUPPORTED_ABI");
        }
        return abi;
    }

    static String requireSha256(String value) {
        if (value == null) throw new IllegalArgumentException("INVALID_SHA256");
        String normalized = value.toLowerCase(Locale.US);
        if (!SHA256.matcher(normalized).matches()) {
            throw new IllegalArgumentException("INVALID_SHA256");
        }
        return normalized;
    }

    static URL requireInitialUrl(String raw) throws MalformedURLException {
        if (raw == null) throw new MalformedURLException("SOURCE_URL_MISSING");
        URL url = new URL(raw);
        validateHttpsUrl(url);
        return url;
    }

    static URL requireSameOriginRedirect(URL origin, URL current, String location)
            throws MalformedURLException {
        if (location == null || location.trim().isEmpty()) {
            throw new MalformedURLException("REDIRECT_LOCATION_MISSING");
        }
        URL redirected = new URL(current, location);
        validateHttpsUrl(redirected);
        if (!sameOrigin(origin, redirected)) {
            throw new MalformedURLException("CROSS_ORIGIN_REDIRECT_BLOCKED");
        }
        return redirected;
    }

    private static void validateHttpsUrl(URL url) throws MalformedURLException {
        if (!"https".equalsIgnoreCase(url.getProtocol())) {
            throw new MalformedURLException("HTTPS_REQUIRED");
        }
        if (url.getHost() == null || url.getHost().trim().isEmpty()) {
            throw new MalformedURLException("HOST_REQUIRED");
        }
        if (url.getUserInfo() != null) {
            throw new MalformedURLException("USER_INFO_BLOCKED");
        }
        if (url.getRef() != null) {
            throw new MalformedURLException("FRAGMENT_BLOCKED");
        }
        if (effectivePort(url) != 443) {
            throw new MalformedURLException("NON_STANDARD_HTTPS_PORT_BLOCKED");
        }
    }

    private static boolean sameOrigin(URL first, URL second) {
        return first.getProtocol().equalsIgnoreCase(second.getProtocol())
                && first.getHost().equalsIgnoreCase(second.getHost())
                && effectivePort(first) == effectivePort(second);
    }

    private static int effectivePort(URL url) {
        return url.getPort() == -1 ? url.getDefaultPort() : url.getPort();
    }
}
