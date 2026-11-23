/* Formules « à trous » : on remplit toutes les cases sauf une, l'inconnue est calculée.
 * Presque toutes les formules du lycée s'écrivent  produit des g(v_i)^e_i = K,
 * avec g(v) = v, sin v ou cos v (angles en degrés). On isole donc l'inconnue directement. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  const char *lab, *unit;
  signed char e; /* exposant dans le produit */
  char trig;     /* 0, 's' (sinus) ou 'c' (cosinus) */
} var_t;

typedef struct {
  const char *name;
  double K;
  int nv;
  char kind; /* 0 : produit ; 'L' : relation de conjugaison */
  var_t v[5];
  const char *def[5];
} formula_t;

static const formula_t FORMS[] = {
    /* 0 : quantité de matière (menu Chimie) */
    {"n = m / M", 1, 3, 0, {{"n (mol)", "mol", 1}, {"M (g/mol)", "g/mol", 1}, {"m (g)", "g", -1}}, {"", "H2O", "9"}},
    {"C = n / V", 1, 3, 0, {{"C (mol/L)", "mol/L", 1}, {"V (L)", "L", 1}, {"n (mol)", "mol", -1}}, {"", "0.5", "0.1"}},
    {"Cm = m / V", 1, 3, 0, {{"Cm (g/L)", "g/L", 1}, {"V (L)", "L", 1}, {"m (g)", "g", -1}}, {"", "0.25", "5"}},
    {"Dilution C1·V1 = C2·V2", 1, 4, 0, {{"C1", "", 1}, {"V1", "", 1}, {"C2", "", -1}, {"V2", "", -1}},
     {"0.1", "", "0.01", "100"}},
    /* 4 : mécanique */
    {"Ec = 1/2·m·v^2", 0.5, 3, 0, {{"Ec (J)", "J", 1}, {"m (kg)", "kg", -1}, {"v (m/s)", "m/s", -2}}, {0}},
    {"Epp = m·g·h", 1, 4, 0, {{"Epp (J)", "J", 1}, {"m (kg)", "kg", -1}, {"g (N/kg)", "N/kg", -1}, {"h (m)", "m", -1}},
     {"", "", "9.81", ""}},
    {"P = m·g (poids)", 1, 3, 0, {{"P (N)", "N", 1}, {"m (kg)", "kg", -1}, {"g (N/kg)", "N/kg", -1}}, {"", "", "9.81"}},
    {"v = d / t", 1, 3, 0, {{"v (m/s)", "m/s", 1}, {"d (m)", "m", -1}, {"t (s)", "s", 1}}, {0}},
    {"W = F·d·cos \xce\xb1", 1, 4, 0,
     {{"W (J)", "J", 1}, {"F (N)", "N", -1}, {"d (m)", "m", -1}, {"\xce\xb1 (\xc2\xb0)", "\xc2\xb0", -1, 'c'}},
     {"", "", "", "0"}},
    {"F = G·m1·m2 / d^2", 6.674e-11, 4, 0, {{"F (N)", "N", 1}, {"m1 (kg)", "kg", -1}, {"m2 (kg)", "kg", -1}, {"d (m)", "m", 2}},
     {0}},
    /* 10 : électricité */
    {"U = R·I", 1, 3, 0, {{"U (V)", "V", 1}, {"R (\xce\xa9)", "\xce\xa9", -1}, {"I (A)", "A", -1}}, {0}},
    {"P = U·I", 1, 3, 0, {{"P (W)", "W", 1}, {"U (V)", "V", -1}, {"I (A)", "A", -1}}, {0}},
    {"E = P·t", 1, 3, 0, {{"E (J)", "J", 1}, {"P (W)", "W", -1}, {"t (s)", "s", -1}}, {0}},
    {"Q = I·t", 1, 3, 0, {{"Q (C)", "C", 1}, {"I (A)", "A", -1}, {"t (s)", "s", -1}}, {0}},
    /* 14 : ondes et lumière */
    {"v = \xce\xbb·f", 1, 3, 0, {{"v (m/s)", "m/s", 1}, {"\xce\xbb (m)", "m", -1}, {"f (Hz)", "Hz", -1}}, {0}},
    {"E = h·f (photon)", 6.62607015e-34, 2, 0, {{"E (J)", "J", 1}, {"f (Hz)", "Hz", -1}}, {0}},
    {"E = hc/\xce\xbb (eV, nm)", 1239.84, 2, 0, {{"E (eV)", "eV", 1}, {"\xce\xbb (nm)", "nm", 1}}, {0}},
    {"n1·sin i1 = n2·sin i2", 1, 4, 0,
     {{"n1", "", 1}, {"i1 (\xc2\xb0)", "\xc2\xb0", 1, 's'}, {"n2", "", -1}, {"i2 (\xc2\xb0)", "\xc2\xb0", -1, 's'}},
     {"1", "", "1.5", ""}},
    {"1/OA' - 1/OA = 1/f'", 0, 3, 'L', {{"OA' (cm)", "cm"}, {"OA (cm)", "cm"}, {"f' (cm)", "cm"}}, {0}},
    {"\xce\xb3 = OA' / OA", 1, 3, 0, {{"\xce\xb3", "", 1}, {"OA'", "", -1}, {"OA", "", 1}}, {0}},
    /* 20 : matière et fluides */
    {"\xcf\x81 = m / V", 1, 3, 0, {{"\xcf\x81 (kg/m^3)", "kg/m^3", 1}, {"m (kg)", "kg", -1}, {"V (m^3)", "m^3", 1}}, {0}},
    {"p = F / S", 1, 3, 0, {{"p (Pa)", "Pa", 1}, {"F (N)", "N", -1}, {"S (m^2)", "m^2", 1}}, {0}},
    {"Mariotte P1·V1 = P2·V2", 1, 4, 0, {{"P1", "", 1}, {"V1", "", 1}, {"P2", "", -1}, {"V2", "", -1}}, {0}},
    {"A = \xce\xb5·l·c (Beer-Lambert)", 1, 4, 0,
     {{"A", "", 1}, {"\xce\xb5 (L/mol/cm)", "L/mol/cm", -1}, {"l (cm)", "cm", -1}, {"c (mol/L)", "mol/L", -1}}, {0}},
};
#define NFORMS (int)(sizeof FORMS / sizeof FORMS[0])

static const struct {
  const char *title;
  int first, n;
} GROUPS[] = {{"Quantité de matière", 0, 4}, {"Mécanique", 4, 6}, {"Électricité", 10, 4},
              {"Ondes et lumière", 14, 6},   {"Matière et fluides", 20, 4}};

static void name_of(char *d, const char *lab) { /* "n (mol)" -> "n" */
  int i = 0;
  while (lab[i] && lab[i] != ' ') { d[i] = lab[i]; i++; }
  d[i] = 0;
}

static bool err(const char *msg) {
  out_clear();
  outf("%s", msg);
  return false;
}

static double trig(char t, double v) { return t == 's' ? sin(v * M_PI / 180) : t == 'c' ? cos(v * M_PI / 180) : v; }

bool formule_explain(int idx, char f[5][FIELDLEN]) {
  const formula_t *F = &FORMS[idx];
  double v[5] = {0};
  int unk = -1, nempty = 0;
  char note[80] = "";
  out_clear();
  for (int i = 0; i < F->nv; i++) {
    bool empty = true;
    for (const char *c = f[i]; *c; c++) empty &= *c == ' ';
    if (empty) { unk = i; nempty++; continue; }
    if (parse_double(f[i], &v[i])) continue;
    double M = !strcmp(F->v[i].unit, "g/mol") ? masse_molaire(f[i], false) : -1; /* M peut être une formule */
    if (M <= 0) {
      char m[96];
      snprintf(m, sizeof m, "Erreur : « %s » n'est pas un nombre.", f[i]);
      return err(m);
    }
    snprintf(note, sizeof note, "M calculée à partir de la formule %s :", f[i]);
    v[i] = M;
  }
  if (nempty != 1) {
    outf("Laisse vide exactement une case : celle à calculer.");
    if (idx == 0) outf("Astuce : dans M, tu peux taper une formule (ex : H2O).");
    return false;
  }

  double r;
  const var_t *u = &F->v[unk];
  if (F->kind == 'L') { /* 0 : OA', 1 : OA, 2 : f' */
    for (int i = 0; i < 3; i++)
      if (i != unk && v[i] == 0) return err("Erreur : une distance nulle est impossible ici.");
    r = unk == 0 ? 1 / (1 / v[2] + 1 / v[1]) : unk == 1 ? 1 / (1 / v[0] - 1 / v[2]) : 1 / (1 / v[0] - 1 / v[1]);
    if (!isfinite(r)) return err("Pas de solution : l'image est à l'infini (objet au foyer).");
  } else {
    double prod = 1;
    for (int i = 0; i < F->nv; i++)
      if (i != unk) prod *= pow(trig(F->v[i].trig, v[i]), F->v[i].e);
    double t = F->K / prod;
    if (!isfinite(t) || t == 0) return err("Erreur : une valeur nulle empêche le calcul.");
    if ((u->e == 2 || u->e == -2) && t < 0) return err("Pas de solution (un carré ne peut pas être négatif).");
    double g = u->e == 1 ? t : u->e == -1 ? 1 / t : u->e == 2 ? sqrt(t) : 1 / sqrt(t);
    if (u->trig) {
      if (fabs(g) > 1)
        return err(u->trig == 's' ? "Impossible : il faudrait sin > 1.\n(Snell-Descartes : réflexion totale, pas de rayon réfracté.)"
                                  : "Impossible : il faudrait cos > 1.");
      r = (u->trig == 's' ? asin(g) : acos(g)) * 180 / M_PI;
    } else {
      r = g;
    }
  }

  char name[16], a[24];
  outf("Formule : %s", F->name);
  outf("");
  outf("Données :");
  if (note[0]) outf("  %s", note);
  for (int i = 0; i < F->nv; i++) {
    if (i == unk) continue;
    name_of(name, F->v[i].lab);
    fmt_num(a, v[i]);
    outf("  %s = %s %s", name, a, F->v[i].unit);
  }
  outf("");
  name_of(name, u->lab);
  fmt_num(a, r);
  outf("Résultat : %s = %s %s", name, a, u->unit);
  return true;
}

void formules_menu(int group) {
  static char bufs[NFORMS][5][FIELDLEN];
  static bool init;
  if (!init) {
    for (int k = 0; k < NFORMS; k++)
      for (int i = 0; i < FORMS[k].nv; i++)
        if (FORMS[k].def[i]) strcpy(bufs[k][i], FORMS[k].def[i]);
    init = true;
  }
  const char *items[8];
  int first = GROUPS[group].first, n = GROUPS[group].n;
  for (int i = 0; i < n; i++) items[i] = FORMS[first + i].name;
  for (;;) {
    int k = ui_menu(GROUPS[group].title, (const char *const *)items, n);
    if (k < 0) return;
    const formula_t *F = &FORMS[first + k];
    const char *labels[5];
    for (int i = 0; i < F->nv; i++) labels[i] = F->v[i].lab;
    while (ui_form(F->name, (const char *const *)labels, bufs[first + k], F->nv)) {
      formule_explain(first + k, bufs[first + k]);
      ui_text(F->name, "");
    }
  }
}
