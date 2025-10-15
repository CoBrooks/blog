#define _XOPEN_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define unreachable assert(0 && "unreachable")

#define XML_IMPLEMENTATION
#include "xml.h"

xml_event parse_until_tag(xml_state *state, const char *contents, const char *tag)
{
    for (;;) {
        xml_event event = xml_next(state, contents);

        if (event.tag == XML_DONE) {
            return event;
        }

        if (event.tag != XML_TAG_ENTER) {
            continue;
        }

        if (strncmp(event.data.tag_enter.ptr, tag, event.data.tag_enter.len) == 0) {
            return event;
        }
    }

    unreachable;
}

int has_tag(xml_state *state, const char *contents, const char *tag)
{
    int depth = 0;
    xml_state start = *state;

    for (;;) {
        xml_event event = xml_next(state, contents);

        if (event.tag == XML_DONE) {
            *state = start;
            return 0;
        }

        if (event.tag == XML_TAG_ENTER) {
            if (depth == 0 && strncmp(event.data.tag_enter.ptr, tag, event.data.tag_enter.len) == 0) {
                *state = start;
                return 1;
            }

            depth++;
        }

        if (event.tag == XML_TAG_EXIT) {
            depth--;
        }

        if (depth < 0) {
            *state = start;
            return 0;
        }
    }

    unreachable;
}

xml_slice get_text(xml_state *state, const char *contents)
{
    for (;;) {
        xml_event event = xml_next(state, contents);

        if (event.tag == XML_DONE || event.tag == XML_TAG_EXIT) {
            return (xml_slice){0};
        }

        if (event.tag == XML_TEXT) {
            return event.data.text;
        }
    }

    unreachable;
}

xml_slice trim_quotes(xml_slice str)
{
    if (str.ptr[0] == '"') {
        str.ptr++;
        str.len--;
    }

    if (str.ptr[str.len - 1] == '"') {
        str.len--;
    }

    return str;
}

xml_slice get_attr(xml_state *state, const char *contents, const char *key)
{
    xml_state initial = *state;

    for (;;) {
        xml_event event = xml_next(state, contents);

        if (event.tag == XML_DONE || event.tag == XML_TAG_EXIT || event.tag == XML_TEXT) {
            *state = initial;
            return (xml_slice){0};
        }

        if (event.tag == XML_ATTRIBUTE && strncmp(event.data.attribute.key.ptr, key, event.data.attribute.key.len) == 0) {
            *state = initial;
            return trim_quotes(event.data.attribute.val);
        }
    }

    unreachable;
}

typedef struct {
    xml_slice title;
    xml_slice date;
    xml_slice link;
    xml_slice source;
} entry;

entry *feed_to_entry(xml_state *state, const char *contents, entry *e)
{
    xml_event start = parse_until_tag(state, contents, "entry");

    if (start.tag == XML_DONE) {
        return NULL;
    }

    xml_state start_state = *state;

    parse_until_tag(state, contents, "title");
    e->title = get_text(state, contents);
    *state = start_state;

    parse_until_tag(state, contents, "updated");
    e->date = get_text(state, contents);
    *state = start_state;

    parse_until_tag(state, contents, "id");
    e->link = get_text(state, contents);
    *state = start_state;

    return e;
}

entry *rss_to_entry(xml_state *state, const char *contents, entry *e)
{
    xml_event start = parse_until_tag(state, contents, "item");

    if (start.tag == XML_DONE) {
        return NULL;
    }

    xml_state start_state = *state;

    parse_until_tag(state, contents, "title");
    e->title = get_text(state, contents);
    *state = start_state;

    parse_until_tag(state, contents, "pubDate");
    e->date = get_text(state, contents);
    *state = start_state;

    parse_until_tag(state, contents, "link");
    e->link = get_text(state, contents);
    *state = start_state;

    return e;
}

xml_slice feed_source(xml_state *state, const char *contents)
{
    xml_state start_state = *state;

    parse_until_tag(state, contents, "id");
    xml_slice id = get_text(state, contents);

    size_t start, end;
    char source_buf[64] = {0};

    if (sscanf(id.ptr, "http%*2[s:]//%ln%*[a-zA-Z0-9.]%ln", &start, &end) < 0) {
      assert(0 && "unable to parse feed source from id tag");
    }

    *state = start_state;

    return (xml_slice){
        .ptr = id.ptr + start,
        .len = end - start,
    };
}

xml_slice rss_source(xml_state *state, const char *contents)
{
    xml_state start_state = *state;

    parse_until_tag(state, contents, "link");
    xml_slice link = get_text(state, contents);

    size_t start, end;
    char source_buf[64] = {0};

    if (sscanf(link.ptr, "http%*2[s:]//%ln%*[a-zA-Z0-9.]%ln", &start, &end) < 0) {
      assert(0 && "unable to parse feed source from link tag");
    }

    *state = start_state;

    return (xml_slice){
        .ptr = link.ptr + start,
        .len = end - start,
    };
}

void print_sanitized(xml_slice slice)
{
    const char *html_lt[127] = {
        ['\''] = "&apos;",
        ['\"'] = "&quot;",
        [';'] = "&semi;",
        ['<'] = "&lt;",
        ['>'] = "&gt;",
    };

    const char *start = slice.ptr;

    for (size_t i = 0; i < slice.len; ++i) {
        unsigned char c = slice.ptr[i];

        if (c > 0x7F) {
            i++;
            continue;
        }

        if (html_lt[c] != NULL) {
            size_t len = &slice.ptr[i] - start;

            printf("%.*s", (int)len, start);
            printf("%s", html_lt[c]);

            start = &slice.ptr[i + 1];
        }
    }

    xml_slice last = {
        .ptr = start,
        .len = slice.len - (start - slice.ptr),
    };

    printf("%.*s", (int)last.len, last.ptr);
}

void print_entry(entry e)
{
    struct tm date = {0};

    if (strptime(e.date.ptr, "%a, %d %h %Y %T", &date) == NULL) {
        assert(strptime(e.date.ptr, "%FT%T", &date) && "unrecognized date format");
    }

    const char date_buffer[64] = {0};
    size_t date_len;

    date_len = strftime((char*)date_buffer, sizeof(date_buffer), "%F", &date);
    printf("/* %.*s */ ", (int)date_len, date_buffer);

    printf("ENTRY(");
    printf("TEXT(");
    print_sanitized(e.title);
    printf("), ");

    date_len = strftime((char*)date_buffer, sizeof(date_buffer), "%h %d, %Y", &date);
    printf("TEXT(%.*s), ", (int)date_len, date_buffer);

    printf("\"%.*s\", ", (int)e.link.len, e.link.ptr);
    printf("TEXT(%.*s)", (int)e.source.len, e.source.ptr);
    printf(")\n");
}

int main(void)
{
    const char *contents = NULL;
    size_t length = 0;

    while (!feof(stdin)) {
        const char buffer[1024] = {0};

        size_t n = fread((void*)buffer, sizeof(char), 1024, stdin);

        contents = realloc((void*)contents, length + n);

        memcpy((void*)&contents[length], buffer, n);

        length += n;
    }

    xml_state state = {0};

    xml_slice (*get_source)(xml_state*, const char*);
    entry* (*get_entry)(xml_state*, const char*, entry*);

    if (has_tag(&state, contents, "feed")) {
        get_source = feed_source;
        get_entry = feed_to_entry;
    } else if (has_tag(&state, contents, "rss")) {
        get_source = rss_source;
        get_entry = rss_to_entry;
    }

    assert(get_entry && "unable to determine type of xml feed");

    xml_slice source = get_source(&state, contents);

    for (;;) {
        entry e = { .source = source };

        if (get_entry(&state, contents, &e) == NULL) {
            break;
        }

        print_entry(e);
    }

    free((void*)contents);

    return 0;
}
