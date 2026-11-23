/* Chronomètre, minuteur, nombres aléatoires. */
#include <stdio.h>
#include <string.h>

#include "ui.h"

static void fmt_ms(char *d, uint32_t ms) {
  sprintf(d, "%02lu:%02lu.%02lu", (unsigned long)(ms / 60000), (unsigned long)(ms / 1000 % 60),
          (unsigned long)(ms / 10 % 100));
}

/* ---------- Chronomètre ---------- */

void chrono_app(void) {
  uint64_t start = 0;
  uint32_t acc = 0, laps[99], shown = UINT32_MAX;
  int nl = 0;
  bool run = false, full = true;
  for (;;) {
    if (g_quit) return;
    uint32_t ms = acc + (run ? (uint32_t)(eadk_timing_millis() - start) : 0);
    char t[24];
    if (full) {
      ui_clear();
      ui_header("Chronomètre", "");
      for (int i = nl > 6 ? nl - 6 : 0, y = 104; i < nl; i++, y += 16) {
        char a[16], b[16], l[48];
        fmt_ms(a, laps[i]);
        fmt_ms(b, laps[i] - (i ? laps[i - 1] : 0));
        snprintf(l, sizeof l, "Tour %2d : %s   (+%s)", i + 1, a, b);
        ui_str(30, y, l, false, C_TEXT, C_BG);
      }
      ui_str(8, 206, "OK : départ / pause      EXE : tour", false, C_GRAY, C_BG);
      ui_str(8, 222, "Effacer (en pause) : remise à zéro", false, C_GRAY, C_BG);
      full = false;
      shown = UINT32_MAX;
    }
    if (ms / 10 != shown) {
      fmt_ms(t, ms);
      ui_big(36, t, run ? C_TEXT : C_GRAY);
      shown = ms / 10;
    }
    eadk_event_t e = ui_event(run ? 20 : -1);
    if (e == eadk_event_back) return;
    if (e == eadk_event_ok) {
      if (run) acc = ms;
      else start = eadk_timing_millis();
      run = !run;
      shown = UINT32_MAX;
    } else if (e == eadk_event_exe && run && nl < 99) {
      laps[nl++] = ms;
      full = true;
    } else if (e == eadk_event_backspace && !run) {
      acc = 0;
      nl = 0;
      full = true;
    }
  }
}

/* ---------- Minuteur ---------- */

static void alert(void) {
  uint8_t br = eadk_backlight_brightness();
  for (int i = 0; !g_quit; i++) {
    eadk_color_t bg = i % 2 ? C_BAD : eadk_color_white, fg = i % 2 ? eadk_color_white : C_BAD;
    eadk_display_push_rect_uniform(eadk_screen_rect, bg);
    ui_str(90, 96, "Temps écoulé !", true, fg, bg);
    ui_str(86, 130, "Appuie sur une touche", false, fg, bg);
    eadk_backlight_set_brightness(i % 2 ? 255 : br);
    if (ui_event(400) != 0xFFFF) break;
  }
  eadk_backlight_set_brightness(br);
}

void minuteur_app(void) {
  static char f[2][FIELDLEN] = {"5", "0"};
  static const char *const labels[] = {"Minutes", "Secondes"};
  while (ui_form("Minuteur", labels, f, 2)) {
    long m = 0, s = 0;
    bool okm = !f[0][0] || parse_long(f[0], &m), oks = !f[1][0] || parse_long(f[1], &s);
    if (!okm || !oks || m < 0 || s < 0 || m * 60 + s <= 0 || m > 999) {
      ui_message("Minuteur", "Entre une durée positive (minutes et secondes).");
      continue;
    }
    uint32_t total = (uint32_t)(m * 60 + s) * 1000, remaining = total, shown = UINT32_MAX;
    uint64_t end = eadk_timing_millis() + total;
    bool run = true;
    ui_clear();
    ui_header("Minuteur", "");
    ui_str(8, 206, "OK : pause / reprise", false, C_GRAY, C_BG);
    ui_str(8, 222, "Effacer : recommencer    Retour : arrêter", false, C_GRAY, C_BG);
    for (;;) {
      uint64_t now = eadk_timing_millis();
      uint32_t rem = run ? (end > now ? (uint32_t)(end - now) : 0) : remaining;
      if (rem == 0) { alert(); break; }
      uint32_t sec = (rem + 999) / 1000;
      if (sec != shown) {
        char t[16];
        sprintf(t, "%02lu:%02lu", (unsigned long)(sec / 60), (unsigned long)(sec % 60));
        ui_big(50, t, run ? C_TEXT : C_GRAY);
        int w = (int)((uint64_t)280 * rem / total);
        eadk_display_push_rect_uniform((eadk_rect_t){20, 130, w, 10}, C_HEAD);
        eadk_display_push_rect_uniform((eadk_rect_t){20 + w, 130, 280 - w, 10}, C_SEL);
        shown = sec;
      }
      eadk_event_t e = ui_event(run ? 50 : -1);
      if (e == eadk_event_back || g_quit) break;
      if (e == eadk_event_ok) {
        if (run) remaining = rem;
        else end = now + remaining;
        run = !run;
        shown = UINT32_MAX;
      } else if (e == eadk_event_backspace) {
        end = now + total;
        remaining = total;
        run = true;
        shown = UINT32_MAX;
      }
    }
  }
}

/* ---------- Aléatoire ---------- */

static void out_list(const long *v, int n) { /* "12, 5, 33" sur plusieurs lignes */
  char line[COLS + 1] = "";
  for (int i = 0; i < n; i++) {
    char t[16];
    snprintf(t, sizeof t, "%ld%s", v[i], i < n - 1 ? ", " : "");
    if (strlen(line) + strlen(t) > COLS) { outf("%s", line); line[0] = 0; }
    strcat(line, t);
  }
  if (line[0]) outf("%s", line);
}

static bool get_long(const char *s, long lo, long hi, long *v) { return parse_long(s, v) && *v >= lo && *v <= hi; }

static void ints(char f[3][FIELDLEN], bool unique) {
  static long v[200], sorted[200];
  long lo, hi, n;
  if (!get_long(f[0], -1000000000, 1000000000, &lo) || !get_long(f[1], -1000000000, 1000000000, &hi) || lo > hi ||
      !get_long(f[2], 1, 200, &n)) {
    out_clear();
    outf("Erreur : min <= max (entre -10^9 et 10^9), et de 1 à 200 nombres.");
    return;
  }
  uint32_t range = (uint32_t)(hi - lo + 1);
  if (unique && (uint32_t)n > range) {
    out_clear();
    outf("Impossible : seulement %lu nombres différents entre %ld et %ld.", (unsigned long)range, lo, hi);
    return;
  }
  for (int i = 0; i < n; i++) {
    bool dup;
    do { /* ponytail: rejet des doublons en O(n^2), n <= 200 */
      v[i] = lo + (long)rnd(range);
      dup = false;
      for (int j = 0; unique && j < i; j++) dup |= v[j] == v[i];
    } while (dup);
  }
  out_clear();
  outf("%ld nombre%s entre %ld et %ld%s :", n, n > 1 ? "s" : "", lo, hi, unique ? ", sans doublon" : "");
  out_list(v, n);
  if (n > 1) {
    memcpy(sorted, v, n * sizeof v[0]);
    for (int i = 1; i < n; i++) /* tri par insertion */
      for (int j = i; j > 0 && sorted[j - 1] > sorted[j]; j--) { long t = sorted[j]; sorted[j] = sorted[j - 1]; sorted[j - 1] = t; }
    outf("");
    outf("Triés :");
    out_list(sorted, n);
  }
}

static void dice(char f[2][FIELDLEN]) {
  long n, faces, v[50], sum = 0;
  out_clear();
  if (!get_long(f[0], 1, 50, &n) || !get_long(f[1], 2, 1000, &faces)) {
    outf("Erreur : de 1 à 50 dés, de 2 à 1000 faces.");
    return;
  }
  for (int i = 0; i < n; i++) sum += v[i] = 1 + rnd(faces);
  outf("%ld dé%s à %ld faces :", n, n > 1 ? "s" : "", faces);
  out_list(v, n);
  outf("");
  outf("Total : %ld", sum);
}

static void coins(char f[1][FIELDLEN]) {
  long n;
  out_clear();
  if (!get_long(f[0], 1, 500, &n)) { outf("Erreur : de 1 à 500 lancers."); return; }
  if (n == 1) { outf("Résultat : %s", rnd(2) ? "PILE" : "FACE"); return; }
  char line[COLS + 1] = "";
  int pile = 0;
  for (int i = 0; i < n; i++) {
    bool p = rnd(2);
    pile += p;
    if (strlen(line) + 2 > COLS) { outf("%s", line); line[0] = 0; }
    strcat(line, p ? "P " : "F ");
  }
  outf("%s", line);
  outf("");
  outf("Pile : %d  (%ld %%)", pile, pile * 100 / n);
  outf("Face : %ld  (%ld %%)", n - pile, (n - pile) * 100 / n);
}

static void shuffle(char f[1][FIELDLEN]) {
  static long v[200];
  long n;
  out_clear();
  if (!get_long(f[0], 2, 200, &n)) { outf("Erreur : n entre 2 et 200."); return; }
  for (int i = 0; i < n; i++) v[i] = i + 1;
  for (int i = n - 1; i > 0; i--) { int j = rnd(i + 1); long t = v[i]; v[i] = v[j]; v[j] = t; } /* Fisher-Yates */
  outf("Les nombres de 1 à %ld mélangés :", n);
  out_list(v, n);
}

void aleatoire_app(void) {
  static const char *const menu[] = {"Entiers au hasard", "Tirage sans doublon", "Lancer de dés", "Pile ou face",
                                     "Mélanger 1 à n"};
  static const char *const l_int[] = {"Minimum", "Maximum", "Combien"};
  static const char *const l_dice[] = {"Nombre de dés", "Faces"};
  static const char *const l_coin[] = {"Lancers"};
  static const char *const l_mix[] = {"n"};
  static char fi[3][FIELDLEN] = {"1", "100", "1"}, fd[2][FIELDLEN] = {"2", "6"}, fc[1][FIELDLEN] = {"10"},
              fm[1][FIELDLEN] = {"30"};
  for (;;) {
    int k = ui_menu("Aléatoire", menu, 5);
    if (k < 0) return;
    for (;;) {
      bool ok = k <= 1 ? ui_form(menu[k], l_int, fi, 3)
              : k == 2 ? ui_form(menu[k], l_dice, fd, 2)
              : k == 3 ? ui_form(menu[k], l_coin, fc, 1)
                       : ui_form(menu[k], l_mix, fm, 1);
      if (!ok) break;
      eadk_event_t e;
      do {
        if (k <= 1) ints(fi, k == 1);
        else if (k == 2) dice(fd);
        else if (k == 3) coins(fc);
        else shuffle(fm);
        e = ui_text(menu[k], "EXE : relancer");
      } while (e == eadk_event_exe);
    }
  }
}

/* ---------- Batterie ---------- */

/* ponytail: courbe tension -> % approximative, à ajuster sur la vraie calculatrice. */
static const float VOLT[][2] = {{4.20f, 100}, {4.10f, 90}, {4.00f, 80}, {3.93f, 70}, {3.87f, 60}, {3.82f, 50},
                                {3.79f, 40},  {3.77f, 30}, {3.74f, 20}, {3.70f, 10}, {3.60f, 5},  {3.30f, 0}};

int pct_from_voltage(float v) { /* courbe typique d'une batterie lithium, au repos */
  if (v >= VOLT[0][0]) return 100;
  for (unsigned i = 1; i < sizeof VOLT / sizeof VOLT[0]; i++)
    if (v >= VOLT[i][0])
      return (int)(VOLT[i][1] + (v - VOLT[i][0]) / (VOLT[i - 1][0] - VOLT[i][0]) * (VOLT[i - 1][1] - VOLT[i][1]) + 0.5f);
  return 0;
}

#if PLATFORM_DEVICE
/* nwlink 1.0.0 ne relie pas les fonctions batterie sur la calculatrice : appel système direct.
 * ponytail: numéros repris d'Epsilon (ion/src/device/shared/drivers/svcall.h, liste stable
 * jusqu'ici) ; s'ils changent dans une future version, l'écran batterie affichera n'importe quoi. */
__attribute__((noinline)) float eadk_battery_voltage(void) {
  float v;
  __asm__ volatile("svc 5 ; vmov %0, s0" : "=r"(v) : : "r0", "r1", "r2", "r3");
  return v;
}
__attribute__((noinline)) bool eadk_battery_is_charging(void) {
  bool c;
  __asm__ volatile("svc 3 ; mov %0, r0" : "=r"(c) : : "r0", "r1", "r2", "r3");
  return c;
}
#endif

void batterie_app(void) {
  uint64_t t0 = eadk_timing_millis();
  float avg = eadk_battery_voltage();
  int p0 = -1;
  ui_clear();
  ui_header("Batterie", "");
  ui_str(8, 222, "Mise à jour chaque seconde", false, C_GRAY, C_BG);
  for (;;) {
    avg = 0.9f * avg + 0.1f * eadk_battery_voltage(); /* lissage : la tension bouge un peu */
    int pct = pct_from_voltage(avg);
    bool charging = eadk_battery_is_charging();
    if (p0 < 0) p0 = pct;
    uint32_t min = (uint32_t)((eadk_timing_millis() - t0) / 60000);

    char t[64];
    sprintf(t, "%d", pct);
    eadk_display_push_rect_uniform((eadk_rect_t){0, 30, 320, 60}, C_BG);
    ui_big(32, t, pct <= 20 ? C_BAD : C_TEXT);
    int w = strlen(t) * 34 - 6;
    ui_str((320 + w) / 2 + 6, 62, "%", true, C_TEXT, C_BG);

    out_clear();
    char v[16];
    fmt_num(v, avg);
    outf("Estimation d'après la tension : %s V", v);
    outf("État : %s", charging ? "en charge" : "sur batterie");
    if (charging) outf("En charge : la tension (et le %%) sont faussés.");
    outf("Depuis l'ouverture : %lu min, %+d %%", (unsigned long)min, pct - p0);
    if (!charging && min >= 10 && p0 > pct) /* au rythme constaté depuis l'ouverture */
      outf("Temps restant mesuré : environ %lu h", (unsigned long)(pct * min / (p0 - pct) / 60));
    else if (!charging)
      outf("Temps restant : laisse l'écran ouvert 10 min");
    eadk_display_push_rect_uniform((eadk_rect_t){0, 96, 320, 120}, C_BG);
    for (int i = 0; i < nout; i++) ui_str(8, 100 + i * 17, out[i], false, C_TEXT, C_BG);

    eadk_event_t e = ui_event(1000);
    if (e == eadk_event_back || g_quit) return;
  }
}
