/* Équilibrage d'équations chimiques.
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

static void print_side(char *dst, int from, int to, const long long x[]) {
  dst[0] = 0;
  for (int j = from; j < to; j++) {
    char t[FIELDLEN + 24];
    if (x[j] == 1) snprintf(t, sizeof t, "%s%s", j > from ? " + " : "", sp_txt[j]);
    else snprintf(t, sizeof t, "%s%ld %s", j > from ? " + " : "", (long)x[j], sp_txt[j]);
    strcat(dst, t);
  }
}

bool chimie_explain(const char *reactifs, const char *produits) {
  out_clear();
  nel = nsp = 0;
  err[0] = 0;
  if (!side(reactifs)) goto error;
  nreac = nsp;
  if (!side(produits)) goto error;

  long long x[MAXSP];
  int r = balance(x);
  if (r == 0) { fail("Impossible d'équilibrer : vérifie les formules (un élément n'apparaît peut-être que d'un côté)"); goto error; }
  if (r == 2) { fail("Plusieurs équilibrages possibles : l'équation mélange plusieurs réactions"); goto error; }

  char l[8 * FIELDLEN], rr[8 * FIELDLEN];
  print_side(l, 0, nreac, x);
  print_side(rr, nreac, nsp, x);
  outf("Équation équilibrée :");
  if (strlen(l) + strlen(rr) + 3 <= COLS) outf("%s \xe2\x86\x92 %s", l, rr);
  else { outf("%s", l); outf("\xe2\x86\x92 %s", rr); } /* trop long : un côté par ligne */
  outf("");
  outf("Vérification (gauche = droite) :");
  for (int e = 0; e < nel; e++) {
    long long g = 0, d = 0;
    for (int j = 0; j < nsp; j++) *(j < nreac ? &g : &d) += x[j] * cnt[j][e];
    outf("  %-2s : %ld = %ld", el[e], (long)g, (long)d);
  }
  bool has_charge = false;
  for (int j = 0; j < nsp; j++) has_charge |= charge[j] != 0;
  if (has_charge) {
    long long g = 0, d = 0;
    for (int j = 0; j < nsp; j++) *(j < nreac ? &g : &d) += x[j] * charge[j];
    outf("  Charges : %+ld = %+ld", (long)g, (long)d);
  }
  return true;

error:
  outf("Erreur : %s", err);
  outf("");
  outf("Exemples d'écriture :");
  outf("  CH4+O2        Ca(OH)2      CuSO4.5H2O");
  outf("  Fe^3+         SO4^2-       e^-  (électron)");
  outf("Majuscule : shift, alpha, puis la lettre.");
  outf("Minuscule : alpha, puis la lettre.");
  return false;
}

void chimie_app(void) {
  static char f[2][FIELDLEN] = {"CH4+O2", "CO2+H2O"};
  static const char *const labels[] = {"Réactifs", "Produits"};
  while (ui_form("Équilibrer", labels, f, 2)) {
    chimie_explain(f[0], f[1]);
    ui_text("Équilibrage", "");
  }
}
