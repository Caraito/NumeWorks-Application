/* Chimie : équilibrage, tableau d'avancement, masse molaire, quantité de matière.
 * Syntaxe : CH4+O2 | Ca(OH)2 | [Fe(CN)6]^3- | CuSO4.5H2O | Fe^3+ | e^- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

#define MAXSP 12
#define MAXEL 16

static const char *ELEMENTS =
    " H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn Fe Co Ni Cu Zn Ga Ge As"
    " Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag Cd In Sn Sb Te I Xe Cs Ba La Ce Pr Nd Pm Sm Eu"
    " Gd Tb Dy Ho Er Tm Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr Ra Ac Th Pa U Np"
    " Pu Am Cm Bk Cf Es Fm Md No Lr Rf Db Sg Bh Hs Mt Ds Rg Cn Nh Fl Mc Lv Ts Og ";

static char el[MAXEL][3];
static int nel, nsp, nreac;
static int cnt[MAXSP][MAXEL], charge[MAXSP];
static char sp_txt[MAXSP][FIELDLEN];
static const char *p;
static char err[80];

static bool fail(const char *msg) {
  if (!err[0]) snprintf(err, sizeof err, "%s", msg);
  return false;
}

static int number(int def) {
  if (*p < '0' || *p > '9') return def;
  int n = 0;
  while (*p >= '0' && *p <= '9' && n < 10000) n = n * 10 + (*p++ - '0');
  return n;
}

static bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
static bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }

static int element(void) {
  char sym[4] = {' ', *p++, 0, 0};
  if (is_lower(*p)) sym[2] = *p++;
  char key[5];
  snprintf(key, sizeof key, "%s ", sym);
  if (!strstr(ELEMENTS, key)) {
    char m[40];
    snprintf(m, sizeof m, "Élément inconnu : %s", sym + 1);
    fail(m);
    return -1;
  }
  for (int i = 0; i < nel; i++)
    if (!strcmp(el[i], sym + 1)) return i;
  if (nel == MAXEL) { fail("Trop d'éléments différents"); return -1; }
  strcpy(el[nel], sym + 1);
  return nel++;
}

/* Lit une suite d'éléments / groupes et ajoute les atomes dans row. */
static bool formula(int row[MAXEL]) {
  bool any = false;
  for (;;) {
    if (is_upper(*p)) {
      int e = element();
      if (e < 0) return false;
      row[e] += number(1);
    } else if (*p == '(' || *p == '[') {
      char close = *p++ == '(' ? ')' : ']';
      int tmp[MAXEL] = {0};
      if (!formula(tmp)) return false;
      if (*p++ != close) return fail(close == ')' ? "Parenthèse non fermée" : "Crochet non fermé");
      int k = number(1);
      for (int i = 0; i < MAXEL; i++) row[i] += tmp[i] * k;
    } else if (is_lower(*p)) {
      return fail("Un symbole commence par une majuscule (ex : H2O, Fe)");
    } else {
      return any ? true : fail("Formule attendue");
    }
    any = true;
  }
}

static bool species(void) {
  if (nsp == MAXSP) return fail("Trop d'espèces");
  int s = nsp++;
  memset(cnt[s], 0, sizeof cnt[s]);
  charge[s] = 0;
  while (*p >= '0' && *p <= '9') p++; /* coefficient déjà écrit : on l'ignore */
  const char *ftxt = p;
  if (*p == 'e' && !is_lower(p[1])) { /* électron */
    p++;
    if (*p == '^') p++;
    if (*p == '-') p++;
    charge[s] = -1;
  } else {
    do { /* hydrates : CuSO4.5H2O */
      int k = number(1), tmp[MAXEL] = {0};
      if (!formula(tmp)) return false;
      for (int i = 0; i < MAXEL; i++) cnt[s][i] += tmp[i] * k;
    } while ((*p == '.' || *p == '*') && p++);
    if (*p == '^') {
      p++;
      int n = number(1);
      if (*p == '+') charge[s] = n;
      else if (*p == '-') charge[s] = -n;
      else return fail("Charge : écris par ex. Fe^3+ ou Cl^-");
      p++;
    }
  }
  int len = p - ftxt;
  if (len >= FIELDLEN) len = FIELDLEN - 1;
  memcpy(sp_txt[s], ftxt, len);
  sp_txt[s][len] = 0;
  return true;
}

static bool side(const char *txt) {
  static char buf[FIELDLEN];
  int j = 0;
  for (; *txt && j < FIELDLEN - 1; txt++)
    if (*txt != ' ') buf[j++] = *txt;
  buf[j] = 0;
  p = buf;
  if (!*p) return fail("Un des deux côtés est vide");
  if (!species()) return false;
  while (*p == '+') {
    p++;
    if (!species()) return false;
  }
  if (*p) {
    char m[40];
    snprintf(m, sizeof m, "Caractère inattendu : %c", *p);
    return fail(m);
  }
  return true;
}

static long long gcdll(long long a, long long b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { long long t = a % b; a = b; b = t; }
  return a;
}

/* Résout M·x = 0 ; renvoie 1 si une solution unique (à un facteur près) existe. */
static int balance(long long x[MAXSP]) {
  static long long a[MAXEL + 1][MAXSP];
  bool has_charge = false;
  for (int j = 0; j < nsp; j++) has_charge |= charge[j] != 0;
  int rows = nel + has_charge, cols = nsp;
  for (int r = 0; r < rows; r++)
    for (int j = 0; j < cols; j++) {
      int v = r < nel ? cnt[j][r] : charge[j];
      a[r][j] = j < nreac ? v : -v;
    }
  int piv[MAXEL + 1], rank = 0;
  for (int c = 0; c < cols && rank < rows; c++) {
    int r = rank;
    while (r < rows && a[r][c] == 0) r++;
    if (r == rows) continue;
    for (int k = 0; k < cols; k++) { long long t = a[r][k]; a[r][k] = a[rank][k]; a[rank][k] = t; }
    for (int r2 = 0; r2 < rows; r2++) {
      if (r2 == rank || a[r2][c] == 0) continue;
      long long f1 = a[rank][c], f2 = a[r2][c], g = 0;
      for (int k = 0; k < cols; k++) {
        a[r2][k] = a[r2][k] * f1 - a[rank][k] * f2;
        g = gcdll(g, a[r2][k]);
      }
      if (g > 1)
        for (int k = 0; k < cols; k++) a[r2][k] /= g;
    }
    piv[rank++] = c;
  }
  int nfree = cols - rank;
  if (nfree != 1) return nfree == 0 ? 0 : 2;
  int f = 0;
  for (int i = 0; i < rank; i++)
    if (piv[i] == f) { f++; i = -1; } /* première colonne sans pivot */
  long long L = 1;
  for (int i = 0; i < rank; i++) {
    long long d = a[i][piv[i]] < 0 ? -a[i][piv[i]] : a[i][piv[i]];
    L = L / gcdll(L, d) * d;
  }
  x[f] = L;
  for (int i = 0; i < rank; i++) x[piv[i]] = -a[i][f] * L / a[i][piv[i]];
  long long g = 0;
  for (int j = 0; j < cols; j++) g = gcdll(g, x[j]);
  bool neg = x[0] < 0;
  for (int j = 0; j < cols; j++) {
    x[j] /= g;
    if (neg) x[j] = -x[j];
    if (x[j] <= 0) return 0;
  }
  return 1;
}

/* Masses molaires atomiques (g/mol), dans l'ordre de ELEMENTS (Z = 1 à 118). */
static const double MASS[118] = {
    1.008, 4.0026, 6.94, 9.0122, 10.81, 12.011, 14.007, 15.999, 18.998, 20.180, 22.990, 24.305,
    26.982, 28.085, 30.974, 32.06, 35.45, 39.948, 39.098, 40.078, 44.956, 47.867, 50.942, 51.996,
    54.938, 55.845, 58.933, 58.693, 63.546, 65.38, 69.723, 72.630, 74.922, 78.971, 79.904, 83.798,
    85.468, 87.62, 88.906, 91.224, 92.906, 95.95, 98, 101.07, 102.91, 106.42, 107.87, 112.41,
    114.82, 118.71, 121.76, 127.60, 126.90, 131.29, 132.91, 137.33, 138.91, 140.12, 140.91, 144.24,
    145, 150.36, 151.96, 157.25, 158.93, 162.50, 164.93, 167.26, 168.93, 173.05, 174.97, 178.49,
    180.95, 183.84, 186.21, 190.23, 192.22, 195.08, 196.97, 200.59, 204.38, 207.2, 208.98, 209,
    210, 222, 223, 226, 227, 232.04, 231.04, 238.03, 237, 244, 243, 247, 247, 251, 252, 257,
    258, 259, 266, 267, 268, 269, 270, 269, 278, 281, 282, 285, 286, 289, 290, 293, 294, 294};

static double mass_of(const char *sym) {
  size_t L = strlen(sym);
  int z = 0;
  for (const char *q = ELEMENTS; (q = strchr(q, ' ')) && q[1];) {
    q++;
    if (!strncmp(q, sym, L) && q[L] == ' ') return MASS[z];
    z++;
  }
  return 0;
}

static long long coef[MAXSP];

static bool parse_balance(const char *reactifs, const char *produits) {
  nel = nsp = 0;
  err[0] = 0;
  if (!side(reactifs)) return false;
  nreac = nsp;
  if (!side(produits)) return false;
  int r = balance(coef);
  if (r == 0) return fail("Impossible d'équilibrer : vérifie les formules (un élément n'apparaît peut-être que d'un côté)");
  if (r == 2) return fail("Plusieurs équilibrages possibles : l'équation mélange plusieurs réactions");
  return true;
}

static void print_side(char *dst, int from, int to) {
  dst[0] = 0;
  for (int j = from; j < to; j++) {
    char t[FIELDLEN + 24];
    if (coef[j] == 1) snprintf(t, sizeof t, "%s%s", j > from ? " + " : "", sp_txt[j]);
    else snprintf(t, sizeof t, "%s%ld %s", j > from ? " + " : "", (long)coef[j], sp_txt[j]);
    strcat(dst, t);
  }
}

static void print_equation(void) {
  char l[8 * FIELDLEN], r[8 * FIELDLEN];
  print_side(l, 0, nreac);
  print_side(r, nreac, nsp);
  outf("Équation équilibrée :");
  if (strlen(l) + strlen(r) + 3 <= COLS) outf("%s \xe2\x86\x92 %s", l, r);
  else { outf("%s", l); outf("\xe2\x86\x92 %s", r); } /* trop long : un côté par ligne */
}

static void print_error(void) {
  outf("Erreur : %s", err);
  outf("");
  outf("Exemples d'écriture :");
  outf("  CH4+O2        Ca(OH)2      CuSO4.5H2O");
  outf("  Fe^3+         SO4^2-       e^-  (électron)");
  outf("Majuscule : shift, alpha, puis la lettre.");
  outf("Minuscule : alpha, puis la lettre.");
}

bool chimie_explain(const char *reactifs, const char *produits) {
  out_clear();
  if (!parse_balance(reactifs, produits)) { print_error(); return false; }
  print_equation();
  outf("");
  outf("Vérification (gauche = droite) :");
  for (int e = 0; e < nel; e++) {
    long long g = 0, d = 0;
    for (int j = 0; j < nsp; j++) *(j < nreac ? &g : &d) += coef[j] * cnt[j][e];
    outf("  %-2s : %ld = %ld", el[e], (long)g, (long)d);
  }
  bool has_charge = false;
  for (int j = 0; j < nsp; j++) has_charge |= charge[j] != 0;
  if (has_charge) {
    long long g = 0, d = 0;
    for (int j = 0; j < nsp; j++) *(j < nreac ? &g : &d) += coef[j] * charge[j];
    outf("  Charges : %+ld = %+ld", (long)g, (long)d);
  }
  return true;
}

/* ---------- Tableau d'avancement ---------- */

bool avancement_explain(const char *reactifs, const char *produits, const char *quantites) {
  out_clear();
  if (!parse_balance(reactifs, produits)) { print_error(); return false; }
  double n0[MAXSP] = {0};
  char buf[FIELDLEN];
  snprintf(buf, sizeof buf, "%s", quantites);
  int k = 0;
  for (char *tok = strtok(buf, ",; "); tok; tok = strtok(NULL, ",; ")) {
    if (k == nsp || !parse_double(tok, &n0[k]) || n0[k] < 0) {
      outf("Erreur : donne une quantité (mol, >= 0) par espèce, dans l'ordre, séparées par des virgules.");
      outf("Ex : 2, 3  (les produits absents valent 0)");
      return false;
    }
    k++;
  }
  if (k < nreac) {
    outf("Erreur : il faut au moins la quantité de chaque réactif (%d valeur%s).", nreac, nreac > 1 ? "s" : "");
    outf("Ex : 2, 3  (séparées par des virgules)");
    return false;
  }
  char a[24], b[24];
  print_equation();
  outf("");
  outf("État initial (mol) :");
  for (int j = 0; j < nsp; j++) { fmt_num(a, n0[j]); outf("  %s : %s", sp_txt[j], a); }
  outf("");
  outf("x max pour chaque réactif (n / coef) :");
  double xmax = 1e300, xs[MAXSP];
  for (int j = 0; j < nreac; j++) {
    xs[j] = n0[j] / coef[j];
    fmt_num(a, n0[j]);
    fmt_num(b, xs[j]);
    outf("  %s : %s / %ld = %s", sp_txt[j], a, (long)coef[j], b);
    if (xs[j] < xmax) xmax = xs[j];
  }
  char lim[8 * FIELDLEN] = "";
  int nlim = 0;
  for (int j = 0; j < nreac; j++)
    if (xs[j] - xmax <= 1e-9 * (xmax > 0 ? xmax : 1)) {
      if (nlim++) strcat(lim, ", ");
      strcat(lim, sp_txt[j]);
    }
  fmt_num(b, xmax);
  if (nlim == nreac && nreac > 1) outf("Mélange stoechiométrique : tous les réactifs sont épuisés.");
  else outf("Réactif limitant : %s", lim);
  outf("x max = %s mol", b);
  outf("");
  outf("État final (mol) :");
  for (int j = 0; j < nsp; j++) {
    double nf = n0[j] + (j < nreac ? -1 : 1) * coef[j] * xmax;
    if (nf < 1e-12) nf = 0;
    char c[24];
    fmt_num(a, n0[j]);
    fmt_num(c, nf);
    outf("  %s : %s %c %ld x max = %s", sp_txt[j], a, j < nreac ? '-' : '+', (long)coef[j], c);
  }
  return true;
}

/* ---------- Masse molaire ---------- */

double masse_molaire(const char *formule, bool detail) {
  nel = nsp = 0;
  err[0] = 0;
  if (!side(formule)) return -1;
  if (nsp != 1) { fail("Une seule formule à la fois"); return -1; }
  double M = 0;
  for (int e = 0; e < nel; e++) {
    double m = mass_of(el[e]), t = cnt[0][e] * m;
    M += t;
    if (detail) {
      char a[24], b[24];
      fmt_num(a, m);
      fmt_num(b, t);
      outf("  %-2s : %d x %s = %s", el[e], cnt[0][e], a, b);
    }
  }
  return M;
}

void masse_explain(const char *formule) {
  out_clear();
  outf("M(%s) = somme des masses des atomes :", formule);
  double M = masse_molaire(formule, true);
  if (M < 0) { out_clear(); print_error(); return; }
  outf("");
  char t[24];
  fmt_num(t, M);
  outf("M = %s g/mol", t);
}

/* ---------- Oxydoréduction (milieu acide) ---------- */

typedef struct {
  const char *ox, *red;
  float e0; /* potentiel standard (V), sert à choisir le sens de la réaction */
} couple_t;

static const couple_t COUPLES[] = {
    {"H2O2", "H2O", 1.78},  {"HClO", "Cl2", 1.63},    {"Ce^4+", "Ce^3+", 1.61},    {"MnO4^-", "Mn^2+", 1.51},
    {"Au^3+", "Au", 1.50},  {"Cl2", "Cl^-", 1.36},     {"Cr2O7^2-", "Cr^3+", 1.33}, {"O2", "H2O", 1.23},
    {"IO3^-", "I2", 1.19},  {"Br2", "Br^-", 1.07},     {"NO3^-", "NO", 0.96},    {"Hg^2+", "Hg", 0.85},
    {"Ag^+", "Ag", 0.80},   {"Fe^3+", "Fe^2+", 0.77},  {"O2", "H2O2", 0.70},     {"I2", "I^-", 0.54},
    {"Cu^2+", "Cu", 0.34},  {"CH3COOH", "CH3CH2OH", 0.19}, {"SO4^2-", "SO2", 0.17}, {"S4O6^2-", "S2O3^2-", 0.08},
    {"H^+", "H2", 0.00},    {"Pb^2+", "Pb", -0.13},    {"Sn^2+", "Sn", -0.14},   {"Ni^2+", "Ni", -0.25},
    {"Fe^2+", "Fe", -0.44}, {"CO2", "H2C2O4", -0.49},  {"Zn^2+", "Zn", -0.76},   {"Al^3+", "Al", -1.66},
    {"Mg^2+", "Mg", -2.37}, {"Na^+", "Na", -2.71},
};
#define NCOUPLES (int)(sizeof COUPLES / sizeof COUPLES[0])

/* Composition « canonique » d'une espèce (éléments triés + charge) : CH3COOH == C2H4O2. */
static bool canon(const char *f, char *d) {
  nel = nsp = 0;
  err[0] = 0;
  if (!side(f) || nsp != 1) return false;
  int ord[MAXEL];
  for (int i = 0; i < nel; i++) ord[i] = i;
  for (int i = 1; i < nel; i++)
    for (int j = i; j > 0 && strcmp(el[ord[j - 1]], el[ord[j]]) > 0; j--) { int t = ord[j]; ord[j] = ord[j - 1]; ord[j - 1] = t; }
  d[0] = 0;
  for (int i = 0; i < nel; i++)
    if (cnt[0][ord[i]]) sprintf(d + strlen(d), "%s%d", el[ord[i]], cnt[0][ord[i]]);
  sprintf(d + strlen(d), "^%d", charge[0]);
  return true;
}

static bool same(const char *a, const char *b) {
  char ka[96], kb[96];
  return canon(a, ka) && canon(b, kb) && !strcmp(ka, kb);
}

/* Équilibre gauche -> droite en ajoutant H+ / H2O si besoin (milieu acide). */
static bool balance_acid(const char *left, const char *right, const char *tail) {
  static const char *const extra[][2] = {{"", ""}, {"+H^+", "+H2O"}, {"+H2O", "+H^+"}, {"+H^+", ""},
                                         {"", "+H^+"}, {"+H2O", ""}, {"", "+H2O"}};
  for (unsigned i = 0; i < sizeof extra / sizeof extra[0]; i++) {
    char l[3 * FIELDLEN], r[3 * FIELDLEN];
    snprintf(l, sizeof l, "%s%s%s", left, extra[i][0], tail); /* tail : « +e^- » en dernier */
    snprintf(r, sizeof r, "%s%s", right, extra[i][1]);
    if (parse_balance(l, r)) return true;
  }
  return false;
}

/* Demi-équation ox + n e- = red ; renvoie n (0 si échec). */
static int half(const char *ox, const char *red, char *line, bool reversed) {
  if (!balance_acid(ox, red, "+e^-")) return 0;
  char a[8 * FIELDLEN], b[8 * FIELDLEN];
  print_side(a, 0, nreac);
  print_side(b, nreac, nsp);
  sprintf(line, "%s = %s", reversed ? b : a, reversed ? a : b);
  return (int)coef[nreac - 1]; /* e^- est la dernière espèce de gauche */
}

static void print_line(const char *line) { /* coupe avant le « = » si trop long */
  const char *eq = strstr(line, " = ");
  if ((int)strlen(line) <= COLS || !eq) { outf("  %s", line); return; }
  outf("  %.*s", (int)(eq - line), line);
  outf("  %s", eq + 1);
}

bool demi_explain(const char *a, const char *b) {
  out_clear();
  char line[16 * FIELDLEN];
  bool any_b = false;
  for (const char *c = b; *c; c++) any_b |= *c != ' ';
  if (any_b) {
    bool ab = half(a, b, line, false) > 0;
    if (!ab && !half(b, a, line, false)) { out_clear(); print_error(); return false; }
    outf("Couple %s / %s", ab ? a : b, ab ? b : a);
    outf("");
    print_line(line);
    return true;
  }
  int found = 0;
  for (int i = 0; i < NCOUPLES; i++) {
    bool is_ox = same(a, COUPLES[i].ox), is_red = !is_ox && same(a, COUPLES[i].red);
    if (!is_ox && !is_red) continue;
    if (!half(COUPLES[i].ox, COUPLES[i].red, line, false)) continue;
    if (found++) outf("");
    outf("Couple %s / %s", COUPLES[i].ox, COUPLES[i].red);
    outf("(%s y est %s)", a, is_ox ? "l'oxydant" : "le réducteur");
    print_line(line);
  }
  if (!found) {
    if (!canon(a, line)) { out_clear(); print_error(); return false; }
    outf("Je ne connais pas de couple pour %s.", a);
    outf("Tape aussi l'espèce conjuguée (ex : Fe^3+ et Fe^2+).");
    return false;
  }
  if (found > 1) { outf(""); outf("%s appartient à %d couples.", a, found); }
  return true;
}

bool bilan_explain(const char *r1, const char *r2) {
  out_clear();
  const char *rs[2] = {r1, r2};
  int bo = -1, br = -1, who = 0; /* couple de l'oxydant, couple du réducteur, réactif oxydant */
  for (int w = 0; w < 2; w++)
    for (int i = 0; i < NCOUPLES; i++) {
      if (!same(rs[w], COUPLES[i].ox)) continue;
      for (int j = 0; j < NCOUPLES; j++)
        if (j != i && same(rs[1 - w], COUPLES[j].red) &&
            (bo < 0 || COUPLES[i].e0 - COUPLES[j].e0 > COUPLES[bo].e0 - COUPLES[br].e0)) {
          bo = i; br = j; who = w;
        }
    }
  if (bo < 0) {
    char k[96];
    if (!canon(r1, k) || !canon(r2, k)) { out_clear(); print_error(); return false; }
    out_clear();
    outf("Je ne trouve pas de couple où l'un est l'oxydant et l'autre le réducteur.");
    outf("Voir « Couples connus ».");
    return false;
  }
  const couple_t *O = &COUPLES[bo], *R = &COUPLES[br];
  char l1[16 * FIELDLEN], l2[16 * FIELDLEN];
  int n1 = half(O->ox, O->red, l1, false), n2 = half(R->ox, R->red, l2, true);
  int L = n1 * n2, g = n1, h = n2;
  while (h) { int t = g % h; g = h; h = t; }
  L /= g;
  outf("Oxydant : %s (couple %s / %s)", rs[who], O->ox, O->red);
  outf("Réducteur : %s (couple %s / %s)", rs[1 - who], R->ox, R->red);
  outf("");
  outf("Demi-équations :");
  print_line(l1);
  if (L / n1 > 1) outf("    (x %d)", L / n1);
  print_line(l2);
  if (L / n2 > 1) outf("    (x %d)", L / n2);
  char left[2 * FIELDLEN], right[2 * FIELDLEN];
  snprintf(left, sizeof left, "%s+%s", O->ox, R->red);
  snprintf(right, sizeof right, "%s+%s", O->red, R->ox);
  outf("");
  if (!balance_acid(left, right, "")) { outf("Équation bilan introuvable."); return false; }
  print_equation();
  outf("(les %d électrons échangés s'éliminent)", L);
  if (O->e0 < R->e0) outf("Attention : d'après les potentiels, cette réaction n'est pas favorisée.");
  return true;
}

static void couples_list(void) {
  out_clear();
  outf("Couples oxydant / réducteur (E\xc2\xb0 en V) :");
  for (int i = 0; i < NCOUPLES; i++) {
    char c[64];
    snprintf(c, sizeof c, "%s / %s", COUPLES[i].ox, COUPLES[i].red);
    outf("  %-24s %+.2f", c, COUPLES[i].e0);
  }
}

void physchim_app(void) {
  static const char *const menu[] = {"Équilibrer une équation", "Oxydoréduction", "Tableau d'avancement",
                                     "Masse molaire", "Quantité de matière", "Formules de physique"};
  static const char *const redox[] = {"Demi-équation", "Équation bilan", "Couples connus"};
  static const char *const phys[] = {"Mécanique", "Électricité", "Ondes et lumière", "Matière et fluides"};
  static const char *const l_eq[] = {"Réactifs", "Produits", "n init (mol)"};
  static const char *const l_mm[] = {"Formule"};
  static const char *const l_demi[] = {"Espèce", "Conjuguée"};
  static const char *const l_bil[] = {"Réactif 1", "Réactif 2"};
  static char eq[3][FIELDLEN] = {"CH4+O2", "CO2+H2O", "2, 3"};
  static char mm[1][FIELDLEN] = {"C6H12O6"};
  static char dm[2][FIELDLEN] = {"MnO4^-", ""};
  static char bl[2][FIELDLEN] = {"MnO4^-", "Fe^2+"};
  for (;;) {
    int k = ui_menu("Physique-chimie", menu, 6), r;
    if (k < 0) return;
    switch (k) {
      case 0: while (ui_form("Équilibrer", l_eq, eq, 2)) { chimie_explain(eq[0], eq[1]); ui_text("Équilibrage", ""); } break;
      case 1:
        if ((r = ui_menu("Oxydoréduction", redox, 3)) < 0) break;
        if (r == 0) while (ui_form("Demi-équation", l_demi, dm, 2)) { demi_explain(dm[0], dm[1]); ui_text("Demi-équation", ""); }
        else if (r == 1) while (ui_form("Équation bilan", l_bil, bl, 2)) { bilan_explain(bl[0], bl[1]); ui_text("Équation bilan", ""); }
        else { couples_list(); ui_text("Couples connus", ""); }
        break;
      case 2: while (ui_form("Avancement", l_eq, eq, 3)) { avancement_explain(eq[0], eq[1], eq[2]); ui_text("Avancement", ""); } break;
      case 3: while (ui_form("Masse molaire", l_mm, mm, 1)) { masse_explain(mm[0]); ui_text("Masse molaire", ""); } break;
      case 4: formules_menu(0); break;
      case 5: if ((r = ui_menu("Formules de physique", phys, 4)) >= 0) formules_menu(r + 1); break;
    }
  }
}
