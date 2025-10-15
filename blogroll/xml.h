#include <stddef.h>

typedef struct {
    const char *ptr;
    size_t len;
} xml_slice;

xml_slice xml_slice_concat(xml_slice a, xml_slice b);
xml_slice xml_slice_trim(xml_slice a);

typedef struct {
    xml_slice key;
    xml_slice val;
} xml_attr;

typedef struct {
    enum {
        XML_DONE,

        XML_PI_ENTER,
        XML_PI_EXIT,

        XML_TAG_ENTER,
        XML_TAG_EXIT,

        XML_TEXT,

        XML_ATTRIBUTE,
    } tag;

    union {
        xml_slice pi_enter;

        xml_slice tag_enter;
        xml_slice tag_exit;

        xml_slice text;

        xml_attr attribute;
    } data;
} xml_event;

typedef struct {
    size_t offset;
    xml_slice current_tag;
} xml_state;

xml_event xml_next(xml_state *state, const char *content);

#ifdef XML_IMPLEMENTATION

#include <assert.h>
#include <stddef.h>
#include <string.h>

xml_slice xml_slice_concat(xml_slice a, xml_slice b)
{
    xml_slice new = {
        .ptr = calloc(a.len + b.len, sizeof(char)),
        .len = a.len + b.len,
    };

    if (a.ptr) memcpy((void*)new.ptr, a.ptr, a.len);
    if (b.ptr) memcpy((void*)new.ptr + a.len, b.ptr, b.len);

    if (a.ptr) free((void*)a.ptr);
    if (b.ptr) free((void*)b.ptr);

    return new;
}

xml_slice xml_slice_trim(xml_slice a)
{
    for (; *a.ptr <= ' '; a.ptr++, a.len--);
    for (; a.len > 0 && a.ptr[a.len - 1] <= ' '; a.len--);

    return a;
}

xml_event xml_next(xml_state *state, const char *content)
{
    enum {
        _START,

        _PI_START,
        _PI_EXIT,

        _TAG_START,
        _TAG_OPEN,
        _TAG_CLOSE,

        _TEXT,
        _CDATA,

        _ATTRIBUTE_KEY,
        _ATTRIBUTE_VAL_START,
        _ATTRIBUTE_VAL_BARE,
        _ATTRIBUTE_VAL_STRING,
    } s = _START;

    size_t start_offset = state->offset;
    xml_event event = {0};

    for (;;) {
        char n = content[state->offset];

        switch (s) {
            case _START: switch (n) {
                case 0:
                    return event;

                case ' ':
                case '\r':
                case '\n':
                case '\t':
                    start_offset = state->offset += 1;
                    continue;

                case '<':
                    s = _TAG_START;
                    start_offset = state->offset += 1;
                    continue;

                case '/':
                    if (state->current_tag.ptr && content[state->offset+1] == '>') {
                        state->offset += 2;

                        event.tag = XML_TAG_EXIT;
                        event.data.tag_exit = state->current_tag;

                        state->current_tag = (xml_slice){0};
                        return event;
                    } else {
                        s = _TEXT;
                    }
                    continue;

                case '?':
                    if (state->current_tag.ptr && content[state->offset+1] == '>') {
                        state->offset += 2;

                        event.tag = XML_PI_EXIT;
                        event.data = (typeof(event.data)){0};

                        state->current_tag = (xml_slice){0};
                        return event;
                    } else {
                        s = _TEXT;
                    }
                    continue;

                case '>':
                    if (state->current_tag.ptr) {
                        state->current_tag = (xml_slice){0};
                        start_offset = state->offset += 1;
                    } else {
                        s = _TEXT;
                    }
                    continue;

                default:
                    if (state->current_tag.ptr) s = _ATTRIBUTE_KEY;
                    else s = _TEXT;
                    continue;
            }

            case _PI_START: switch (n) {
                case ' ':
                case '\n':
                    event.tag = XML_PI_ENTER;
                    event.data.pi_enter.ptr = &content[start_offset];
                    event.data.pi_enter.len = state->offset - start_offset;
                    event.data.pi_enter = xml_slice_trim(event.data.pi_enter);

                    state->offset += 1;

                    state->current_tag = event.data.pi_enter;

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _TAG_START: switch (n) {
                case '/':
                    s = _TAG_CLOSE;
                    start_offset = state->offset += 1;
                    continue;

                case '?': if (start_offset == state->offset) {
                    s = _PI_START;
                    start_offset = state->offset += 1;
                    continue;
                }

                default:
                    s = _TAG_OPEN;
                    continue;
            }

            case _TAG_OPEN: switch (n) {
                case ' ':
                case '>':
                    event.tag = XML_TAG_ENTER;
                    event.data.tag_enter.ptr = &content[start_offset];
                    event.data.tag_enter.len = state->offset - start_offset;
                    event.data.tag_enter = xml_slice_trim(event.data.tag_enter);

                    if (n != '>') {
                        state->current_tag = event.data.tag_enter;
                    }

                    if (strncmp(event.data.tag_enter.ptr, "![CDATA[", 8) == 0) {
                        start_offset = state->offset -= event.data.tag_enter.len - 8; // move offset back by "![CDATA[".len

                        s = _CDATA;
                        continue;
                    }

                    state->offset += 1;

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _TAG_CLOSE: switch (n) {
                case '>':
                    event.tag = XML_TAG_EXIT;
                    event.data.tag_exit.ptr = &content[start_offset];
                    event.data.tag_exit.len = state->offset - start_offset;
                    event.data.tag_exit = xml_slice_trim(event.data.tag_exit);

                    state->offset += 1;
                    state->current_tag = (xml_slice){0};

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _TEXT: switch (n) {
                case 0:
                case '<':
                    event.tag = XML_TEXT;
                    event.data.text.ptr = &content[start_offset];
                    event.data.text.len = state->offset - start_offset;
                    event.data.text = xml_slice_trim(event.data.text);

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _CDATA: switch (n) {
                case ']':
                    if (strncmp(&content[state->offset], "]]>", 3) != 0) {
                        state->offset += 1;
                        continue;
                    }

                    event.tag = XML_TEXT;
                    event.data.text.ptr = &content[start_offset];
                    event.data.text.len = state->offset - start_offset;
                    event.data.text = xml_slice_trim(event.data.text);

                    state->offset += 3;

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _ATTRIBUTE_KEY: switch (n) {
                case '=':
                    event.tag = XML_ATTRIBUTE;
                    event.data.attribute.key.ptr = &content[start_offset];
                    event.data.attribute.key.len = state->offset - start_offset;

                    start_offset = state->offset += 1;

                    s = _ATTRIBUTE_VAL_START;
                    continue;

                default:
                    state->offset += 1;
                    continue;
            }

            case _ATTRIBUTE_VAL_START: switch (n) {
                case '"':
                    s = _ATTRIBUTE_VAL_STRING;
                    state->offset += 1;
                    continue;

                default:
                    s = _ATTRIBUTE_VAL_BARE;
                    continue;
            }

            case _ATTRIBUTE_VAL_BARE: switch (n) {
                case ' ':
                case '>':
                    event.data.attribute.val.ptr = &content[start_offset];
                    event.data.attribute.val.len = state->offset - start_offset;

                    return event;

                default:
                    state->offset += 1;
                    continue;
            }

            case _ATTRIBUTE_VAL_STRING: switch (n) {
                case '"':
                    state->offset += 1;

                    event.data.attribute.val.ptr = &content[start_offset];
                    event.data.attribute.val.len = state->offset - start_offset;

                    return event;

                case '\\':
                    state->offset += 1;
                // FALLTHROUGH
                default:
                    state->offset += 1;
                    continue;
            }

            default:
                assert(0 && "unreachable");
        }

        assert(0 && "unreachable");
    }

    assert(0 && "unreachable");
}

#endif // XML_IMPLEMENTATION
