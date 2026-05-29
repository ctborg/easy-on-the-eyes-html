#include "test_support.h"

#include "engine_html.h"

#include <stdlib.h>
#include <string.h>

void run_engine_html_tests(void) {
    Config cfg = test_config();
    char err[128] = {0};
    const char *html = "<div><p class='x'>Hi<br></p></div>";
    char *out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html formats", out != NULL);
    test_expect("html quote normalization", out && strstr(out, "class=\"x\"") != NULL);
    test_expect("html void self close", out && strstr(out, "<br />") != NULL);
    free(out);
    html = "<script>if(x){y()}</script>";
    out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html script preserved", out && strstr(out, "if(x){y()}") != NULL);
    test_expect("html script close formatted", out && strstr(out, "\n</script>\n") != NULL);
    free(out);
    html = "<div><p>Hello <a href='x'>link</a> and <span class='y'>span</span>.</p></div>";
    out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html anchor spacing", out && strstr(out, "Hello <a href=\"x\">link</a> and <span class=\"y\">span</span>.") != NULL);
    test_expect("html span inline", out && strstr(out, "<span class=\"y\">span</span>.") != NULL);
    free(out);
    html = "<!DOCTYPE html><html lang='en'><head><title>borg.fyi</title><meta charset='UTF-8'></head><body><canvas id='bg'></canvas><main><article><span class='project-meta'>Music</span><div><a href='https://example.com'>Open</a></div></article></main></body></html>";
    out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html document tags split", out && strstr(out, "<html lang=\"en\">\n  <head>\n    <title>") != NULL);
    test_expect("html title compact", out && strstr(out, "<title>borg.fyi</title>") != NULL);
    test_expect("html body split", out && strstr(out, "  <body>\n    <canvas id=\"bg\">") != NULL);
    test_expect("html empty canvas compact", out && strstr(out, "<canvas id=\"bg\"></canvas>") != NULL);
    test_expect("html standalone span indented", out && strstr(out, "        <span class=\"project-meta\">Music</span>") != NULL);
    test_expect("html standalone anchor indented", out && strstr(out, "          <a href=\"https://example.com\">Open</a>") != NULL);
    free(out);
    html = "<div><address>123 Main</address><blockquote><p>Quote</p></blockquote><dl><dt>Term</dt><dd>Definition</dd></dl><details><summary>More</summary><p>Body</p></details><video><source src='movie.mp4'><track src='captions.vtt'></video></div>";
    out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html address block", out && strstr(out, "\n  <address>") != NULL);
    test_expect("html dl children block", out && strstr(out, "\n    <dt>Term</dt>\n    <dd>Definition</dd>") != NULL);
    test_expect("html details block", out && strstr(out, "\n  <details>\n    <summary>More</summary>") != NULL);
    test_expect("html media void children", out && strstr(out, "<source src=\"movie.mp4\" />") != NULL && strstr(out, "<track src=\"captions.vtt\" />") != NULL);
    free(out);
    html = "<pre>  keep\\n    space</pre><textarea>hello\\n  world</textarea>";
    out = engine_html_format(html, strlen(html), &cfg, err, sizeof(err));
    test_expect("html pre raw preserved", out && strstr(out, "  keep\\n    space") != NULL);
    test_expect("html textarea raw preserved", out && strstr(out, "hello\\n  world") != NULL);
    free(out);
}
