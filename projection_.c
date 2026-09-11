#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415

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

// Posicoes (X, Y) independentes para cada objeto
float posX_Retangulo = 0.0f, posY_Retangulo = 0.0f, posZ_Retangulo = 0.0f;
float cam_X = 0.0f, cam_Y = 0.0f, cam_Z = 6.0f;

// Valor da mudança de angulo
float angulo = 45.0f;

// Dimensoes da janela para mapeamento de coordenadas do mouse
int larguraJanela = 900;
int alturaJanela = 900;

void ConfiguraVisualizacao()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        cam_X, cam_Y, cam_Z, // Posicao
        0, 1, 0,             // Alvo referẽncia
        0, 1, 0              // Vetor UP
    );
}

void ConfiguraProjecao()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(angulo, larguraJanela / alturaJanela, 0.4, 500);

    // Define matriz de projeção 2D, porém com volume?
    // glOrtho(-1.0f, 1.0f,
    //         -1.0f, 1.0f,
    //         -1.0f, 1.0f);
}

void Face(int angulo, float x, float y, float z)
{
    glPushMatrix();
    glRotated(angulo, x, y, z);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.3f, -0.3f, 0.3f);
    glVertex3f(-0.3f, 0.3f, 0.3f);
    glVertex3f(0.3f, 0.3f, 0.3f);
    glVertex3f(0.3f, -0.3f, 0.3f);
    glEnd();
    glPopMatrix();
}

void Retangulo(float posX_Retangulo, float posY_Retangulo, float posZ_Retangulo)
{
    glPushMatrix();
    // glTranslatef(posX_Retangulo, posY_Retangulo, posZ_Retangulo);

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
    ConfiguraVisualizacao();

    // Push & Pop para pilha de rotação. Permite rodar apenas uma figura, se desejável. Roda as figuras dento do bloco
    glPushMatrix();
    glRotated(angulo, rotation[0], rotation[1], rotation[2]);  // Recebe o vetor unitátio do eixo que o objeto irá rodar
    Retangulo(posX_Retangulo, posY_Retangulo, posZ_Retangulo); // Como foi chamado dentro do bloco, roda as figuras do bloco
    glTranslatef(1, 1, 0);                                     // Podemos usar translate para trocar a posição ao invés de gerar numa posição diferente;
    Retangulo(posX_Retangulo, posY_Retangulo, posZ_Retangulo);
    //Retangulo(posX_Retangulo + 2, posY_Retangulo, posZ_Retangulo);
    glPopMatrix();
    // Retangulo(posX_Retangulo - 2, posY_Retangulo, posZ_Retangulo); //Retangulo que não será rodado, pois está fora do bloco

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
    case '1':
        rotation[0]++;
        if (rotation[0] > 1 || rotation[0] < 0)
            rotation[0] = 0;
        break; // Seleciona Retangulo
    case '2':
        rotation[1]++;
        if (rotation[1] > 1 || rotation[1] < 0)
            rotation[1] = 0;
        break; // Seleciona Losango
    case '3':
        rotation[2]++;
        if (rotation[2] > 1 || rotation[2] < 0)
            rotation[2] = 0;
        break; 
    case 'W':
    case 'w':
        cam_Y++;
        break; 
    case 'S':
    case 's':
        cam_Y--;
        break; 
    case 'D':
    case 'd':
        cam_X++;
        break; // Seleciona cor Amarelo
    case 'A':
    case 'a':
        cam_X--;
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
    float passo = 1.0f;
    float *ang = NULL;

    ang = &angulo;

    if (ang != NULL)
    {
        switch (key)
        {
        case GLUT_KEY_LEFT:
            *ang -= passo;
            break;
        case GLUT_KEY_RIGHT:
            *ang += passo;
            break;
        case GLUT_KEY_UP:
            *ang += passo;
            break;
        case GLUT_KEY_DOWN:
            *ang -= passo;
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

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