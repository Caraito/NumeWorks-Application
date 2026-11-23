#ifndef UI_H
#define UI_H

#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>

#define RGB(r, g, b) ((eadk_color_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_BG eadk_color_white
#define C_TEXT RGB(30, 30, 30)
#define C_HEAD RGB(255, 181, 49)
#define C_SEL RGB(255, 232, 186)
#define C_GRAY RGB(120, 120, 120)
#define C_GOOD RGB(0, 140, 60)
#define C_BAD RGB(200, 30, 30)

/* Petite police : 7x14 px, 45 colonnes. Grande : 10x18 px, 32 colonnes. */
#define COLS 45
#define MAXLINES 260
#define LINELEN 96 /* octets (UTF-8), >= COLS caractères */
#define FIELDLEN 64

extern char out[MAXLINES][LINELEN];
extern int nout;
extern bool g_quit;

/* Tampon de texte affiché par ui_text(). Les lignes trop longues sont coupées. */
void out_clear(void);
void outf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
bool out_has(const char *needle); /* pour les tests */

void ui_clear(void);
void ui_header(const char *title, const char *hint);
void ui_str(int x, int y, const char *s, bool large, eadk_color_t fg, eadk_color_t bg);
eadk_event_t ui_event(int32_t timeout_ms); /* 0xFFFF si rien */

int ui_menu(const char *title, const char *const items[], int n); /* -1 = retour */
bool ui_form(const char *title, const char *const labels[], char bufs[][FIELDLEN], int n);
eadk_event_t ui_text(const char *title, const char *hint); /* retour / OK / EXE */
void ui_message(const char *title, const char *msg);

void ui_big(int y, const char *s, eadk_color_t fg); /* chiffres 7 segments, centrés */

uint32_t rnd(uint32_t n); /* uniforme dans [0, n) */
bool parse_double(const char *s, double *v);
bool parse_long(const char *s, long *v);
void fmt_num(char *dst, double v); /* 4 décimales max, zéros inutiles retirés */

/* Les outils */
void chimie_app(void);
void maths_app(void);
void binaire_app(void);
void chrono_app(void);
void minuteur_app(void);
void aleatoire_app(void);
void batterie_app(void);

/* Parties « calcul » testables (remplissent out[]) */
bool chimie_explain(const char *reactifs, const char *produits);
bool avancement_explain(const char *reactifs, const char *produits, const char *quantites);
double masse_molaire(const char *formule, bool detail); /* < 0 si erreur */
void masse_explain(const char *formule);
void qte_explain(int relation, char f[4][FIELDLEN]);
void second_degre_explain(const char *a, const char *b, const char *c);
void suite_explain(bool geo, const char *u0, const char *raison, const char *n);
int pct_from_voltage(float v);
void bin_convert_explain(const char *s, int base);
void bin_op_explain(const char *a, const char *b, bool sub);
void c2_encode_explain(const char *val, const char *bits);
void c2_decode_explain(const char *bits);
void ieee_explain(const char *val);

#endif
