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
#define MAX_BOLAS 16
#define RAIO_BOLA 0.08f
#define RAIO_CACAPA 0.14f       // Raio das 6 caçapas (um pouco maior que as bolas)
#define ATRITO 0.988f

#define MESA_LARGURA 2.2f      // Eixo X
#define MESA_COMPRIMENTO 4.0f  // Eixo Z
#define MESA_ALTURA_TAMPOC 0.1f

// ==========================================
// ESTRUTURAS DE DADOS
// ==========================================
typedef struct {
    float x, y, z;
    float vx, vz;
    int ativa;                  // 1 = na mesa, 0 = encaçapada
    float r, g, b;
    float rot_x, rot_z;
} Bola;

typedef struct {
    float x, z;
} Cacapa;

typedef struct {
    float cam_dist;
    float cam_pitch;
    float cam_yaw;
} Camera;

typedef struct {
    float angulo;
    float forca;
    float desvio_impacto;
    int mirando;
} Taco;

// ==========================================
// VARIÁVEIS GLOBAIS DE ESTADO
// ==========================================
Bola bolas[MAX_BOLAS];
Cacapa cacapas[6];
Camera cam = { 5.5f, 35.0f, 0.0f };
Taco taco = { 0.0f, 0.15f, 0.0f, 1 };

int mouse_last_x, mouse_last_y;
int mouse_button_down = -1;
int pontuacao = 0;

const float cores_bolas[15][3] = {
    {1.0f, 0.8f, 0.0f}, {0.0f, 0.2f, 0.8f}, {0.9f, 0.1f, 0.1f},
    {0.4f, 0.1f, 0.5f}, {1.0f, 0.4f, 0.0f}, {0.1f, 0.5f, 0.1f},
    {0.5f, 0.0f, 0.0f}, {0.1f, 0.1f, 0.1f}, {1.0f, 0.8f, 0.0f},
    {0.0f, 0.2f, 0.8f}, {0.9f, 0.1f, 0.1f}, {0.4f, 0.1f, 0.5f},
    {1.0f, 0.4f, 0.0f}, {0.1f, 0.5f, 0.1f}, {0.5f, 0.0f, 0.0f}
};

// ==========================================
// INICIALIZAÇÃO DA MESA, CAÇAPAS E BOLA
// ==========================================
void inicializar_cacapas() {
    float lx = (MESA_LARGURA / 2.0f) - 0.04f;
    float lz = (MESA_COMPRIMENTO / 2.0f) - 0.04f;

    // Cantos
    cacapas[0] = (Cacapa){ -lx, -lz }; // Cantos superiores
    cacapas[1] = (Cacapa){  lx, -lz };
    cacapas[2] = (Cacapa){ -lx,  lz }; // Cantos inferiores
    cacapas[3] = (Cacapa){  lx,  lz };

    // Meios
    cacapas[4] = (Cacapa){ -lx, 0.0f }; // Meio esquerdo
    cacapas[5] = (Cacapa){  lx, 0.0f }; // Meio direito
}

void inicializar_jogo() {
    pontuacao = 0;
    inicializar_cacapas();

    // Bola Branca
    bolas[0].x = 0.0f;
    bolas[0].y = RAIO_BOLA;
    bolas[0].z = MESA_COMPRIMENTO * 0.25f;
    bolas[0].vx = 0.0f; bolas[0].vz = 0.0f;
    bolas[0].ativa = 1;
    bolas[0].r = 0.95f; bolas[0].g = 0.95f; bolas[0].b = 0.95f;
    bolas[0].rot_x = 0.0f; bolas[0].rot_z = 0.0f;

    // Disposição em Triângulo
    float start_z = -MESA_COMPRIMENTO * 0.2f;
    int idx = 1;
    float spacing = RAIO_BOLA * 2.05f;

    for (int row = 0; row < 5; row++) {
        float z_row = start_z - (row * spacing * sin(M_PI / 3.0f));
        float start_x = - (row * spacing / 2.0f);
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
// FISICA E DETEÇÃO DE ENCAÇAPAMENTO
// ==========================================
void verificar_encaçapamento(int i) {
    for (int c = 0; c < 6; c++) {
        float dx = bolas[i].x - cacapas[c].x;
        float dz = bolas[i].z - cacapas[c].z;
        float dist = sqrt(dx * dx + dz * dz);

        // Se a bola entrar no raio da caçapa
        if (dist < RAIO_CACAPA) {
            bolas[i].ativa = 0;
            bolas[i].vx = 0.0f;
            bolas[i].vz = 0.0f;

            if (i == 0) {
                // Se for a bola branca, reposiciona na linha inicial
                printf("Falta! Bola branca encaçapada.\n");
                bolas[0].x = 0.0f;
                bolas[0].z = MESA_COMPRIMENTO * 0.25f;
                bolas[0].ativa = 1;
            } else {
                pontuacao++;
            }
            break;
        }
    }
}

void atualizar_fisica() {
    int movimento = 0;

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        if (fabs(bolas[i].vx) > 0.0001f || fabs(bolas[i].vz) > 0.0001f) {
            movimento = 1;
            bolas[i].x += bolas[i].vx;
            bolas[i].z += bolas[i].vz;

            bolas[i].rot_x += (bolas[i].vz / RAIO_BOLA) * (180.0f / M_PI);
            bolas[i].rot_z -= (bolas[i].vx / RAIO_BOLA) * (180.0f / M_PI);

            bolas[i].vx *= ATRITO;
            bolas[i].vz *= ATRITO;

            if (fabs(bolas[i].vx) < 0.0002f) bolas[i].vx = 0.0f;
            if (fabs(bolas[i].vz) < 0.0002f) bolas[i].vz = 0.0f;

            // Verificar se caiu numa caçapa
            verificar_encaçapamento(i);
        }

        if (!bolas[i].ativa) continue;

        // Colisão com as Bordas
        float lim_x = (MESA_LARGURA / 2.0f) - RAIO_BOLA;
        float lim_z = (MESA_COMPRIMENTO / 2.0f) - RAIO_BOLA;

        if (bolas[i].x < -lim_x) { bolas[i].x = -lim_x; bolas[i].vx = -bolas[i].vx; }
        else if (bolas[i].x > lim_x) { bolas[i].x = lim_x; bolas[i].vx = -bolas[i].vx; }

        if (bolas[i].z < -lim_z) { bolas[i].z = -lim_z; bolas[i].vz = -bolas[i].vz; }
        else if (bolas[i].z > lim_z) { bolas[i].z = lim_z; bolas[i].vz = -bolas[i].vz; }
    }

    // Colisão Bola-Bola
    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;
        for (int j = i + 1; j < MAX_BOLAS; j++) {
            if (!bolas[j].ativa) continue;

            float dx = bolas[j].x - bolas[i].x;
            float dz = bolas[j].z - bolas[i].z;
            float dist = sqrt(dx * dx + dz * dz);
            float minDist = 2.0f * RAIO_BOLA;

            if (dist < minDist && dist > 0.0f) {
                float nx = dx / dist;
                float nz = dz / dist;

                float kx = bolas[i].vx - bolas[j].vx;
                float kz = bolas[i].vz - bolas[j].vz;
                float p = 2.0f * (nx * kx + nz * kz) / 2.0f;

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
    if (!taco.mirando) return;

    float angulo_efetivo = taco.angulo + taco.desvio_impacto;
    bolas[0].vx = -sin(angulo_efetivo) * taco.forca;
    bolas[0].vz = -cos(angulo_efetivo) * taco.forca;

    taco.mirando = 0;
}

// ==========================================
// RENDERIZAÇÃO GRÁFICA OPENGL 3D
// ==========================================
void desenhar_cacapas() {
    // Material escuro (Preto/Cinza para os buracos)
    GLfloat mat_cacapa[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cacapa);

    for (int i = 0; i < 6; i++) {
        glPushMatrix();
        // Ligeiramente acima da mesa para evitar Z-fighting visual
        glTranslatef(cacapas[i].x, 0.001f, cacapas[i].z);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        // Desenha os buracos cilíndricos na mesa
        GLUquadric* quad = gluNewQuadric();
        gluDisk(quad, 0.0, RAIO_CACAPA, 24, 1);
        gluDeleteQuadric(quad);

        glPopMatrix();
    }
}

void desenhar_mesa() {
    // Material do Pano Verde
    GLfloat mat_ambiente[] = { 0.0f, 0.4f, 0.1f, 1.0f };
    GLfloat mat_difusa[]   = { 0.1f, 0.7f, 0.2f, 1.0f };
    GLfloat mat_especular[]= { 0.1f, 0.2f, 0.1f, 1.0f };

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambiente);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_difusa);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_especular);
    glMaterialf (GL_FRONT, GL_SHININESS, 10.0f);

    // Tampo Principal Verde
    glPushMatrix();
    glTranslatef(0.0f, -MESA_ALTURA_TAMPOC / 2.0f, 0.0f);
    glScalef(MESA_LARGURA, MESA_ALTURA_TAMPOC, MESA_COMPRIMENTO);
    glutSolidCube(1.0);
    glPopMatrix();

    // Bordas de Madeira
    GLfloat mat_madeira_amb[] = { 0.3f, 0.15f, 0.05f, 1.0f };
    GLfloat mat_madeira_dif[] = { 0.5f, 0.25f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_madeira_amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_madeira_dif);

    float espessura = 0.25f;
    float alt_borda = 0.18f;

    // Bordas Laterais
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * (MESA_LARGURA / 2.0f + espessura / 2.0f), alt_borda / 4.0f, 0.0f);
        glScalef(espessura, alt_borda, MESA_COMPRIMENTO + 2.0f * espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Bordas Superior e Inferior
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(0.0f, alt_borda / 4.0f, side * (MESA_COMPRIMENTO / 2.0f + espessura / 2.0f));
        glScalef(MESA_LARGURA, alt_borda, espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Renderizar as 6 caçapas por cima do tampo
    desenhar_cacapas();
}

void desenhar_bolas() {
    GLfloat mat_especular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_especular);
    glMaterialf (GL_FRONT, GL_SHININESS, 64.0f);

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        GLfloat mat_cor[] = { bolas[i].r, bolas[i].g, bolas[i].b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cor);

        glPushMatrix();
        glTranslatef(bolas[i].x, bolas[i].y, bolas[i].z);
        glRotatef(bolas[i].rot_x, 1.0f, 0.0f, 0.0f);
        glRotatef(bolas[i].rot_z, 0.0f, 0.0f, 1.0f);
        glutSolidSphere(RAIO_BOLA, 24, 24);
        glPopMatrix();
    }
}

void desenhar_taco_e_mira() {
    if (!taco.mirando || !bolas[0].ativa) return;

    glPushMatrix();
    glTranslatef(bolas[0].x, bolas[0].y, bolas[0].z);
    glRotatef(taco.angulo * (180.0f / M_PI), 0.0f, 1.0f, 0.0f);

    // Linha de Guia
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, -2.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    // Taco 3D
    GLfloat mat_taco[] = { 0.7f, 0.5f, 0.2f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_taco);

    float recuo = 0.15f + (taco.forca * 0.8f);
    glTranslatef(taco.desvio_impacto, 0.02f, recuo);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, 0.015, 0.03, 1.8, 16, 16);
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

    glColor3f(1.0f, 1.0f, 1.0f);
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
// CALLBACKS DO GLUT
// ==========================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float rad_yaw = cam.cam_yaw * (M_PI / 180.0f);
    float rad_pitch = cam.cam_pitch * (M_PI / 180.0f);

    float eye_x = cam.cam_dist * sin(rad_yaw) * cos(rad_pitch);
    float eye_y = cam.cam_dist * sin(rad_pitch);
    float eye_z = cam.cam_dist * cos(rad_yaw) * cos(rad_pitch);

    float target_x = bolas[0].x;
    float target_z = bolas[0].z;

    gluLookAt(eye_x + target_x, eye_y, eye_z + target_z,
              target_x, 0.0f, target_z,
              0.0f, 1.0f, 0.0f);

    GLfloat light_pos[] = { 0.0f, 4.0f, 0.0f, 1.0f };
    GLfloat light_amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_dif[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);

    desenhar_mesa();
    desenhar_bolas();
    desenhar_taco_e_mira();

    char hud[160];
    snprintf(hud, sizeof(hud), "Bolas Encaçapadas: %d/15 | Forca: %.2f | Efeito: %.3f | [Espaco]: Tacada | [R]: Reset",
             pontuacao, taco.forca, taco.desvio_impacto);
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
        case 'r': case 'R':
            inicializar_jogo();
            break;
        case 'w': case 'W':
            if (taco.forca < 0.45f) taco.forca += 0.02f;
            break;
        case 's': case 'S':
            if (taco.forca > 0.05f) taco.forca -= 0.02f;
            break;
        case 'a': case 'A':
            if (taco.desvio_impacto > -0.03f) taco.desvio_impacto -= 0.005f;
            break;
        case 'd': case 'D':
            if (taco.desvio_impacto < 0.03f) taco.desvio_impacto += 0.005f;
            break;
        case 27: // ESC
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

void mouse_click(int button, int state, int x, int y) {
    mouse_button_down = (state == GLUT_DOWN) ? button : -1;
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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutCreateWindow("Simulador 3D de Sinuca com Cacapas - OpenGL");

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