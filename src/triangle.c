/* Résolution de triangles quelconques : 3 côtés, 2 côtés + 1 angle, 1 côté + 2 angles.
 * Convention : le côté a est opposé à l'angle A, etc. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  double s[3], a[3]; /* côtés, angles (radians) */
} tri_t;

static tri_t sol[2];
static int nsol;
static bool rad_unit;

static const char SN[] = "abc", AN[] = "ABC";

static double clamp1(double x) { return x > 1 ? 1 : x < -1 ? -1 : x; }

static void angles_from_sides(tri_t *t) { /* Al-Kashi */
  for (int i = 0; i < 3; i++) {
    double sj = t->s[(i + 1) % 3], sk = t->s[(i + 2) % 3];
    t->a[i] = acos(clamp1((sj * sj + sk * sk - t->s[i] * t->s[i]) / (2 * sj * sk)));
  }
}

static void sides_from_sines(tri_t *t, int known) {
  double ratio = t->s[known] / sin(t->a[known]);
  for (int i = 0; i < 3; i++)
    if (i != known) t->s[i] = ratio * sin(t->a[i]);
}

static void fmt_angle(char *d, double a) {
  fmt_num(d, rad_unit ? a : a * 180 / M_PI);
  strcat(d, rad_unit ? " rad" : "\xc2\xb0");
}

static bool near(double x, double y) { return fabs(x - y) <= 1e-9 * fmax(fabs(x), fabs(y)); }

static void print_tri(const tri_t *t) {
  char s[24], a[24];
  for (int i = 0; i < 3; i++) {
    fmt_num(s, t->s[i]);
    fmt_angle(a, t->a[i]);
    outf("  %c = %-14s %c = %s", SN[i], s, AN[i], a);
  }
  fmt_num(s, t->s[0] + t->s[1] + t->s[2]);
  outf("  Périmètre = %s", s);
  fmt_num(s, 0.5 * t->s[1] * t->s[2] * sin(t->a[0]));
  outf("  Aire = %s", s);

  if (near(t->s[0], t->s[1]) && near(t->s[1], t->s[2])) outf("  Triangle équilatéral");
  else
    for (int i = 0; i < 3; i++)
      if (near(t->s[(i + 1) % 3], t->s[(i + 2) % 3])) outf("  Triangle isocèle en %c", AN[i]);
  for (int i = 0; i < 3; i++) {
    int j = (i + 1) % 3, k = (i + 2) % 3;
    if (fabs(t->a[i] - M_PI / 2) < 1e-9) {
      char x[24], y[24], z[24];
      fmt_num(x, t->s[i] * t->s[i]);
      fmt_num(y, t->s[j] * t->s[j]);
      fmt_num(z, t->s[k] * t->s[k]);
      outf("  Triangle rectangle en %c (Pythagore) :", AN[i]);
      outf("  %c^2 = %c^2 + %c^2  (%s = %s + %s)", SN[i], SN[j], SN[k], x, y, z);
    } else if (t->a[i] > M_PI / 2) {
      outf("  Triangle obtusangle (angle %c > 90\xc2\xb0)", AN[i]);
    }
  }
}

void triangle_explain(char f[6][FIELDLEN], bool radians) {
  out_clear();
  nsol = 0;
  rad_unit = radians;
  tri_t t;
  bool ks[3], ka[3];
  int ns = 0, na = 0;
  double unit = radians ? 1 : M_PI / 180;
  for (int i = 0; i < 6; i++) {
    bool known = false;
    for (const char *c = f[i]; *c; c++) known |= *c != ' ';
    double v = 0;
    if (known && (!parse_double(f[i], &v) || v <= 0)) {
      outf("Erreur : la valeur de %c doit être un nombre > 0.", i < 3 ? SN[i] : AN[i - 3]);
      return;
    }
    if (i < 3) { ks[i] = known; t.s[i] = v; ns += known; }
    else { ka[i - 3] = known; t.a[i - 3] = v * unit; na += known; }
    if (i >= 3 && known && t.a[i - 3] >= M_PI) {
      outf("Erreur : un angle doit être < %s.", radians ? "pi rad" : "180\xc2\xb0");
      return;
    }
  }
  if (ns + na != 3 || ns == 0) {
    outf("Remplis exactement 3 cases, dont au moins un côté.");
    outf("");
    outf("Rappel : le côté a est en face de l'angle A, b en face de B, c en face de C.");
    return;
  }

  if (ns == 3) {
    for (int i = 0; i < 3; i++)
      if (t.s[i] >= t.s[(i + 1) % 3] + t.s[(i + 2) % 3]) {
        outf("Ces 3 longueurs ne forment pas un triangle : %c >= %c + %c (inégalité triangulaire).",
             SN[i], SN[(i + 1) % 3], SN[(i + 2) % 3]);
        return;
      }
    outf("Méthode : 3 côtés connus -> Al-Kashi");
    outf("  cos A = (b^2 + c^2 - a^2) / (2bc)");
    angles_from_sides(&t);
    sol[nsol++] = t;
  } else if (ns == 2) {
    int ai = ka[0] ? 0 : ka[1] ? 1 : 2;
    if (!ks[ai]) { /* l'angle est entre les deux côtés connus */
      int j = (ai + 1) % 3, k = (ai + 2) % 3;
      outf("Méthode : 2 côtés et l'angle entre eux -> Al-Kashi");
      outf("  %c^2 = %c^2 + %c^2 - 2%c%c\xc2\xb7" "cos %c", SN[ai], SN[j], SN[k], SN[j], SN[k], AN[ai]);
      t.s[ai] = sqrt(t.s[j] * t.s[j] + t.s[k] * t.s[k] - 2 * t.s[j] * t.s[k] * cos(t.a[ai]));
      angles_from_sides(&t);
      sol[nsol++] = t;
    } else { /* côté-côté-angle : loi des sinus, éventuellement 2 solutions */
      int j = ks[(ai + 1) % 3] ? (ai + 1) % 3 : (ai + 2) % 3, k = 3 - ai - j;
      outf("Méthode : loi des sinus");
      outf("  %c / sin %c = %c / sin %c", SN[ai], AN[ai], SN[j], AN[j]);
      double sv = t.s[j] * sin(t.a[ai]) / t.s[ai];
      if (sv > 1 + 1e-12) {
        outf("sin %c vaudrait %.4f > 1 : aucun triangle possible.", AN[j], sv);
        return;
      }
      double cand[2] = {asin(clamp1(sv)), M_PI - asin(clamp1(sv))};
      for (int c = 0; c < (sv < 1 - 1e-12 ? 2 : 1); c++) {
        if (t.a[ai] + cand[c] >= M_PI - 1e-12) continue;
        tri_t u = t;
        u.a[j] = cand[c];
        u.a[k] = M_PI - u.a[ai] - u.a[j];
        sides_from_sines(&u, ai);
        sol[nsol++] = u;
      }
      if (nsol == 0) { outf("Aucun triangle possible avec ces valeurs."); return; }
      if (nsol == 2) outf("Cas ambigu : 2 triangles possibles !");
    }
  } else { /* 1 côté, 2 angles */
    int miss = !ka[0] ? 0 : !ka[1] ? 1 : 2, known = ks[0] ? 0 : ks[1] ? 1 : 2;
    t.a[miss] = M_PI - t.a[(miss + 1) % 3] - t.a[(miss + 2) % 3];
    if (t.a[miss] <= 1e-12) {
      outf("Erreur : la somme des deux angles atteint %s.", radians ? "pi" : "180\xc2\xb0");
      return;
    }
    outf("Méthode : %c = %s - %c - %c, puis loi des sinus", AN[miss], radians ? "pi" : "180\xc2\xb0",
         AN[(miss + 1) % 3], AN[(miss + 2) % 3]);
    sides_from_sines(&t, known);
    sol[nsol++] = t;
  }

  for (int i = 0; i < nsol; i++) {
    outf("");
    if (nsol > 1) outf("Triangle n\xc2\xb0%d :", i + 1);
    print_tri(&sol[i]);
  }
}

static void draw(int idx) {
  const tri_t *t = &sol[idx];
  /* A en (0,0), B en (c,0), C en (b cos A, b sin A) */
  double px[3] = {0, t->s[2], t->s[1] * cos(t->a[0])}, py[3] = {0, 0, t->s[1] * sin(t->a[0])};
  double minx = fmin(0, px[2]), maxx = fmax(px[1], px[2]);
  double k = fmin(260 / (maxx - minx), 160 / py[2]);
  int X[3], Y[3];
  for (int i = 0; i < 3; i++) {
    X[i] = 30 + (int)((px[i] - minx) * k);
    Y[i] = 210 - (int)(py[i] * k);
  }
  ui_clear();
  char h[16] = "";
  if (nsol > 1) snprintf(h, sizeof h, "%d/2 OK", idx + 1);
  ui_header("Dessin (à l'échelle)", h);
  for (int i = 0; i < 3; i++) ui_line(X[i], Y[i], X[(i + 1) % 3], Y[(i + 1) % 3], C_TEXT);
  int gx = (X[0] + X[1] + X[2]) / 3, gy = (Y[0] + Y[1] + Y[2]) / 3;
  for (int i = 0; i < 3; i++) {
    /* sommet : décalé vers l'extérieur du triangle */
    double dx = X[i] - gx, dy = Y[i] - gy, n = sqrt(dx * dx + dy * dy) + 1e-9;
    char lab[2] = {AN[i], 0};
    double d = 4 + 5 * fabs(dx / n) + 9 * fabs(dy / n); /* sort la boîte du texte du triangle */
    int lx = X[i] + (int)(dx / n * d) - 5, ly = Y[i] + (int)(dy / n * d) - 9;
    ui_str(lx < 0 ? 0 : lx > 310 ? 310 : lx, ly < 24 ? 24 : ly > 222 ? 222 : ly, lab, true, C_HEAD, C_BG);
    /* côté opposé : au milieu, décalé vers l'extérieur */
    int j = (i + 1) % 3, m = (i + 2) % 3;
    int mx = (X[j] + X[m]) / 2, my = (Y[j] + Y[m]) / 2;
    dx = -(Y[m] - Y[j]); dy = X[m] - X[j]; /* perpendiculaire au côté, côté opposé au sommet */
    if (dx * (mx - X[i]) + dy * (my - Y[i]) < 0) { dx = -dx; dy = -dy; }
    n = sqrt(dx * dx + dy * dy) + 1e-9;
    char v[24], txt[32];
    fmt_num(v, t->s[i]);
    snprintf(txt, sizeof txt, "%c=%s", SN[i], v);
    int w = strlen(txt) * 7;
    d = 4 + w / 2.0 * fabs(dx / n) + 7 * fabs(dy / n);
    lx = mx + (int)(dx / n * d) - w / 2;
    ly = my + (int)(dy / n * d) - 7;
    ui_str(lx < 0 ? 0 : lx > 320 - w ? 320 - w : lx, ly < 24 ? 24 : ly > 226 ? 226 : ly, txt, false, C_GRAY, C_BG);
  }
}

void triangle_app(void) {
  static const char *const units[] = {"Angles en degrés", "Angles en radians"};
  static const char *const labels[] = {"Côté a", "Côté b", "Côté c", "Angle A", "Angle B", "Angle C"};
  static char f[6][FIELDLEN] = {"3", "4", "", "", "", "90"};
  int u = ui_menu("Triangle", units, 2);
  if (u < 0) return;
  while (ui_form(u ? "Triangle (rad)" : "Triangle (\xc2\xb0)", labels, f, 6)) {
    triangle_explain(f, u == 1);
    while (ui_text("Triangle", nsol ? "OK : dessin" : "") == eadk_event_ok && nsol) {
      int idx = 0;
      for (;;) {
        draw(idx);
        eadk_event_t e = ui_event(-1);
        if (e == eadk_event_back) break;
        if ((e == eadk_event_ok || e == eadk_event_right || e == eadk_event_left) && nsol > 1) idx ^= 1;
      }
    }
  }
}
