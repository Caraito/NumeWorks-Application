#include "ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char out[MAXLINES][LINELEN];
int nout;
bool g_quit;

/* ---------- Tampon de texte ---------- */

void out_clear(void) { nout = 0; }

static void add_line(const char *s, int len) {
  if (nout >= MAXLINES) return;
  if (len > LINELEN - 1) {
    len = LINELEN - 1;
    while (len > 0 && (s[len] & 0xC0) == 0x80) len--; /* ne pas couper un caractère UTF-8 */
  }
  memcpy(out[nout], s, len);
  out[nout][len] = 0;
  nout++;
}

static void add_wrapped(const char *s) {
  for (;;) {
    int cols = 0, i = 0, lastsp = -1;
    while (s[i]) {
      if ((s[i] & 0xC0) != 0x80) {
        if (cols == COLS) break;
        cols++;
      }
      if (s[i] == ' ') lastsp = i;
      i++;
    }
    if (!s[i]) { add_line(s, i); return; }
    int brk = lastsp > 0 ? lastsp : i;
    add_line(s, brk);
    s += brk;
    while (*s == ' ') s++;
    if (!*s) return;
  }
}

void outf(const char *fmt, ...) {
  char buf[600];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  char *p = buf;
  for (;;) {
    char *nl = strchr(p, '\n');
    if (nl) *nl = 0;
    add_wrapped(p);
    if (!nl) break;
    p = nl + 1;
  }
}

bool out_has(const char *needle) {
  for (int i = 0; i < nout; i++)
    if (strstr(out[i], needle)) return true;
  return false;
}

/* ---------- Dessin ---------- */

/* La police NumWorks n'a pas les lettres accentuées précomposées : elle dessine
 * « e » + accent combinant. On convertit donc é -> e + U+0301, etc. */
static const char nfd_base[33] = "aaaaaa\0ceeeeiiii\0nooooo\0\0uuuuy\0y";
static const unsigned char nfd_mark[32] = {
    0x80, 0x81, 0x82, 0x83, 0x88, 0x8A, 0,    0xA7, 0x80, 0x81, 0x82,
    0x88, 0x80, 0x81, 0x82, 0x88, 0,    0x83, 0x80, 0x81, 0x82, 0x83,
    0x88, 0,    0,    0x80, 0x81, 0x82, 0x88, 0x81, 0,    0x88};

static const char *nfd(const char *s) {
  static char d[3 * LINELEN];
  int j = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p && j < (int)sizeof d - 4; p++) {
    if (p[0] == 0xC3 && p[1] >= 0x80 && p[1] != 0x9F) {
      int idx = (p[1] | 0x20) - 0xA0;
      if (nfd_base[idx]) {
        d[j++] = p[1] < 0xA0 ? nfd_base[idx] - 32 : nfd_base[idx];
        d[j++] = (char)0xCC;
        d[j++] = (char)nfd_mark[idx];
        p++;
        continue;
      }
    }
    d[j++] = *p;
  }
  d[j] = 0;
  return d;
}

void ui_str(int x, int y, const char *s, bool large, eadk_color_t fg, eadk_color_t bg) {
  eadk_display_draw_string(nfd(s), (eadk_point_t){x, y}, large, fg, bg);
}

void ui_clear(void) { eadk_display_push_rect_uniform(eadk_screen_rect, C_BG); }

void ui_header(const char *title, const char *hint) {
  eadk_display_push_rect_uniform((eadk_rect_t){0, 0, 320, 22}, C_HEAD);
  ui_str(6, 2, title, true, eadk_color_white, C_HEAD);
  int tw = 0, w = 0;
  for (const char *p = title; *p; p++) tw += (*p & 0xC0) != 0x80;
  for (const char *p = hint ? hint : ""; *p; p++) w += (*p & 0xC0) != 0x80;
  if (w && 6 + 10 * tw + 8 + 7 * w <= 314) ui_str(314 - 7 * w, 4, hint, false, eadk_color_white, C_HEAD);
}

eadk_event_t ui_event(int32_t timeout_ms) {
  if (g_quit) return eadk_event_back;
  /* Les identifiants >= 216 sont des événements spéciaux (aucune touche). */
  if (timeout_ms >= 0) {
    int32_t t = timeout_ms;
    eadk_event_t e = eadk_event_get(&t);
    return e >= 216 ? 0xFFFF : e;
  }
  for (;;) {
    int32_t t = 100000;
    eadk_event_t e = eadk_event_get(&t);
    if (e < 216) return e;
  }
}

/* Caractère produit par une touche, ou NULL. */
static const char *ev_text(eadk_event_t e) {
  static char b[2];
  b[1] = 0;
  if (e >= 126 && e <= 142) { b[0] = 'a' + e - 126; return b; }
  if (e >= 144 && e <= 148) { b[0] = 'r' + e - 144; return b; }
  if (e >= 150 && e <= 153) { b[0] = 'w' + e - 150; return b; }
  if (e >= 180 && e <= 196) { b[0] = 'A' + e - 180; return b; }
  if (e >= 198 && e <= 202) { b[0] = 'R' + e - 198; return b; }
  if (e >= 204 && e <= 207) { b[0] = 'W' + e - 204; return b; }
  switch (e) {
    case eadk_event_zero: return "0";
    case eadk_event_one: return "1";
    case eadk_event_two: return "2";
    case eadk_event_three: return "3";
    case eadk_event_four: return "4";
    case eadk_event_five: return "5";
    case eadk_event_six: return "6";
    case eadk_event_seven: return "7";
    case eadk_event_eight: return "8";
    case eadk_event_nine: return "9";
    case eadk_event_dot: return ".";
    case eadk_event_comma: return ",";
    case eadk_event_plus: return "+";
    case eadk_event_minus: return "-";
    case eadk_event_multiplication: return "*";
    case eadk_event_division: return "/";
    case eadk_event_power: return "^";
    case eadk_event_left_parenthesis: return "(";
    case eadk_event_right_parenthesis: return ")";
    case eadk_event_left_bracket: return "[";
    case eadk_event_right_bracket: return "]";
    case eadk_event_ee: return "e";
    case eadk_event_space: return " ";
    case eadk_event_equal: return "=";
    case eadk_event_greater: return ">";
    case eadk_event_lower: return "<";
    case eadk_event_underscore: return "_";
    default: return NULL;
  }
}

/* ---------- Écrans génériques ---------- */

int ui_menu(const char *title, const char *const items[], int n) {
  /* ponytail: position mémorisée par nombre d'éléments, pas par menu ; deux menus
   * de même taille partagent leur position (sans danger, juste cosmétique). */
  static int last_sel[8];
  int sel = last_sel[n & 7];
  for (;;) {
    if (g_quit) return -1;
    ui_clear();
    ui_header(title, "");
    for (int i = 0; i < n; i++) {
      int y = 28 + i * 26;
      eadk_color_t bg = i == sel ? C_SEL : C_BG;
      eadk_display_push_rect_uniform((eadk_rect_t){0, y, 320, 26}, bg);
      char line[64];
      snprintf(line, sizeof line, "%d  %s", i + 1, items[i]);
      ui_str(10, y + 4, line, true, C_TEXT, bg);
    }
    eadk_event_t e = ui_event(-1);
    if (e == eadk_event_up) sel = (sel + n - 1) % n;
    else if (e == eadk_event_down) sel = (sel + 1) % n;
    else if (e == eadk_event_back) return -1;
    else if (e == eadk_event_ok || e == eadk_event_exe) { last_sel[n & 7] = sel; return sel; }
    else {
      const char *t = ev_text(e);
      if (t && *t >= '1' && *t <= '0' + n) { last_sel[n & 7] = *t - '1'; return *t - '1'; }
    }
  }
}

static int utf8_len(const char *s) {
  int n = 0;
  for (; *s; s++) n += (*s & 0xC0) != 0x80;
  return n;
}

bool ui_form(const char *title, const char *const labels[], char bufs[][FIELDLEN], int n) {
  int sel = 0;
  for (;;) {
    if (g_quit) return false;
    ui_clear();
    ui_header(title, "EXE : valider");
    for (int i = 0; i < n; i++) {
      int y = 28 + i * 32;
      ui_str(8, y + 5, labels[i], false, C_TEXT, C_BG);
      eadk_color_t border = i == sel ? C_HEAD : C_GRAY;
      int bw = i == sel ? 2 : 1;
      eadk_display_push_rect_uniform((eadk_rect_t){118, y, 196, 26}, border);
      eadk_display_push_rect_uniform((eadk_rect_t){118 + bw, y + bw, 196 - 2 * bw, 26 - 2 * bw}, C_BG);
      const char *v = bufs[i];
      int len = utf8_len(v);
      while (len > 18) { v++; while ((*v & 0xC0) == 0x80) v++; len--; } /* affiche la fin */
      ui_str(123, y + 4, v, true, C_TEXT, C_BG);
      if (i == sel) eadk_display_push_rect_uniform((eadk_rect_t){123 + 10 * len, y + 4, 2, 18}, C_HEAD);
    }
    ui_str(8, 224, "haut/bas : changer de champ", false, C_GRAY, C_BG);
    eadk_event_t e = ui_event(-1);
    char *b = bufs[sel];
    int l = strlen(b);
    if (e == eadk_event_back) return false;
    else if (e == eadk_event_exe) return true;
    else if (e == eadk_event_ok) { if (sel == n - 1) return true; sel++; }
    else if (e == eadk_event_up) sel = (sel + n - 1) % n;
    else if (e == eadk_event_down) sel = (sel + 1) % n;
    else if (e == eadk_event_backspace) {
      while (l > 0 && (b[l - 1] & 0xC0) == 0x80) l--;
      if (l > 0) l--;
      b[l] = 0;
    } else if (e == eadk_event_clear) b[0] = 0;
    else {
      const char *t = ev_text(e);
      if (t && l + 1 < FIELDLEN) { b[l] = *t; b[l + 1] = 0; }
    }
  }
}

eadk_event_t ui_text(const char *title, const char *hint) {
  const int vis = 15;
  int top = 0, maxtop = nout > vis ? nout - vis : 0;
  for (;;) {
    if (g_quit) return eadk_event_back;
    ui_clear();
    ui_header(title, hint);
    for (int i = 0; i < vis && top + i < nout; i++)
      ui_str(4, 25 + i * 14, out[top + i], false, C_TEXT, C_BG);
    if (maxtop > 0) { /* barre de défilement */
      int h = 215 * vis / nout, y = 24 + (215 - h) * top / maxtop;
      eadk_display_push_rect_uniform((eadk_rect_t){316, 24, 4, 216}, C_SEL);
      eadk_display_push_rect_uniform((eadk_rect_t){316, y, 4, h}, C_HEAD);
    }
    eadk_event_t e = ui_event(-1);
    if (e == eadk_event_up && top > 0) top--;
    else if (e == eadk_event_down && top < maxtop) top++;
    else if (e == eadk_event_left) top = top > vis ? top - vis : 0;
    else if (e == eadk_event_right) top = top + vis < maxtop ? top + vis : maxtop;
    else if (e == eadk_event_back || e == eadk_event_ok || e == eadk_event_exe) return e;
  }
}

void ui_message(const char *title, const char *msg) {
  out_clear();
  outf("%s", msg);
  ui_text(title, "");
}

/* ---------- Gros chiffres (7 segments) ---------- */

#define SEG_W 28
#define SEG_H 52
#define SEG_T 6

static void seg_digit(int x, int y, int d, eadk_color_t fg) {
  static const uint8_t mask[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  const int w = SEG_W, h2 = SEG_H / 2, t = SEG_T, v = h2 - t - t / 2;
  const eadk_rect_t r[7] = {
      {x + t, y, w - 2 * t, t},                 /* a */
      {x + w - t, y + t, t, v},                 /* b */
      {x + w - t, y + h2 + t / 2, t, v},        /* c */
      {x + t, y + SEG_H - t, w - 2 * t, t},     /* d */
      {x, y + h2 + t / 2, t, v},                /* e */
      {x, y + t, t, v},                         /* f */
      {x + t, y + h2 - t / 2, w - 2 * t, t},    /* g */
  };
  for (int i = 0; i < 7; i++)
    eadk_display_push_rect_uniform(r[i], (mask[d] >> i) & 1 ? fg : RGB(238, 238, 238));
}

void ui_big(int y, const char *s, eadk_color_t fg) {
  int w = 0;
  for (const char *p = s; *p; p++) w += (*p >= '0' && *p <= '9' ? SEG_W : 12) + 6;
  int x = (320 - w + 6) / 2;
  for (const char *p = s; *p; p++) {
    if (*p >= '0' && *p <= '9') { seg_digit(x, y, *p - '0', fg); x += SEG_W + 6; continue; }
    eadk_display_push_rect_uniform((eadk_rect_t){x, y, 12, SEG_H}, C_BG);
    if (*p == ':') {
      eadk_display_push_rect_uniform((eadk_rect_t){x + 3, y + SEG_H / 3 - 3, SEG_T, SEG_T}, fg);
      eadk_display_push_rect_uniform((eadk_rect_t){x + 3, y + 2 * SEG_H / 3 - 3, SEG_T, SEG_T}, fg);
    } else if (*p == '.') {
      eadk_display_push_rect_uniform((eadk_rect_t){x + 3, y + SEG_H - SEG_T, SEG_T, SEG_T}, fg);
    }
    x += 12 + 6;
  }
}

void ui_line(int x0, int y0, int x1, int y1, eadk_color_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    if (x0 >= 0 && y0 >= 0 && x0 < 319 && y0 < 239)
      eadk_display_push_rect_uniform((eadk_rect_t){x0, y0, 2, 2}, c);
    if (x0 == x1 && y0 == y1) return;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

/* ---------- Divers ---------- */

uint32_t rnd(uint32_t n) {
  if (n == 0) return 0;
  uint32_t lim = UINT32_MAX - UINT32_MAX % n, r;
  do r = eadk_random(); while (r >= lim);
  return r % n;
}

static bool clean_num(const char *s, char *d) {
  int j = 0;
  for (; *s && j < FIELDLEN - 1; s++) {
    if (*s == ' ') continue;
    d[j++] = *s == ',' ? '.' : *s;
  }
  d[j] = 0;
  return j > 0;
}

bool parse_double(const char *s, double *v) {
  char d[FIELDLEN], *end;
  if (!clean_num(s, d)) return false;
  *v = strtod(d, &end);
  return *end == 0;
}

bool parse_long(const char *s, long *v) {
  char d[FIELDLEN], *end;
  if (!clean_num(s, d)) return false;
  *v = strtol(d, &end, 10);
  return *end == 0;
}

void fmt_num(char *dst, double v) {
  if (v != 0 && (v > 1e9 || v < -1e9 || (v < 1e-4 && v > -1e-4))) {
    sprintf(dst, "%.5g", v);
    return;
  }
  sprintf(dst, "%.4f", v);
  char *p = dst + strlen(dst) - 1;
  while (*p == '0') *p-- = 0;
  if (*p == '.') *p = 0;
  if (!strcmp(dst, "-0")) strcpy(dst, "0");
}
