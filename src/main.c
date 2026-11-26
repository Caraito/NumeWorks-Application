#include <stdio.h>
#include <string.h>

#include "ui.h"

const char eadk_app_name[]
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_app_name")))
#endif
    = "Outils";

const uint32_t eadk_api_level
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_api_level")))
#endif
    = 0;

#if PLATFORM_DEVICE
/* strtod (newlib) référence assert -> abort -> _exit/_kill, absents sur la calculatrice. */
__attribute__((used, externally_visible)) void __assert_func(const char *f, int l, const char *fn, const char *e) {
  for (;;) {}
}
#endif

/* Code demandé à chaque ouverture. Seule son empreinte (FNV-1a 64 bits) est dans le programme. */
#define CODE_HASH 0x535803367596e288ULL

static bool code_ok(const char *s) {
  uint64_t h = 0xcbf29ce484222325ULL;
  for (; *s; s++) h = (h ^ (uint8_t)*s) * 0x100000001b3ULL;
  return h == CODE_HASH;
}

static int digit(eadk_event_t e) {
  static const eadk_event_t k[10] = {eadk_event_zero, eadk_event_one, eadk_event_two,   eadk_event_three, eadk_event_four,
                                     eadk_event_five, eadk_event_six, eadk_event_seven, eadk_event_eight, eadk_event_nine};
  for (int i = 0; i < 10; i++)
    if (e == k[i]) return i;
  return -1;
}

static void lock_draw(int n, const char *msg) {
  char stars[17];
  memset(stars, '*', n);
  stars[n] = 0;
  ui_clear();
  ui_header("Code d'accès", "EXE : valider");
  ui_str(60, 66, "Entre le code :", false, C_TEXT, C_BG);
  eadk_display_push_rect_uniform((eadk_rect_t){60, 86, 200, 30}, C_HEAD);
  eadk_display_push_rect_uniform((eadk_rect_t){62, 88, 196, 26}, C_BG);
  ui_str(160 - 5 * n, 92, stars, true, C_TEXT, C_BG);
  ui_str(60, 130, msg, false, C_BAD, C_BG);
}

/* false = l'utilisateur quitte. Un code faux bloque la saisie 5 s (contre l'essai de tous les codes). */
static bool unlock(void) {
  char code[17] = "";
  int n = 0;
  for (;;) {
    lock_draw(n, "");
    eadk_event_t e = ui_event(-1);
    int d = digit(e);
    if (e == eadk_event_back) return false;
    if (d >= 0 && n < 16) { code[n++] = '0' + d; code[n] = 0; }
    else if (e == eadk_event_backspace && n) code[--n] = 0;
    else if ((e == eadk_event_exe || e == eadk_event_ok) && n) {
      if (code_ok(code)) return true;
      n = 0;
      code[0] = 0;
      uint64_t end = eadk_timing_millis() + 5000;
      for (uint64_t now; (now = eadk_timing_millis()) < end;) { /* les touches sont ignorées, sauf retour */
        char msg[48];
        snprintf(msg, sizeof msg, "Code faux. Réessaie dans %d s", (int)((end - now + 999) / 1000));
        lock_draw(0, msg);
        if (ui_event(end - now < 1000 ? end - now : 1000) == eadk_event_back) return false;
      }
    }
  }
}

int main(void) {
  if (!unlock()) return 0;
  static const char *const items[] = {"Maths", "Physique-chimie", "NSI", "Utilitaires"};
  static void (*const apps[])(void) = {maths_app, physchim_app, binaire_app, utilitaires_app};
  for (;;) {
    int k = ui_menu("Outils", items, 4);
    if (k < 0) return 0;
    apps[k]();
  }
}
