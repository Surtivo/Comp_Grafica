//*****************************************************
// Feito com ajuda da JS****
//*****************************************************

#include <stdlib.h>
#include <GL/glut.h>

float angY = 30.0f;
float angX = -20.0f;

void Face()
{
    // quadrado de lado 1 no plano z = 0.5
    glBegin(GL_QUADS);
    // Vertices em ordem anti-horaria:
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glEnd();
}

void Cubo()
{
    // frente - vermelha
    glColor3f(1.0f, 0.0f, 0.0f);
    Face();

    // tras - verde
    glPushMatrix();
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    Face();
    glPopMatrix();

    // direita - azul
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    Face();
    glPopMatrix();

    // esquerda - amarela
    glPushMatrix();
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    Face();
    glPopMatrix();

    // topo - magenta
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 1.0f);
    Face();
    glPopMatrix();

    // base - ciano
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 1.0f);
    Face();
    glPopMatrix();
}

// Funcao de  callback de redesenho da janela de visualizacao
void Desenha(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // desloca o mundo em relacao ao observador inicial
    glTranslatef(-0.85f, 0.20f, 0.0f);

    glRotatef(angX, 1.0f, 0.0f, 0.0f);
    glRotatef(angY, 0.0f, 1.0f, 0.0f);

    Cubo();

    // segundo cubo um atras do outro, na mesma reta do eixo x
    glPushMatrix();
    glTranslatef(2.0f, 0.0f, 0.0f);
    Cubo();
    glPopMatrix();

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

    // Estabelece a janela de selecao (esquerda, direita, inferior, superior)
    // mantendo a proporcao com a janela de visualizacao
    if (w <= h)
        glOrtho(-1.6f, 1.6f, -1.6f * h / w, 1.6f * h / w, -2.0f, 2.0f);
    else
        glOrtho(-1.6f * w / h, 1.6f * w / h, -1.6f, 1.6f, -2.0f, 2.0f);
}

// Funcao callback chamada para gerenciar eventos de teclas
void Teclado(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0);

    glutPostRedisplay();
}

// Função callback chamada para gerenciar eventos do teclado
// para teclas especiais, tais como F1, PgDn e Home
void TeclasEspeciais(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT)
    {
        angY -= 5.0f;
    }
    if (key == GLUT_KEY_RIGHT)
    {
        angY += 5.0f;
    }
    if (key == GLUT_KEY_UP)
    {
        angX -= 5.0f;
    }
    if (key == GLUT_KEY_DOWN)
    {
        angX += 5.0f;
    }
    glutPostRedisplay();
}

// Funcao responsavel por inicializar parametros e variaveis
void Inicializa(void)
{
    // Define a cor de fundo da janela de visualizaca como branca
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);
}

// Programa Principal
int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);

    // Especifica a posicao inicial da janela GLUT
    glutInitWindowPosition(5, 5);

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(450, 450);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Cubo OpenGL");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(Desenha);

    // Registra a funcao callback de redimensionamento da janela de visualizacao
    glutReshapeFunc(AlteraTamanhoJanela);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(Teclado);

    // Registra a funcao callback para tratamento das teclas especiais (F1, Home, etc...)
    glutSpecialFunc(TeclasEspeciais);

    // Chama a funcao responsavel por fazer as inicializacoes
    Inicializa();

    // Inicia o processamento e aguarda interacoes do usuario
    glutMainLoop();

    return 0;
}
