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
float eadk_battery_voltage(void) { return 3.9f; }
bool eadk_battery_is_charging(void) { return false; }

static int fails;

static void expect(const char *what, const char *needle) {
  if (!out_has(needle)) {
    fails++;
    printf("ÉCHEC %s : « %s » absent. Sortie :\n", what, needle);
    for (int i = 0; i < nout; i++) printf("  | %s\n", out[i]);
  }
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

  masse_explain("C6H12O6");
  expect("masse molaire", "M = 180.156 g/mol");
  expect("masse détail", "C  : 6 x 12.011 = 72.066");
  masse_explain("Ca(OH)2");
  expect("masse parenthèses", "M = 74.092 g/mol");

  avancement_explain("CH4+O2", "CO2+H2O", "2, 3");
  expect("limitant", "Réactif limitant : O2");
  expect("xmax", "x max = 1.5 mol");
  expect("final réactif", "CH4 : 2 - 1 x max = 0.5");
  expect("final produit", "H2O : 0 + 2 x max = 3");
  avancement_explain("CH4+O2", "CO2+H2O", "2, 4");
  expect("stoechio", "stoechiométrique");
  avancement_explain("CH4+O2", "CO2+H2O", "2");
  expect("quantités manquantes", "au moins la quantité");

  {
    char q[4][FIELDLEN] = {"", "H2O", "9"};
    qte_explain(0, q);
    expect("n=m/M formule", "M calculée à partir de la formule H2O");
    expect("n=m/M M", "M = 18.015 g/mol");
    expect("n=m/M", "Résultat : n = 0.4996 mol");
    char d[4][FIELDLEN] = {"0.1", "", "0.01", "100"};
    qte_explain(3, d);
    expect("dilution", "Résultat : V1 = 10");
    char e[4][FIELDLEN] = {"1", "", ""};
    qte_explain(1, e);
    expect("deux vides", "exactement une case");
  }

  second_degre_explain("2", "-4", "-6");
  expect("delta", "\xce\x94 = b^2 - 4ac = (-4)^2 - 4 \xc3\x97 2 \xc3\x97 (-6) = 64");
  expect("x1", "x1 = -1");
  expect("factorisée", "f(x) = 2(x + 1)(x - 3)");
  expect("canonique", "f(x) = 2(x - 1)^2 - 8");
  expect("sommet", "Sommet S(1 ; -8) : minimum");
  expect("signe", "f(x) < 0 sur ]-1 ; 3[");
  second_degre_explain("1", "-2", "-1");
  expect("racine exacte", "1 - \xe2\x88\x9a" "2");
  second_degre_explain("1", "0", "-2");
  expect("racine exacte b=0", "x1 = -\xe2\x88\x9a" "2");
  second_degre_explain("2", "1", "-2");
  expect("racine exacte fraction", "(-1 - \xe2\x88\x9a" "17)/4");
  second_degre_explain("1", "2", "1");
  expect("double", "f(x) = (x + 1)^2");
  second_degre_explain("1", "0", "1");
  expect("delta négatif", "pas de racine réelle");
  second_degre_explain("0", "1", "1");
  expect("a nul", "a = 0");

  suite_explain(false, "3", "2", "10");
  expect("arith un", "u(10) = 3 + 10 \xc3\x97 2 = 23");
  expect("arith somme", "= 143");
  suite_explain(true, "2", "3", "5");
  expect("geo un", "= 486");
  expect("geo somme", "= 728");
  expect("geo variation", "croissante");

  if (pct_from_voltage(4.2f) != 100 || pct_from_voltage(3.82f) != 50 || pct_from_voltage(3.0f) != 0 ||
      pct_from_voltage(3.85f) != 56) {
    fails++;
    printf("ÉCHEC batterie : %d %d %d %d\n", pct_from_voltage(4.2f), pct_from_voltage(3.82f), pct_from_voltage(3.0f),
           pct_from_voltage(3.85f));
  }

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
