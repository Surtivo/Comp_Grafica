#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include "mybib.h"
#include "mybib.c"

#define PI 3.1415

ObjModel model;

GLfloat rotation[3] = {0.0f, 0.0f, 0.0f};

// Posicoes (X, Y) independentes para cada objeto
float posX_Retangulo = 0.0f, posY_Retangulo = 0.0f, posZ_Retangulo = 0.0f;
float cam_X = 0.0f, cam_Y = 0.0f, cam_Z = 6.0f;

float cam_Rad = 6.0f;     // Distancia da camera
float cam_AngleXZ = 0.0f; // Angulo da camera
float cam_AngleYZ = 0.0f; // Angulo da camera

// Valor da mudança de angulo
float angulo = 45.0f;

// Dimensoes da janela para mapeamento de coordenadas do mouse
int larguraJanela = 900;
int alturaJanela = 900;

void ConfiguraVisualizacao()
{

    cam_X = cam_Rad * sin(cam_AngleXZ * PI / 180.0f);
    cam_Z = cam_Rad * cos(cam_AngleXZ * PI / 180.0f);

    cam_Y = cam_Rad * sin(cam_AngleYZ * PI / 180.0f);
    // cam_Z = cam_Rad * cos(cam_AngleYZ * PI / 180.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        cam_X, cam_Y, cam_Z, // Posicao
        0, 0, 0,             // Alvo referẽncia
        0, 1, 0              // Vetor UP
    );
}

void ConfiguraProjecao()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(angulo, larguraJanela / alturaJanela, 0.4, 500);
}

void Desenha(void)
{
    // Limpa os buffers de Cor e Profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ConfiguraVisualizacao();

    // Push & Pop para pilha de rotação. Permite rodar apenas uma figura, se desejável. Roda as figuras dento do bloco
    loadOBJ("testednv2.obj", "testednv2.mtl", &model);
    // InicializaMatriz();
    drawModel(&model);

    glutSwapBuffers();
}

void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
    // Evita a divisao por zero
    if (h == 0)
        h = 1;

    // Especifica as dimensoes da Viewport
    glViewport(0, 0, w, h);

    // Inicializa o sistema de coordenadas
    ConfiguraProjecao();

    // Como a função de vi
    // glMatrixMode(GL_MODELVIEW);
    // glLoadIdentity();
}

void Teclado(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0); // ESC para sair

    switch (key)
    {
    case 'W':
    case 'w':
        cam_AngleYZ += 5.0f; // Incrementa o ângulo da câmera
        break;
    case 'S':
    case 's':
        cam_AngleYZ -= 5.0f; // Decrementa o ângulo da câmera
        break;
    case 'D':
    case 'd':
        cam_AngleXZ += 5.0f; // Incrementa o ângulo da câmera
        break;               // Seleciona cor Amarelo
    case 'A':
    case 'a':
        cam_AngleXZ -= 5.0f; // Decrementa o ângulo da câmera
        break;
    case 'E':
    case 'e':
        cam_Z++;
        break; // Seleciona cor Amarelo
    case 'Q':
    case 'q':
        cam_Z--;
        break;
    }
    glutPostRedisplay();
}

// Função callback chamada para gerenciar eventos do mouse
void GerenciaMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
        if (state == GLUT_DOWN)
        {
            // Executar uma acao definida pelo programador
            // quando o usuario clicar com o botao do mouse (LMB)
        }
    glutPostRedisplay();
}

void TeclasEspeciais(int key, int x, int y)
{

    glutPostRedisplay();
}

void Inicializa(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);

    // Habilita o teste de profundidade (Z-Buffer)
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    // Inicializa GLUT com suporte a profundidade (GLUT_DEPTH)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowPosition(5, 5);
    glutInitWindowSize(larguraJanela, alturaJanela);
    glutCreateWindow("Esfera do Blender Girando");

    glutDisplayFunc(Desenha);
    glutReshapeFunc(AlteraTamanhoJanela);
    glutKeyboardFunc(Teclado);
    glutMouseFunc(GerenciaMouse);
    glutSpecialFunc(TeclasEspeciais);

    Inicializa();

    glutMainLoop();
    return 0;
}
// gcc luz_exemplo.c -o programa -lglut -lGLU -lGL -lm