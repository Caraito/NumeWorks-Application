/* Maths : second degré (avec valeurs exactes quand c'est possible) et suites. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

#define DELTA "\xce\x94"
#define SQRT "\xe2\x88\x9a"
#define INF "\xe2\x88\x9e"
#define TIMES "\xc3\x97"

static long gcdl(long a, long b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { long t = a % b; a = b; b = t; }
  return a;
}

static void frac(char *d, long num, long den) { /* fraction réduite "p/q" ou "p" */
  long g = gcdl(num, den);
  if (g) { num /= g; den /= g; }
  if (den < 0) { num = -num; den = -den; }
  if (den == 1) sprintf(d, "%ld", num);
  else sprintf(d, "%ld/%ld", num, den);
}

static void par(char *d, double v) { /* nombre, entre parenthèses s'il est négatif */
  char t[24];
  fmt_num(t, v);
  if (v < 0) sprintf(d, "(%s)", t);
  else strcpy(d, t);
}

static bool is_int(double v) { return v == floor(v) && fabs(v) < 1e6; }

/* "a" devant x : "" pour 1, "-" pour -1 */
static void lead(char *d, double a) {
  if (a == 1) d[0] = 0;
  else if (a == -1) strcpy(d, "-");
  else fmt_num(d, a);
}

/* " + 3" / " - 3" ; exact = texte déjà formaté de |v| ou NULL */
static void signed_term(char *d, double v, const char *txt_abs) {
  char t[32];
  if (txt_abs) strcpy(t, txt_abs);
  else fmt_num(t, fabs(v));
  sprintf(d, " %c %s", v < 0 ? '-' : '+', t);
}

void second_degre_explain(const char *sa, const char *sb, const char *sc) {
  out_clear();
  double a, b, c;
  if (!parse_double(sa, &a) || !parse_double(sb, &b) || !parse_double(sc, &c)) {
    outf("Erreur : a, b et c doivent être des nombres (0 si absent).");
    return;
  }
  if (a == 0) { outf("a = 0 : ce n'est pas un polynôme du second degré."); return; }

  char t[96], la[24], pa[24], pb[24], pc[24], s[32];
  lead(la, a);
  sprintf(t, "f(x) = %sx^2", la);
  if (b) { lead(s, fabs(b)); strcat(t, b < 0 ? " - " : " + "); strcat(t, s); strcat(t, "x"); }
  if (c) { signed_term(s, c, NULL); strcat(t, s); }
  outf("%s", t);
  outf("");

  double D = b * b - 4 * a * c;
  par(pa, a); par(pb, b); par(pc, c);
  fmt_num(s, D);
  outf(DELTA " = b^2 - 4ac = %s^2 - 4 " TIMES " %s " TIMES " %s = %s", pb, pa, pc, s);

  bool exact = is_int(a) && is_int(b) && is_int(c);
  long A = (long)a, B = (long)b;
  double alpha = -b / (2 * a), beta = -D / (4 * a);
  char sal[32], sbe[32];
  if (exact) { frac(sal, -B, 2 * A); frac(sbe, -(long)D, 4 * A); }
  else { fmt_num(sal, alpha); fmt_num(sbe, beta); }

  char r1[48] = "", r2[48] = "", lo[48], hi[48];
  double x1 = 0, x2 = 0;
  bool rational = false;
  if (D > 0) {
    x1 = (-b - sqrt(D)) / (2 * a);
    x2 = (-b + sqrt(D)) / (2 * a);
    outf(DELTA " > 0 : deux racines réelles");
    long Dl = (long)D, k = 1, m = Dl;
    if (exact) /* sqrt(D) = k*sqrt(m) avec m sans facteur carré */
      for (long f = 2; f * f <= m; f++)
        while (m % (f * f) == 0) { m /= f * f; k *= f; }
    if (exact && m == 1) {
      rational = true;
      frac(r1, -B - k, 2 * A);
      frac(r2, -B + k, 2 * A);
    } else if (exact) { /* forme exacte (B' ± K√m) / D' */
      long g = gcdl(gcdl(B, k), 2 * A), Bp = -B / g, K = k / g, Dp = 2 * A / g;
      if (Dp < 0) { Bp = -Bp; K = -K; Dp = -Dp; }
      for (int i = 0; i < 2; i++) {
        long kk = i ? K : -K;
        char num[64], rad[24];
        if (labs(kk) == 1) sprintf(rad, SQRT "%ld", m);
        else sprintf(rad, "%ld" SQRT "%ld", labs(kk), m);
        if (Bp) sprintf(num, "%ld %c %s", Bp, kk < 0 ? '-' : '+', rad);
        else sprintf(num, "%s%s", kk < 0 ? "-" : "", rad);
        if (Dp == 1) strcpy(i ? r2 : r1, num);
        else sprintf(i ? r2 : r1, Bp ? "(%s)/%ld" : "%s/%ld", num, Dp);
      }
    }
    char d1[24], d2[24];
    fmt_num(d1, x1);
    fmt_num(d2, x2);
    outf("  x1 et x2 = (-b \xc2\xb1 " SQRT DELTA ") / (2a)");
    if (r1[0] && !rational) {
      outf("  x1 = %s \xe2\x89\x88 %s", r1, d1);
      outf("  x2 = %s \xe2\x89\x88 %s", r2, d2);
    } else {
      if (!rational) { strcpy(r1, d1); strcpy(r2, d2); }
      outf("  x1 = %s", r1);
      outf("  x2 = %s", r2);
    }
    bool ord = x1 < x2;
    strcpy(lo, ord ? r1 : r2);
    strcpy(hi, ord ? r2 : r1);
    outf("  Somme = -b/a, produit = c/a");
    outf("");
    if (rational || !exact) {
      char f1[40], f2[40];
      signed_term(f1, -x1, rational ? (r1[0] == '-' ? r1 + 1 : r1) : NULL);
      signed_term(f2, -x2, rational ? (r2[0] == '-' ? r2 + 1 : r2) : NULL);
      outf("Forme factorisée : f(x) = %s(x%s)(x%s)", la, f1, f2);
    } else {
      outf("Forme factorisée : f(x) = %s(x - x1)(x - x2)", la);
    }
  } else if (D == 0) {
    outf(DELTA " = 0 : une racine double");
    outf("  x0 = -b/(2a) = %s", sal);
    char f0[40];
    signed_term(f0, -alpha, exact ? (sal[0] == '-' ? sal + 1 : sal) : NULL);
    outf("");
    outf("Forme factorisée : f(x) = %s(x%s)^2", la, f0);
  } else {
    outf(DELTA " < 0 : pas de racine réelle,");
    outf("pas de forme factorisée.");
  }

  outf("");
  char fa[40], fb[40];
  signed_term(fa, -alpha, exact ? (sal[0] == '-' ? sal + 1 : sal) : NULL);
  outf("Forme canonique : a(x - " "\xce\xb1" ")^2 + " "\xce\xb2");
  outf("  " "\xce\xb1" " = -b/(2a) = %s ; " "\xce\xb2" " = f(" "\xce\xb1" ") = %s", sal, sbe);
  if (beta) signed_term(fb, beta, exact ? (sbe[0] == '-' ? sbe + 1 : sbe) : NULL);
  else fb[0] = 0;
  outf("  f(x) = %s(x%s)^2%s", la, alpha ? fa : "", fb);
  if (!alpha) outf("  (soit f(x) = %sx^2%s)", la, fb);
  outf("Sommet S(%s ; %s) : %s (a %s 0)", sal, sbe, a > 0 ? "minimum" : "maximum", a > 0 ? ">" : "<");

  outf("");
  const char *sa_ = a > 0 ? ">" : "<", *so = a > 0 ? "<" : ">";
  outf("Signe de f(x) (signe de a hors des racines) :");
  if (D > 0) {
    outf("  f(x) %s 0 sur ]-" INF " ; %s[ et ]%s ; +" INF "[", sa_, lo, hi);
    outf("  f(x) %s 0 sur ]%s ; %s[", so, lo, hi);
    outf("  f(x) = 0 en x = %s et x = %s", lo, hi);
  } else if (D == 0) {
    outf("  f(x) %s 0 pour tout x, sauf f(%s) = 0", sa_, sal);
  } else {
    outf("  f(x) %s 0 pour tout x", sa_);
  }
}

void suite_explain(bool geo, const char *su0, const char *sr, const char *sn) {
  out_clear();
  double u0, r;
  long n;
  if (!parse_double(su0, &u0) || !parse_double(sr, &r) || !parse_long(sn, &n) || n < 0 || n > 10000) {
    outf("Erreur : u0 et la raison sont des nombres, n un entier entre 0 et 10000.");
    return;
  }
  char a[24], b[24], c[24], d[24];
  fmt_num(a, u0);
  par(b, r);
  if (!geo) {
    double un = u0 + n * r, S = (n + 1) * (u0 + un) / 2;
    outf("Suite arithmétique : u(n+1) = u(n) + r");
    outf("");
    outf("Terme général : u(n) = u0 + n " TIMES " r");
    outf("  u(n) = %s + %s n", a, b);
    fmt_num(c, un);
    outf("  u(%ld) = %s + %ld " TIMES " %s = %s", n, a, n, b, c);
    outf("");
    outf("Somme S = u0 + u1 + ... + u(%ld) :", n);
    outf("  S = nb termes " TIMES " (premier + dernier) / 2");
    fmt_num(d, S);
    outf("  S = %ld " TIMES " (%s + %s) / 2 = %s", n + 1, a, c, d);
    outf("");
    outf("Sens de variation : %s", r > 0 ? "croissante (r > 0)" : r < 0 ? "décroissante (r < 0)" : "constante (r = 0)");
  } else {
    double un = u0 * pow(r, n), S = r == 1 ? (n + 1) * u0 : u0 * (1 - pow(r, n + 1)) / (1 - r);
    outf("Suite géométrique : u(n+1) = u(n) " TIMES " q");
    outf("");
    outf("Terme général : u(n) = u0 " TIMES " q^n");
    outf("  u(n) = %s " TIMES " %s^n", a, b);
    fmt_num(c, un);
    outf("  u(%ld) = %s " TIMES " %s^%ld = %s", n, a, b, n, c);
    outf("");
    outf("Somme S = u0 + u1 + ... + u(%ld) :", n);
    fmt_num(d, S);
    if (r == 1) outf("  q = 1 : S = (n+1) " TIMES " u0 = %s", d);
    else {
      outf("  S = u0 " TIMES " (1 - q^(n+1)) / (1 - q)");
      outf("  S = %s " TIMES " (1 - %s^%ld) / (1 - %s) = %s", a, b, n + 1, b, d);
    }
    outf("");
    const char *v = u0 == 0 || r == 1 ? "constante"
                  : r == 0           ? "nulle à partir du rang 1"
                  : r < 0            ? "ni croissante ni décroissante (q < 0 : signes alternés)"
                  : (r > 1) == (u0 > 0) ? "croissante"
                                        : "décroissante";
    outf("Sens de variation : %s", v);
    if (r > 0 && r != 1 && u0 != 0) outf("  (u0 %s 0 et q %s 1)", u0 > 0 ? ">" : "<", r > 1 ? ">" : "<");
  }
  outf("");
  outf("Premiers termes :");
  char line[400] = "";
  double u = u0;
  for (int i = 0; i < 6 && i <= n + 5; i++) {
    char t[40];
    fmt_num(a, u);
    snprintf(t, sizeof t, "%su%d = %s", i ? ", " : "", i, a);
    strcat(line, t);
    u = geo ? u * r : u + r;
  }
  outf("  %s", line);
}

void maths_app(void) {
  static const char *const menu[] = {"Second degré", "Suite arithmétique", "Suite géométrique"};
  static const char *const l_sd[] = {"a", "b", "c"};
  static const char *const l_su[] = {"u0", "Raison", "n"};
  static char sd[3][FIELDLEN] = {"2", "-4", "-6"};
  static char su[2][3][FIELDLEN] = {{"3", "2", "10"}, {"2", "3", "5"}};
  for (;;) {
    int k = ui_menu("Maths", menu, 3);
    if (k < 0) return;
    if (k == 0)
      while (ui_form("ax^2 + bx + c", l_sd, sd, 3)) { second_degre_explain(sd[0], sd[1], sd[2]); ui_text("Second degré", ""); }
    else
      while (ui_form(menu[k], l_su, su[k - 1], 3)) { suite_explain(k == 2, su[k - 1][0], su[k - 1][1], su[k - 1][2]); ui_text(menu[k], ""); }
  }
}
