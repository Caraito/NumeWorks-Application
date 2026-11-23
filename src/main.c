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

int main(void) {
  static const char *const items[] = {"Chimie", "Physique", "Maths", "Binaire", "Chronomètre", "Minuteur",
                                      "Nombres aléatoires", "Batterie"};
  static void (*const apps[])(void) = {chimie_app, physique_app, maths_app, binaire_app, chrono_app, minuteur_app,
                                       aleatoire_app, batterie_app};
  for (;;) {
    int k = ui_menu("Outils", items, 8);
    if (k < 0) return 0;
    apps[k]();
  }
}
