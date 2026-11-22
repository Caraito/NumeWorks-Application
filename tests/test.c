/* Tests des calculs, sur l'ordinateur : make test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

/* Bouchons de l'API calculatrice (rien à afficher pendant les tests). */
void eadk_display_push_rect_uniform(eadk_rect_t r, eadk_color_t c) {}
void eadk_display_draw_string(const char *t, eadk_point_t p, bool l, eadk_color_t a, eadk_color_t b) {}
eadk_event_t eadk_event_get(int32_t *t) { return eadk_event_back; }
uint32_t eadk_random(void) { return rand(); }
void eadk_backlight_set_brightness(uint8_t b) {}
uint8_t eadk_backlight_brightness(void) { return 100; }
uint32_t _eadk_timing_millis_low(void) { return 0; }
uint32_t _eadk_timing_millis_high(void) { return 0; }

static int fails;

static void expect(const char *what, const char *needle) {
  if (!out_has(needle)) {
    fails++;
    printf("ÉCHEC %s : « %s » absent. Sortie :\n", what, needle);
    for (int i = 0; i < nout; i++) printf("  | %s\n", out[i]);
  }
}

static void tri(const char *a, const char *b, const char *c, const char *A, const char *B, const char *C) {
  char f[6][FIELDLEN];
  const char *v[6] = {a, b, c, A, B, C};
  for (int i = 0; i < 6; i++) strcpy(f[i], v[i]);
  triangle_explain(f, false);
}

int main(void) {
  chimie_explain("CH4+O2", "CO2+H2O");
  expect("combustion", "CH4 + 2 O2 \xe2\x86\x92 CO2 + 2 H2O");
  chimie_explain("Ca(OH)2 + HCl", "CaCl2 + H2O");
  expect("parenthèses", "Ca(OH)2 + 2 HCl \xe2\x86\x92 CaCl2 + 2 H2O");
  chimie_explain("CuSO4.5H2O", "CuSO4+H2O");
  expect("hydrate", "CuSO4.5H2O \xe2\x86\x92 CuSO4 + 5 H2O");
  chimie_explain("MnO4^-+Fe^2++H^+", "Mn^2++Fe^3++H2O");
  expect("ions", "MnO4^- + 5 Fe^2+ + 8 H^+");
  expect("ions 2", "\xe2\x86\x92 Mn^2+ + 5 Fe^3+ + 4 H2O");
  expect("charges", "Charges : +17 = +17");
  chimie_explain("Fe^3+ + e^-", "Fe^2+");
  expect("électron", "Fe^3+ + e^- \xe2\x86\x92 Fe^2+");
  chimie_explain("K4[Fe(CN)6]+KMnO4+H2SO4", "KHSO4+Fe2(SO4)3+MnSO4+HNO3+CO2+H2O");
  expect("difficile", "10 K4[Fe(CN)6] + 122 KMnO4 + 299 H2SO4");
  expect("difficile 2", "162 KHSO4 + 5 Fe2(SO4)3 + 122 MnSO4 + 60");
  chimie_explain("h2o", "H2+O2");
  expect("minuscule", "majuscule");
  chimie_explain("Xy2", "X");
  expect("inconnu", "Élément inconnu : Xy");
  chimie_explain("H2", "O2");
  expect("impossible", "Impossible");
  chimie_explain("Ca(OH2", "H2O");
  expect("parenthèse", "Parenthèse non fermée");

  tri("3", "4", "", "", "", "90");
  expect("SAS c", "c = 5 ");
  expect("SAS A", "A = 36.8699");
  expect("SAS aire", "Aire = 6");
  expect("rectangle", "Triangle rectangle en C");
  tri("3", "4", "5", "", "", "");
  expect("SSS", "C = 90");
  tri("5", "8", "", "30", "", "");
  expect("SSA ambigu", "2 triangles possibles");
  expect("SSA B1", "B = 53.1301");
  expect("SSA B2", "B = 126.8699");
  tri("10", "", "", "60", "60", "");
  expect("ASA", "Triangle équilatéral");
  tri("1", "2", "5", "", "", "");
  expect("inégalité", "inégalité triangulaire");
  tri("1", "", "", "", "", "");
  expect("pas assez", "exactement 3 cases");

  bin_convert_explain("156", 10);
  expect("dec->bin", "Binaire     : 1001 1100");
  expect("dec->hex", "Hexadécimal : 9C");
  expect("dec->oct", "Octal       : 234");
  expect("divisions", "156 = 2 x 78 + 0");
  bin_convert_explain("9c", 16);
  expect("hex->dec", "Décimal     : 156");
  expect("hex puissances", "9x16^1 + 12x16^0");
  bin_convert_explain("102", 2);
  expect("bin invalide", "Erreur");

  bin_op_explain("10110", "1011", false);
  expect("addition", "100001");
  expect("addition vérif", "22 + 11 = 33");
  bin_op_explain("1011", "10110", true);
  expect("soustraction négative", "-1011");
  expect("soustraction vérif", "11 - 22 = -11");

  c2_encode_explain("-37", "8");
  expect("C2", "Résultat : 1101 1011");
  expect("C2 hexa", "Hexadécimal : DB");
  c2_encode_explain("200", "8");
  expect("C2 hors limites", "de -128 à 127");
  c2_decode_explain("11011011");
  expect("C2 décodage", "Valeur : -37");

  ieee_explain("0.1");
  expect("IEEE 0.1", "Hexadécimal : 3DCCCCCD");
  expect("IEEE arrondi", "Erreur d'arrondi");
  ieee_explain("-6.75");
  expect("IEEE -6.75", "Hexadécimal : C0D80000");
  expect("IEEE exact", "Représentation exacte");

  printf(fails ? "%d test(s) en échec\n" : "Tous les tests passent.\n", fails);
  return fails != 0;
}
