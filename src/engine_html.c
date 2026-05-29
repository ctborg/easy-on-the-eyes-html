#include "engine_html.h"

#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool starts_ci(const char *s, size_t len, size_t pos, const char *needle) {
    size_t n = strlen(needle);
    if (pos + n > len) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        if (tolower((unsigned char)s[pos + i]) != tolower((unsigned char)needle[i])) {
            return false;
        }
    }
    return true;
}

static const char *find_ci(const char *s, size_t len, size_t pos, const char *needle) {
    size_t n = strlen(needle);
    if (n == 0) {
        return s + pos;
    }
    for (size_t i = pos; i + n <= len; i++) {
        if (starts_ci(s, len, i, needle)) {
            return s + i;
        }
    }
    return NULL;
}

static bool name_in_list(const char *name, const char *const *list) {
    for (int i = 0; list[i]; i++) {
        if (strcmp(name, list[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_void_name(const char *name) {
    static const char *const voids[] = {
        "area","base","br","col","embed","frame","hr","img","input","link",
        "meta","param","source","track","wbr",NULL
    };
    return name_in_list(name, voids);
}

static bool is_inline_name(const char *name) {
    static const char *const inline_names[] = {
        "a","abbr","acronym","b","bdi","bdo","big","button","cite","code",
        "data","del","dfn","em","font","i","ins","kbd","label","mark","meter",
        "nobr","output","progress","q","rb","rp","rt","rtc","ruby","s","samp",
        "selectedcontent","slot","small","span","strike","strong","sub",
        "summary","sup","time","tt","u","var",NULL
    };
    return name_in_list(name, inline_names);
}

static bool is_block_name(const char *name) {
    if (name[0] == 'h' && name[1] >= '1' && name[1] <= '6' && name[2] == '\0') {
        return true;
    }
    if (is_void_name(name) || is_inline_name(name) || name[0] == '\0') {
        return false;
    }
    static const char *const block_names[] = {
        "address","article","aside","audio","blockquote","body","canvas",
        "caption","center","colgroup","datalist","dd","details","dialog",
        "dir","div","dl","dt","fieldset","fencedframe","figcaption","figure",
        "footer","form","frameset","geolocation","head","header","hgroup",
        "html","iframe","legend","li","main","map","marquee","menu","nav",
        "noembed","noframes","noscript","object","ol","optgroup","option",
        "p","picture","plaintext","pre","script","search","section","select",
        "style","table","tbody","td","template","textarea","tfoot","th",
        "thead","title","tr","ul","video","xmp",NULL
    };
    return name_in_list(name, block_names);
}

static bool is_compact_text_name(const char *name) {
    static const char *const compact[] = {
        "caption","dd","dt","legend","li","option","p","summary","title",
        "h1","h2","h3","h4","h5","h6",NULL
    };
    return name_in_list(name, compact);
}

static bool is_raw_text_name(const char *name) {
    static const char *const raw_names[] = {"script","style","pre","textarea","xmp","plaintext",NULL};
    return name_in_list(name, raw_names);
}

static void tag_name(const char *src, size_t len, size_t pos, char *name, size_t name_len) {
    size_t i = pos;
    if (i < len && src[i] == '<') {
        i++;
    }
    if (i < len && src[i] == '/') {
        i++;
    }
    size_t n = 0;
    while (i < len && (isalnum((unsigned char)src[i]) || src[i] == '-')) {
        if (n + 1 < name_len) {
            name[n++] = (char)tolower((unsigned char)src[i]);
        }
        i++;
    }
    name[n] = '\0';
}

static bool append_normalized_tag(StrBuf *out, const char *src, size_t start, size_t end, bool make_self_closing) {
    char quote = '\0';
    bool in_value = false;
    for (size_t i = start; i < end; i++) {
        char c = src[i];
        if ((c == '\'' || c == '"') && (i == start || src[i - 1] != '\\')) {
            if (!in_value) {
                in_value = true;
                quote = c;
                c = '"';
            } else if (quote == c) {
                in_value = false;
                c = '"';
            }
        }
        if (!sb_append_char(out, c)) {
            return false;
        }
    }
    if (make_self_closing) {
        while (out->len > 0 && (out->data[out->len - 1] == ' ' || out->data[out->len - 1] == '/' || out->data[out->len - 1] == '>')) {
            out->len--;
            out->data[out->len] = '\0';
        }
        return sb_append(out, " />");
    }
    return true;
}

static size_t skip_html_ws(const char *src, size_t len, size_t pos) {
    while (pos < len && isspace((unsigned char)src[pos])) {
        pos++;
    }
    return pos;
}

static bool is_closing_tag_for(const char *src, size_t len, size_t pos, const char *name) {
    if (pos + 3 > len || src[pos] != '<' || src[pos + 1] != '/') {
        return false;
    }
    char found[32];
    tag_name(src, len, pos, found, sizeof(found));
    return strcmp(found, name) == 0;
}

static size_t find_tag_end(const char *src, size_t len, size_t pos) {
    char quote = '\0';
    while (pos < len) {
        char c = src[pos];
        if ((c == '\'' || c == '"') && (pos == 0 || src[pos - 1] != '\\')) {
            quote = quote == '\0' ? c : (quote == c ? '\0' : quote);
        } else if (c == '>' && quote == '\0') {
            return pos + 1;
        }
        pos++;
    }
    return len;
}

static void trim_html_line_tail(StrBuf *out) {
    while (out->len > 0 && (out->data[out->len - 1] == ' ' || out->data[out->len - 1] == '\t')) {
        out->len--;
        out->data[out->len] = '\0';
    }
}

static bool append_text_node(StrBuf *out, const char *src, size_t start, size_t end, const Config *cfg, int depth, bool *line_start) {
    size_t a = start;
    size_t b = end;
    while (a < b && isspace((unsigned char)src[a])) {
        a++;
    }
    while (b > a && isspace((unsigned char)src[b - 1])) {
        b--;
    }
    if (b <= a) {
        return true;
    }
    bool had_leading_space = a > start;
    bool had_trailing_space = b < end;
    if (*line_start) {
        if (!sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
            return false;
        }
    } else if (had_leading_space && out->len > 0 &&
               out->data[out->len - 1] != ' ' && out->data[out->len - 1] != '\n' &&
               out->data[out->len - 1] != '\t') {
        if (!sb_append_char(out, ' ')) {
            return false;
        }
    }
    for (size_t i = a; i < b; i++) {
        if (isspace((unsigned char)src[i])) {
            while (i + 1 < b && isspace((unsigned char)src[i + 1])) {
                i++;
            }
            if (!sb_append_char(out, ' ')) {
                return false;
            }
        } else if (!sb_append_char(out, src[i])) {
            return false;
        }
    }
    if (had_trailing_space) {
        if (out->len > 0 && out->data[out->len - 1] != ' ') {
            if (!sb_append_char(out, ' ')) {
                return false;
            }
        }
    }
    *line_start = false;
    return true;
}

static bool append_tag_at_current_level(StrBuf *out, const char *src, size_t start, size_t end, bool block, bool void_tag, const Config *cfg, int depth, bool *line_start) {
    if (block || void_tag) {
        if (!*line_start) {
            trim_html_line_tail(out);
            if (!sb_append_char(out, '\n')) {
                return false;
            }
        }
        if (!sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
            return false;
        }
    } else if (*line_start) {
        if (!sb_append_indent(out, depth, cfg->indent_size, cfg->use_tabs == 1)) {
            return false;
        }
    }
    return append_normalized_tag(out, src, start, end, void_tag);
}

char *engine_html_format(const char *src, size_t src_len, const Config *cfg, char *err, size_t err_len) {
    (void)err;
    (void)err_len;
    StrBuf out;
    sb_init(&out);
    int depth = 0;
    bool line_start = true;
    for (size_t i = 0; i < src_len;) {
        if (src[i] != '<') {
            size_t start = i;
            while (i < src_len && src[i] != '<') {
                i++;
            }
            if (!append_text_node(&out, src, start, i, cfg, depth, &line_start)) {
                sb_free(&out);
                return NULL;
            }
            continue;
        }
        if (starts_ci(src, src_len, i, "<!--")) {
            char *endp = strstr(src + i + 4, "-->");
            size_t end = endp ? (size_t)(endp - src) + 3 : src_len;
            if (!line_start) {
                if (!sb_append_char(&out, '\n')) {
                    sb_free(&out);
                    return NULL;
                }
            }
            if (!sb_append_indent(&out, depth, cfg->indent_size, cfg->use_tabs == 1) ||
                !sb_append_n(&out, src + i, end - i) ||
                !sb_append_char(&out, '\n')) {
                sb_free(&out);
                return NULL;
            }
            line_start = true;
            i = end;
            continue;
        }
        size_t tag_end = i;
        char quote = '\0';
        while (tag_end < src_len) {
            char c = src[tag_end];
            if ((c == '\'' || c == '"') && (tag_end == i || src[tag_end - 1] != '\\')) {
                quote = quote == '\0' ? c : (quote == c ? '\0' : quote);
            } else if (c == '>' && quote == '\0') {
                tag_end++;
                break;
            }
            tag_end++;
        }
        if (tag_end > src_len) {
            tag_end = src_len;
        }
        char name[32];
        tag_name(src, src_len, i, name, sizeof(name));
        bool closing = i + 1 < src_len && src[i + 1] == '/';
        bool doctype = starts_ci(src, src_len, i, "<!DOCTYPE");
        bool block = is_block_name(name) || doctype;
        bool void_tag = is_void_name(name);
        if (!closing && is_raw_text_name(name)) {
            char close_pat[48];
            snprintf(close_pat, sizeof(close_pat), "</%s>", name);
            const char *close = find_ci(src, src_len, tag_end, close_pat);
            size_t content_end = close ? (size_t)(close - src) : tag_end;
            size_t content_trim_end = content_end;
            while (content_trim_end > tag_end &&
                   (src[content_trim_end - 1] == ' ' || src[content_trim_end - 1] == '\t')) {
                content_trim_end--;
            }
            size_t end = close ? content_end + strlen(close_pat) : tag_end;
            if (!line_start) {
                trim_html_line_tail(&out);
                if (!sb_append_char(&out, '\n')) {
                    sb_free(&out);
                    return NULL;
                }
            }
            if (!sb_append_indent(&out, depth, cfg->indent_size, cfg->use_tabs == 1) ||
                !append_normalized_tag(&out, src, i, tag_end, false)) {
                sb_free(&out);
                return NULL;
            }
            if (content_trim_end > tag_end) {
                if (!sb_append_n(&out, src + tag_end, content_trim_end - tag_end)) {
                    sb_free(&out);
                    return NULL;
                }
            }
            if (close) {
                if (out.len == 0 || out.data[out.len - 1] != '\n') {
                    if (!sb_append_char(&out, '\n')) {
                        sb_free(&out);
                        return NULL;
                    }
                }
                if (!sb_append_indent(&out, depth, cfg->indent_size, cfg->use_tabs == 1) ||
                    !sb_append_n(&out, src + content_end, strlen(close_pat))) {
                    sb_free(&out);
                    return NULL;
                }
            }
            if (!sb_append_char(&out, '\n')) {
                sb_free(&out);
                return NULL;
            }
            line_start = true;
            i = end;
            continue;
        }
        if (!closing && !void_tag) {
            size_t after_open_ws = skip_html_ws(src, src_len, tag_end);
            if (is_closing_tag_for(src, src_len, after_open_ws, name)) {
                size_t close_end = find_tag_end(src, src_len, after_open_ws);
                if (!append_tag_at_current_level(&out, src, i, tag_end, block, false, cfg, depth, &line_start) ||
                    !append_normalized_tag(&out, src, after_open_ws, close_end, false)) {
                    sb_free(&out);
                    return NULL;
                }
                if (block || doctype) {
                    if (!sb_append_char(&out, '\n')) {
                        sb_free(&out);
                        return NULL;
                    }
                    line_start = true;
                } else {
                    line_start = false;
                }
                i = close_end;
                continue;
            }
            if (is_compact_text_name(name)) {
                size_t close_pos = tag_end;
                while (close_pos < src_len && src[close_pos] != '<') {
                    close_pos++;
                }
                size_t text_start = tag_end;
                size_t text_end = close_pos;
                while (text_start < text_end && isspace((unsigned char)src[text_start])) {
                    text_start++;
                }
                while (text_end > text_start && isspace((unsigned char)src[text_end - 1])) {
                    text_end--;
                }
                if (is_closing_tag_for(src, src_len, close_pos, name) && text_end - text_start <= 100) {
                    size_t close_end = find_tag_end(src, src_len, close_pos);
                    if (!append_tag_at_current_level(&out, src, i, tag_end, block, false, cfg, depth, &line_start) ||
                        !sb_append_n(&out, src + text_start, text_end - text_start) ||
                        !append_normalized_tag(&out, src, close_pos, close_end, false)) {
                        sb_free(&out);
                        return NULL;
                    }
                    if (block || doctype) {
                        if (!sb_append_char(&out, '\n')) {
                            sb_free(&out);
                            return NULL;
                        }
                        line_start = true;
                    } else {
                        line_start = false;
                    }
                    i = close_end;
                    continue;
                }
            }
        }
        if (closing && depth > 0 && block) {
            depth--;
        }
        if (!append_tag_at_current_level(&out, src, i, tag_end, block, void_tag, cfg, depth, &line_start)) {
            sb_free(&out);
            return NULL;
        }
        if (block || void_tag || doctype) {
            if (!sb_append_char(&out, '\n')) {
                sb_free(&out);
                return NULL;
            }
            line_start = true;
        } else {
            line_start = false;
        }
        if (!doctype && !closing && block && !void_tag && src[tag_end >= 2 ? tag_end - 2 : tag_end - 1] != '/') {
            depth++;
        }
        i = tag_end;
    }
    if (!line_start) {
        if (!sb_append_char(&out, '\n')) {
            sb_free(&out);
            return NULL;
        }
    }
    return sb_take(&out);
}
