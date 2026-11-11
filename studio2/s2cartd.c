/* s2cartd -- the RCA Studio II FujiNet cartridge's core0, for the MiSTer.
 *
 * The StudioII core's rtl/fujinet/fujinet_cart.sv is the cart's core1 (the
 * per-read decision). This daemon is everything else, built from the cart
 * firmware's own sources exactly as pico/studio2/emu/studio2_fujinet.cpp
 * builds the MAME device: fujimail, fujibus, s2map, fuji_load, s2_text and the
 * baked CONFIG. It keeps the FPGA's tables and memories in step with its own
 * view, takes the reads the FPGA reports, and talks FujiBus to fujinet-pc
 * (FUJINET_TARGET=RS232, BoIP) over TCP.
 *
 *   s2cartd [-d /dev/ttyS1] [-b 1562500] [-t 127.0.0.1:9995] [-v]
 *
 * The link is the core's UART: SLIP frames, see fujinet_cart.sv for the map.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
/* termios2 for an arbitrary baud; <asm/termbits.h> clashes with <termios.h> */
#include <asm/ioctls.h>
struct termios2 {
    tcflag_t c_iflag, c_oflag, c_cflag, c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed, c_ospeed;
};
#ifndef BOTHER
#define BOTHER 0010000
#endif
#endif

#include "fuji_mailbox.h"
#include "fujimail.h"
#include "fujitcp.h"
#include "s2map.h"
#include "s2_text.h"
#include "fuji_load.h"
#include "fujiconfigrom.h"

#define S2_RING_SWAP 0xFFFFu          /* as s2_cart.h: a swap, never a hotspot */

/* ---------------------------------------------------------------- link map */

#define LA_BUF(v)      ((uint32_t)(v) << 16)
#define LA_ARENA       0x20000u
#define LA_RASTER(b)   (0x20800u + ((uint32_t)(b) << 10))
#define LA_PGTBL(v)    (0x21000u + ((uint32_t)(v) << 9))
#define LA_BKTBL(v)    (0x21400u + ((uint32_t)(v) << 9))
#define LA_CTRL        0x22000u
enum { C_CTRL = 0, C_LIVE, C_ARMED, C_PENDING, C_MBX0, C_MBX1, C_NBK0, C_NBK1, C_PING, C_BUSRESET };

#define K_BUF   0u
#define K_ZERO  1u
#define K_FF    2u
#define K_ARENA 3u

#define SL_END  0xC0
#define SL_ESC  0xDB
#define SL_EEND 0xDC
#define SL_EESC 0xDD

static int verbose;
static int link_fd = -1;

static void logf_(const char *fmt, ...)
{
    va_list ap;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    fprintf(stderr, "[%ld.%03ld] s2cartd: ", (long)tv.tv_sec % 100000, (long)tv.tv_usec / 1000);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
#define LOG(...)  logf_(__VA_ARGS__)
#define VLOG(...) do { if (verbose) logf_(__VA_ARGS__); } while (0)

/* ---------------------------------------------------------------- link out */

static uint8_t tx_buf[65536];
static size_t tx_len;

static void tx_flush(void)
{
    size_t off = 0;

    while (off < tx_len) {
        ssize_t n = write(link_fd, tx_buf + off, tx_len - off);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                struct pollfd p = { link_fd, POLLOUT, 0 };
                poll(&p, 1, 100);
                continue;
            }
            LOG("link write: %s", strerror(errno));
            exit(1);
        }
        off += (size_t)n;
    }
    tx_len = 0;
}

static void tx_byte(uint8_t b)
{
    if (tx_len + 2 > sizeof tx_buf)
        tx_flush();
    if (b == SL_END) {
        tx_buf[tx_len++] = SL_ESC;
        tx_buf[tx_len++] = SL_EEND;
    } else if (b == SL_ESC) {
        tx_buf[tx_len++] = SL_ESC;
        tx_buf[tx_len++] = SL_EESC;
    } else {
        tx_buf[tx_len++] = b;
    }
}

static void tx_end(void)
{
    if (tx_len + 1 > sizeof tx_buf)
        tx_flush();
    tx_buf[tx_len++] = SL_END;
}

/* One run of sequential writes; pokes to the next address join the open run. */
static uint32_t run_a;
static uint32_t run_n;

static void run_close(void)
{
    if (run_n) {
        tx_end();
        run_n = 0;
    }
}

static void lwrite(uint32_t a, const uint8_t *d, size_t n)
{
    while (n) {
        if (!run_n || a != run_a + run_n || run_n >= 512) {
            run_close();
            tx_end();
            tx_byte('W');
            tx_byte((uint8_t)(a >> 16));
            tx_byte((uint8_t)(a >> 8));
            tx_byte((uint8_t)a);
            run_a = a;
        }
        tx_byte(*d++);
        run_n++;
        a++;
        n--;
    }
}

static void lwrite1(uint32_t a, uint8_t v) { lwrite(a, &v, 1); }

static void lctrl(unsigned reg, uint8_t v) { lwrite1(LA_CTRL + reg, v); }

static void lwrite16(uint32_t a, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
    lwrite(a, b, 2);
}

static void link_flush(void)
{
    run_close();
    tx_flush();
}

/* ---------------------------------------------------------------- the cart */

static uint8_t image_buf[2][S2MAP_BUF_MAX];
static uint8_t arena[FN_ARENA_SIZE];
static s2_text_t text;
static fuji_load_t load;
static s2_copy_t copyreg;
static uint8_t sent_raster[2][FN_RASTER_SIZE];

static uint8_t *push;
static uint32_t push_len;

/* hotspot reads in order; S2_RING_SWAP marks a swap */
#define EVQ_LEN 65536
static uint16_t evq[EVQ_LEN];
static unsigned evq_head, evq_tail;

static uint8_t fpga_ver, fpga_status;
static bool fpga_seen;

static void evq_push(uint16_t a)
{
    unsigned n = (evq_head + 1) % EVQ_LEN;

    if (n == evq_tail) {
        LOG("event queue full: dropped $%04X", a);
        return;
    }
    evq[evq_head] = a;
    evq_head = n;
}

static uint16_t entry(unsigned v, const uint8_t *page, const uint8_t *wpage, unsigned type)
{
    const uint8_t *b = image_buf[v];
    unsigned kind = K_ZERO, idx = 0;
    bool wr = false;

    if (page >= b && page < b + S2MAP_BUF_MAX) {
        kind = K_BUF;
        idx = (unsigned)((page - b) / S2MAP_PAGE);
        wr = wpage != NULL;
    } else if (page >= arena && page < arena + FN_ARENA_SIZE) {
        kind = K_ARENA;
        idx = (unsigned)((page - arena) / S2MAP_PAGE);
    } else if (page == s2map_ff_page) {
        kind = K_FF;
    }
    return (uint16_t)((type << 14) | (wr ? 0x2000u : 0) | (kind << 8) | idx);
}

/* Everything view v serves: its tables, flags and the buffer pages it names. */
static void upload_view(unsigned v)
{
    const s2_view_t *vw = &load.view[v];
    const s2map_plan_t *p = &load.plan[v];
    static bool used[S2MAP_PAGES];
    unsigned pg, bk, w, k;

    memset(used, 0, sizeof used);
    for (pg = 0; pg < S2MAP_PAGES; pg++) {
        lwrite16(LA_PGTBL(v) + pg * 2, entry(v, vw->page[pg], vw->wpage[pg], vw->type[pg]));
        if (p->src[pg] >= S2SRC_BUF(0))
            used[p->src[pg] - 2u] = true;
    }
    for (bk = 0; bk < S2MAP_NBANKS; bk++)
        for (w = 0; w < 16; w++) {
            uint16_t e = (uint16_t)(K_FF << 8);

            if (w < S2MAP_WIN_PAGES) {
                e = entry(v, vw->bank_page[bk][w], vw->bank_wpage[bk][w], 0);
                if (p->bank_src[bk][w] >= S2SRC_BUF(0))
                    used[p->bank_src[bk][w] - 2u] = true;
            }
            lwrite16(LA_BKTBL(v) + (bk * 16 + w) * 2, e);
        }
    for (k = 0; k < S2MAP_PAGES && k * S2MAP_PAGE < S2MAP_BUF_MAX; k++)
        if (used[k])
            lwrite(LA_BUF(v) + k * S2MAP_PAGE, image_buf[v] + k * S2MAP_PAGE, S2MAP_PAGE);
    lctrl(C_MBX0 + v, vw->mailbox ? 1 : 0);
    lctrl(C_NBK0 + v, vw->nbanks);
    VLOG("view %u: %s, %u blocks, %u banks%s", v, p->st2 ? "ST2" : "raw", p->blocks,
         vw->nbanks, vw->mailbox ? ", mailbox" : "");
}

static void poke(unsigned offset, uint8_t value)
{
    s2_text_boot_poke(&text, offset, value);
    if (offset >= FN_R_PAINT_END)
        return;
    arena[offset] = value;
    lwrite1(LA_ARENA + offset, value);
}

static void publish_staged(void)
{
    const s2map_plan_t *p = &load.plan[load.live ^ 1u];
    uint8_t v = 0;
    unsigned i;

    if (load.staged) {
        v = FN_STAGED_READY;
        if (p->claim)
            v |= FN_STAGED_CLAIM;
        if (load.staged_config)
            v |= FN_STAGED_CONFIG;
    }
    for (i = 0; i < 4; i++)
        poke(FN_R_STAGED_CRC + i, (uint8_t)(p->crc >> (8 * i)));
    poke(FN_R_STAGED, v);
    poke(FN_R_ARMED, load.armed ? 1 : 0);
}

/* The new image is running (the FPGA swapped on the BIOS's first read). */
static void swapped(void)
{
    bool mailbox = load.view[load.live].mailbox;

    if (mailbox) {
        s2_text_init(&text, arena + FN_R_DATA);
        fujimail_paint();
    }
    publish_staged();
    poke(FN_R_SWAPS, load.swaps);
    poke(FN_R_MODE, mailbox ? FN_MODE_APP : FN_MODE_GAME);
    LOG("swap %u -> %s", load.swaps, mailbox ? "app" : "game");
}

static void compose(void)
{
    struct s2_disp *d = &text.disp;
    unsigned b, i;

    if (!s2_text_compose(&text))
        return;
    b = d->pending;
    if (b == S2D_NONE)
        return;
    for (i = 0; i < FN_RASTER_SIZE; ) {
        unsigned j;

        if (d->buf[b][i] == sent_raster[b][i]) {
            i++;
            continue;
        }
        for (j = i; j < FN_RASTER_SIZE && d->buf[b][j] != sent_raster[b][j]; j++)
            ;
        lwrite(LA_RASTER(b) + i, d->buf[b] + i, j - i);
        memcpy(sent_raster[b] + i, d->buf[b] + i, j - i);
        i = j;
    }
    lctrl(C_PENDING, (uint8_t)(0x80u | b));
}

/* Image bytes core0 changed in the live view (a COPY): to the FPGA. */
static void push_range(uint16_t a, uint16_t n)
{
    const s2_view_t *vw = &load.view[load.live];
    uint8_t *base = image_buf[load.live];

    while (n--) {
        uint8_t *wp = vw->wpage[a >> 8];

        if (wp) {
            uint32_t off = (uint32_t)(wp - base) + (a & 0xFFu);
            lwrite1(LA_BUF(load.live) + off, base[off]);
        }
        a++;
    }
}

/* ---------------------------------------------------------------- fujimail port */

static bool c_link_up(void) { return fujitcp_active(); }

static uint8_t c_stream_open(int stream, uint32_t size)
{
    uint8_t err;

    if (stream != FN_STREAM_ROM)
        return 0;
    err = s2map_gate(size);
    if (err)
        return err;
    push = fuji_load_target(&load);
    lctrl(C_ARMED, 0);                  /* never swap to a buffer being written */
    push_len = 0;
    publish_staged();
    return 0;
}

static void c_stream_write(int stream, const uint8_t *chunk, unsigned len)
{
    if (stream != FN_STREAM_ROM || !push)
        return;
    if (push_len + len > S2MAP_BUF_MAX)
        len = S2MAP_BUF_MAX - push_len;
    memcpy(push + push_len, chunk, len);
    push_len += len;
}

static uint8_t c_stream_close(int stream, uint32_t got, bool aborted)
{
    uint8_t err;

    if (stream != FN_STREAM_ROM)
        return 0;
    if (aborted || !push || push_len == 0) {
        push = NULL;
        return 0;
    }
    if (got > push_len) {
        push = NULL;
        return FN_BOOT_ERR_TOOBIG;
    }
    err = fuji_load_commit(&load, push_len);
    push = NULL;
    if (!err) {
        upload_view(load.live ^ 1u);
        LOG("staged %u bytes, %u blocks%s", load.plan[load.live ^ 1u].size,
            load.plan[load.live ^ 1u].blocks, load.plan[load.live ^ 1u].claim ? ", claimed" : "");
    }
    publish_staged();
    return err;
}

static void c_arm_swap(void)
{
    fuji_load_arm(&load);
    lctrl(C_ARMED, load.armed ? 1 : 0);
    publish_staged();
}

static void c_own_reg(uint8_t reg, uint8_t value)
{
    if (reg == FN_REG_CONFIG && value == FN_CONFIG_MAGIC) {
        lctrl(C_ARMED, 0);
        fuji_load_stage_config(&load, _configrom, FUJI_CONFIGROM_SIZE);
        upload_view(load.live ^ 1u);
        fuji_load_arm(&load);
        lctrl(C_ARMED, load.armed ? 1 : 0);
        publish_staged();
        return;
    }
    {
        s2_copy_t before = copyreg;

        if (s2map_copy_reg(&copyreg, &load.view[load.live], reg, value)) {
            (void)before;
            push_range(copyreg.dst, copyreg.len);
            poke(FN_R_COPYDONE, copyreg.done);
        }
    }
}

static void c_on_txn(const fujimail_txn_t *t)
{
    VLOG("txn dev=%02X cmd=%02X txlen=%u seq=%u -> err=%d reply=%02X rxlen=%u",
         t->device, t->command, t->txlen, t->seq, t->status, t->reply_cmd, t->rxlen);
}

static const fujimail_port_t port = {
    poke, c_link_up, fujitcp_transact, fujitcp_send_bare,
    c_stream_open, c_stream_write, c_stream_close, c_arm_swap,
    NULL, NULL, c_on_txn, NULL,
    c_own_reg,
};

/* ---------------------------------------------------------------- link in */

static uint8_t rx_frame[16];
static unsigned rx_n;
static bool rx_esc;

static void on_event(const uint8_t *e)
{
    switch (e[0]) {
    case 'H':
        evq_push((uint16_t)((e[1] << 8) | e[2]));
        break;
    case 'P':
        (void)s2map_poke(&load.view[load.live], (uint16_t)((e[1] << 8) | e[2]), e[3]);
        break;
    case 'B':
        s2map_bank(&load.view[load.live], e[1]);
        break;
    case 'S':
        if (load.armed && load.staged && e[1] != load.live) {
            (void)fuji_load_swap(&load);
        } else {
            load.live = e[1] & 1u;
            load.armed = false;
            load.staged = false;
            load.swaps++;
            s2map_bank(&load.view[load.live], 0);
            LOG("swap forced to view %u", load.live);
        }
        evq_push(S2_RING_SWAP);
        break;
    case 'F':
        s2_disp_flip(&text.disp);
        break;
    case 'O':
        fpga_seen = true;
        fpga_ver = e[1];
        fpga_status = e[2];
        if (e[3])
            LOG("FPGA dropped %u events", e[3]);
        VLOG("FPGA v%u status %02X", fpga_ver, fpga_status);
        break;
    default:
        LOG("unknown event %02X", e[0]);
        break;
    }
}

/* Take what the link has; wait up to `ms` for the first byte. */
static void link_pump(int ms)
{
    uint8_t b[4096];
    struct pollfd p = { link_fd, POLLIN, 0 };

    if (poll(&p, 1, ms) <= 0)
        return;
    for (;;) {
        ssize_t n = read(link_fd, b, sizeof b), i;

        if (n <= 0)
            break;
        for (i = 0; i < n; i++) {
            uint8_t c = b[i];

            if (c == SL_END) {
                if (rx_n == 4)
                    on_event(rx_frame);
                else if (rx_n)
                    LOG("bad frame (%u bytes)", rx_n);
                rx_n = 0;
                rx_esc = false;
                continue;
            }
            if (c == SL_ESC) {
                rx_esc = true;
                continue;
            }
            if (rx_esc) {
                c = c == SL_EEND ? SL_END : c == SL_EESC ? SL_ESC : c;
                rx_esc = false;
            }
            if (rx_n < sizeof rx_frame)
                rx_frame[rx_n++] = c;
        }
        if (n < (ssize_t)sizeof b)
            break;
    }
}

/* While a transaction waits: as studio2_fujinet_device::idle, draw what the
 * console sent since, up to the first mailbox read. */
static void idle(void)
{
    link_pump(0);
    while (evq_tail != evq_head) {
        uint16_t a = evq[evq_tail];

        if (a < FN_TEXT_BASE || a == S2_RING_SWAP)
            break;
        evq_tail = (evq_tail + 1) % EVQ_LEN;
        s2_text_event(&text, a);
    }
    compose();
    link_flush();
}

/* ---------------------------------------------------------------- setup */

static int open_link(const char *dev, unsigned baud)
{
    struct termios t;
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0) {
        LOG("cannot open %s: %s", dev, strerror(errno));
        return -1;
    }
    if (tcgetattr(fd, &t) == 0) {
        cfmakeraw(&t);
        t.c_cflag |= CLOCAL | CREAD;
        t.c_cflag &= ~CRTSCTS;
        cfsetispeed(&t, B115200);
        cfsetospeed(&t, B115200);
        tcsetattr(fd, TCSANOW, &t);
    }
#ifdef __linux__
    if (baud) {
        struct termios2 t2;

        if (ioctl(fd, TCGETS2, &t2) == 0) {
            t2.c_cflag &= ~CBAUD;
            t2.c_cflag |= BOTHER;
            t2.c_ispeed = baud;
            t2.c_ospeed = baud;
            if (ioctl(fd, TCSETS2, &t2) != 0)
                LOG("cannot set %u baud: %s", baud, strerror(errno));
        }
    }
#else
    (void)baud;
#endif
    tcflush(fd, TCIOFLUSH);
    return fd;
}

static uint32_t now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

/* Power-on, as fuji_cart.c serve_config: CONFIG in view 0, the mailbox live. */
static void cart_init(void)
{
    unsigned v;

    lctrl(C_CTRL, 1);                   /* hold CLEAR while the cart is built */
    lctrl(C_BUSRESET, 0);
    memset(arena, 0, FN_H_REGSEL);
    memset(arena + FN_H_REGSEL, 0xFF, FN_ARENA_SIZE - FN_H_REGSEL);
    arena[FN_H_REGSEL + FN_HOT_STUB] = 0xEC;
    arena[FN_H_REGSEL + FN_HOT_STUB + 1] = 0x70;
    arena[FN_H_REGSEL + FN_HOT_T] = 0x23;
    s2_text_init(&text, arena + FN_R_DATA);
    fuji_load_init(&load, image_buf[0], image_buf[1], arena, &text.disp,
                   _configrom, FUJI_CONFIGROM_SIZE);
    memset(&copyreg, 0, sizeof copyreg);
    evq_head = evq_tail = 0;

    for (v = 0; v < 2; v++) {
        unsigned pg;

        for (pg = 0; pg < S2MAP_PAGES; pg++)
            lwrite16(LA_PGTBL(v) + pg * 2, 0);
        lctrl(C_MBX0 + v, 0);
        lctrl(C_NBK0 + v, 0);
    }
    memset(sent_raster, 0, sizeof sent_raster);
    lwrite(LA_RASTER(0), sent_raster[0], FN_RASTER_SIZE);
    lwrite(LA_RASTER(1), sent_raster[1], FN_RASTER_SIZE);
    lwrite(LA_ARENA, arena, FN_ARENA_SIZE);
    upload_view(load.live);
    lctrl(C_LIVE, load.live);
    lctrl(C_ARMED, 0);
    lctrl(C_PENDING, 0);

    fujimail_init(&port);
    fujimail_paint();
    lctrl(C_CTRL, 2);                   /* release CLEAR: the BIOS starts */
    lctrl(C_PING, 0);
    link_flush();
    LOG("cart up: CONFIG %u bytes", (unsigned)FUJI_CONFIGROM_SIZE);
}

static volatile sig_atomic_t stop;
static void on_signal(int s) { (void)s; stop = 1; }

int main(int argc, char **argv)
{
    const char *dev = "/dev/ttyS1", *tcp = "127.0.0.1:9995";
    unsigned baud = 1562500;
    uint32_t last_ping = 0;
    int c, tries;

    while ((c = getopt(argc, argv, "d:b:t:v")) != -1) {
        switch (c) {
        case 'd': dev = optarg; break;
        case 'b': baud = (unsigned)strtoul(optarg, NULL, 0); break;
        case 't': tcp = optarg; break;
        case 'v': verbose = 1; break;
        default:
            fprintf(stderr, "usage: %s [-d tty] [-b baud] [-t host:port] [-v]\n", argv[0]);
            return 2;
        }
    }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN);

    link_fd = open_link(dev, baud);
    if (link_fd < 0)
        return 1;
    LOG("link %s at %u baud", dev, baud);

    for (tries = 0; fujitcp_init(tcp) != 0; tries++) {
        if (stop || tries == 60)
            break;
        sleep(1);
    }
    fujitcp_set_idle(idle);

    cart_init();

    while (!stop) {
        uint32_t t;

        link_pump(evq_tail == evq_head ? 4 : 0);
        while (evq_tail != evq_head) {
            uint16_t a = evq[evq_tail];

            evq_tail = (evq_tail + 1) % EVQ_LEN;
            if (a == S2_RING_SWAP)
                swapped();
            else if (a >= FN_TEXT_BASE)
                s2_text_event(&text, a);
            else
                fujimail_read_hotspot((uint16_t)(a - FN_ARENA_BASE));
            compose();
            link_flush();
            link_pump(0);
        }
        t = now_ms();
        s2_text_tick(&text, t);
        compose();
        if (t - last_ping > 2000) {
            lctrl(C_PING, 0);
            last_ping = t;
        }
        link_flush();
        if (!fujitcp_active()) {
            static uint32_t last_try;

            if (t - last_try > 2000) {
                last_try = t;
                if (fujitcp_init(tcp) == 0)
                    fujimail_paint();
            }
        }
    }
    lctrl(C_CTRL, 0);
    link_flush();
    fujitcp_close();
    return 0;
}
