package pzanimator.agent;

/** Reads single top-level string fields from the small JSON commands the app sends. */
final class MiniJson {
    private MiniJson() {}

    /** Returns the string value of {@code "key": "..."}, or null when absent or not a string. */
    static String stringField(String json, String key) {
        if (json == null) {
            return null;
        }
        int keyAt = json.indexOf('"' + key + '"');
        if (keyAt < 0) {
            return null;
        }
        int i = json.indexOf(':', keyAt + key.length() + 2);
        if (i < 0) {
            return null;
        }
        i++;
        while (i < json.length() && Character.isWhitespace(json.charAt(i))) {
            i++;
        }
        if (i >= json.length() || json.charAt(i) != '"') {
            return null;
        }
        StringBuilder value = new StringBuilder();
        for (i++; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '"') {
                return value.toString();
            }
            if (c == '\\' && i + 1 < json.length()) {
                char next = json.charAt(++i);
                switch (next) {
                    case 'n' -> value.append('\n');
                    case 't' -> value.append('\t');
                    case 'r' -> value.append('\r');
                    case 'u' -> {
                        if (i + 4 < json.length()) {
                            value.append((char) Integer.parseInt(json.substring(i + 1, i + 5), 16));
                            i += 4;
                        }
                    }
                    default -> value.append(next);
                }
            } else {
                value.append(c);
            }
        }
        return null;
    }
}
