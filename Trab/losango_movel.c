//*****************************************************
//
// QuadradoVerde.c
// Um programa OpenGL simples que abre uma janela GLUT
// e faz o desenho de um quadrado verde em fundo branco
//
//*****************************************************

#include <stdlib.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415
int varObj = -1;

int losXUP = 1;
int losYR = 1;
int losXDOWN = -1;
int losYL = -1;

// Variáveis para controlar a translação do losango
float transX = 0.0f;
float transY = 0.0f;

// Variáveis auxiliares para o controle de arrasto
bool arrastando = false;
float mouseInicX = 0.0f;
float mouseInicY = 0.0f;

// Variáveis para armazenar o tamanho atual da janela (usadas na conversão de coordenadas)
int larguraJanela = 450;
int alturaJanela = 450;

GLfloat cores[3][3] = {
    {1.0f, 0.0f, 0.0f}, // Vermelho
    {0.0f, 0.0f, 1.0f}, // Azul
    {1.0f, 1.0f, 0.0f}  // Amarelo
};

int corAtiva = -1;

void Losango()
{
    if (varObj == 2)
    {
        switch (corAtiva)
        {
        case 1:
            glColor3f(1.0f, 0.0f, 0.0f); // Correção: Vermelho baseado na matriz de cores comentada
            break;
        case 2:
            glColor3f(0.0f, 1.0f, 0.0f); // Verde
            break;
        case 3:
            glColor3f(0.0f, 0.0f, 1.0f); // Azul
            break;
        default:
            glColor3f(1.0f, 1.0f, 0.0f); // Amarelo padrão caso nenhuma cor seja acionada
            break;
        }
    }
    else
    {
        glColor3f(1.0f, 1.0f, 0.0f);
    }

    glBegin(GL_QUADS);

    // Vertices em ordem anti-horaria modificados pela translação atual:
    glVertex2f(losXDOWN + transX, -0.0f + transY);
    glVertex2f(-0.0f + transX, losYR + transY);
    glVertex2f(losXUP + transX, 0.0f + transY);
    glVertex2f(0.0f + transX, losYL + transY);
    glEnd();
}

// Funcao de  callback de redesenho da janela de visualizacao
void Desenha(void)
{
    // Limpa a janela de visualizacao com a cor
    // de fundo definida previamente
    glClear(GL_COLOR_BUFFER_BIT);

    Losango();

    // Executa os comandos OpenGL para atualizar o frame buffer (exibe o desenho da tela)
    glFlush();
}

// Funcao callback chamada quando o tamanho da janela eh alterado
void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
    // Evita a divisao por zero
    if (h == 0)
        h = 1;

    // Guarda as dimensões atuais da janela para uso no cálculo do mouse
    larguraJanela = w;
    alturaJanela = h;

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

// Função auxiliar para converter a coordenada de pixel do Mouse para o Mundo OpenGL 2D
void ConverteCoordenadas(int x, int y, float *mundoX, float *mundoY)
{
    float esq, dir, inf, sup;

    // Recria os limites matemáticos usados no gluOrtho2D
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

    // Mapeamento linear do pixel da tela para o espaço cartesiano
    *mundoX = esq + ((float)x / larguraJanela) * (dir - esq);
    *mundoY = sup - ((float)y / alturaJanela) * (sup - inf); // Inverte Y porque na tela o 0 é no topo
}

// Função callback chamada para gerenciar eventos do mouse (clique)
void GerenciaMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            float mX, mY;
            ConverteCoordenadas(x, y, &mX, &mY);

            // Equação simplificada de detecção para a área do Losango (Bounding Box / Diamante)
            // Verifica se o clique está dentro dos limites espaciais atuais do Losango deslocado
            float dx = fabs(mX - transX);
            float dy = fabs(mY - transY);

            if ((dx / 1.0f) + (dy / 1.0f) <= 1.0f)
            {
                arrastando = true;
                mouseInicX = mX - transX; // Salva a distância do clique até o centro do objeto
                mouseInicY = mY - transY;
            }
        }
        else if (state == GLUT_UP)
        {
            arrastando = false;
        }
    }
    glutPostRedisplay();
}

// Função callback chamada para gerenciar o movimento do mouse com o botão pressionado
void GerenciaMovimento(int x, int y)
{
    if (arrastando)
    {
        float mX, mY;
        ConverteCoordenadas(x, y, &mX, &mY);

        // Atualiza a translação subtraindo a folga inicial para evitar pulos no clique
        transX = mX - mouseInicX;
        transY = mY - mouseInicY;

        glutPostRedisplay();
    }
}

// Função callback chamada para gerenciar eventos do teclado para teclas especiais
void TeclasEspeciais(int key, int x, int y)
{
    if (key == GLUT_KEY_UP)
    {
        transY += 0.05f; // Move o losango para cima via teclado
    }
    if (key == GLUT_KEY_DOWN)
    {
        transY -= 0.05f; // Move o losango para baixo via teclado
    }
    glutPostRedisplay();
}

// Funcao responsavel por inicializar parametros e variaveis
void Inicializa(void)
{
    // Define a cor de fundo da janela de visualizaca como branca
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
