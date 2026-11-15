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
#define ATRITO 0.988f

// Dimensões da Mesa (Área de jogo)
#define MESA_LARGURA 2.2f      // Eixo X
#define MESA_COMPRIMENTO 4.0f  // Eixo Z
#define MESA_ALTURA_TAMPOC 0.1f

// ==========================================
// ESTRUTURAS DE DADOS
// ==========================================
typedef struct {
    float x, y, z;
    float vx, vz;           // Componentes de velocidade x & z
    int ativa;
    float r, g, b;
    float rot_x, rot_z;     // Rotação visual das bolas
} Bola;

typedef struct {
    float cam_dist;         // Distância da câmera
    float cam_pitch;        // Ângulo vertical
    float cam_yaw;          // Ângulo horizontal
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
    // Repete cores
    {1.0f, 0.8f, 0.0f}, // 9
    {0.0f, 0.2f, 0.8f}, // 10
    {0.9f, 0.1f, 0.1f}, // 11
    {0.4f, 0.1f, 0.5f}, // 12
    {1.0f, 0.4f, 0.0f}, // 13
    {0.1f, 0.5f, 0.1f}, // 14
    {0.5f, 0.0f, 0.0f}  // 15
};

// ==========================================
// INICIALIZAÇÃO DA MESA E DA FÍSICA
// ==========================================
void inicializar_jogo() {
    // Bola Branca (Índice 0)
    bolas[0].x = 0.0f;
    bolas[0].y = RAIO_BOLA;
    bolas[0].z = MESA_COMPRIMENTO * 0.25f;      // Distância da bola branca para o triângulo de bolas coloridas
    bolas[0].vx = 0.0f; bolas[0].vz = 0.0f;
    bolas[0].ativa = 1;
    bolas[0].r = 0.95f; bolas[0].g = 0.95f; bolas[0].b = 0.95f;
    bolas[0].rot_x = 0.0f; bolas[0].rot_z = 0.0f;

    // Disposição em Triângulo das 15 Bolas Coloridas
    float start_z = -MESA_COMPRIMENTO * 0.2f;   // Distância do triângulo de bolas coloridas para a borda da mesa (-0.8)
    int idx = 1;
    float spacing = RAIO_BOLA * 2.05f;          // Pequena folga entre bolas (0,164f)

    for (int row = 0; row < 5; row++) {         // 5 linhas do triângulo
        
        float z_row = start_z - (row * spacing * sin(M_PI / 3.0f));     // Posição Z da linha atual
        float start_x = - (row * spacing / 2.0f);                       // Posição X inicial da linha atual (centralizado)

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

    // Movimento e Atrito
    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        if (fabs(bolas[i].vx) > 0.0001f || fabs(bolas[i].vz) > 0.0001f) {
            movimento = 1;
            bolas[i].x += bolas[i].vx;
            bolas[i].z += bolas[i].vz;

            // Simulação de rotação visual da bola conforme ela rola
            bolas[i].rot_x += (bolas[i].vz / RAIO_BOLA) * (180.0f / M_PI);
            bolas[i].rot_z -= (bolas[i].vx / RAIO_BOLA) * (180.0f / M_PI);

            // Atrito linear simples
            bolas[i].vx *= ATRITO;
            bolas[i].vz *= ATRITO;

            // Evita que a bola continue se movendo infinitamente devido a erros de precisão
            // Parada mais brusca quando a velocidade mínima é muito alta
            if (fabs(bolas[i].vx) < 0.0002f) bolas[i].vx = 0.0f;    
            if (fabs(bolas[i].vz) < 0.0002f) bolas[i].vz = 0.0f;
        }

        // Colisão com as Bordas
        // Compensando para o raio da bola (centro da bola não pode ultrapassar a borda falsa (borda - raio da bola) e a borda da bola "bate" na borda da mesa)
        float lim_x = (MESA_LARGURA / 2.0f) - RAIO_BOLA;
        float lim_z = (MESA_COMPRIMENTO / 2.0f) - RAIO_BOLA;

        // Colisão com as bordas da mesa (inversão de velocidade)
        if (bolas[i].x < -lim_x) { bolas[i].x = -lim_x; bolas[i].vx = -bolas[i].vx; }
        else if (bolas[i].x > lim_x) { bolas[i].x = lim_x; bolas[i].vx = -bolas[i].vx; }

        // Colisão com as bordas da mesa (inversão de velocidade)
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
            float dist = sqrt(dx * dx + dz * dz);
            float minDist = 2.0f * RAIO_BOLA;       // Distância mínima para colisão (diâmetro da bola, raio de uma + o raio da outra)

            if (dist < minDist && dist > 0.0f) {
                // Normal da colisão
                float nx = dx / dist;
                float nz = dz / dist;

                // Troca de impulso elástico (massas iguais)
                float kx = bolas[i].vx - bolas[j].vx;
                float kz = bolas[i].vz - bolas[j].vz;
                // float p = 2.0f * (nx * kx + nz * kz) / 2.0f;
                float p = (nx * kx + nz * kz);      // Projeção do vetor velocidade relativa na normal da colisão

                // Atualiza velocidades das bolas após a colisão:
                // Bola[i] perde velocidade (impulso) na direção da normal
                bolas[i].vx -= p * nx;
                bolas[i].vz -= p * nz;
                // Bola[j] ganha essa velocidade
                bolas[j].vx += p * nx;
                bolas[j].vz += p * nz;

                // Separação geométrica para evitar colagem (overlap)
                float overlap = 0.5f * (minDist - dist);
                // Move as bolas para fora da colisão na direção da normal
                bolas[i].x -= overlap * nx;
                bolas[i].z -= overlap * nz;
                bolas[j].x += overlap * nx;
                bolas[j].z += overlap * nz;
            }
        }
    }

    // Retorna ao modo de mira se todas as bolas pararem
    if (!movimento && !taco.mirando) {
        taco.mirando = 1;
    }
}

// Executa a tacada na bola branca
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
void desenhar_mesa() {
    // Material do Pano Verde
    GLfloat mat_ambiente[] = { 0.0f, 0.4f, 0.1f, 1.0f };
    GLfloat mat_difusa[]   = { 0.1f, 0.7f, 0.2f, 1.0f };
    GLfloat mat_especular[]= { 0.1f, 0.2f, 0.1f, 1.0f };
    GLfloat mat_brilho     = 10.0f;

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambiente);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_difusa);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_especular);
    glMaterialf (GL_FRONT, GL_SHININESS, mat_brilho);

    // Tampo Principal Verde
    // Redimensiona o cubo unitário para as dimensões da mesa
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

    // Borda Esquerda & Direita
    for (int side = -1; side <= 1; side += 2) {
        // Posiciona a borda lateral (esquerda ou direita) com base na largura da mesa e na espessura da borda, evitando que a borda fique dentro do tampo da mesa.
        glPushMatrix();
        glTranslatef(side * (MESA_LARGURA / 2.0f + espessura / 2.0f), alt_borda / 4.0f, 0.0f);
        glScalef(espessura, alt_borda, MESA_COMPRIMENTO + 2.0f * espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Borda Superior & Inferior
    for (int side = -1; side <= 1; side += 2) {
        // Posiciona a borda superior ou inferior com base no comprimento da mesa e na espessura da borda, evitando que a borda fique dentro do tampo da mesa.
        glPushMatrix();
        glTranslatef(0.0f, alt_borda / 4.0f, side * (MESA_COMPRIMENTO / 2.0f + espessura / 2.0f));
        glScalef(MESA_LARGURA, alt_borda, espessura);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

void desenhar_bolas() {
    // Material especular para as bolas (reflexo de luz)
    GLfloat mat_especular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_especular);
    glMaterialf (GL_FRONT, GL_SHININESS, 64.0f);

    for (int i = 0; i < MAX_BOLAS; i++) {
        if (!bolas[i].ativa) continue;

        GLfloat mat_cor[] = { bolas[i].r, bolas[i].g, bolas[i].b, 1.0f };
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cor);

        // Redimensiona a esfera sólida para o tamanho da bola de sinuca e aplica a rotação visual simulando o giro da bola conforme ela se move.
        glPushMatrix();
        glTranslatef(bolas[i].x, bolas[i].y, bolas[i].z);
        glRotatef(bolas[i].rot_x, 1.0f, 0.0f, 0.0f);
        glRotatef(bolas[i].rot_z, 0.0f, 0.0f, 1.0f);
        glutSolidSphere(RAIO_BOLA, 24, 24);
        glPopMatrix();
    }
}

void desenhar_taco_e_mira() {
    if (!taco.mirando) return;

    glPushMatrix();
    glTranslatef(bolas[0].x, bolas[0].y, bolas[0].z);
    glRotatef(taco.angulo * (180.0f / M_PI), 0.0f, 1.0f, 0.0f);

    // Linha de Guia / Mira
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

    // Deslocamento para trás baseado na força
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
// CALLBACKS DO GLUT (DISPLAY, CAM, INPUT)
// ==========================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Posição Orbital da Câmera 3D (converte angulos de graus para radianos)
    float rad_yaw = cam.cam_yaw * (M_PI / 180.0f);
    float rad_pitch = cam.cam_pitch * (M_PI / 180.0f);

    // Calcula a posição da câmera em coordenadas cartesianas a partir dos ângulos de yaw e pitch, mantendo uma distância fixa da cena.
    float eye_x = cam.cam_dist * sin(rad_yaw) * cos(rad_pitch);
    float eye_y = cam.cam_dist * sin(rad_pitch);
    float eye_z = cam.cam_dist * cos(rad_yaw) * cos(rad_pitch);

    // A câmera foca na bola branca ou no centro da mesa
    float target_x = bolas[0].x;
    float target_z = bolas[0].z;

    // Configura a visão da câmera usando gluLookAt, posicionando a câmera na posição calculada e olhando para a bola branca (soma de target).
    gluLookAt(eye_x + target_x, eye_y, eye_z + target_z,
              target_x, 0.0f, target_z, 0.0f, 1.0f, 0.0f);

    // Configuração de Iluminação
    GLfloat light_pos[] = { 0.0f, 4.0f, 0.0f, 1.0f };
    GLfloat light_amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_dif[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);

    // Renderizar Elementos
    desenhar_mesa();
    desenhar_bolas();
    desenhar_taco_e_mira();

    // Interface HUD
    char hud[128];
    snprintf(hud, sizeof(hud), "Forca: %.2f | Efeito: %.3f | [Espaco]: Dar Tacada | [R]: Reiniciar",
             taco.forca, taco.desvio_impacto);
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

// Controle via Teclado
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

// Teclas Especiais (Setas do teclado orientam o taco)
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

// Interação de Câmera com Mouse (Orbitar com botão esquerdo)
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

// Setup das Propriedades do OpenGL
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
    glutInitWindowSize(640, 480);
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