/*
 * Trabalho Final de Computacao Grafica - CEFET/RJ UnED Petropolis
 * Projeto 4: Simulador de Montagem e Construcao
 *
 * Compilar:  gcc montagem.c -o montagem -lglut -lGLU -lGL -lm
 * Teste da logica (sem janela):  gcc -DTESTE montagem.c -o teste -lm && ./teste
 *
 * Resumo da modelagem
 * -------------------
 *  - Cada peca tem POSE = (pos, R): pos = centro no mundo, R = matriz 3x3 de
 *    rotacao (coluna j = eixo local j expresso em coordenadas do mundo).
 *      ponto_mundo = R * ponto_local + pos
 *  - Camera orbital propria (yaw, pitch, distancia) + gluPerspective.
 *    O raio de picking e construido na mao a partir da mesma camera.
 *  - Selecao: raio do mouse x OBB (raio levado ao espaco local da peca).
 *  - Colisao: SAT entre caixas orientadas (OBB, 15 eixos). A caixa de cada peca
 *    e usada como volume de colisao (inclusive para o telhado prismatico).
 *  - Encaixe (snap): dist(pos, alvo) < EPS_POS  e  angulo(R, R_alvo) < EPS_ANG,
 *    levando em conta as SIMETRIAS da forma (uma barra girada 180 graus e igual).
 *  - Animacao: interpolacao suave ate o alvo, camera automatica na vitoria,
 *    "pulso" dos gabaritos.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef TESTE
#include <GL/glut.h>
#endif

#define PI 3.14159265358979f
#define MAXP 16

/* ---- tolerancias geometricas (criterio de "posicionada corretamente") ---- */
#define EPS_POS   0.60f   /* distancia maxima do centro ao alvo para encaixar   */
#define EPS_ANG   20.0f   /* diferenca angular maxima (graus) para encaixar     */
#define EPS_PROX  1.60f   /* distancia a partir da qual o gabarito acende       */
#define EPS_COL   0.003f  /* folga: encostar nao e colidir                      */
#define EPS_FINAL 0.01f   /* verificacao final da estrutura (pos) e 0.5 graus   */
#define LIM       14.0f   /* a mesa/chao vai de -LIM a +LIM                     */
#define FOV       50.0f

/* ================================ MATEMATICA ============================== */
typedef struct { float x, y, z; } V3;
typedef struct { float m[3][3]; } M3;

static V3 v3(float x, float y, float z) { V3 r = {x, y, z}; return r; }
static V3 vadd(V3 a, V3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static V3 vsub(V3 a, V3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static V3 vmul(V3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static float vdot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static V3 vcross(V3 a, V3 b) { return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static float vlen(V3 a) { return sqrtf(vdot(a, a)); }
static V3 vnorm(V3 a) { float l = vlen(a); return l > 1e-9f ? vmul(a, 1.0f / l) : a; }

static M3 m3_id(void) {
    M3 r; memset(&r, 0, sizeof r);
    r.m[0][0] = r.m[1][1] = r.m[2][2] = 1.0f;
    return r;
}

static M3 m3_mul(M3 a, M3 b) {
    M3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
    return r;
}

static M3 m3_T(M3 a) {
    M3 r;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i][j] = a.m[j][i];
    return r;
}

static V3 m3_apply(M3 a, V3 v) {
    return v3(a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z,
              a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z,
              a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z);
}

static V3 m3_col(M3 a, int j) { return v3(a.m[0][j], a.m[1][j], a.m[2][j]); }

/* rotacao de 'graus' em torno do eixo 0=X, 1=Y, 2=Z */
static M3 m3_rot(int eixo, float graus) {
    float a = graus * PI / 180.0f, c = cosf(a), s = sinf(a);
    M3 r = m3_id();
    if (eixo == 0) { r.m[1][1] = c; r.m[1][2] = -s; r.m[2][1] = s; r.m[2][2] = c; }
    if (eixo == 1) { r.m[0][0] = c; r.m[0][2] = s; r.m[2][0] = -s; r.m[2][2] = c; }
    if (eixo == 2) { r.m[0][0] = c; r.m[0][1] = -s; r.m[1][0] = s; r.m[1][1] = c; }
    return r;
}

/* Gram-Schmidt nas colunas: evita acumulo de erro numerico apos muitas rotacoes */
static M3 m3_ortho(M3 a) {
    V3 c0 = vnorm(m3_col(a, 0));
    V3 c1 = m3_col(a, 1);
    c1 = vnorm(vsub(c1, vmul(c0, vdot(c1, c0))));
    V3 c2 = vcross(c0, c1);
    M3 r;
    r.m[0][0] = c0.x; r.m[1][0] = c0.y; r.m[2][0] = c0.z;
    r.m[0][1] = c1.x; r.m[1][1] = c1.y; r.m[2][1] = c1.z;
    r.m[0][2] = c2.x; r.m[1][2] = c2.y; r.m[2][2] = c2.z;
    return r;
}

/* angulo (graus) da rotacao relativa a * b^T :  cos(theta) = (tr - 1) / 2 */
static float m3_angulo(M3 a, M3 b) {
    M3 rel = m3_mul(a, m3_T(b));
    float c = (rel.m[0][0] + rel.m[1][1] + rel.m[2][2] - 1.0f) * 0.5f;
    if (c > 1) c = 1;
    if (c < -1) c = -1;
    return acosf(c) * 180.0f / PI;
}

/* interpolacao normalizada de matrizes de rotacao (suficiente p/ angulos pequenos) */
static M3 m3_lerp(M3 a, M3 b, float t) {
    M3 r;
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++)
        r.m[i][j] = a.m[i][j] * (1 - t) + b.m[i][j] * t;
    return m3_ortho(r);
}

/* ================================== FORMAS ================================ */
enum { T_CAIXA, T_PRISMA };
enum { F_LAJE, F_PAREDE, F_VIGA, F_TELHADO, F_SAPATA, F_PILAR, F_LINTEL, F_BLOCO,
       F_TOPO, NFORMAS };

typedef struct {
    const char *nome;
    int tipo;
    V3 h;              /* semi-dimensoes (half extents) no espaco local */
    float cor[3];
    int nsim;          /* simetrias rotacionais da forma (matrizes locais) */
    M3 sim[4];
} Forma;

static Forma formas[NFORMAS];

static void def_forma(int id, const char *nome, int tipo, float hx, float hy, float hz,
                      float r, float g, float b) {
                        
    Forma *f = &formas[id];
    f->nome = nome; f->tipo = tipo; f->h = v3(hx, hy, hz);
    f->cor[0] = r; f->cor[1] = g; f->cor[2] = b;
    f->sim[0] = m3_id();
    if (tipo == T_CAIXA) {            /* caixa com 3 dimensoes distintas: grupo de Klein */
        f->nsim = 4;
        f->sim[1] = m3_rot(0, 180); f->sim[2] = m3_rot(1, 180); f->sim[3] = m3_rot(2, 180);
    } else {                          /* prisma triangular isosceles: giro de 180 em Y */
        f->nsim = 2;
        f->sim[1] = m3_rot(1, 180);
    }
}

static void init_formas(void) {
    /* nivel 1 - Casinha */
    def_forma(F_LAJE,    "Laje",      T_CAIXA,  2.00f, 0.20f, 1.50f, 0.62f, 0.62f, 0.66f);
    def_forma(F_PAREDE,  "Parede",    T_CAIXA,  0.25f, 1.00f, 1.50f, 0.85f, 0.55f, 0.35f);
    def_forma(F_VIGA,    "Viga",      T_CAIXA,  1.75f, 0.20f, 1.50f, 0.55f, 0.40f, 0.30f);
    def_forma(F_TELHADO, "Telhado",   T_PRISMA, 2.20f, 0.60f, 1.70f, 0.75f, 0.25f, 0.25f);
    /* nivel 2 - Portico */
    def_forma(F_SAPATA,  "Sapata",    T_CAIXA,  0.60f, 0.20f, 0.60f, 0.55f, 0.55f, 0.60f);
    def_forma(F_PILAR,   "Pilar",     T_CAIXA,  0.30f, 1.20f, 0.30f, 0.80f, 0.75f, 0.60f);
    def_forma(F_LINTEL,  "Verga",     T_CAIXA,  2.10f, 0.25f, 0.50f, 0.45f, 0.55f, 0.80f);
    def_forma(F_BLOCO,   "Bloco",     T_CAIXA,  0.50f, 0.40f, 0.30f, 0.35f, 0.70f, 0.45f);
    def_forma(F_TOPO,    "Pinaculo",  T_PRISMA, 1.00f, 0.50f, 0.60f, 0.85f, 0.70f, 0.20f);
}

static float meia_altura(int forma, M3 R) {   /* extensao vertical da OBB */
    V3 h = formas[forma].h;
    return fabsf(R.m[1][0]) * h.x + fabsf(R.m[1][1]) * h.y + fabsf(R.m[1][2]) * h.z;
}

/* menor angulo entre R (com simetrias da forma) e o alvo Rt; devolve simetria usada */
static float ang_sim(int forma, M3 R, M3 Rt, int *sim) {
    float best = 1e9f; int bi = 0;
    for (int s = 0; s < formas[forma].nsim; s++) {
        float a = m3_angulo(m3_mul(R, formas[forma].sim[s]), Rt);
        if (a < best) { best = a; bi = s; }
    }
    if (sim) *sim = bi;
    return best;
}

/* ================================== CENA ================================== */
typedef struct {
    int forma;
    V3 pos; M3 R;
    int travada;                 /* encaixada em um alvo */
    int alvo;                    /* indice do alvo ocupado (ou -1) */
    float invalida;              /* tempo restante do flash vermelho */
    int anim; float at;          /* animacao de encaixe */
    V3 p0, p1; M3 R0, R1;
} Peca;

typedef struct { int forma; V3 pos; M3 R; int ocupado; } Alvo;

static Peca pecas[MAXP]; static int npecas;
static Alvo alvos[MAXP]; static int nalvos;
static int nivel = 0, sel = -1, drag = -1;
static int vitoria = 0, encaixes = 0;
static float tempo = 0, tglobal = 0;
static char msg[128] = ""; static float msg_t = 0;
static int mostra_gabarito = 1;
static const char *nome_nivel[2] = {"Nivel 1 - Casinha", "Nivel 2 - Portico"};

static void mensagem(const char *s) { strncpy(msg, s, sizeof msg - 1); msg[sizeof msg - 1] = 0; msg_t = 3.0f; }

/* ============================== COLISAO (OBB/SAT) ========================= */
static int obb_colide(V3 ca, M3 ra, V3 ha, V3 cb, M3 rb, V3 hb) {
    V3 A[3], B[3], axes[15]; int n = 0;
    for (int j = 0; j < 3; j++) { A[j] = m3_col(ra, j); B[j] = m3_col(rb, j); }
    V3 T = vsub(cb, ca);
    for (int i = 0; i < 3; i++) axes[n++] = A[i];
    for (int i = 0; i < 3; i++) axes[n++] = B[i];
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) axes[n++] = vcross(A[i], B[j]);
    for (int k = 0; k < 15; k++) {
        float l = vlen(axes[k]);
        if (l < 1e-4f) continue;                       /* eixos paralelos: ignora */
        V3 L = vmul(axes[k], 1.0f / l);
        float rA = fabsf(vdot(L, A[0])) * ha.x + fabsf(vdot(L, A[1])) * ha.y + fabsf(vdot(L, A[2])) * ha.z;
        float rB = fabsf(vdot(L, B[0])) * hb.x + fabsf(vdot(L, B[1])) * hb.y + fabsf(vdot(L, B[2])) * hb.z;
        if (fabsf(vdot(T, L)) > rA + rB - EPS_COL) return 0;   /* eixo separador */
    }
    return 1;
}
/* devolve o indice da primeira peca (diferente de i) que colide com a pose dada, ou -1 */
static int colide_com_outros(int i, V3 pos, M3 R) {
    for (int j = 0; j < npecas; j++) {
        if (j == i) continue;
        if (obb_colide(pos, R, formas[pecas[i].forma].h,
                       pecas[j].pos, pecas[j].R, formas[pecas[j].forma].h)) return j;
    }
    return -1;
}

/* ================================ MOVIMENTO =============================== */
static int pose_valida(int i, V3 p, M3 R) { return colide_com_outros(i, p, R) < 0; }

/* Move a peca em direcao a 'np' em passos pequenos (evita "atravessar" outras pecas)
   e, se bater, tenta deslizar ao longo dos eixos livres. */
static void mover_peca(int i, V3 np) {
    Peca *p = &pecas[i];
    float mh = meia_altura(p->forma, p->R);
    if (np.y < mh) np.y = mh;                               /* nao entra no chao */
    if (np.x > LIM) np.x = LIM;
    if (np.x < -LIM) np.x = -LIM;
    if (np.z > LIM) np.z = LIM;
    if (np.z < -LIM) np.z = -LIM;
    V3 d = vsub(np, p->pos);
    int n = (int)ceilf(vlen(d) / 0.2f);
    if (n < 1) n = 1;
    V3 st = vmul(d, 1.0f / n);
    int bloqueou = 0;
    for (int k = 0; k < n; k++) {
        V3 c = p->pos, t = vadd(c, st);
        V3 cand[4] = { t, v3(t.x, t.y, c.z), v3(c.x, t.y, t.z), v3(c.x, t.y, c.z) };
        int ok = 0;
        for (int q = 0; q < 4; q++) {
            if (pose_valida(i, cand[q], p->R)) { p->pos = cand[q]; ok = 1; if (q) bloqueou = 1; break; }
        }
        if (!ok) { bloqueou = 1; break; }
    }
    if (bloqueou) p->invalida = 0.30f;                       /* sinaliza posicao invalida */
}

/* alvo livre mais proximo (mesma forma). Preenche distancia, angulo e simetria. */
static int melhor_alvo(int i, float *dist, float *ang, int *sim) {
    Peca *p = &pecas[i];
    int best = -1; float bd = 1e9f;
    for (int a = 0; a < nalvos; a++) {
        if (alvos[a].forma != p->forma) continue;
        if (alvos[a].ocupado >= 0 && alvos[a].ocupado != i) continue;
        float d = vlen(vsub(p->pos, alvos[a].pos));
        if (d < bd) { bd = d; best = a; }
    }
    if (best >= 0) { *dist = bd; *ang = ang_sim(p->forma, p->R, alvos[best].R, sim); }
    return best;
}

static int tentar_snap(int i) {
    Peca *p = &pecas[i];
    if (p->travada) return 0;
    float d, a; int s;
    int al = melhor_alvo(i, &d, &a, &s);
    if (al < 0 || d >= EPS_POS || a >= EPS_ANG) return 0;
    Alvo *t = &alvos[al];
    if (colide_com_outros(i, t->pos, t->R) >= 0) {
        p->invalida = 0.30f; mensagem("Encaixe bloqueado: ha outra peca no lugar");
        return 0;
    }
    p->p0 = p->pos; p->R0 = m3_mul(p->R, formas[p->forma].sim[s]);  /* equivalente visual */
    p->p1 = t->pos; p->R1 = t->R;
    p->anim = 1; p->at = 0; p->travada = 1; p->alvo = al; t->ocupado = i;
    encaixes++;
    mensagem("Encaixou!");
    return 1;
}

static void rotacionar(int i, int eixo, float graus) {
    Peca *p = &pecas[i];
    if (p->travada) { mensagem("Peca encaixada (U para soltar)"); return; }
    M3 nr = m3_ortho(m3_mul(m3_rot(eixo, graus), p->R));    /* rotacao em torno de eixo do MUNDO */
    V3 np = p->pos;
    float mh = meia_altura(p->forma, nr);
    if (np.y < mh) np.y = mh;
    if (colide_com_outros(i, np, nr) >= 0) {
        p->invalida = 0.30f; mensagem("Rotacao bloqueada por colisao: afaste/levante a peca");
        return;
    }
    p->R = nr; p->pos = np;
    tentar_snap(i);
}

static void verificar_vitoria(void) {
    if (vitoria || nalvos == 0) return;
    for (int a = 0; a < nalvos; a++) {
        int i = alvos[a].ocupado;
        if (i < 0 || pecas[i].anim) return;
        if (vlen(vsub(pecas[i].pos, alvos[a].pos)) > EPS_FINAL) return;
        if (ang_sim(pecas[i].forma, pecas[i].R, alvos[a].R, NULL) > 0.5f) return;
    }
    vitoria = 1;
    mensagem("ESTRUTURA CONCLUIDA E VERIFICADA!");
}

static void atualizar(float dt) {
    tglobal += dt;
    if (!vitoria) tempo += dt;
    if (msg_t > 0) msg_t -= dt;
    for (int i = 0; i < npecas; i++) {
        Peca *p = &pecas[i];
        if (p->invalida > 0) p->invalida -= dt;
        if (p->anim) {
            p->at += dt / 0.30f;
            float t = p->at >= 1 ? 1 : p->at;
            float e = t * t * (3 - 2 * t);                    /* smoothstep */
            p->pos = vadd(vmul(p->p0, 1 - e), vmul(p->p1, e));
            p->R = m3_lerp(p->R0, p->R1, e);
            if (p->at >= 1) { p->anim = 0; p->pos = p->p1; p->R = p->R1; }
        }
    }
    verificar_vitoria();
}

/* ================================== NIVEIS ================================ */
static unsigned rng = 12345u;
static int rnd(int n) { rng = rng * 1664525u + 1013904223u; return (int)((rng >> 16) % (unsigned)n); }

static void add_alvo(int f, float x, float y, float z) {
    alvos[nalvos].forma = f; alvos[nalvos].pos = v3(x, y, z);
    alvos[nalvos].R = m3_id(); alvos[nalvos].ocupado = -1; nalvos++;
}
static void add_peca(int f) {
    Peca *p = &pecas[npecas++];
    memset(p, 0, sizeof *p);
    p->forma = f; p->R = m3_id(); p->alvo = -1;
}

/* espalha as pecas num anel, em orientacoes embaralhadas (e sem colisao) */
static void espalhar(void) {
    for (int i = 0; i < npecas; i++) {
        Peca *p = &pecas[i];
        M3 R;
        for (int tent = 0; tent < 50; tent++) {
            R = m3_rot(1, 15.0f * rnd(24));
            int tilt = rnd(3);
            if (tilt == 1) R = m3_mul(R, m3_rot(0, 90));
            if (tilt == 2) R = m3_mul(R, m3_rot(2, 90));
            if (ang_sim(p->forma, R, m3_id(), NULL) > 30.0f) break;   /* nao pode ja estar certa */
        }
        p->R = R;
        float ang = 2 * PI * i / npecas + 0.5f, raio = 6.0f + (i % 2) * 1.6f;
        for (int t = 0; t < 40; t++, raio += 0.7f) {
            p->pos = v3(raio * cosf(ang), meia_altura(p->forma, R), raio * sinf(ang));
            if (fabsf(p->pos.x) < LIM && fabsf(p->pos.z) < LIM && colide_com_outros(i, p->pos, R) < 0) break;
        }
    }
}

static void carregar_nivel(int n) {
    nivel = n; npecas = nalvos = 0; sel = drag = -1; vitoria = encaixes = 0; tempo = 0;
    rng = 12345u + 77u * n;
    if (n == 0) {
        add_alvo(F_LAJE,    0.00f, 0.20f, 0);
        add_alvo(F_PAREDE, -1.75f, 1.40f, 0);
        add_alvo(F_PAREDE,  1.75f, 1.40f, 0);
        add_alvo(F_VIGA,    0.00f, 2.60f, 0);
        add_alvo(F_TELHADO, 0.00f, 3.40f, 0);
    } else {
        add_alvo(F_SAPATA, -1.50f, 0.20f, 0);
        add_alvo(F_SAPATA,  1.50f, 0.20f, 0);
        add_alvo(F_PILAR,  -1.50f, 1.60f, 0);
        add_alvo(F_PILAR,   1.50f, 1.60f, 0);
        add_alvo(F_LINTEL,  0.00f, 3.05f, 0);
        add_alvo(F_BLOCO,   0.00f, 3.70f, 0);
        add_alvo(F_TOPO,    0.00f, 4.60f, 0);
    }
    for (int a = 0; a < nalvos; a++) add_peca(alvos[a].forma);
    espalhar();
    mensagem(n == 0 ? "Monte a casinha seguindo o gabarito" : "Monte o portico seguindo o gabarito");
}

/* ================================== CAMERA ================================ */
static float cam_yaw = 35, cam_pitch = 32, cam_dist = 17;
static V3 cam_tgt = {0, 1.8f, 0};
static int W = 1100, H = 720;

static V3 camera_eye(void) {
    float y = cam_yaw * PI / 180, p = cam_pitch * PI / 180;
    return vadd(cam_tgt, v3(cam_dist * cosf(p) * sinf(y), cam_dist * sinf(p), cam_dist * cosf(p) * cosf(y)));
}
static void camera_reset(void) { cam_yaw = 35; cam_pitch = 32; cam_dist = 17; cam_tgt = v3(0, 1.8f, 0); }

/* raio do pixel (mx,my) no mundo: mesma camera usada para desenhar */
static void raio_do_mouse(int mx, int my, V3 *o, V3 *d) {
    V3 eye = camera_eye();
    V3 f = vnorm(vsub(cam_tgt, eye));
    V3 r = vnorm(vcross(f, v3(0, 1, 0)));
    V3 u = vcross(r, f);
    float t = tanf(FOV * 0.5f * PI / 180), asp = (float)W / (float)H;
    float nx = 2.0f * mx / W - 1.0f, ny = 1.0f - 2.0f * my / H;
    *o = eye;
    *d = vnorm(vadd(f, vadd(vmul(r, nx * asp * t), vmul(u, ny * t))));
}

/* raio x OBB (metodo das "slabs" no espaco local da peca). -1 se nao acertar */
static float raio_obb(V3 o, V3 d, V3 c, M3 R, V3 h) {
    M3 Rt = m3_T(R);
    V3 ol = m3_apply(Rt, vsub(o, c)), dl = m3_apply(Rt, d);
    float oo[3] = {ol.x, ol.y, ol.z}, dd[3] = {dl.x, dl.y, dl.z}, hh[3] = {h.x, h.y, h.z};
    float tmin = 0, tmax = 1e9f;
    for (int k = 0; k < 3; k++) {
        if (fabsf(dd[k]) < 1e-8f) { if (fabsf(oo[k]) > hh[k]) return -1; continue; }
        float t1 = (-hh[k] - oo[k]) / dd[k], t2 = (hh[k] - oo[k]) / dd[k];
        if (t1 > t2) { float tt = t1; t1 = t2; t2 = tt; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax) return -1;
    }
    return tmin;
}
static int pickar(int mx, int my) {
    V3 o, d; raio_do_mouse(mx, my, &o, &d);
    int best = -1; float bt = 1e9f;
    for (int i = 0; i < npecas; i++) {
        float t = raio_obb(o, d, pecas[i].pos, pecas[i].R, formas[pecas[i].forma].h);
        if (t >= 0 && t < bt) { bt = t; best = i; }
    }
    return best;
}

/* ================================ RENDERIZACAO ============================ */
#ifndef TESTE
static void aplica_modelo(V3 p, M3 R) {
    float m[16] = { R.m[0][0], R.m[1][0], R.m[2][0], 0,
                    R.m[0][1], R.m[1][1], R.m[2][1], 0,
                    R.m[0][2], R.m[1][2], R.m[2][2], 0,
                    p.x, p.y, p.z, 1 };
    glMultMatrixf(m);
}

static void v(float x, float y, float z) { glVertex3f(x, y, z); }

static void desenha_solido(const Forma *f) {
    float x = f->h.x, y = f->h.y, z = f->h.z;
    if (f->tipo == T_CAIXA) {
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);  v(-x, -y, z); v(x, -y, z); v(x, y, z); v(-x, y, z);
        glNormal3f(0, 0, -1); v(x, -y, -z); v(-x, -y, -z); v(-x, y, -z); v(x, y, -z);
        glNormal3f(1, 0, 0);  v(x, -y, z); v(x, -y, -z); v(x, y, -z); v(x, y, z);
        glNormal3f(-1, 0, 0); v(-x, -y, -z); v(-x, -y, z); v(-x, y, z); v(-x, y, -z);
        glNormal3f(0, 1, 0);  v(-x, y, z); v(x, y, z); v(x, y, -z); v(-x, y, -z);
        glNormal3f(0, -1, 0); v(-x, -y, -z); v(x, -y, -z); v(x, -y, z); v(-x, -y, z);
        glEnd();
    } else {                                   /* prisma triangular (secao no plano XY) */
        float l = sqrtf(4 * y * y + x * x), nx = 2 * y / l, ny = x / l;
        glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);  v(-x, -y, z); v(x, -y, z); v(0, y, z);
        glNormal3f(0, 0, -1); v(x, -y, -z); v(-x, -y, -z); v(0, y, -z);
        glEnd();
        glBegin(GL_QUADS);
        glNormal3f(0, -1, 0); v(-x, -y, -z); v(x, -y, -z); v(x, -y, z); v(-x, -y, z);
        glNormal3f(nx, ny, 0);  v(x, -y, z); v(x, -y, -z); v(0, y, -z); v(0, y, z);
        glNormal3f(-nx, ny, 0); v(-x, -y, -z); v(-x, -y, z); v(0, y, z); v(0, y, -z);
        glEnd();
    }
}
static void desenha_arestas(const Forma *f) {
    float x = f->h.x, y = f->h.y, z = f->h.z;
    if (f->tipo == T_CAIXA) {
        glBegin(GL_LINE_LOOP); v(-x, -y, z); v(x, -y, z); v(x, y, z); v(-x, y, z); glEnd();
        glBegin(GL_LINE_LOOP); v(-x, -y, -z); v(x, -y, -z); v(x, y, -z); v(-x, y, -z); glEnd();
        glBegin(GL_LINES);
        v(-x, -y, -z); v(-x, -y, z); v(x, -y, -z); v(x, -y, z);
        v(x, y, -z); v(x, y, z); v(-x, y, -z); v(-x, y, z);
        glEnd();
    } else {
        glBegin(GL_LINE_LOOP); v(-x, -y, z); v(x, -y, z); v(0, y, z); glEnd();
        glBegin(GL_LINE_LOOP); v(-x, -y, -z); v(x, -y, -z); v(0, y, -z); glEnd();
        glBegin(GL_LINES);
        v(-x, -y, -z); v(-x, -y, z); v(x, -y, -z); v(x, -y, z); v(0, y, -z); v(0, y, z);
        glEnd();
    }
}

static void texto(float x, float y, const char *s, void *fonte) {
    glRasterPos2f(x, y);
    for (; *s; s++) glutBitmapCharacter(fonte, (unsigned char)*s);
}

static const V3 LUZ = {0.45f, 1.0f, 0.30f};   /* direcao PARA a luz (direcional) */

static void desenha_hud(void) {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, W, 0, H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    char b[200];
    glColor3f(1, 1, 1);
    sprintf(b, "%s   |   Encaixes: %d/%d   |   Tempo: %02d:%02d", nome_nivel[nivel],
            encaixes, nalvos, (int)tempo / 60, (int)tempo % 60);
    texto(12, H - 22, b, GLUT_BITMAP_HELVETICA_18);

    if (sel >= 0) {
        Peca *p = &pecas[sel];
        float d = 0, a = 0; int s = 0;
        int al = p->travada ? -1 : melhor_alvo(sel, &d, &a, &s);
        if (p->travada) sprintf(b, "Selecionada: %s  (encaixada - U para soltar)", formas[p->forma].nome);
        else if (al >= 0) sprintf(b, "Selecionada: %s   dist ao alvo %.2f (<%.2f)   angulo %.1f (<%.0f)",
                                  formas[p->forma].nome, d, EPS_POS, a, EPS_ANG);
        else sprintf(b, "Selecionada: %s", formas[p->forma].nome);
        glColor3f(1, 0.9f, 0.3f);
        texto(12, H - 44, b, GLUT_BITMAP_HELVETICA_12);
    }
    if (msg_t > 0) { glColor3f(0.6f, 1, 0.6f); texto(12, H - 68, msg, GLUT_BITMAP_HELVETICA_18); }

    glColor3f(0.92f, 0.92f, 0.92f);
    const char *ajuda[] = {
        "Mouse esq: selecionar/arrastar peca (no plano)  |  Shift+arrastar: sobe/desce  |  Dir/Esq vazio: orbitar  |  Roda: zoom",
        "Q/E: gira Y   A/D: gira X   Z/C: gira Z (15 graus)   |   Setas: empurra peca   PgUp/PgDn: altura   Tab: proxima peca",
        "U: soltar peca encaixada   H: gabarito   R: reiniciar   1/2: nivel   Espaco: resetar camera   Esc: sair" };
    for (int i = 0; i < 3; i++) texto(12, 10 + (2 - i) * 16, ajuda[i], GLUT_BITMAP_HELVETICA_12);

    if (vitoria) {
        glColor3f(1, 0.95f, 0.4f);
        texto(W / 2 - 170, H / 2 + 120, "ESTRUTURA CONCLUIDA!", GLUT_BITMAP_TIMES_ROMAN_24);
        sprintf(b, "Tempo: %02d:%02d  -  R reinicia, 1/2 troca de nivel", (int)tempo / 60, (int)tempo % 60);
        glColor3f(1, 1, 1);
        texto(W / 2 - 170, H / 2 + 95, b, GLUT_BITMAP_HELVETICA_18);
    }
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

static void display(void) {
    glClearColor(0.60f, 0.76f, 0.92f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, W, H);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(FOV, (double)W / H, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    V3 e = camera_eye();
    gluLookAt(e.x, e.y, e.z, cam_tgt.x, cam_tgt.y, cam_tgt.z, 0, 1, 0);

    /* iluminacao: uma luz direcional (w=0) + ambiente */
    float lp[4] = {LUZ.x, LUZ.y, LUZ.z, 0}, amb[4] = {0.32f, 0.32f, 0.36f, 1}, dif[4] = {0.9f, 0.88f, 0.82f, 1};
    float esp[4] = {0.25f, 0.25f, 0.25f, 1};
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_NORMALIZE); glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glLightfv(GL_LIGHT0, GL_POSITION, lp); glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, esp); glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24);

    /* chao */
    glColor3f(0.55f, 0.62f, 0.50f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0); v(-LIM - 2, 0, -LIM - 2); v(-LIM - 2, 0, LIM + 2); v(LIM + 2, 0, LIM + 2); v(LIM + 2, 0, -LIM - 2);
    glEnd();
    glDisable(GL_LIGHTING);
    glColor3f(0.45f, 0.52f, 0.41f);
    glBegin(GL_LINES);
    for (int i = -(int)LIM; i <= (int)LIM; i += 2) {
        v((float)i, 0.01f, -LIM); v((float)i, 0.01f, LIM);
        v(-LIM, 0.01f, (float)i); v(LIM, 0.01f, (float)i);
    }
    glEnd();
    glEnable(GL_LIGHTING);

    /* sombras planares projetadas no chao (luz direcional) */
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    float a = LUZ.x / LUZ.y, c = LUZ.z / LUZ.y;
    float sm[16] = { 1, 0, 0, 0,   -a, 0, -c, 0,   0, 0, 1, 0,   0, 0.02f, 0, 1 };
    glColor4f(0, 0, 0, 0.28f);
    for (int i = 0; i < npecas; i++) {
        glPushMatrix(); glMultMatrixf(sm); aplica_modelo(pecas[i].pos, pecas[i].R);
        desenha_solido(&formas[pecas[i].forma]); glPopMatrix();
    }
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    /* pecas */
    glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1, 1);
    for (int i = 0; i < npecas; i++) {
        Peca *p = &pecas[i]; Forma *f = &formas[p->forma];
        float col[3] = {f->cor[0], f->cor[1], f->cor[2]};
        if (p->travada) { col[0] = col[0] * 0.55f + 0.1f; col[1] = col[1] * 0.55f + 0.42f; col[2] = col[2] * 0.55f + 0.1f; }
        if (p->invalida > 0) { col[0] = 0.95f; col[1] = 0.15f; col[2] = 0.15f; }
        if (i == sel) { for (int k = 0; k < 3; k++) { col[k] += 0.18f; if (col[k] > 1) col[k] = 1; } }
        glPushMatrix(); aplica_modelo(p->pos, p->R);
        glColor3fv(col); desenha_solido(f);
        glPopMatrix();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LIGHTING);
    for (int i = 0; i < npecas; i++) {                       /* arestas */
        Peca *p = &pecas[i];
        glPushMatrix(); aplica_modelo(p->pos, p->R);
        if (i == sel) { glLineWidth(3.5f); glColor3f(1, 0.9f, 0.1f); }
        else { glLineWidth(1.3f); glColor3f(0.1f, 0.1f, 0.12f); }
        desenha_arestas(&formas[p->forma]);
        glPopMatrix();
    }

    /* gabaritos (alvos livres) - translucidos, com destaque quando a peca esta perto */
    if (mostra_gabarito) {
        int destaque = -1, bom = 0;
        if (sel >= 0 && !pecas[sel].travada) {
            float d, an; int s;
            int al = melhor_alvo(sel, &d, &an, &s);
            if (al >= 0 && d < EPS_PROX) { destaque = al; bom = (d < EPS_POS && an < EPS_ANG); }
        }
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
        float pulso = 0.5f + 0.5f * sinf(tglobal * 3.0f);
        for (int k = 0; k < nalvos; k++) {
            if (alvos[k].ocupado >= 0) continue;
            float r = 0.75f, g = 0.85f, b = 1.0f, al = 0.10f + 0.06f * pulso;
            if (k == destaque) {
                if (bom) { r = 0.2f; g = 1; b = 0.3f; } else { r = 1; g = 0.85f; b = 0.1f; }
                al = 0.30f;
            }
            glPushMatrix(); aplica_modelo(alvos[k].pos, alvos[k].R);
            glColor4f(r, g, b, al); desenha_solido(&formas[alvos[k].forma]);
            glLineWidth(2); glColor4f(r, g, b, 0.9f); desenha_arestas(&formas[alvos[k].forma]);
            glPopMatrix();
        }
        glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    }
    glLineWidth(1);
    desenha_hud();
    glutSwapBuffers();
}

static void reshape(int w, int h) { W = w; H = h > 0 ? h : 1; }

static int ultimo = 0;
static void timer(int vv) {
    (void)vv;
    int t = glutGet(GLUT_ELAPSED_TIME);
    float dt = (t - ultimo) / 1000.0f; ultimo = t;
    if (dt > 0.1f) dt = 0.1f;
    atualizar(dt);
    if (vitoria) cam_yaw += 18.0f * dt;                       /* camera gira na vitoria */
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

/* ================================== ENTRADA =============================== */
enum { M_NADA, M_ARRASTA, M_ORBITA, M_PAN };
static int modo = M_NADA, lx, ly, vertical = 0;
static float off_x, off_z;

static void mouse(int botao, int estado, int x, int y) {
    if (botao == 3 || botao == 4) {                           /* roda */
        if (estado == GLUT_DOWN) { cam_dist *= (botao == 3) ? 0.92f : 1.09f; if (cam_dist < 4) cam_dist = 4; if (cam_dist > 45) cam_dist = 45; }
        return;
    }
    lx = x; ly = y;
    if (estado == GLUT_UP) {
        if (modo == M_ARRASTA && drag >= 0) tentar_snap(drag);   /* soltou: tenta encaixar */
        modo = M_NADA; drag = -1;
        return;
    }
    if (botao == GLUT_LEFT_BUTTON) {
        int i = pickar(x, y);
        sel = i;
        if (i >= 0) {
            if (pecas[i].travada) { mensagem("Peca encaixada (U para soltar)"); modo = M_NADA; return; }
            vertical = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
            V3 o, d; raio_do_mouse(x, y, &o, &d);
            if (fabsf(d.y) > 1e-6f) {
                float t = (pecas[i].pos.y - o.y) / d.y;
                V3 hit = vadd(o, vmul(d, t));
                off_x = pecas[i].pos.x - hit.x; off_z = pecas[i].pos.z - hit.z;
            } else { off_x = off_z = 0; }
            drag = i; modo = M_ARRASTA;
        } else modo = M_ORBITA;
    } else if (botao == GLUT_RIGHT_BUTTON) modo = M_ORBITA;
    else if (botao == GLUT_MIDDLE_BUTTON) modo = M_PAN;
}

static void motion(int x, int y) {
    int dx = x - lx, dy = y - ly; lx = x; ly = y;
    if (modo == M_ORBITA) {
        cam_yaw -= dx * 0.4f; cam_pitch += dy * 0.4f;
        if (cam_pitch < 4) cam_pitch = 4;
        if (cam_pitch > 88) cam_pitch = 88;
    } else if (modo == M_PAN) {
        float y0 = cam_yaw * PI / 180, k = cam_dist * 0.0015f;
        cam_tgt = vadd(cam_tgt, vadd(vmul(v3(cosf(y0), 0, -sinf(y0)), -dx * k), vmul(v3(-sinf(y0), 0, -cosf(y0)), dy * k)));
    } else if (modo == M_ARRASTA && drag >= 0 && !pecas[drag].travada) {
        Peca *p = &pecas[drag];
        V3 np = p->pos;
        if (vertical) {
            np.y -= dy * 0.02f * (cam_dist / 17.0f);
        } else {
            V3 o, d; raio_do_mouse(x, y, &o, &d);
            if (fabsf(d.y) > 1e-6f) {
                float t = (p->pos.y - o.y) / d.y;             /* interseccao raio x plano y=cte */
                if (t > 0) { V3 hit = vadd(o, vmul(d, t)); np.x = hit.x + off_x; np.z = hit.z + off_z; }
            }
        }
        mover_peca(drag, np);
    }
}

static void mover_sel(V3 delta) {
    if (sel >= 0 && !pecas[sel].travada) { mover_peca(sel, vadd(pecas[sel].pos, delta)); tentar_snap(sel); }
    else if (sel < 0) cam_tgt = vadd(cam_tgt, delta);
}

static void teclado(unsigned char k, int x, int y) {
    (void)x; (void)y;
    if (k >= 'A' && k <= 'Z') k = (unsigned char)(k - 'A' + 'a');
    switch (k) {
    case 27: exit(0);
    case 'q': if (sel >= 0) rotacionar(sel, 1, +15); break;
    case 'e': if (sel >= 0) rotacionar(sel, 1, -15); break;
    case 'a': if (sel >= 0) rotacionar(sel, 0, -15); break;
    case 'd': if (sel >= 0) rotacionar(sel, 0, +15); break;
    case 'z': if (sel >= 0) rotacionar(sel, 2, +15); break;
    case 'c': if (sel >= 0) rotacionar(sel, 2, -15); break;
    case 'u':
        if (sel >= 0 && pecas[sel].travada) {
            Peca *p = &pecas[sel];
            alvos[p->alvo].ocupado = -1; p->travada = 0; p->anim = 0; p->alvo = -1;
            encaixes--; vitoria = 0; mensagem("Peca solta");
        }
        break;
    case 'h': mostra_gabarito = !mostra_gabarito; break;
    case 'r': carregar_nivel(nivel); break;
    case '1': carregar_nivel(0); break;
    case '2': carregar_nivel(1); break;
    case ' ': camera_reset(); break;
    case '+': case '=': cam_dist *= 0.92f; break;
    case '-': cam_dist *= 1.09f; break;
    case '\t': if (npecas) { int i = sel; do { i = (i + 1) % npecas; } while (pecas[i].travada && i != sel); sel = i; } break;
    }
}

static void especial(int k, int x, int y) {
    (void)x; (void)y;
    float yw = cam_yaw * PI / 180, s = 0.25f;
    V3 frente = v3(-sinf(yw), 0, -cosf(yw)), dir = v3(cosf(yw), 0, -sinf(yw));
    switch (k) {
    case GLUT_KEY_UP:        mover_sel(vmul(frente, s)); break;
    case GLUT_KEY_DOWN:      mover_sel(vmul(frente, -s)); break;
    case GLUT_KEY_RIGHT:     mover_sel(vmul(dir, s)); break;
    case GLUT_KEY_LEFT:      mover_sel(vmul(dir, -s)); break;
    case GLUT_KEY_PAGE_UP:   if (sel >= 0) mover_sel(v3(0, s, 0)); break;
    case GLUT_KEY_PAGE_DOWN: if (sel >= 0) mover_sel(v3(0, -s, 0)); break;
    }
}

int main(int argc, char **argv) {
    init_formas();
    carregar_nivel(0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(W, H);
    glutCreateWindow("Simulador de Montagem e Construcao - CG 2026/2");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(teclado);
    glutSpecialFunc(especial);
    ultimo = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}

#else /* ============================ TESTES DA LOGICA ====================== */
static int falhas = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FALHOU: %s\n", m); falhas++; } else printf("ok: %s\n", m); } while (0)

int main(void) {
    init_formas();
    for (int n = 0; n < 2; n++) {
        carregar_nivel(n);
        int livre = 1;
        for (int i = 0; i < npecas; i++) if (colide_com_outros(i, pecas[i].pos, pecas[i].R) >= 0) livre = 0;
        CHECK(livre, "estado inicial sem colisoes");
        /* picking: raio vindo de cima no centro de cada peca */
        int okpick = 1;
        for (int i = 0; i < npecas; i++) {
            float t = raio_obb(v3(pecas[i].pos.x, pecas[i].pos.y + 20, pecas[i].pos.z), v3(0, -1, 0),
                               pecas[i].pos, pecas[i].R, formas[pecas[i].forma].h);
            if (t < 0) okpick = 0;
        }
        CHECK(okpick, "raio x OBB acerta o centro das pecas");
        /* monta de baixo para cima com pose imperfeita (deslocada e girada) */
        for (int a = 0; a < nalvos; a++) {
            int i = a;                               /* peca a ja tem a forma do alvo a */
            pecas[i].pos = vadd(alvos[a].pos, v3(0.25f, 0.12f, -0.2f));
            pecas[i].R = m3_ortho(m3_mul(m3_rot(1, 15), m3_mul(formas[alvos[a].forma].sim[1], alvos[a].R)));
            int r = tentar_snap(i);
            CHECK(r, "snap aceito (pos e angulo dentro da tolerancia, com simetria)");
            for (int k = 0; k < 20; k++) atualizar(0.05f);
        }
        CHECK(vitoria, "estrutura final verificada");
        CHECK(encaixes == nalvos, "contagem de encaixes");
    }
    /* casos negativos */
    carregar_nivel(0);
    pecas[0].pos = vadd(alvos[0].pos, v3(1.2f, 0, 0)); pecas[0].R = m3_id();
    CHECK(!tentar_snap(0), "longe demais: nao encaixa");
    pecas[0].pos = alvos[0].pos; pecas[0].R = m3_rot(1, 45);
    CHECK(!tentar_snap(0), "angulo grande: nao encaixa");
    pecas[0].R = m3_id();
    pecas[1].pos = v3(0.2f, 0.2f, 0); pecas[1].R = m3_id();   /* parede dentro da laje */
    CHECK(colide_com_outros(0, pecas[0].pos, pecas[0].R) >= 0, "sobreposicao detectada");
    pecas[1].pos = v3(0, 0.4f + 1.0f, 0);                     /* parede apoiada: so encosta */
    CHECK(colide_com_outros(0, pecas[0].pos, pecas[0].R) < 0, "encostar nao e colidir");
    /* mover_peca respeita colisao */
    pecas[1].pos = v3(5, 1.0f, 0); pecas[1].R = m3_id();
    mover_peca(1, v3(0, 1.0f, 0));
    CHECK(colide_com_outros(1, pecas[1].pos, pecas[1].R) < 0, "mover_peca nao atravessa outra peca");
    CHECK(pecas[1].pos.x > 2.2f, "peca parou na frente da laje");
    printf(falhas ? "\n%d FALHAS\n" : "\nTODOS OS TESTES PASSARAM\n", falhas);
    return falhas != 0;
}
#endif
