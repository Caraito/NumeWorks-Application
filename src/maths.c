/* Maths : second degré (avec valeurs exactes quand c'est possible), suites, cercle trigonométrique. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

#define DELTA "\xce\x94"
#define SQRT "\xe2\x88\x9a"
#define INF "\xe2\x88\x9e"
#define TIMES "\xc3\x97"
#define PI "\xcf\x80"
#define DEG "\xc2\xb0"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

/* ---------- Cercle trigonométrique ---------- */

static const int trig_deg[16] = {0, 30, 45, 60, 90, 120, 135, 150, 180, 210, 225, 240, 270, 300, 315, 330};
static const char *const trig_rad[16] = {"0",      PI "/6",      PI "/4",      PI "/3",      PI "/2",      "2" PI "/3",
                                         "3" PI "/4", "5" PI "/6", PI,          "7" PI "/6", "5" PI "/4", "4" PI "/3",
                                         "3" PI "/2", "5" PI "/3", "7" PI "/4", "11" PI "/6"};

static double trig_val(int k, bool sinus) { /* cos ou sin de l'angle k, 0 exact au lieu de 6e-17 */
  double a = trig_deg[k] * M_PI / 180, v = sinus ? sin(a) : cos(a);
  return fabs(v) < 1e-9 ? 0 : v;
}

static const char *exact(double v) { /* valeur exacte d'un cos / sin remarquable */
  static const char *const s[] = {"0", "1/2", SQRT "2/2", SQRT "3/2", "1", "-1/2", "-" SQRT "2/2", "-" SQRT "3/2", "-1"};
  static const double x[] = {0, 0.5, 0.70710678, 0.86602540, 1};
  for (int i = 0; i < 5; i++)
    if (fabs(fabs(v) - x[i]) < 1e-6) return s[v < 0 && i ? i + 4 : i];
  return "?";
}

static int ulen(const char *s) {
  int n = 0;
  for (; *s; s++) n += (*s & 0xC0) != 0x80;
  return n;
}

static void pad(char *d, const char *s, int w) { /* ajoute s puis des espaces jusqu'à w caractères */
  strcat(d, s);
  for (int n = ulen(s); n < w; n++) strcat(d, " ");
}

void trigo_table(void) {
  out_clear();
  char l[LINELEN] = "";
  pad(l, "Degrés", 8); pad(l, "Radians", 9); pad(l, "cos", 9); strcat(l, "sin");
  outf("%s", l);
  for (int k = 0; k < 16; k++) {
    char d[8];
    sprintf(d, "%d" DEG, trig_deg[k]);
    l[0] = 0;
    pad(l, d, 8); pad(l, trig_rad[k], 9); pad(l, exact(trig_val(k, false)), 9); strcat(l, exact(trig_val(k, true)));
    outf("%s", l);
  }
  outf("");
  outf("À retenir, de 0 à " PI "/2 :");
  outf("  cos : 1, " SQRT "3/2, " SQRT "2/2, 1/2, 0");
  outf("  sin : 0, 1/2, " SQRT "2/2, " SQRT "3/2, 1");
  outf("Radians = degrés " TIMES " " PI " / 180");
  outf("cos" "Â²" "x + sin" "Â²" "x = 1");
  outf("");
  outf("Angles associés :");
  outf("  cos(-x) = cos x        sin(-x) = -sin x");
  outf("  cos(" PI " - x) = -cos x    sin(" PI " - x) = sin x");
  outf("  cos(" PI " + x) = -cos x    sin(" PI " + x) = -sin x");
  outf("  cos(" PI "/2 - x) = sin x   sin(" PI "/2 - x) = cos x");
}

/* Angle « p π / q » : 13π/6, -π/3, 2π, 5/6π, ou « pi » au lieu de π. */
static bool parse_pi(const char *s, long *p, long *q) {
  char d[FIELDLEN];
  int j = 0;
  bool pi = false;
  for (; *s && j < FIELDLEN - 1; s++) {
    if (!strncmp(s, PI, 2) || !strncmp(s, "pi", 2)) { pi = true; s++; }
    else if (*s != ' ') d[j++] = *s;
  }
  d[j] = 0;
  char *sl = strchr(d, '/');
  *q = 1;
  if (sl) { *sl = 0; if (!parse_long(sl + 1, q) || *q == 0) return false; }
  if (!d[0] || !strcmp(d, "+")) *p = 1;
  else if (!strcmp(d, "-")) *p = -1;
  else if (!parse_long(d, p)) return false;
  if (*q < 0) { *p = -*p; *q = -*q; }
  return pi && labs(*p) <= 10000 && *q <= 10000; /* ponytail: bornes pour rester dans un long 32 bits */
}

static void pi_str(char *d, long p, long q) { /* « 13π/6 », « -π », « 0 » */
  long g = gcdl(p, q);
  if (g) { p /= g; q /= g; }
  if (!p) { strcpy(d, "0"); return; }
  if (p == 1) strcpy(d, PI);
  else if (p == -1) strcpy(d, "-" PI);
  else sprintf(d, "%ld" PI, p);
  if (q != 1) sprintf(d + strlen(d), "/%ld", q);
}

static long princ(long p, long q) { /* numérateur de la mesure dans ]-π ; π] */
  long r = p % (2 * q);
  if (r > q) r -= 2 * q;
  if (r <= -q) r += 2 * q;
  return r;
}

static void principale(char *d, long p, long q) { pi_str(d, princ(p, q), q); }

void mesures_explain(const char *sa) {
  out_clear();
  long p, q;
  if (!parse_pi(sa, &p, &q)) {
    outf("Écris l'angle avec " PI " (touche " PI ") :");
    outf("  37" PI "/6, -" PI "/3, 2" PI ", 5/6" PI "...");
    return;
  }
  long g = gcdl(p, q);
  p /= g; q /= g;
  long r = princ(p, q), k = (p - r) / (2 * q), pos = r < 0 ? r + 2 * q : r;
  char a[24], m[24], t[24];
  pi_str(a, p, q);
  pi_str(m, r, q);
  outf("a = %s", a);
  outf("");
  outf("Mesure principale (dans ]-" PI " ; " PI "]) :");
  if (k) outf("  %s = %s + 2" PI " " TIMES " %s%ld%s", a, m, k < 0 ? "(" : "", k, k < 0 ? ")" : "");
  outf("  -> %s", m);
  outf("");
  pi_str(t, pos, q);
  outf("Plus petite mesure positive : %s", t);
  pi_str(t, pos ? pos - 2 * q : -2 * q, q);
  outf("Plus grande mesure négative : %s", t);
  outf("");
  outf("Autres mesures (même point, + 2k" PI ") :");
  for (long i = -3; i <= 3; i++) {
    pi_str(t, r + 2 * i * q, q);
    outf("  k = %2ld : %s%s", i, t, i ? "" : "  (principale)");
  }
}

void meme_point_explain(const char *sa, const char *sb) {
  out_clear();
  long p1, q1, p2, q2;
  if (!parse_pi(sa, &p1, &q1) || !parse_pi(sb, &p2, &q2)) {
    outf("Écris les angles avec " PI " (touche " PI ") :");
    outf("  13" PI "/6, -" PI "/3, 2" PI ", 5/6" PI "...");
    return;
  }
  char a[24], b[24], df[24], k[24];
  pi_str(a, p1, q1);
  pi_str(b, p2, q2);
  long num = p1 * q2 - p2 * q1, den = q1 * q2; /* a - b = num/den × π */
  pi_str(df, num, den);
  frac(k, num, 2 * den);
  outf("a = %s et b = %s", a, b);
  outf("");
  outf("Même point image si a = b + 2k" PI ", k entier.");
  outf("a - b = %s", df);
  outf("k = (a - b) / 2" PI " = %s", k);
  outf("");
  if (num % (2 * den) == 0) {
    outf("k est entier : MÊME point image.");
    outf("%s = %s + 2" PI " " TIMES " %s%s%s", a, b, num < 0 ? "(" : "", k, num < 0 ? ")" : "");
  } else {
    outf("k n'est pas entier : points DIFFÉRENTS.");
  }
  outf("");
  principale(a, p1, q1);
  principale(b, p2, q2);
  outf("Mesures principales (dans ]-" PI " ; " PI "]) :");
  outf("  a : %s     b : %s", a, b);
}

static void dot(int x, int y, int s, eadk_color_t c) {
  eadk_display_push_rect_uniform((eadk_rect_t){x - s / 2, y - s / 2, s, s}, c);
}

static void trigo_app(void) {
  enum { CX = 100, CY = 130, R = 72, RL = 94 }; /* centre, rayon, rayon des étiquettes */
  const eadk_color_t blue = RGB(30, 100, 220);
  static int k = 1;
  for (;;) {
    ui_clear();
    ui_header("Cercle trigo", "OK : tableau");
    eadk_display_push_rect_uniform((eadk_rect_t){CX - R - 8, CY, 2 * R + 16, 1}, C_GRAY);
    eadk_display_push_rect_uniform((eadk_rect_t){CX, CY - R - 8, 1, 2 * R + 16}, C_GRAY);
    for (int i = 0; i < 720; i++) dot(CX + lround(R * cos(i * M_PI / 360)), CY - lround(R * sin(i * M_PI / 360)), 2, C_TEXT);

    double c = trig_val(k, false), s = trig_val(k, true);
    int px = CX + lround(R * c), py = CY - lround(R * s);
    for (int y = py < CY ? py : CY; y <= (py < CY ? CY : py); y++) if (y / 3 % 2) dot(px, y, 1, C_GRAY); /* pointillés */
    for (int x = px < CX ? px : CX; x <= (px < CX ? CX : px); x++) if (x / 3 % 2) dot(x, py, 1, C_GRAY);
    eadk_display_push_rect_uniform((eadk_rect_t){px < CX ? px : CX, CY - 1, abs(px - CX) + 1, 3}, blue); /* cos */
    eadk_display_push_rect_uniform((eadk_rect_t){CX - 1, py < CY ? py : CY, 3, abs(py - CY) + 1}, C_BAD); /* sin */
    for (int j = 0; j <= R; j++) dot(CX + lround(j * c), CY - lround(j * s), 2, C_HEAD);

    for (int i = 0; i < 16; i++) {
      double ci = trig_val(i, false), si = trig_val(i, true);
      dot(CX + lround(R * ci), CY - lround(R * si), i == k ? 7 : 5, i == k ? C_HEAD : C_GRAY);
      int w = 7 * ulen(trig_rad[i]);
      ui_str(CX + lround(RL * ci) - w / 2, CY - lround(RL * si) - 7, trig_rad[i], false, i == k ? C_TEXT : C_GRAY, C_BG);
    }

    char t[40], v[24];
    sprintf(t, "%d" DEG, trig_deg[k]);
    ui_str(206, 32, t, true, C_TEXT, C_BG);
    sprintf(t, "%s rad", trig_rad[k]);
    ui_str(206, 54, t, true, C_TEXT, C_BG);
    sprintf(t, "cos = %s", exact(c));
    ui_str(206, 90, t, true, blue, C_BG);
    fmt_num(v, c);
    sprintf(t, "\xe2\x89\x88 %s", v);
    ui_str(206, 110, t, false, C_GRAY, C_BG);
    sprintf(t, "sin = %s", exact(s));
    ui_str(206, 138, t, true, C_BAD, C_BG);
    fmt_num(v, s);
    sprintf(t, "\xe2\x89\x88 %s", v);
    ui_str(206, 158, t, false, C_GRAY, C_BG);
    ui_str(206, 200, "Flèches :", false, C_GRAY, C_BG);
    ui_str(206, 214, "changer l'angle", false, C_GRAY, C_BG);

    eadk_event_t e = ui_event(-1);
    if (e == eadk_event_back) return;
    if (e == eadk_event_right || e == eadk_event_up) k = (k + 1) % 16;
    else if (e == eadk_event_left || e == eadk_event_down) k = (k + 15) % 16;
    else if (e == eadk_event_ok || e == eadk_event_exe) { trigo_table(); ui_text("Valeurs remarquables", ""); }
  }
}

/* Avec π : radians -> degrés. Sans π : degrés -> radians. */
void deg_rad_explain(const char *sa) {
  out_clear();
  long p, q;
  double v;
  char t[24], w[24];
  if (parse_pi(sa, &p, &q)) {
    pi_str(t, p, q);
    frac(w, 180 * p, q);
    outf("%s rad = %s " TIMES " 180" DEG " / " PI, t, t);
    outf("  = %s" DEG, w);
    if ((180 * p) % q) { fmt_num(w, 180.0 * p / q); outf("  " "\xe2\x89\x88" " %s" DEG, w); }
  } else if (parse_long(sa, &p) && labs(p) <= 100000) {
    pi_str(t, p, 180);
    outf("%ld" DEG " = %ld " TIMES " " PI " / 180", p, p);
    outf("  = %s rad", t);
    fmt_num(w, p * M_PI / 180);
    outf("  " "\xe2\x89\x88" " %s rad", w);
  } else if (parse_double(sa, &v)) {
    fmt_num(t, v);
    fmt_num(w, v * M_PI / 180);
    outf("%s" DEG " = %s " TIMES " " PI " / 180", t, t);
    outf("  " "\xe2\x89\x88" " %s rad", w);
  } else {
    outf("Degrés : un nombre (150, -45, 22,5).");
    outf("Radians : avec " PI " (5" PI "/6, -" PI "/4).");
    return;
  }
  outf("");
  outf("Degrés \xe2\x86\x92 radians : " TIMES " " PI " / 180");
  outf("Radians \xe2\x86\x92 degrés : " TIMES " 180 / " PI);
  outf(PI " rad = 180" DEG);
}

static void trigo_menu(void) {
  static const char *const menu[] = {"Cercle trigonométrique", "Degrés et radians", "Même point image", "Mesure principale"};
  static const char *const l_dr[] = {"Angle (" DEG " ou " PI ")"};
  static char dr[1][FIELDLEN] = {"150"};
  static const char *const l_mp[] = {"Angle a", "Angle b"};
  static char mp[2][FIELDLEN] = {"13" PI "/6", PI "/6"};
  static const char *const l_me[] = {"Angle"};
  static char me[1][FIELDLEN] = {"37" PI "/6"};
  for (;;) {
    int k = ui_menu("Trigonométrie", menu, 4);
    if (k < 0) return;
    if (k == 0) trigo_app();
    else if (k == 1)
      while (ui_form(menu[1], l_dr, dr, 1)) { deg_rad_explain(dr[0]); ui_text(menu[1], ""); }
    else if (k == 2)
      while (ui_form(menu[2], l_mp, mp, 2)) { meme_point_explain(mp[0], mp[1]); ui_text(menu[2], ""); }
    else
      while (ui_form(menu[3], l_me, me, 1)) { mesures_explain(me[0]); ui_text(menu[3], ""); }
  }
}

void maths_app(void) {
  static const char *const menu[] = {"Second degré", "Suite arithmétique", "Suite géométrique", "Trigonométrie"};
  static const char *const l_sd[] = {"a", "b", "c"};
  static const char *const l_su[] = {"u0", "Raison", "n"};
  static char sd[3][FIELDLEN] = {"2", "-4", "-6"};
  static char su[2][3][FIELDLEN] = {{"3", "2", "10"}, {"2", "3", "5"}};
  for (;;) {
    int k = ui_menu("Maths", menu, 4);
    if (k < 0) return;
    if (k == 0)
      while (ui_form("ax^2 + bx + c", l_sd, sd, 3)) { second_degre_explain(sd[0], sd[1], sd[2]); ui_text("Second degré", ""); }
    else if (k == 3)
      trigo_menu();
    else
      while (ui_form(menu[k], l_su, su[k - 1], 3)) { suite_explain(k == 2, su[k - 1][0], su[k - 1][1], su[k - 1][2]); ui_text(menu[k], ""); }
  }
}
