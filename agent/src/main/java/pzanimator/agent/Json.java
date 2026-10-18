package pzanimator.agent;

/** Tiny streaming JSON writer; the game jar ships no JSON library we can rely on. */
final class Json {
    private final StringBuilder out = new StringBuilder(256);
    private boolean needComma;

    Json beginObject() {
        separator();
        out.append('{');
        needComma = false;
        return this;
    }

    Json endObject() {
        out.append('}');
        needComma = true;
        return this;
    }

    Json beginArray() {
        separator();
        out.append('[');
        needComma = false;
        return this;
    }

    Json endArray() {
        out.append(']');
        needComma = true;
        return this;
    }

    Json name(String name) {
        separator();
        string(name);
        out.append(':');
        needComma = false;
        return this;
    }

    Json field(String name, String value) {
        name(name);
        if (value == null) {
            out.append("null");
        } else {
            string(value);
        }
        needComma = true;
        return this;
    }

    Json field(String name, long value) {
        name(name);
        out.append(value);
        needComma = true;
        return this;
    }

    Json field(String name, boolean value) {
        name(name);
        out.append(value);
        needComma = true;
        return this;
    }

    Json nullValue() {
        separator();
        out.append("null");
        needComma = true;
        return this;
    }

    Json floats(float... values) {
        beginArray();
        for (int i = 0; i < values.length; i++) {
            if (i > 0) {
                out.append(',');
            }
            float v = values[i];
            out.append(Float.isFinite(v) ? Float.toString(v) : "null");
        }
        return endArray();
    }

    @Override
    public String toString() {
        return out.toString();
    }

    private void separator() {
        if (needComma) {
            out.append(',');
        }
    }

    private void string(String s) {
        out.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"' -> out.append("\\\"");
                case '\\' -> out.append("\\\\");
                case '\n' -> out.append("\\n");
                case '\r' -> out.append("\\r");
                case '\t' -> out.append("\\t");
                default -> {
                    if (c < 0x20) {
                        out.append(String.format("\\u%04x", (int) c));
                    } else {
                        out.append(c);
                    }
                }
            }
        }
        out.append('"');
    }
}
