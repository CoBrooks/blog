#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define unreachable assert(0 && "unreachable")

#define XML_IMPLEMENTATION
#include "xml.h"

// difftime helpers
const time_t DAY_S = (60 * 60 * 24);
const time_t WEEK_S = (60 * 60 * 24 * 7);
const time_t MONTH_S = (60 * 60 * 24 * 30);
const time_t YEAR_S = (60 * 60 * 24 * 365);

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
    if (str.len <= 2) return str;

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

xml_slice hostname_from_url(xml_slice url)
{
    size_t start = 0;
    size_t len = 0;

    if (memcmp(url.ptr, "https://", 8) == 0) {
        start = 8;
    } else if (memcmp(url.ptr, "http://", 7) == 0) {
        start = 7;
    } else {
        return (xml_slice){0};
    }

    while (url.ptr[start + len] != 0 && url.ptr[start + len] != '/') {
        len++;
    }

    return (xml_slice){
        .ptr = url.ptr + start,
        .len = len,
    };
}

xml_slice feed_source(xml_state *state, const char *contents)
{
    xml_state start_state = *state;

    parse_until_tag(state, contents, "id");
    xml_slice id = get_text(state, contents);

    *state = start_state;

    xml_slice source = hostname_from_url(id);
    assert(source.ptr != NULL && "unable to parse feed source from id tag");

    return source;
}

xml_slice rss_source(xml_state *state, const char *contents)
{
    xml_state start_state = *state;

    parse_until_tag(state, contents, "link");
    xml_slice link = get_text(state, contents);

    *state = start_state;

    xml_slice source = hostname_from_url(link);
    assert(source.ptr != NULL && "unable to parse feed source from link tag");

    return source;
}

void print_sanitized(xml_slice slice)
{
    const char *html_lt[127] = {
        ['\''] = "&apos;",
        ['\"'] = "&quot;",
        [';'] = "&semi;",
        ['<'] = "&lt;",
        ['>'] = "&gt;",
        ['`'] = "&grave;",
        ['('] = "&#40;",
        [')'] = "&#41;",
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

struct tm *parse_date(xml_slice str, struct tm *date)
{
    // Formats with similar prefixes should be ordered by descending length
    const char *formats[] = {
        "%a, %d %b %Y %T",
        "%FT%T%z",
        "%FT%T",

        // Fix for https://verdagon.dev/blog/when-to-use-memory-safe-part-1
        //   <pubDate>Fri, Oct 7 2022 10:15:00 -0400</pubDate>
        "%a, %b %d %Y %T",

        // Fix for https://glfmn.io/posts/tidal-innards/
        //   <updated>2026-09-16</updated>
        "%F",
    };

    char buffer[256] = {0};
    assert(str.len < sizeof(buffer) / sizeof(*buffer));
    memcpy(buffer, str.ptr, str.len);

    for (int i = 0; i < sizeof(formats) / sizeof(*formats); ++i) {
        if (strptime(buffer, formats[i], date) != NULL) return date;
    }

    return NULL;
}

void print_entry(entry e, int idx)
{
    struct tm date = {0};

    if (parse_date(e.date, &date) == NULL) {
        fprintf(stderr, "failed to parse date: %.*s\n", (int)e.date.len, e.date.ptr);
        assert(0 && "unrecognized date format");
    }

    char date_buffer[64] = {0};
    size_t date_len = 0;

    date_len = strftime((char*)date_buffer, sizeof(date_buffer) / sizeof(*date_buffer), "%F", &date);
    printf("/* %.*s */ ", (int)date_len, date_buffer);

    printf("ENTRY(");
    printf("TEXT(");
    print_sanitized(e.title);
    printf("), ");

    date_len = strftime((char*)date_buffer, sizeof(date_buffer) / sizeof(*date_buffer), "%h %d, %Y", &date);
    printf("TEXT(%.*s), ", (int)date_len, date_buffer);

    printf("\"%.*s\", ", (int)e.link.len, e.link.ptr);
    printf("TEXT(%.*s), ", (int)e.source.len, e.source.ptr);

    // tags
    time_t now_s = time(NULL);
    time_t date_s = timegm(&date);
    time_t diff_s = now_s - date_s;

    printf("TEXT(");
    if (diff_s < DAY_S)   printf("today ");
    if (diff_s < WEEK_S)  printf("this-week ");
    if (diff_s < MONTH_S) printf("this-month ");
    if (diff_s < YEAR_S)  printf("this-year ");

    if (idx < 1)  printf("top-one ");
    if (idx < 5)  printf("top-five ");
    if (idx < 10) printf("top-ten ");

    printf("%.*s)", (int)e.source.len, e.source.ptr);

    printf(")\n");
}

int main(void)
{
    const char *contents = NULL;
    size_t length = 0;

    while (!feof(stdin)) {
        const char buffer[1024] = {0};

        size_t n = fread((void*)buffer, sizeof(char), 1024, stdin);

        // guarantee `contents` buffer ends with NULL byte
        contents = realloc((void*)contents, length + n + 1);

        memcpy((void*)&contents[length], buffer, n);

        length += n;
    }

    xml_state state = {0};

    xml_slice (*get_source)(xml_state*, const char*) = NULL;
    entry* (*get_entry)(xml_state*, const char*, entry*) = NULL;

    if (has_tag(&state, contents, "feed")) {
        get_source = feed_source;
        get_entry = feed_to_entry;
    } else if (has_tag(&state, contents, "rss")) {
        get_source = rss_source;
        get_entry = rss_to_entry;
    }

    assert(get_entry && "unable to determine type of xml feed");

    xml_slice source = get_source(&state, contents);

    for (int idx = 0;; ++idx) {
        entry e = { .source = source };

        if (get_entry(&state, contents, &e) == NULL) {
            break;
        }

        print_entry(e, idx);
    }

    free((void*)contents);

    return 0;
}
