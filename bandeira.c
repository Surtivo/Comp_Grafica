//*****************************************************
//
// QuadradoVerde.c
// Um programa OpenGL simples que abre uma janela GLUT
// e faz o desenho de um quadrado verde em fundo branco
//
//*****************************************************

#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415
int varObj = -1;

GLfloat cores[3][3] = {
    {1.0f, 0.0f, 0.0f}, // Vermelho
    {0.0f, 0.0f, 1.0f}, // Azul
    {1.0f, 1.0f, 0.0f}  // Amarelo
};

int corAtiva = -1;

void Elipse(float raio_hor, float raio_ver)
{
    if (varObj == 3)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(0.0f, 1.0f, 0.0f);
            break;
        case 2:
            glColor3f(0.0f, 0.0f, 1.0f);
            break;
        case 3:
            glColor3f(1.0f, 1.0f, 0.0f);
            break;
        default:
            break;
        }
    }
    else
    {
        glColor3f(0.0f, 0.0f, 1.0f);
    }

    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(0.0F, 0.0F); // Centro;
    for (int i = 0; i <= 360; i += 10)
    {
        float angulo = (i * PI / 180.0f);
        glVertex3f(cos(angulo) * raio_hor, sin(angulo) * raio_ver, 0);
    }
    glEnd();
}

void Losango()
{
    if (varObj == 2)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(0.0f, 1.0f, 0.0f);
            break;
        case 2:
            glColor3f(0.0f, 0.0f, 1.0f);
            break;
        case 3:
            glColor3f(1.0f, 1.0f, 0.0f);
            break;
        default:
            break;
        }
    }
    else
    {
        glColor3f(1.0f, 1.0f, 0.0f);
    }

    glBegin(GL_QUADS);

    // Vertices em ordem anti-horaria:
    glVertex2f(-1.0f, -0.0f);
    glVertex2f(-0.0f, 1.0f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(0.0f, -1.0f);
    glEnd();
}

void Retangulo()
{
    if (varObj == 1)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(0.0f, 1.0f, 0.0f);
            break;
        case 2:
            glColor3f(0.0f, 0.0f, 1.0f);
            break;
        case 3:
            glColor3f(1.0f, 1.0f, 0.0f);
            break;
        default:
            break;
        }
    }
    else
    {
        glColor3f(0.0f, 1.0f, 0.0f);
    }
    glBegin(GL_QUADS);

    // Vertices em ordem anti-horaria:
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);
    glEnd();
}

// Funcao de  callback de redesenho da janela de visualizacao
void Desenha(void)
{
    // Limpa a janela de visualizacao com a cor
    // de fundo definida previamente
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0f, 1.0f, 0.0f);
    Retangulo();
    Losango();
    Elipse(0.5, 0.5);

    // Executa os comandos OpenGL para atualizar o frame buffer (exibe o desenho da tela)
    glFlush();
}

// Funcao callback chamada quando o tamanho da janela eh alterado
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

    // Estabelece a janela de selecao (esquerda, direita, inferior,
    // superior) mantendo a proporcao com a janela de visualizacao
    if (w <= h)
        gluOrtho2D(-1.0f, 1.0f, -1.0f * h / w, 1.0f * h / w);
    else
        gluOrtho2D(-1.0f * w / h, 1.0f * w / h, -1.0f, 1.0f);
}

// Funcao callback chamada para gerenciar eventos de teclas
void Teclado(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0);

    switch (key)
    {
    case '1':
        varObj = 1;
        break;
    case '2':
        varObj = 2;
        break;
    case '3':
        varObj = 3;
        break;
    case 'R':
    case 'r': // muda a cor corrente para vermelho
        corAtiva = 1;
        break;
    case 'G':
    case 'g': // muda a cor corrente para verde
        corAtiva = 2;
        break;
    case 'B':
    case 'b': // muda a cor corrente para azul
        corAtiva = 3;
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
            // Executarah uma acao definida pelo programador
            // quando o usuario clicar com o botao do mouse (LMB)
        }
    glutPostRedisplay();
}

// Função callback chamada para gerenciar eventos do teclado
// para teclas especiais, tais como F1, PgDn e Home
void TeclasEspeciais(int key, int x, int y)
{
    if (key == GLUT_KEY_UP)
    {
        // acao definida ao pressionar a tecla seta acima
    }
    if (key == GLUT_KEY_DOWN)
    {
        // acao definida ao pressionar a tecla seta abaixo
    }
    glutPostRedisplay();
}

// Funcao responsavel por inicializar parametros e variaveis
void Inicializa(void)
{
    // Define a cor de fundo da janela de visualizaca como branca
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Altera a cor dos vertices para verde inicialmente
    glColor3f(0.0f, 1.0f, 0.0f);
}

// Programa Principal
int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

    // Especifica a posicao inicial da janela GLUT
    glutInitWindowPosition(5, 5);

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(450, 450);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Bandeira OpenGL");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(Desenha);

    // Registra a funcao callback de redimensionamento da janela de visualizacao
    glutReshapeFunc(AlteraTamanhoJanela);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(Teclado);

    // Registra a funcao callback para tratamento do mouse
    glutMouseFunc(GerenciaMouse);

    // Registra a funcao callback para tratamento das teclas especiais (F1, Home, etc...)
    glutSpecialFunc(TeclasEspeciais);

    // Chama a funcao responsavel por fazer as inicializacoes
    Inicializa();

    // Inicia o processamento e aguarda interacoes do usuario
    glutMainLoop();

    return 0;
}
