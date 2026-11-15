#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415
int varObj = -1;

float losXUP = 0.4f;
float losYR = 0.0f;
float losXDOWN = -0.4f;
float losYL = -0.4f;

// Variáveis para controlar a translação do losango
float transX = 0.0f;
float transY = 0.5f;

// Variáveis para controlar a translação do retângulo
float transXQ = 0.0f;
float transYQ = 0.5f;

// Variáveis auxiliares para o controle de arrasto
bool arrastando = false;
float mouseInicX = 0.0f;
float mouseInicY = 0.0f;

// Variáveis para armazenar o tamanho atual da janela (usadas na conversão de coordenadas)
int larguraJanela = 450;
int alturaJanela = 450;

int forma = 0;

GLfloat cores[3][3] = {
    {1.0f, 0.0f, 0.0f}, // Vermelho
    {0.0f, 0.0f, 1.0f}, // Azul
    {1.0f, 1.0f, 0.0f}  // Amarelo
};

int corAtiva = -1;

void Losango()
{
    // ID do Losango é 1 conforme mapeado na função Desenha_forma
    if (varObj == 1)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
            break;
        case 2:
            glColor3f(0.0f, 1.0f, 0.0f); // Verde
            break;
        case 3:
            glColor3f(0.0f, 0.0f, 1.0f); // Azul
            break;
        default:
            glColor3f(1.0f, 1.0f, 0.0f); // Amarelo padrão
            break;
        }
    }
    else
    {
        glColor3f(1.0f, 1.0f, 0.0f);
    }

    glBegin(GL_QUADS);
    // Vértices do losango rotacionados/ajustados baseados no centro (transX, transY)
    glVertex2f(losXDOWN + transX, 0.0f + transY);
    glVertex2f(0.0f + transX, losYL + transY);
    glVertex2f(losXUP + transX, 0.0f + transY);
    glVertex2f(0.0f + transX, fabs(losYL) + transY);
    glEnd();
}

void Retangulo()
{
    // ID do Retângulo é 2 conforme mapeado na função Desenha_forma
    if (varObj == 2)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
            break;
        case 2:
            glColor3f(0.0f, 0.0f, 1.0f); // Azul
            break;
        case 3:
            glColor3f(1.0f, 1.0f, 0.0f); // Amarelo
            break;
        default:
            glColor3f(0.0f, 1.0f, 0.0f); // Verde padrão
            break;
        }
    }
    else
    {
        glColor3f(0.0f, 1.0f, 0.0f);
    }

    glBegin(GL_QUADS);
    glVertex2f(losXDOWN + transXQ, losYL + transYQ);
    glVertex2f(losXUP + transXQ, losYL + transYQ);
    glVertex2f(losXUP + transXQ, fabs(losYL) + transYQ);
    glVertex2f(losXDOWN + transXQ, fabs(losYL) + transYQ);
    glEnd();
}

void alter_form(int indice)
{
    forma = indice;
}

int request_form()
{
    return forma;
}

void Desenha_forma()
{
    // 1 para Losango, 2 para Retângulo
    int index = 1;

    switch (index)
    {
    case 1:
        Losango();
        alter_form(index);
        break;

    case 2:
        Retangulo();
        alter_form(index);

    default:
        break;
    }
}

void Desenha(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    Desenha_forma();
    glFlush();
}

void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
    if (h == 0)
        h = 1;

    larguraJanela = w;
    alturaJanela = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w <= h)
        gluOrtho2D(-1.0f, 1.0f, -1.0f * h / w, 1.0f * h / w);
    else
        gluOrtho2D(-1.0f * w / h, 1.0f * w / h, -1.0f, 1.0f);
}

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
    case 'r':
        corAtiva = 1;
        break;
    case 'G':
    case 'g':
        corAtiva = 2;
        break;
    case 'B':
    case 'b':
        corAtiva = 3;
        break;
    }
    glutPostRedisplay();
}

void ConverteCoordenadas(int x, int y, float *mundoX, float *mundoY)
{
    float esq, dir, inf, sup;

    if (larguraJanela <= alturaJanela)
    {
        esq = -1.0f;
        dir = 1.0f;
        inf = -1.0f * (float)alturaJanela / larguraJanela;
        sup = 1.0f * (float)alturaJanela / larguraJanela;
    }
    else
    {
        esq = -1.0f * (float)larguraJanela / alturaJanela;
        dir = 1.0f * (float)larguraJanela / alturaJanela;
        inf = -1.0f;
        sup = 1.0f;
    }

    *mundoX = esq + ((float)x / larguraJanela) * (dir - esq);
    *mundoY = sup - ((float)y / alturaJanela) * (sup - inf);
}

void GerenciaMouse(int button, int state, int x, int y)
{
    int index = request_form();

    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            float mX, mY;
            ConverteCoordenadas(x, y, &mX, &mY);
            float dx, dy;

            switch (index)
            {
            case 1: // LÓGICA DO LOSANGO
                dx = fabs(mX - transX);
                dy = fabs(mY - transY);

                // Equação geométrica de região interna de um Losango/Diamante regular
                // Baseado nos limites de raio horizontal (0.4) e vertical (0.4)
                if ((dx / 0.4f) + (dy / 0.4f) <= 1.0f)
                {
                    arrastando = true;
                    mouseInicX = mX - transX;
                    mouseInicY = mY - transY;
                }
                break;

            case 2: // LÓGICA DO RETÂNGULO
                dx = mX - transXQ;
                dy = mY - transYQ;

                // Verificação Bounding Box tradicional (retangular)
                // Checa se o mouse está dentro de [-0.4, 0.4] nos eixos X e Y locais do objeto
                if (dx >= -0.4f && dx <= 0.4f && dy >= -0.4f && dy <= 0.4f)
                {
                    arrastando = true;
                    mouseInicX = mX - transXQ;
                    mouseInicY = mY - transYQ;
                }
                break;
            }
        }
        else if (state == GLUT_UP)
        {
            arrastando = false;
        }
    }
    glutPostRedisplay();
}

void GerenciaMovimento(int x, int y)
{
    if (arrastando)
    {
        float mX, mY;
        ConverteCoordenadas(x, y, &mX, &mY);
        int index = request_form();

        // Move estritamente o objeto ativo na tela
        if (index == 1)
        {
            transX = mX - mouseInicX;
            transY = mY - mouseInicY;
        }
        else if (index == 2)
        {
            transXQ = mX - mouseInicX;
            transYQ = mY - mouseInicY;
        }

        glutPostRedisplay();
    }
}

void TeclasEspeciais(int key, int x, int y)
{
    int index = request_form();
    float *targetY = (index == 1) ? &transY : &transYQ;

    if (key == GLUT_KEY_UP)
    {
        *targetY += 0.05f;
    }
    if (key == GLUT_KEY_DOWN)
    {
        *targetY -= 0.05f;
    }
    glutPostRedisplay();
}

void Inicializa(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// Programa Principal
int main(int argc, char **argv)
{
    srand(time(NULL));
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

    // Especifica a posicao inicial da janela GLUT
    glutInitWindowPosition(5, 5);

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(450, 450);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Losango OpenGL");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(Desenha);

    // Registra a funcao callback de redimensionamento da janela de visualizacao
    glutReshapeFunc(AlteraTamanhoJanela);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(Teclado);

    // Registra a funcao callback para tratamento do clique do mouse
    glutMouseFunc(GerenciaMouse);

    // REGISTRO NOVO: Escuta o arrastar do mouse com clique pressionado
    glutMotionFunc(GerenciaMovimento);

    // Registra a funcao callback para tratamento das teclas especiais (F1, Home, etc...)
    glutSpecialFunc(TeclasEspeciais);

    // Chama a funcao responsavel por fazer as inicializacoes
    Inicializa();

    // Inicia o processamento e aguarda interacoes do usuario
    glutMainLoop();

    return 0;
}
