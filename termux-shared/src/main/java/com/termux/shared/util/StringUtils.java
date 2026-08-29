package com.termux.shared.util;

import java.util.ArrayList;
import java.util.List;

/**
 * Autoral StringUtils — replaces Guava com.google.common.base.Strings
 *
 * Provides commonly-used string utilities without external dependencies.
 * Zero Guava dependency after this module is adopted.
 */
public class StringUtils {

    /**
     * Returns true if the given string is null or empty.
     *
     * @param s The string to check
     * @return true if null or length == 0
     */
    public static boolean isNullOrEmpty(String s) {
        return s == null || s.length() == 0;
    }

    /**
     * Returns true if the string is empty or contains only whitespace.
     *
     * @param s The string to check
     * @return true if null, empty, or whitespace-only
     */
    public static boolean isNullOrEmptyOrWhitespace(String s) {
        if (s == null || s.length() == 0) {
            return true;
        }
        for (int i = 0; i < s.length(); i++) {
            if (!Character.isWhitespace(s.charAt(i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Joins array of strings with separator.
     * Replaces Guava Joiner.on(separator).join(Iterable).
     *
     * @param separator The string to put between elements
     * @param parts The strings to join
     * @return Joined string
     */
    public static String join(String separator, String... parts) {
        if (parts == null || parts.length == 0) {
            return "";
        }

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < parts.length; i++) {
            if (parts[i] != null) {
                sb.append(parts[i]);
            }
            if (i < parts.length - 1) {
                sb.append(separator);
            }
        }
        return sb.toString();
    }

    /**
     * Joins Iterable of strings with separator.
     * Replaces Guava Joiner.on(separator).join(Iterable).
     *
     * @param separator The string to put between elements
     * @param parts The iterable of strings to join
     * @return Joined string
     */
    public static String join(String separator, Iterable<String> parts) {
        if (parts == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder();
        boolean first = true;
        for (String part : parts) {
            if (!first) {
                sb.append(separator);
            }
            if (part != null) {
                sb.append(part);
            }
            first = false;
        }
        return sb.toString();
    }

    /**
     * Splits string by separator, omitting empty strings.
     *
     * @param s The string to split
     * @param separator The separator pattern
     * @return List of non-empty parts
     */
    public static List<String> split(String s, String separator) {
        List<String> result = new ArrayList<>();
        if (isNullOrEmpty(s)) {
            return result;
        }

        if (isNullOrEmpty(separator)) {
            result.add(s);
            return result;
        }

        String[] parts = s.split(java.util.regex.Pattern.quote(separator), -1);
        for (String part : parts) {
            if (part.length() > 0) {
                result.add(part);
            }
        }
        return result;
    }

    /**
     * Repeats a string N times.
     *
     * @param s The string to repeat
     * @param count Number of repetitions
     * @return Repeated string
     */
    public static String repeat(String s, int count) {
        if (isNullOrEmpty(s) || count <= 0) {
            return "";
        }

        StringBuilder sb = new StringBuilder(s.length() * count);
        for (int i = 0; i < count; i++) {
            sb.append(s);
        }
        return sb.toString();
    }

    /**
     * Pads string on the left to desired length with pad character.
     *
     * @param s The string to pad
     * @param minLength Minimum desired length
     * @param padChar The character to pad with
     * @return Padded string
     */
    public static String padStart(String s, int minLength, char padChar) {
        if (s == null || s.length() >= minLength) {
            return s == null ? "" : s;
        }

        int padCount = minLength - s.length();
        return repeat(String.valueOf(padChar), padCount) + s;
    }

    /**
     * Pads string on the right to desired length with pad character.
     *
     * @param s The string to pad
     * @param minLength Minimum desired length
     * @param padChar The character to pad with
     * @return Padded string
     */
    public static String padEnd(String s, int minLength, char padChar) {
        if (s == null || s.length() >= minLength) {
            return s == null ? "" : s;
        }

        int padCount = minLength - s.length();
        return s + repeat(String.valueOf(padChar), padCount);
    }

    /**
     * Checks if string contains only ASCII characters.
     *
     * @param s The string to check
     * @return true if all chars are ASCII (0-127)
     */
    public static boolean isAscii(String s) {
        if (s == null) {
            return true;
        }

        for (int i = 0; i < s.length(); i++) {
            if (s.charAt(i) > 127) {
                return false;
            }
        }
        return true;
    }

    /**
     * Converts string to hex representation (lowercase).
     *
     * @param s The string to convert
     * @return Hex string
     */
    public static String toHex(String s) {
        if (s == null) {
            return "";
        }

        StringBuilder sb = new StringBuilder(s.length() * 2);
        for (char c : s.toCharArray()) {
            sb.append(String.format("%02x", (int) c));
        }
        return sb.toString();
    }

    /**
     * Converts byte array to hex representation (lowercase).
     *
     * @param bytes The bytes to convert
     * @return Hex string
     */
    public static String bytesToHex(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            return "";
        }

        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte b : bytes) {
            sb.append(String.format("%02x", b & 0xFF));
        }
        return sb.toString();
    }

    /**
     * Converts hex string to byte array.
     *
     * @param hex The hex string to convert
     * @return Byte array
     * @throws IllegalArgumentException if hex string is invalid
     */
    public static byte[] hexToBytes(String hex) {
        if (isNullOrEmpty(hex) || hex.length() % 2 != 0) {
            throw new IllegalArgumentException("Invalid hex string");
        }

        byte[] result = new byte[hex.length() / 2];
        for (int i = 0; i < result.length; i++) {
            String hexByte = hex.substring(i * 2, i * 2 + 2);
            result[i] = (byte) Integer.parseInt(hexByte, 16);
        }
        return result;
    }

    /**
     * Escapes string for use in shell command.
     *
     * @param s The string to escape
     * @return Shell-escaped string
     */
    public static String escapeShell(String s) {
        if (isNullOrEmpty(s)) {
            return "''";
        }

        if (!s.contains(" ") && !s.contains("'") && !s.contains("\"") &&
            !s.contains("$") && !s.contains("`") && !s.contains("\\")) {
            return s;
        }

        return "'" + s.replace("'", "'\\''") + "'";
    }
}
