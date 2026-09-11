#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415

// Objeto selecionado (1 = Retangulo, 2 = Losango, 3 = Elipse)
int varObj = -1;

// Matriz de cores RGB
// cores[0] = Vermelho | cores[1] = Azul | cores[2] = Amarelo
GLfloat cores[3][3] = {
    {1.0f, 0.0f, 0.0f}, // Indice 0: Vermelho
    {0.0f, 0.0f, 1.0f}, // Indice 1: Azul
    {1.0f, 1.0f, 0.0f}  // Indice 2: Amarelo
};

// Indice para a cor selecionada (1, 2 ou 3)
int corAtiva = -1;

// Profundidades fixas para o Z-Buffer (Valores maiores ficam na frente)
float zRetangulo = 0.0f; // Ao fundo
float zLosango = 0.2f;   // No meio
float zElipse = 0.4f;    // A frente

// Posicoes (X, Y) independentes para cada objeto
float posX_Retangulo = 0.0f, posY_Retangulo = 0.0f;
float posX_Losango = 0.0f, posY_Losango = 0.0f;
float posX_Elipse = 0.0f, posY_Elipse = 0.0f;

// Dimensoes da janela para mapeamento de coordenadas do mouse
int larguraJanela = 450;
int alturaJanela = 450;

void Elipse(float raio_hor, float raio_ver)
{
    glPushMatrix();
    glTranslatef(posX_Elipse, posY_Elipse, 0.0f);

    // Usa a matriz 'cores' em vez do switch case se o objeto estiver selecionado
    if (varObj == 3 && corAtiva >= 1 && corAtiva <= 3)
    {
        glColor3fv(cores[corAtiva - 1]);
    }
    else
    {
        glColor3f(0.0f, 0.0f, 1.0f); // Cor padrao: Azul
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, zElipse); // Centro da elipse
    for (int i = 0; i <= 360; i += 10)
    {
        float angulo = (i * PI / 180.0f);
        glVertex3f(cos(angulo) * raio_hor, sin(angulo) * raio_ver, zElipse);
    }
    glEnd();

    glPopMatrix();
}

void Losango()
{
    glPushMatrix();
    glTranslatef(posX_Losango, posY_Losango, 0.0f);

    if (varObj == 2 && corAtiva >= 1 && corAtiva <= 3)
    {
        glColor3fv(cores[corAtiva - 1]);
    }
    else
    {
        glColor3f(1.0f, 1.0f, 0.0f); // Cor padrao: Amarelo
    }

    glBegin(GL_QUADS);
    glVertex3f(-1.0f, -0.0f, zLosango);
    glVertex3f(-0.0f, 1.0f, zLosango);
    glVertex3f(1.0f, 0.0f, zLosango);
    glVertex3f(0.0f, -1.0f, zLosango);
    glEnd();

    glPopMatrix();
}

void Retangulo()
{
    glPushMatrix();
    glTranslatef(posX_Retangulo, posY_Retangulo, 0.0f);

    if (varObj == 1 && corAtiva >= 1 && corAtiva <= 3)
    {
        glColor3fv(cores[corAtiva - 1]);
    }
    else
    {
        glColor3f(0.0f, 1.0f, 0.0f); // Cor padrao: Verde
    }

    glBegin(GL_QUADS);
    glVertex3f(-1.0f, -1.0f, zRetangulo);
    glVertex3f(-1.0f, 1.0f, zRetangulo);
    glVertex3f(1.0f, 1.0f, zRetangulo);
    glVertex3f(1.0f, -1.0f, zRetangulo);
    glEnd();

    glPopMatrix();
}

void Desenha(void)
{
    // Limpa os buffers de Cor e Profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Push & Pop para pilha de rotação. Permite rodar apenas uma figura, se desejável
    glPushMatrix();
    // Recebe o vetor unitátio do eixo que o objeto irá rodar
    // Como foi chamado antes de todas as figuras, vai rodar todas as formas
    glRotated(85, 0, 1, 0);
    // O Z-Buffer cuida da sobreposicao correta independentemente da ordem
    Retangulo();
    Losango();
    Elipse(0.5, 0.5);
    glPopMatrix();

    glFlush();
}

void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
    // Evita a divisao por zero
    if (h == 0)
        h = 1;

    // Especifica as dimensoes da Viewport
    glViewport(0, 0, w, h);

    // Inicializa o sistema de coordenadas
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Define matriz de projeção 2D, porém com volume?
    glOrtho(-1.0f, 1.0f,
            -1.0f, 1.0f,
            -5.0f, 5.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Teclado(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0); // ESC para sair

    switch (key)
    {
    case '1':
        varObj = 1;
        break; // Seleciona Retangulo
    case '2':
        varObj = 2;
        break; // Seleciona Losango
    case '3':
        varObj = 3;
        break; // Seleciona Elipse
    case 'R':
    case 'r':
        corAtiva = 1;
        break; // Seleciona cor Vermelho
    case 'G':
    case 'g':
        corAtiva = 2;
        break; // Seleciona cor Azul
    case 'B':
    case 'b':
        corAtiva = 3;
        break; // Seleciona cor Amarelo
    }
    glutPostRedisplay();
}

// Função callback chamada para gerenciar eventos do mouse
void GerenciaMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
        if (state == GLUT_DOWN)
        {
            // Executarah uma acao definida pelo programador
            // quando o usuario clicar com o botao do mouse (LMB)
        }
    glutPostRedisplay();
}

void TeclasEspeciais(int key, int x, int y)
{
    float passo = 0.1f;
    float *px = NULL;
    float *py = NULL;

    if (varObj == 1)
    {
        px = &posX_Retangulo;
        py = &posY_Retangulo;
    }
    else if (varObj == 2)
    {
        px = &posX_Losango;
        py = &posY_Losango;
    }
    else if (varObj == 3)
    {
        px = &posX_Elipse;
        py = &posY_Elipse;
    }

    if (px != NULL && py != NULL)
    {
        switch (key)
        {
        case GLUT_KEY_LEFT:
            *px -= passo;
            break;
        case GLUT_KEY_RIGHT:
            *px += passo;
            break;
        case GLUT_KEY_UP:
            *py += passo;
            break;
        case GLUT_KEY_DOWN:
            *py -= passo;
            break;
        }
    }

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
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowPosition(5, 5);
    glutInitWindowSize(larguraJanela, alturaJanela);
    glutCreateWindow("Bandeira OpenGL - Z-Buffer & Interacao");

    glutDisplayFunc(Desenha);
    glutReshapeFunc(AlteraTamanhoJanela);
    glutKeyboardFunc(Teclado);
    glutMouseFunc(GerenciaMouse);
    glutSpecialFunc(TeclasEspeciais);

    Inicializa();

    glutMainLoop();
    return 0;
}