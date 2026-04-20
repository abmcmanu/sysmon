/*
 * `sysmonctl top`: a small full-screen dashboard.
 *
 * Plain ANSI escape sequences and termios, no curses: the program must
 * build anywhere sysmond builds (see docs/adr/0006-tui-without-ncurses.md).
 * The screen is redrawn in one write() per refresh to avoid flicker.
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "client.h"
#include "sb.h"
#include "tui.h"

#define MAX_METRICS 256
#define SPARK_MAX 120

#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define CYAN   "\033[36m"

typedef struct kv {
    char name[64];
    double value;
} kv_t;

static kv_t kv[MAX_METRICS];
static int nkv;
static struct termios saved_tio;
static int raw_mode;
static volatile sig_atomic_t quit;
static volatile sig_atomic_t resized;
static int show_help;
static int paused;
static int no_color;
static char filter[64];
static int typing;
static char daemon_version[32];

static void on_signal(int sig)
{
    if (sig == SIGWINCH) {
        resized = 1;
        return;
    }
    (void)sig;
    quit = 1;
}

static void term_restore(void)
{
    if (!raw_mode)
        return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tio);
    fputs("\033[?25h\033[?1049l", stdout); /* cursor on, leave alt screen */
    fflush(stdout);
    raw_mode = 0;
}

static int term_raw(void)
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "sysmonctl top: stdin and stdout must be a terminal\n");
        return -1;
    }
    if (tcgetattr(STDIN_FILENO, &saved_tio) < 0)
        return -1;
    struct termios t = saved_tio;
    t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) < 0)
        return -1;
    raw_mode = 1;
    atexit(term_restore);
    fputs("\033[?1049h\033[?25l", stdout); /* alt screen, cursor off */
    return 0;
}

static void term_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row && ws.ws_col) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

/* NO_COLOR (https://no-color.org): drop SGR sequences, keep cursor control */
static void emit(sb_t *o)
{
    if (no_color && o->data) {
        size_t j = 0;
        for (size_t i = 0; i < o->len; i++) {
            if (o->data[i] == '\033' && i + 1 < o->len && o->data[i + 1] == '[') {
                size_t k = i + 2;
                while (k < o->len && (isdigit((unsigned char)o->data[k]) || o->data[k] == ';'))
                    k++;
                if (k < o->len && o->data[k] == 'm') {
                    i = k;
                    continue;
                }
            }
            o->data[j++] = o->data[i];
        }
        o->len = j;
    }
    fwrite(o->data, 1, o->len, stdout);
    fflush(stdout);
}

static int cmp_kv(const void *a, const void *b)
{
    return strcmp(((const kv_t *)a)->name, ((const kv_t *)b)->name);
}

static int fetch(const char *sock)
{
    sb_t out;
    sb_init(&out);
    if (ctl_query(sock, "list", &out) < 0) {
        sb_free(&out);
        return -1;
    }
    nkv = 0;
    char *save = NULL;
    for (char *line = out.data ? strtok_r(out.data, "\n", &save) : NULL;
         line && nkv < MAX_METRICS; line = strtok_r(NULL, "\n", &save)) {
        if (sscanf(line, "%63s %lf", kv[nkv].name, &kv[nkv].value) == 2)
            nkv++;
    }
    sb_free(&out);
    qsort(kv, (size_t)nkv, sizeof kv[0], cmp_kv);
    return 0;
}

static double val(const char *name)
{
    for (int i = 0; i < nkv; i++)
        if (strcmp(kv[i].name, name) == 0)
            return kv[i].value;
    return NAN;
}

static const char *level_color(double pct)
{
    if (pct >= 90)
        return RED;
    if (pct >= 70)
        return YELLOW;
    return GREEN;
}

static void bar(sb_t *o, double pct, int width)
{
    if (isnan(pct))
        pct = 0;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    int fill = (int)(pct * width / 100.0 + 0.5);
    sb_puts(o, level_color(pct));
    for (int i = 0; i < width; i++)
        sb_puts(o, i < fill ? "█" : DIM "░" RESET "");
    sb_puts(o, RESET);
}

static void human_bytes(double b, char *buf, size_t size)
{
    static const char *units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
    int u = 0;
    while (b >= 1024.0 && u < 4) {
        b /= 1024.0;
        u++;
    }
    snprintf(buf, size, u ? "%.1f %s" : "%.0f %s", b, units[u]);
}

static int history(const char *sock, const char *metric, int n, double *dst)
{
    char cmd[128];
    sb_t out;
    int count = 0;

    snprintf(cmd, sizeof cmd, "history %s %d", metric, n);
    sb_init(&out);
    if (ctl_query(sock, cmd, &out) == 0 && out.data && strncmp(out.data, "ERR", 3) != 0) {
        char *save = NULL;
        for (char *l = strtok_r(out.data, "\n", &save); l && count < n;
             l = strtok_r(NULL, "\n", &save))
            dst[count++] = atof(l);
    }
    sb_free(&out);
    return count;
}

static void spark(sb_t *o, const double *v, int n, double lo, double hi)
{
    static const char *blocks[] = { "▁", "▂", "▃", "▄",
                                    "▅", "▆", "▇", "█" };
    sb_puts(o, CYAN);
    for (int i = 0; i < n; i++) {
        double x = isnan(v[i]) ? lo : v[i];
        int idx = hi > lo ? (int)((x - lo) / (hi - lo) * 7.0 + 0.5) : 0;
        if (idx < 0)
            idx = 0;
        if (idx > 7)
            idx = 7;
        sb_puts(o, blocks[idx]);
    }
    sb_puts(o, RESET);
}

static void render(const char *sock, int interval_ms, int connected)
{
    int rows, cols, used = 0;
    char host[64] = "?", ts[16], a[32], b[32];
    time_t t = time(NULL);
    struct tm tm;
    sb_t o;

    term_size(&rows, &cols);
    gethostname(host, sizeof host - 1);
    localtime_r(&t, &tm);
    strftime(ts, sizeof ts, "%H:%M:%S", &tm);

    sb_init(&o);
    sb_puts(&o, "\033[H\033[2J");
    sb_printf(&o, BOLD "sysmon top" RESET "  %s  %s  every %.1fs  " DIM "q quit"
                  "  +/- speed"
                  "  h help"
                  RESET "\n\n",
              host, ts, interval_ms / 1000.0);
    used += 2;
    if (daemon_version[0]) {
        sb_printf(&o, DIM "  %s" RESET "\n", daemon_version);
        used++;
    }
    if (paused) {
        sb_puts(&o, YELLOW "  paused, space to resume" RESET "\n");
        used++;
    }

    if (!connected) {
        sb_printf(&o, RED "  cannot reach sysmond at %s, retrying..." RESET "\n", sock);
        emit(&o);
        sb_free(&o);
        return;
    }

    int barw = cols - 40;
    if (barw > 50)
        barw = 50;
    if (barw < 10)
        barw = 10;

    double cpu = val("cpu_usage_percent");
    sb_printf(&o, "  CPU   %5.1f%% ", isnan(cpu) ? 0.0 : cpu);
    bar(&o, cpu, barw);
    double h[SPARK_MAX];
    int sw = cols - barw - 18;
    if (sw > SPARK_MAX)
        sw = SPARK_MAX;
    if (sw > 8) {
        int n = history(sock, "cpu_usage_percent", sw, h);
        sb_puts(&o, "  ");
        spark(&o, h, n, 0, 100);
    }
    sb_puts(&o, "\n");

    double mem = val("mem_used_percent");
    sb_printf(&o, "  MEM   %5.1f%% ", isnan(mem) ? 0.0 : mem);
    bar(&o, mem, barw);
    if (sw > 8) {
        int n = history(sock, "mem_used_percent", sw, h);
        sb_puts(&o, "  ");
        spark(&o, h, n, 0, 100);
    }
    human_bytes(val("mem_used_bytes"), a, sizeof a);
    human_bytes(val("mem_total_bytes"), b, sizeof b);
    sb_printf(&o, "\n        " DIM "%s / %s" RESET "\n", a, b);
    used += 3;

    double swap_total = val("swap_total_bytes");
    if (!isnan(swap_total) && swap_total > 0) {
        double swap = 100.0 * val("swap_used_bytes") / swap_total;
        sb_printf(&o, "  SWAP  %5.1f%% ", swap);
        bar(&o, swap, barw);
        sb_puts(&o, "\n");
        used++;
    }
    sb_printf(&o, "  LOAD  %.2f %.2f %.2f\n", val("load1"), val("load5"), val("load15"));
    used++;

    sb_puts(&o, "\n");
    used++;
    for (int i = 0; i < nkv; i++) {
        const char *n = kv[i].name;
        size_t len = strlen(n);
        const char *suffix = "_used_percent";
        if (strncmp(n, "disk_", 5) != 0 || len <= 5 + strlen(suffix) ||
            strcmp(n + len - strlen(suffix), suffix) != 0)
            continue;
        sb_printf(&o, "  %-6.*s%5.1f%% ", (int)(len - 5 - strlen(suffix)), n + 5, kv[i].value);
        bar(&o, kv[i].value, barw);
        sb_puts(&o, "\n");
        used++;
    }
    double rx = val("net_rx_bytes_per_second"), tx = val("net_tx_bytes_per_second");
    if (!isnan(rx) && !isnan(tx)) {
        human_bytes(rx, a, sizeof a);
        human_bytes(tx, b, sizeof b);
        sb_printf(&o, "  NET   " GREEN "↓" RESET " %s/s   " CYAN "↑" RESET " %s/s\n", a, b);
        used++;
    }
    double up = val("uptime_seconds");
    if (!isnan(up)) {
        long s = (long)up;
        sb_printf(&o, "  UP    %ldd %02ld:%02ld\n", s / 86400, (s / 3600) % 24, (s / 60) % 60);
        used++;
    }

    double firing = val("alerts_firing");
    if (!isnan(firing) && firing > 0) {
        sb_printf(&o, "\n  " RED BOLD "%d alert(s) firing" RESET DIM "  (sysmonctl alerts)" RESET "\n",
                  (int)firing);
        used += 2;
    }

    if (show_help) {
        sb_puts(&o, "\n  " BOLD "keys" RESET "\n"
                    "  q        quit\n"
                    "  h ?      show / hide this help\n"
                    "  space    pause / resume\n"
                    "  + -      faster / slower refresh\n"
                    "  /        filter metrics, Enter to keep, Esc to clear\n"
        );
        emit(&o);
        sb_free(&o);
        return;
    }

    /* everything else, in as many columns as fit */
    if (typing)
        sb_printf(&o, "\n  filter: %s\u2588\n", filter);
    else if (filter[0])
        sb_printf(&o, "\n  " DIM "metrics matching" RESET " %s\n", filter);
    else
        sb_puts(&o, "\n  " DIM "all metrics" RESET "\n");
    used += 2;
    int colw = 46, ncol = (cols - 2) / colw;
    if (ncol < 1)
        ncol = 1;
    int room = (rows - used - 1) * ncol;
    int shown = 0;
    for (int i = 0; i < nkv && shown < room; i++) {
        if (filter[0] && !strstr(kv[i].name, filter))
            continue;
        sb_printf(&o, "  %-30.30s %12.4g", kv[i].name, kv[i].value);
        if (++shown % ncol == 0)
            sb_puts(&o, "\n");
    }
    if (shown % ncol)
        sb_puts(&o, "\n");

    emit(&o);
    sb_free(&o);
}

int tui_run(const char *sock, int interval_ms)
{
    if (interval_ms < 200)
        interval_ms = 200;
    if (fetch(sock) < 0) {
        fprintf(stderr, "sysmonctl: cannot reach sysmond at %s: %s\n", sock, strerror(errno));
        return -1;
    }
    const char *nc = getenv("NO_COLOR");
    no_color = nc && *nc;
    sb_t v;
    sb_init(&v);
    if (ctl_query(sock, "version", &v) == 0 && v.data) {
        v.data[strcspn(v.data, "\n")] = '\0';
        snprintf(daemon_version, sizeof daemon_version, "%s", v.data);
    }
    sb_free(&v);
    if (term_raw() < 0)
        return -1;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGWINCH, &sa, NULL);

    int connected = 1;
    while (!quit) {
        render(sock, interval_ms, connected);

        struct pollfd p = { STDIN_FILENO, POLLIN, 0 };
        int n = poll(&p, 1, interval_ms);
        if (n < 0 && errno == EINTR && resized) {
            resized = 0; /* redraw at the new size, data can wait */
            continue;
        }
        if (n > 0) {
            char c;
            if (read(STDIN_FILENO, &c, 1) == 1) {
                size_t l = strlen(filter);
                if (typing) {
                    if (c == '\r' || c == '\n')
                        typing = 0;
                    else if (c == 27) {
                        typing = 0;
                        filter[0] = '\0';
                    } else if ((c == 127 || c == 8) && l)
                        filter[l - 1] = '\0';
                    else if (c >= 32 && c < 127 && l + 1 < sizeof filter) {
                        filter[l] = c;
                        filter[l + 1] = '\0';
                    }
                    continue;
                }
                if (c == '/') {
                    typing = 1;
                    filter[0] = '\0';
                    continue;
                }
                if (c == 'q' || c == 'Q')
                    break;
                if (c == 'h' || c == '?')
                    show_help = !show_help;
                if (c == ' ')
                    paused = !paused;
                if (c == '+' && interval_ms > 250)
                    interval_ms /= 2;
                if (c == '-' && interval_ms < 10000)
                    interval_ms *= 2;
            }
        }
        if (!paused)
            connected = fetch(sock) == 0;
    }
    term_restore();
    return 0;
}
