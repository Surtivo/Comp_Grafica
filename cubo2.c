#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415

// Objeto selecionado (1 = Retangulo, 2 = Losango, 3 = Elipse)
int varObj = -1;
GLfloat rotation[3] = {0.0f, 0.0f, 0.0f};

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

// Posicoes (X, Y) independentes para cada objeto
float posX_Retangulo = 0.0f, posY_Retangulo = 0.0f, posZ_Retangulo = 0.0f;
float anguloX = 0.0f;
float anguloY = 0.0f;
float anguloZ = 0.0f;

// Dimensoes da janela para mapeamento de coordenadas do mouse
int larguraJanela = 450;
int alturaJanela = 450;

void Face(int angulo, float x, float y, float z)
{
    glPushMatrix();
    glRotated(angulo, x, y, z);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glEnd();
    glPopMatrix();
}

void Retangulo()
{
    glPushMatrix();
    glTranslatef(posX_Retangulo, posY_Retangulo, posZ_Retangulo);
    // Recebe o vetor unitátio do eixo que o objeto irá rodar
    // Como foi chamado antes de todas as figuras, vai rodar todas as formas
    glRotated(anguloX, 1.0f, 0.0f, 0.0f);
    glRotated(anguloY, 0.0f, 1.0f, 0.0f);
    glRotated(anguloZ, 0.0f, 0.0f, 1.0f);

    glColor3f(1.0f, 1.0f, 0.0f); // Cor: Amarelo
    Face(0, 0, 0, 1);
    glColor3f(1.0f, 0.0f, 0.0f); // Cor: Vermelho
    Face(180, 1, 0, 0);
    glColor3f(0.0f, 0.0f, 1.0f); // Cor: Azul
    Face(90, 0, 1, 0);
    glColor3f(0.0f, 1.0f, 0.0f); // Cor: Verde
    Face(-90, 0, 1, 0);
    glColor3f(0.0f, 1.0f, 1.0f);
    Face(90, 1, 0, 0);
    glColor3f(0.2f, 0.6f, 0.3f);
    Face(-90, 1, 0, 0);

    glPopMatrix();
}

void Desenha(void)
{
    // Limpa os buffers de Cor e Profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Push & Pop para pilha de rotação. Permite rodar apenas uma figura, se desejável
    glPushMatrix();
    // O Z-Buffer cuida da sobreposicao correta independentemente da ordem
    glPopMatrix();

    Retangulo();
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

    float passo = 1.0f;
    
    float *angX = NULL;
    float *angY = NULL;
    float *angZ = NULL;

    angX = &anguloX;
    angY = &anguloY;
    angZ = &anguloZ;

    if (angX != NULL)
    {
        switch (key)
        {
        case 'S':
        case 's':
            *angX -= passo;
            break;
        case 'W':
        case 'w':
            *angX += passo;
            break;
        }
    }
    if (angY != NULL)
    {
        switch (key)
        {
        case 'A':
        case 'a':
            *angY -= passo;
            break;
        case 'D':
        case 'd':
            *angY += passo;
            break;
        }
    }
    if (angZ != NULL)
    {
        switch (key)
        {
        case 'Q':
        case 'q':
            *angZ -= passo;
            break;
        case 'E':
        case 'e':
            *angZ += passo;
            break;
        }
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
    float passo = 1.0f;
    
    float *angX = NULL;
    float *angY = NULL;
    float *angZ = NULL;

    angX = &anguloX;
    angY = &anguloY;
    angZ = &anguloZ;

    if (angX != NULL)
    {
        switch (key)
        {
        case GLUT_KEY_LEFT:
            *angX -= passo;
            break;
        case GLUT_KEY_RIGHT:
            *angX += passo;
            break;
        }
    }
    if (angY != NULL)
    {
        switch (key)
        {
        case GLUT_KEY_UP:
            *angY -= passo;
            break;
        case GLUT_KEY_DOWN:
            *angY += passo;
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