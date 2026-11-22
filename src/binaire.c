/* Outils binaires : conversions, opérations posées, complément à 2, IEEE 754, quiz.
 * printf de newlib-nano ne gère pas %lld : on reste en long / unsigned long (32 bits). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

static const char DIGITS[] = "0123456789ABCDEF";

static void strip(const char *s, char *d) {
  int j = 0;
  for (; *s && j < FIELDLEN - 1; s++)
    if (*s != ' ' && *s != '_') d[j++] = *s;
  d[j] = 0;
}

static bool parse_base(const char *s, int base, uint32_t *v) {
  char d[FIELDLEN];
  strip(s, d);
  if (!d[0]) return false;
  uint64_t x = 0;
  for (char *p = d; *p; p++) {
    int c = *p >= 'a' ? *p - 'a' + 10 : *p >= 'A' ? *p - 'A' + 10 : *p - '0';
    if (c < 0 || c >= base) return false;
    x = x * base + c;
    if (x > 0xFFFFFFFFu) return false;
  }
  *v = (uint32_t)x;
  return true;
}

static void to_base(uint32_t v, int base, char *d) {
  char t[40];
  int n = 0;
  do { t[n++] = DIGITS[v % base]; v /= base; } while (v);
  for (int i = 0; i < n; i++) d[i] = t[n - 1 - i];
  d[n] = 0;
}

static void bits_n(uint32_t v, int n, char *d) { /* n bits, zéros à gauche compris */
  for (int i = 0; i < n; i++) d[i] = (v >> (n - 1 - i)) & 1 ? '1' : '0';
  d[n] = 0;
}

static void grouped(const char *s, int g, char *d) { /* espace tous les g chiffres depuis la droite */
  int n = strlen(s), j = 0;
  for (int i = 0; i < n; i++) {
    if (i && (n - i) % g == 0) d[j++] = ' ';
    d[j++] = s[i];
  }
  d[j] = 0;
}

static void groups_table(const char *bin, int g) {
  int n = strlen(bin), pad = (g - n % g) % g;
  char b[48];
  memset(b, '0', pad);
  strcpy(b + pad, bin);
  for (int i = 0; b[i]; i += g) {
    char grp[5] = {0};
    memcpy(grp, b + i, g);
    uint32_t v;
    parse_base(grp, 2, &v);
    outf("  %s -> %c", grp, DIGITS[v]);
  }
}

static void powers_sum(const char *digits, int base) {
  char a[300] = "", b[300] = "", t[40];
  int n = strlen(digits);
  for (int i = 0; i < n; i++) {
    int c = strchr(DIGITS, digits[i]) - DIGITS;
    if (!c) continue;
    if (base == 2) snprintf(t, sizeof t, "%s2^%d", a[0] ? " + " : "", n - 1 - i);
    else snprintf(t, sizeof t, "%s%dx%d^%d", a[0] ? " + " : "", c, base, n - 1 - i);
    strcat(a, t);
    snprintf(t, sizeof t, "%s%lu", b[0] ? " + " : "", (unsigned long)(c * (uint32_t)pow(base, n - 1 - i)));
    strcat(b, t);
  }
  if (!a[0]) strcpy(a, "0");
  outf("  %s", a);
  outf("  = %s", b[0] ? b : "0");
}

void bin_convert_explain(const char *s, int base) {
  out_clear();
  uint32_t v;
  if (!parse_base(s, base, &v)) {
    outf("Erreur : « %s » n'est pas un nombre valide en base %d (max 32 bits).", s, base);
    if (base == 16) outf("Lettres A à F : alpha puis la touche (ou shift, alpha).");
    return;
  }
  char bin[40], bin_g[48], hex[12], oct[16], src[40];
  to_base(v, 2, bin);
  grouped(bin, 4, bin_g);
  to_base(v, 16, hex);
  to_base(v, 8, oct);
  to_base(v, base, src);
  outf("Décimal     : %lu", (unsigned long)v);
  outf("Binaire     : %s", bin_g);
  outf("Hexadécimal : %s", hex);
  outf("Octal       : %s", oct);
  outf("");
  if (base == 10) {
    outf("Binaire : divisions successives par 2");
    if (!v) outf("  0 = 2 x 0 + 0");
    for (uint32_t x = v; x; x /= 2)
      outf("  %lu = 2 x %lu + %lu", (unsigned long)x, (unsigned long)(x / 2), (unsigned long)(x % 2));
    outf("On lit les restes de bas en haut : %s", bin);
  } else if (base == 2) {
    outf("Décimal : somme des puissances de 2 (bits à 1)");
    powers_sum(bin, 2);
    outf("  = %lu", (unsigned long)v);
  } else {
    outf("Binaire : chaque chiffre -> %d bits", base == 16 ? 4 : 3);
    for (char *p = src; *p; p++) {
      char b[8];
      bits_n(strchr(DIGITS, *p) - DIGITS, base == 16 ? 4 : 3, b);
      outf("  %c -> %s", *p, b);
    }
    outf("Décimal : somme des puissances de %d", base);
    powers_sum(src, base);
    outf("  = %lu", (unsigned long)v);
  }
  if (base != 16) {
    outf("");
    outf("Hexadécimal : groupes de 4 bits (depuis la droite)");
    groups_table(bin, 4);
  }
  if (base != 8) {
    outf("");
    outf("Octal : groupes de 3 bits (depuis la droite)");
    groups_table(bin, 3);
  }
}

static bool read_bin(const char *s, char *d, uint32_t *v) {
  strip(s, d);
  if (strlen(d) > 32 || !parse_base(d, 2, v)) return false;
  char *p = d;
  while (p[0] == '0' && p[1]) p++; /* zéros de tête inutiles */
  memmove(d, p, strlen(p) + 1);
  return true;
}

void bin_op_explain(const char *sa, const char *sb, bool sub) {
  out_clear();
  char a[FIELDLEN], b[FIELDLEN];
  uint32_t va, vb;
  if (!read_bin(sa, a, &va) || !read_bin(sb, b, &vb)) {
    outf("Erreur : écris deux nombres binaires (que des 0 et des 1, 32 chiffres max).");
    return;
  }
  bool neg = false;
  if (sub && va < vb) {
    outf("%s < %s : on calcule %s - %s, puis on met un signe -.", a, b, b, a);
    outf("");
    char t[FIELDLEN];
    strcpy(t, a); strcpy(a, b); strcpy(b, t);
    uint32_t x = va; va = vb; vb = x;
    neg = true;
  }
  int la = strlen(a), lb = strlen(b), n = (la > lb ? la : lb) + 1;
  int carry[40] = {0}, r[40], c = 0;
  for (int i = 0; i < n; i++) {
    int x = i < la ? a[la - 1 - i] - '0' : 0, y = i < lb ? b[lb - 1 - i] - '0' : 0;
    if (!sub) { int s = x + y + c; r[i] = s & 1; c = s >> 1; }
    else { int d = x - y - c; c = d < 0; r[i] = d < 0 ? d + 2 : d; }
    carry[i + 1] = c;
  }
  char res[40], car[40];
  int j = 0;
  for (int i = n - 1; i >= 0; i--) if (j || r[i] || i == 0) res[j++] = '0' + r[i];
  res[j] = 0;
  for (int i = 0; i < n; i++) car[i] = carry[n - 1 - i] ? '1' : ' ';
  car[n] = 0;
  outf("Retenues   %s", car);
  outf("           %*s", n, a);
  outf("         %c %*s", sub ? '-' : '+', n, b);
  char dash[40];
  memset(dash, '-', n + 2);
  dash[n + 2] = 0;
  outf("         %s", dash);
  char rs[42];
  snprintf(rs, sizeof rs, "%s%s", neg ? "-" : "", res);
  outf("          %*s", n + 1, rs);
  outf("");
  if (!sub) outf("Règles : 0+0=0  0+1=1  1+1=10 (0, retenue 1)  1+1+1=11 (1, retenue 1)");
  else outf("Règles : 0-0=0  1-0=1  1-1=0  0-1 : on emprunte (10-1=1, retenue 1 sur la colonne suivante)");
  outf("");
  unsigned long vr = sub ? (unsigned long)(va - vb) : (unsigned long)va + vb;
  outf("Vérification en décimal :");
  if (neg) outf("  %lu - %lu = -%lu", (unsigned long)vb, (unsigned long)va, vr);
  else outf("  %lu %c %lu = %lu", (unsigned long)va, sub ? '-' : '+', (unsigned long)vb, vr);
}

void c2_encode_explain(const char *sval, const char *sbits) {
  out_clear();
  long x, n;
  if (!parse_long(sval, &x) || !parse_long(sbits, &n) || n < 2 || n > 32) {
    outf("Erreur : entre un entier et un nombre de bits entre 2 et 32.");
    return;
  }
  double lo = -pow(2, n - 1), hi = pow(2, n - 1) - 1;
  if (x < lo || x > hi) {
    outf("Impossible : sur %ld bits on code les entiers de %.0f à %.0f.", n, lo, hi);
    return;
  }
  uint32_t mask = n == 32 ? 0xFFFFFFFFu : (1u << n) - 1, enc = (uint32_t)x & mask;
  char b1[40], g1[48];
  outf("Complément à 2 de %ld sur %ld bits", x, n);
  outf("(on peut coder de %.0f à %.0f)", lo, hi);
  outf("");
  if (x >= 0) {
    outf("%ld est positif : on l'écrit en binaire", x);
    outf("sur %ld bits (zéros à gauche) :", n);
  } else {
    uint32_t ax = (uint32_t)(-(int64_t)x);
    bits_n(ax, n, b1); grouped(b1, 4, g1);
    outf("1) |%ld| = %lu en binaire sur %ld bits :", x, (unsigned long)ax, n);
    outf("     %s", g1);
    bits_n(~ax & mask, n, b1); grouped(b1, 4, g1);
    outf("2) On inverse tous les bits :");
    outf("     %s", g1);
    outf("3) On ajoute 1 :");
  }
  bits_n(enc, n, b1);
  grouped(b1, 4, g1);
  outf("     %s", g1);
  outf("");
  outf("Résultat : %s", g1);
  outf("Hexadécimal : %lX", (unsigned long)enc);
  outf("Bit de poids fort = %c -> nombre %s", b1[0], b1[0] == '1' ? "négatif" : "positif ou nul");
  if (x < 0) outf("Autre méthode : 2^%ld - %ld = %.0f", n, -x, pow(2, n) + x);
}

void c2_decode_explain(const char *sbits) {
  out_clear();
  char b[FIELDLEN];
  uint32_t v;
  strip(sbits, b);
  int n = strlen(b);
  if (n < 2 || n > 32 || !parse_base(b, 2, &v)) {
    outf("Erreur : entre de 2 à 32 bits (0 et 1). Le nombre de bits tapés = la taille du codage.");
    return;
  }
  char g[48];
  grouped(b, 4, g);
  outf("%s (%d bits, complément à 2)", g, n);
  outf("");
  if (b[0] == '0') {
    outf("Bit de poids fort = 0 -> positif :");
    outf("on lit le binaire normalement = %lu", (unsigned long)v);
    return;
  }
  uint32_t rest = v & ((1u << (n - 1)) - 1);
  long val = (long)((int64_t)rest - ((int64_t)1 << (n - 1)));
  outf("Bit de poids fort = 1 -> négatif.");
  outf("");
  outf("Méthode 1 : le bit de poids fort vaut -2^%d", n - 1);
  outf("  -%.0f + %lu = %ld", pow(2, n - 1), (unsigned long)rest, val);
  outf("");
  uint32_t mask = n == 32 ? 0xFFFFFFFFu : (1u << n) - 1, inv = ~v & mask;
  char t[40], tg[48];
  bits_n(inv, n, t);
  grouped(t, 4, tg);
  outf("Méthode 2 : inverser les bits puis ajouter 1");
  outf("  inversé : %s", tg);
  bits_n((inv + 1) & mask, n, t);
  grouped(t, 4, tg);
  outf("  + 1     : %s = %lu", tg, (unsigned long)((inv + 1) & mask));
  outf("  donc la valeur est -%lu", (unsigned long)((inv + 1) & mask));
  outf("");
  outf("Valeur : %ld", val);
}

void ieee_explain(const char *sval) {
  out_clear();
  double d;
  if (!parse_double(sval, &d)) {
    outf("Erreur : nombre invalide (ex : 0.1  -6.75  1e-3).");
    return;
  }
  float f = (float)d;
  uint32_t bits;
  memcpy(&bits, &f, 4);
  int s = bits >> 31, E = (bits >> 23) & 0xFF;
  uint32_t M = bits & 0x7FFFFF;
  char mb[24], eb[9];
  bits_n(M, 23, mb);
  bits_n(E, 8, eb);
  outf("IEEE 754 simple précision (32 bits)");
  outf("= 1 bit de signe + 8 d'exposant + 23 de mantisse");
  outf("");
  if (isinf(f)) {
    outf("%s est trop grand pour 32 bits (max ~3,4e38) : il devient l'infini.", sval);
  } else if (d == 0) {
    outf("0 est un cas spécial : exposant et mantisse à 0.");
  } else {
    outf("1) Signe : %s -> %d", s ? "négatif" : "positif", s);
    double a = fabs(d), ip = floor(a), fp = a - ip;
    outf("2) %s en binaire :", sval[0] == '-' ? sval + 1 : sval);
    if (ip < 4294967296.0) {
      char ib[40];
      to_base((uint32_t)ip, 2, ib);
      outf("   partie entière %.0f -> %s", ip, ib);
    } else {
      outf("   (partie entière trop grande pour le détail)");
    }
    if (fp > 0) {
      outf("   partie décimale : on multiplie par 2");
      for (int k = 0; k < 8 && fp > 0; k++) {
        double y = fp * 2;
        outf("   %.6g x 2 = %.6g -> %d", fp, y, y >= 1);
        fp = y >= 1 ? y - 1 : y;
      }
      if (fp > 0) outf("   ... (ça peut ne jamais s'arrêter !)");
    }
    if (E) {
      outf("3) On écrit |x| = 1,mantisse x 2^e :");
      outf("   1,%s x 2^%d", mb, E - 127);
      outf("4) Exposant stocké = e + 127 = %d", E);
      outf("   en binaire sur 8 bits : %s", eb);
      outf("5) Mantisse = les 23 bits après la virgule");
    } else {
      outf("3) Nombre très petit : il est « dénormalisé »");
      outf("   |x| = 0,mantisse x 2^-126 ; exposant = 00000000");
    }
  }
  outf("");
  outf("Résultat :");
  outf("  S | Exposant | Mantisse");
  outf("  %d | %s | %s", s, eb, mb);
  outf("  Hexadécimal : %08lX", (unsigned long)bits);
  if (!isinf(f)) {
    outf("");
    outf("Valeur réellement stockée :");
    outf("  %.17g", (double)f);
    if ((double)f == d) outf("Représentation exacte !");
    else outf("Erreur d'arrondi : %.3g -> %s n'est pas exactement représentable.", fabs((double)f - d), sval);
  }
}

/* ---------- Entraînement ---------- */

static void quiz(void) {
  static const char *const lab[] = {"Réponse"};
  int score = 0;
  for (int q = 0; q < 10; q++) {
    int type = rnd(5);
    uint32_t v = rnd(256), expect = v, got;
    int base = 10;
    char title[48], good[48], b[40], g[48];
    to_base(v, 2, b);
    grouped(b, 4, g);
    switch (type) {
      case 0: snprintf(title, sizeof title, "%lu en binaire ?", (unsigned long)v); base = 2; strcpy(good, g); break;
      case 1: snprintf(title, sizeof title, "%s en décimal ?", g); snprintf(good, sizeof good, "%lu", (unsigned long)v); break;
      case 2: snprintf(title, sizeof title, "%lu en hexa ?", (unsigned long)v); base = 16; to_base(v, 16, good); break;
      case 3: to_base(v, 16, b); snprintf(title, sizeof title, "%s(16) en décimal ?", b); snprintf(good, sizeof good, "%lu", (unsigned long)v); break;
      default: {
        long x = -1 - (long)rnd(128);
        expect = (uint32_t)x & 0xFF;
        base = 2;
        snprintf(title, sizeof title, "%ld en C2 8 bits ?", x);
        bits_n(expect, 8, b);
        grouped(b, 4, good);
      }
    }
    char head[64];
    snprintf(head, sizeof head, "%d/10  %s", q + 1, title);
    static char ans[1][FIELDLEN];
    ans[0][0] = 0;
    if (!ui_form(head, lab, ans, 1)) return;
    bool ok = parse_base(ans[0], base, &got) && got == expect;
    score += ok;
    out_clear();
    outf("Question : %s", title);
    if (ok) outf("Bonne réponse : %s", good);
    else { outf("Ta réponse : %s", ans[0]); outf("Bonne réponse : %s", good); }
    outf("");
    static const char *const tips[] = {
        "Astuce : divisions successives par 2, on lit les restes de bas en haut.",
        "Astuce : additionne les puissances de 2 des bits à 1 (128, 64, 32, 16, 8, 4, 2, 1).",
        "Astuce : passe par le binaire et fais des groupes de 4 bits.",
        "Astuce : premier chiffre x 16 + second chiffre (A=10 ... F=15).",
        "Astuce : |x| en binaire sur 8 bits, on inverse les bits, on ajoute 1."};
    if (!ok) outf("%s", tips[type]);
    outf("Score : %d / %d", score, q + 1);
    if (ui_text(ok ? "Bravo !" : "Raté...", "OK : suite") == eadk_event_back) return;
  }
  out_clear();
  outf("Score final : %d / 10", score);
  outf("");
  outf("%s", score == 10 ? "Parfait !" : score >= 7 ? "Très bien, encore un petit effort !" : "Continue à t'entraîner, ça va venir !");
  ui_text("Résultat", "");
}

void binaire_app(void) {
  static const char *const menu[] = {"Conversion de bases", "Addition / soustraction", "Complément à 2",
                                     "Flottants (IEEE 754)", "Entraînement"};
  static const char *const from[] = {"Depuis le décimal", "Depuis le binaire", "Depuis l'hexadécimal",
                                     "Depuis l'octal"};
  static const int bases[] = {10, 2, 16, 8};
  static const char *const ops[] = {"Addition", "Soustraction"};
  static const char *const c2m[] = {"Décimal -> binaire", "Binaire -> décimal"};
  static char conv[4][1][FIELDLEN] = {{"156"}, {"10011100"}, {"9C"}, {"234"}};
  static char op[2][FIELDLEN] = {"10110", "1011"};
  static char c2e[2][FIELDLEN] = {"-37", "8"};
  static char c2d[1][FIELDLEN] = {"11011011"};
  static char fl[1][FIELDLEN] = {"0.1"};
  static const char *const l_nombre[] = {"Nombre"};
  static const char *const l_op[] = {"Nombre 1", "Nombre 2"};
  static const char *const l_c2e[] = {"Entier", "Nb de bits"};
  static const char *const l_c2d[] = {"Bits"};
  for (;;) {
    int m = ui_menu("Binaire", menu, 5), k;
    if (m < 0) return;
    switch (m) {
      case 0:
        if ((k = ui_menu("Conversion", from, 4)) < 0) break;
        while (ui_form(from[k], l_nombre, conv[k], 1)) { bin_convert_explain(conv[k][0], bases[k]); ui_text("Conversion", ""); }
        break;
      case 1:
        if ((k = ui_menu("Opération posée", ops, 2)) < 0) break;
        while (ui_form(ops[k], l_op, op, 2)) { bin_op_explain(op[0], op[1], k == 1); ui_text(ops[k], ""); }
        break;
      case 2:
        if ((k = ui_menu("Complément à 2", c2m, 2)) < 0) break;
        if (k == 0) while (ui_form(c2m[0], l_c2e, c2e, 2)) { c2_encode_explain(c2e[0], c2e[1]); ui_text("Complément à 2", ""); }
        else while (ui_form(c2m[1], l_c2d, c2d, 1)) { c2_decode_explain(c2d[0]); ui_text("Complément à 2", ""); }
        break;
      case 3:
        while (ui_form("Flottant 32 bits", l_nombre, fl, 1)) { ieee_explain(fl[0]); ui_text("IEEE 754", ""); }
        break;
      case 4: quiz(); break;
    }
  }
}
