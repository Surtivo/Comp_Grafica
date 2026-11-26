#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// CONSTANTES & CONFIGURAÇÕES FÍSICAS
// ==========================================
#define MAX_BOLAS 4
#define RAIO_BOLA 0.08f
#define ATRITO 0.988f

// Dimensões da Mesa (Área de jogo)
#define MESA_LARGURA 2.2f       // Eixo X
#define MESA_COMPRIMENTO 4.0f   // Eixo Z
#define MESA_ALTURA_TAMPOC 0.1f
#define MESA_PE_ALTURA 0.75f
#define CHAO_Y (-MESA_ALTURA_TAMPOC - MESA_PE_ALTURA)

#define LUZ_X 0.0f
#define LUZ_Y 4.0f
#define LUZ_Z 0.0f

// ==========================================
// ESTRUTURAS DE DADOS
// ==========================================
typedef struct {
    float x, y, z;
    float vx, vz;       // Componentes de velocidade x & z
    int ativa;
    float r, g, b;
    float rot_x, rot_z; // Rotação visual das bolas
} Bola;

typedef struct {
    float cam_dist;
    float cam_pitch; // Ângulo vertical
    float cam_yaw;   // Ângulo horizontal
} Camera;

typedef struct {
    float angulo;           // Direção da tacada (em radianos)
    float forca;            // Intensidade da força (0.05 a 0.5)
    float desvio_impacto;   // Ponto de impacto lateral (-0.03 a +0.03)
    int mirando;            // 1 se estiver no modo de mira
} Taco;

// ==========================================
// VARIÁVEIS GLOBAIS DE ESTADO
// ==========================================
Bola bolas[MAX_BOLAS];
Camera cam = { 5.5f, 35.0f, 0.0f };
Taco taco = { 0.0f, 0.15f, 0.0f, 1 };

int mouse_last_x, mouse_last_y;
int mouse_button_down = -1;
int posicionando_bola_branca = 1;

// Cores das 15 bolas de sinuca (padrão)
const float cores_bolas[15][3] = {
    {1.0f, 0.8f, 0.0f}, // 1: Amarela
    {0.0f, 0.2f, 0.8f}, // 2: Azul
    {0.9f, 0.1f, 0.1f}, // 3: Vermelha
    {0.4f, 0.1f, 0.5f}, // 4: Roxa
    {1.0f, 0.4f, 0.0f}, // 5: Laranja
    {0.1f, 0.5f, 0.1f}, // 6: Verde
    {0.5f, 0.0f, 0.0f}, // 7: Vinho / Castanha
    {0.1f, 0.1f, 0.1f}, // 8: Preta
    {1.0f, 0.8f, 0.0f}, // 9
    {0.0f, 0.2f, 0.8f}, // 10
    {0.9f, 0.1f, 0.1f}, // 11
    {0.4f, 0.1f, 0.5f}, // 12
    {1.0f, 0.4f, 0.0f}, // 13
    {0.1f, 0.5f, 0.1f}, // 14
    {0.5f, 0.0f, 0.0f}  // 15
};

static void definir_material(const GLfloat amb[4], const GLfloat dif[4],
                             const GLfloat spec[4], GLfloat brilho) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, brilho);
}

// ==========================================
// INICIALIZAÇÃO DA MESA E DA FÍSICA
// ==========================================
void inicializar_jogo() {
    posicionando_bola_branca = 1;

    // Bola Branca (Índice 0)
    bolas[0].x = 0.0f;
    bolas[0].y = RAIO_BOLA;
    bolas[0].z = MESA_COMPRIMENTO * 0.25f;
    bolas[0].vx = 0.0f; bolas[0].vz = 0.0f;
    bolas[0].ativa = 1;
    bolas[0].r = 0.94f; bolas[0].g = 0.94f; bolas[0].b = 0.92f;
    bolas[0].rot_x = 0.0f; bolas[0].rot_z = 0.0f;

    // Disposição em Triângulo das Bolas Coloridas
    float start_z = -MESA_COMPRIMENTO * 0.2f;
    int idx = 1;
    float spacing = RAIO_BOLA * 2.05f;

    for (int row = 0; row < 5; row++) {
        float z_row = start_z - (row * spacing * sinf(M_PI / 3.0f));
        float start_x = -(row * spacing / 2.0f);

        for (int col = 0; col <= row; col++) {
            if (idx >= MAX_BOLAS) break;
            bolas[idx].x = start_x + (col * spacing);
            bolas[idx].y = RAIO_BOLA;
            bolas[idx].z = z_row;
            bolas[idx].vx = 0.0f; bolas[idx].vz = 0.0f;
            bolas[idx].ativa = 1;
            bolas[idx].r = cores_bolas[idx - 1][0];
            bolas[idx].g = cores_bolas[idx - 1][1];
            bolas[idx].b = cores_bolas[idx - 1][2];
            bolas[idx].rot_x = 0.0f; bolas[idx].rot_z = 0.0f;
            idx++;
        }
    }

    taco.mirando = 1;
}

// ==========================================
// MOTOR FÍSICO (ATUALIZAÇÃO DE ESTADO)
// ==========================================
void atualizar_fisica() {
    int movimento = 0;

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        if (fabsf(bolas[i].vx) > 0.0001f || fabsf(bolas[i].vz) > 0.0001f) {
            movimento = 1;
            bolas[i].x += bolas[i].vx;
            bolas[i].z += bolas[i].vz;

            // Simulação de rotação visual da bola conforme ela rola
            bolas[i].rot_x += (bolas[i].vz / RAIO_BOLA) * (180.0f / M_PI);
            bolas[i].rot_z -= (bolas[i].vx / RAIO_BOLA) * (180.0f / M_PI);

            bolas[i].vx *= ATRITO;
            bolas[i].vz *= ATRITO;

            if (fabsf(bolas[i].vx) < 0.0002f) bolas[i].vx = 0.0f;
            if (fabsf(bolas[i].vz) < 0.0002f) bolas[i].vz = 0.0f;
        }

        float lim_x = (MESA_LARGURA / 2.0f) - RAIO_BOLA;
        float lim_z = (MESA_COMPRIMENTO / 2.0f) - RAIO_BOLA;

        if (bolas[i].x < -lim_x) { bolas[i].x = -lim_x; bolas[i].vx = -bolas[i].vx; }
        else if (bolas[i].x > lim_x) { bolas[i].x = lim_x; bolas[i].vx = -bolas[i].vx; }

        if (bolas[i].z < -lim_z) { bolas[i].z = -lim_z; bolas[i].vz = -bolas[i].vz; }
        else if (bolas[i].z > lim_z) { bolas[i].z = lim_z; bolas[i].vz = -bolas[i].vz; }
    }

    // Colisão Elástica Bola-Bola
    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;
        for (int j = i + 1; j < MAX_BOLAS; j++) {
            if (!bolas[j].ativa) continue;

            float dx = bolas[j].x - bolas[i].x;
            float dz = bolas[j].z - bolas[i].z;
            float dist = sqrtf(dx * dx + dz * dz);
            float minDist = 2.0f * RAIO_BOLA;

            if (dist < minDist && dist > 0.0f) {
                float nx = dx / dist;
                float nz = dz / dist;

                float kx = bolas[i].vx - bolas[j].vx;
                float kz = bolas[i].vz - bolas[j].vz;
                float p = (nx * kx + nz * kz);

                bolas[i].vx -= p * nx;
                bolas[i].vz -= p * nz;
                bolas[j].vx += p * nx;
                bolas[j].vz += p * nz;

                float overlap = 0.5f * (minDist - dist);
                bolas[i].x -= overlap * nx;
                bolas[i].z -= overlap * nz;
                bolas[j].x += overlap * nx;
                bolas[j].z += overlap * nz;
            }
        }
    }

    if (!movimento && !taco.mirando) {
        taco.mirando = 1;
    }
}

void disparar_tacada() {
    if (!taco.mirando || posicionando_bola_branca) return;

    float angulo_efetivo = taco.angulo + taco.desvio_impacto;
    bolas[0].vx = -sinf(angulo_efetivo) * taco.forca;
    bolas[0].vz = -cosf(angulo_efetivo) * taco.forca;

    taco.mirando = 0;
}

// ==========================================
// RENDERIZAÇÃO GRÁFICA OPENGL 3D
// ==========================================
void desenhar_cenario() {
    // Piso quadriculado simples
    const int metade_grade = 8;
    const float tam_quadrado = 1.5f;

    GLfloat piso_claro_amb[]  = { 0.14f, 0.13f, 0.12f, 1.0f };
    GLfloat piso_claro_dif[]  = { 0.36f, 0.33f, 0.29f, 1.0f };
    GLfloat piso_escuro_amb[] = { 0.08f, 0.08f, 0.09f, 1.0f };
    GLfloat piso_escuro_dif[] = { 0.19f, 0.19f, 0.21f, 1.0f };
    GLfloat piso_spec[]       = { 0.15f, 0.15f, 0.15f, 1.0f };

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    for (int gz = -metade_grade; gz < metade_grade; gz++) {
        for (int gx = -metade_grade; gx < metade_grade; gx++) {
            int par = ((gx + gz) % 2 == 0);
            if (par) {
                definir_material(piso_claro_amb, piso_claro_dif, piso_spec, 18.0f);
            } else {
                definir_material(piso_escuro_amb, piso_escuro_dif, piso_spec, 18.0f);
            }
            float x0 = gx * tam_quadrado;
            float x1 = x0 + tam_quadrado;
            float z0 = gz * tam_quadrado;
            float z1 = z0 + tam_quadrado;
            glVertex3f(x0, CHAO_Y, z0);
            glVertex3f(x0, CHAO_Y, z1);
            glVertex3f(x1, CHAO_Y, z1);
            glVertex3f(x1, CHAO_Y, z0);
        }
    }
    glEnd();

    // Paredes simples ao fundo e laterais
    GLfloat parede_amb[]  = { 0.11f, 0.13f, 0.16f, 1.0f };
    GLfloat parede_dif[]  = { 0.26f, 0.30f, 0.36f, 1.0f };
    GLfloat parede_spec[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    definir_material(parede_amb, parede_dif, parede_spec, 8.0f);

    float sala_metade = metade_grade * tam_quadrado; // 12.0f
    float parede_altura = 6.5f;

    // Parede do fundo (-Z)
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-sala_metade, CHAO_Y, -sala_metade);
    glVertex3f( sala_metade, CHAO_Y, -sala_metade);
    glVertex3f( sala_metade, CHAO_Y + parede_altura, -sala_metade);
    glVertex3f(-sala_metade, CHAO_Y + parede_altura, -sala_metade);
    glEnd();

    // Parede esquerda (-X)
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-sala_metade, CHAO_Y, -sala_metade);
    glVertex3f(-sala_metade, CHAO_Y,  sala_metade);
    glVertex3f(-sala_metade, CHAO_Y + parede_altura,  sala_metade);
    glVertex3f(-sala_metade, CHAO_Y + parede_altura, -sala_metade);
    glEnd();

    // Parede direita (+X)
    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(sala_metade, CHAO_Y,  sala_metade);
    glVertex3f(sala_metade, CHAO_Y, -sala_metade);
    glVertex3f(sala_metade, CHAO_Y + parede_altura, -sala_metade);
    glVertex3f(sala_metade, CHAO_Y + parede_altura,  sala_metade);
    glEnd();

    // Luminária pendente simples acima da mesa
    GLfloat lamp_amb[]  = { 0.15f, 0.15f, 0.15f, 1.0f };
    GLfloat lamp_dif[]  = { 0.25f, 0.25f, 0.28f, 1.0f };
    GLfloat lamp_spec[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    definir_material(lamp_amb, lamp_dif, lamp_spec, 80.0f);

    glPushMatrix();
    glTranslatef(LUZ_X, LUZ_Y + 0.18f, LUZ_Z);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(0.65f, 0.25f, 24, 1);
    glPopMatrix();
}

void desenhar_mesa() {
    // Material do Pano Verde (aveludado: baixa especularidade, cor profunda)
    GLfloat pano_amb[]  = { 0.02f, 0.22f, 0.08f, 1.0f };
    GLfloat pano_dif[]  = { 0.06f, 0.56f, 0.20f, 1.0f };
    GLfloat pano_spec[] = { 0.08f, 0.14f, 0.08f, 1.0f };
    definir_material(pano_amb, pano_dif, pano_spec, 12.0f);

    // Tampo Principal Verde
    glPushMatrix();
    glTranslatef(0.0f, -MESA_ALTURA_TAMPOC / 2.0f, 0.0f);
    glScalef(MESA_LARGURA, MESA_ALTURA_TAMPOC, MESA_COMPRIMENTO);
    glutSolidCube(1.0);
    glPopMatrix();

    // Material da Madeira (bordas, saia e pés)
    GLfloat madeira_amb[]  = { 0.16f, 0.07f, 0.02f, 1.0f };
    GLfloat madeira_dif[]  = { 0.44f, 0.20f, 0.07f, 1.0f };
    GLfloat madeira_spec[] = { 0.45f, 0.30f, 0.18f, 1.0f };
    definir_material(madeira_amb, madeira_dif, madeira_spec, 48.0f);

    float espessura = 0.25f;
    float alt_borda = 0.18f;

    // Borda Esquerda & Direita
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * (MESA_LARGURA / 2.0f + espessura / 2.0f), alt_borda / 4.0f, 0.0f);
        glScalef(espessura, alt_borda, MESA_COMPRIMENTO + 2.0f * espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Borda Superior & Inferior
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(0.0f, alt_borda / 4.0f, side * (MESA_COMPRIMENTO / 2.0f + espessura / 2.0f));
        glScalef(MESA_LARGURA, alt_borda, espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Saia estrutural logo abaixo do tampo
    float alt_saia = 0.14f;
    float y_saia = -MESA_ALTURA_TAMPOC - (alt_saia / 2.0f);
    glPushMatrix();
    glTranslatef(0.0f, y_saia, 0.0f);
    glScalef(MESA_LARGURA * 0.92f, alt_saia, MESA_COMPRIMENTO * 0.92f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Quatro pés apoiados no chão (CHAO_Y)
    float pe_largura = 0.18f;
    float offset_x = (MESA_LARGURA / 2.0f) - (pe_largura * 0.75f);
    float offset_z = (MESA_COMPRIMENTO / 2.0f) - (pe_largura * 0.75f);
    float y_pe_centro = -MESA_ALTURA_TAMPOC - (MESA_PE_ALTURA / 2.0f);

    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
            glTranslatef(sx * offset_x, y_pe_centro, sz * offset_z);
            glScalef(pe_largura, MESA_PE_ALTURA, pe_largura);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    }
}

void desenhar_bolas() {
    GLfloat bola_spec[] = { 0.95f, 0.95f, 0.95f, 1.0f };

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        GLfloat bola_amb[] = { bolas[i].r * 0.22f, bolas[i].g * 0.22f, bolas[i].b * 0.22f, 1.0f };
        GLfloat bola_dif[] = { bolas[i].r, bolas[i].g, bolas[i].b, 1.0f };
        definir_material(bola_amb, bola_dif, bola_spec, 96.0f);

        glPushMatrix();
        glTranslatef(bolas[i].x, bolas[i].y, bolas[i].z);
        glRotatef(bolas[i].rot_x, 1.0f, 0.0f, 0.0f);
        glRotatef(bolas[i].rot_z, 0.0f, 0.0f, 1.0f);

        // Corpo esférico principal
        glutSolidSphere(RAIO_BOLA, 32, 32);

        // Faixa equatorial contrastante para facilitar a visualização da rotação
        if (i == 0) {
            // Bola branca: faixa vermelha/vinho discreta e ponto polar
            GLfloat marca_amb[] = { 0.18f, 0.03f, 0.03f, 1.0f };
            GLfloat marca_dif[] = { 0.75f, 0.12f, 0.12f, 1.0f };
            definir_material(marca_amb, marca_dif, bola_spec, 96.0f);
        } else {
            // Bolas coloridas: faixa creme clara
            GLfloat marca_amb[] = { 0.22f, 0.20f, 0.16f, 1.0f };
            GLfloat marca_dif[] = { 0.95f, 0.92f, 0.84f, 1.0f };
            definir_material(marca_amb, marca_dif, bola_spec, 96.0f);
        }

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidTorus(RAIO_BOLA * 0.14f, RAIO_BOLA * 0.87f, 12, 28);
        glPopMatrix();

        glPopMatrix();
    }
}

void desenhar_sombras() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        float escala = LUZ_Y / (LUZ_Y - bolas[i].y);
        float sombra_x = LUZ_X + (bolas[i].x - LUZ_X) * escala;
        float sombra_z = LUZ_Z + (bolas[i].z - LUZ_Z) * escala;
        float raio = RAIO_BOLA * escala;

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.0f, 0.0f, 0.0f, 0.28f);
        glVertex3f(sombra_x, 0.002f, sombra_z);
        for (int segmento = 0; segmento <= 32; segmento++) {
            float angulo = 2.0f * (float)M_PI * segmento / 32.0f;
            float x = sombra_x + raio * cosf(angulo);
            float z = sombra_z + raio * sinf(angulo);

            if (x < -MESA_LARGURA / 2.0f) x = -MESA_LARGURA / 2.0f;
            if (x > MESA_LARGURA / 2.0f) x = MESA_LARGURA / 2.0f;
            if (z < -MESA_COMPRIMENTO / 2.0f) z = -MESA_COMPRIMENTO / 2.0f;
            if (z > MESA_COMPRIMENTO / 2.0f) z = MESA_COMPRIMENTO / 2.0f;

            glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
            glVertex3f(x, 0.002f, z);
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void desenhar_taco_e_mira() {
    if (!taco.mirando) return;

    glPushMatrix();
    glTranslatef(bolas[0].x, bolas[0].y, bolas[0].z);
    glRotatef(taco.angulo * (180.0f / M_PI), 0.0f, 1.0f, 0.0f);

    // Linha de Guia / Mira
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.95f, 0.7f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, -2.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    // Taco 3D (madeira clara + sola azulada na ponta)
    GLfloat taco_amb[]  = { 0.22f, 0.14f, 0.05f, 1.0f };
    GLfloat taco_dif[]  = { 0.78f, 0.56f, 0.24f, 1.0f };
    GLfloat taco_spec[] = { 0.65f, 0.55f, 0.35f, 1.0f };
    definir_material(taco_amb, taco_dif, taco_spec, 64.0f);

    float recuo = 2.0f + (taco.forca * 0.8f);
    glTranslatef(taco.desvio_impacto, 0.02f, recuo);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, 0.028, 0.014, 1.8, 20, 20);
    gluDeleteQuadric(quad);

    glPopMatrix();
}

void renderizar_texto_2d(float x, float y, const char* texto) {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.95f, 0.95f, 0.92f);
    glRasterPos2f(x, y);
    for (const char* c = texto; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);
}

// ==========================================
// CALLBACKS DO GLUT (DISPLAY, CAM, INPUT)
// ==========================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float rad_yaw = cam.cam_yaw * (M_PI / 180.0f);
    float rad_pitch = cam.cam_pitch * (M_PI / 180.0f);

    float eye_x = cam.cam_dist * sinf(rad_yaw) * cosf(rad_pitch);
    float eye_y = cam.cam_dist * sinf(rad_pitch);
    float eye_z = cam.cam_dist * cosf(rad_yaw) * cosf(rad_pitch);

    gluLookAt(eye_x, eye_y, eye_z,
              0.0f, -0.25f, 0.0f,
              0.0f, 1.0f, 0.0f);

    // Luz Principal (pendente acima da mesa)
    GLfloat light0_pos[]  = { LUZ_X, LUZ_Y, LUZ_Z, 1.0f };
    GLfloat light0_amb[]  = { 0.16f, 0.16f, 0.15f, 1.0f };
    GLfloat light0_dif[]  = { 0.95f, 0.92f, 0.85f, 1.0f };
    GLfloat light0_spec[] = { 1.0f, 0.98f, 0.92f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_spec);

    // Luz Secundária de preenchimento (suaviza sombras duras nos pés e paredes)
    GLfloat light1_pos[]  = { -4.0f, 2.2f, 5.0f, 1.0f };
    GLfloat light1_amb[]  = { 0.05f, 0.05f, 0.06f, 1.0f };
    GLfloat light1_dif[]  = { 0.32f, 0.34f, 0.40f, 1.0f };
    GLfloat light1_spec[] = { 0.25f, 0.25f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_amb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_dif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_spec);

    desenhar_cenario();
    desenhar_mesa();
    desenhar_sombras();
    desenhar_bolas();
    desenhar_taco_e_mira();

    char hud[140];
    if (posicionando_bola_branca) {
        snprintf(hud, sizeof(hud), "Clique na mesa para posicionar a bola branca (R: Reiniciar)");
    } else {
        snprintf(hud, sizeof(hud), "Forca: %.2f | Efeito: %.3f | [Espaco]: Dar Tacada | [R]: Reiniciar",
                 taco.forca, taco.desvio_impacto);
    }
    renderizar_texto_2d(10, 20, hud);

    glutSwapBuffers();
}

void idle() {
    atualizar_fisica();
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)w / (GLfloat)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ' ':
            disparar_tacada();
            break;
        case 'r':
        case 'R':
            inicializar_jogo();
            break;
        case 'w': case 'W':
            if (taco.forca < 0.45f) taco.forca += 0.005f;
            break;
        case 's': case 'S':
            if (taco.forca > 0.05f) taco.forca -= 0.005f;
            break;
        case 'a': case 'A':
            if (taco.desvio_impacto > -0.03f) taco.desvio_impacto -= 0.009f;
            break;
        case 'd': case 'D':
            if (taco.desvio_impacto < 0.03f) taco.desvio_impacto += 0.009f;
            break;
        case 27:
            exit(0);
    }
}

void special_keys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            taco.angulo += 0.03f;
            break;
        case GLUT_KEY_RIGHT:
            taco.angulo -= 0.03f;
            break;
    }
}

int posicionar_bola_branca_com_mouse(int x, int y) {
    GLdouble modelview[16], projection[16];
    GLint viewport[4];
    GLdouble near_x, near_y, near_z;
    GLdouble far_x, far_y, far_z;
    int window_y = glutGet(GLUT_WINDOW_HEIGHT) - y;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (!gluUnProject(x, window_y, 0.0, modelview, projection, viewport,
                      &near_x, &near_y, &near_z) ||
        !gluUnProject(x, window_y, 1.0, modelview, projection, viewport,
                      &far_x, &far_y, &far_z)) {
        return 0;
    }

    double ray_y = far_y - near_y;
    if (fabs(ray_y) < 1e-8) return 0;

    double distance = -near_y / ray_y;
    if (distance < 0.0) return 0;

    float position_x = (float)(near_x + distance * (far_x - near_x));
    float position_z = (float)(near_z + distance * (far_z - near_z));
    float lim_x = (MESA_LARGURA / 2.0f) - RAIO_BOLA;
    float lim_z = (MESA_COMPRIMENTO / 2.0f) - RAIO_BOLA;

    if (position_x < -lim_x || position_x > lim_x ||
        position_z < -lim_z || position_z > lim_z) {
        return 0;
    }

    for (int i = 1; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        float dx = position_x - bolas[i].x;
        float dz = position_z - bolas[i].z;
        float distancia_minima = 2.0f * RAIO_BOLA;
        if (dx * dx + dz * dz < distancia_minima * distancia_minima) {
            return 0;
        }
    }

    bolas[0].x = position_x;
    bolas[0].y = RAIO_BOLA;
    bolas[0].z = position_z;
    bolas[0].vx = 0.0f;
    bolas[0].vz = 0.0f;
    posicionando_bola_branca = 0;
    return 1;
}

void mouse_click(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN &&
        posicionando_bola_branca) {
        posicionar_bola_branca_com_mouse(x, y);
        mouse_button_down = -1;
        glutPostRedisplay();
        return;
    }

    mouse_button_down = (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) ? button : -1;
    mouse_last_x = x;
    mouse_last_y = y;
}

void mouse_motion(int x, int y) {
    int dx = x - mouse_last_x;
    int dy = y - mouse_last_y;

    if (mouse_button_down == GLUT_LEFT_BUTTON) {
        cam.cam_yaw += dx * 0.4f;
        cam.cam_pitch += dy * 0.4f;

        if (cam.cam_pitch < 10.0f) cam.cam_pitch = 10.0f;
        if (cam.cam_pitch > 85.0f) cam.cam_pitch = 85.0f;
    }

    mouse_last_x = x;
    mouse_last_y = y;
}

void setup_opengl() {
    GLfloat luz_ambiente_global[] = { 0.14f, 0.14f, 0.15f, 1.0f };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luz_ambiente_global);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Simulador 3D de Sinuca - OpenGL");

    setup_opengl();
    inicializar_jogo();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutMouseFunc(mouse_click);
    glutMotionFunc(mouse_motion);

    glutMainLoop();
    return 0;
}