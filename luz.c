#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>

#define PI 3.1415

GLfloat rotation[3] = {0.0f, 0.0f, 0.0f};

// Matriz de cores RGB
GLfloat cores[3][3] = {
    {1.0f, 0.0f, 0.0f}, // Indice 0: Vermelho
    {0.0f, 0.0f, 1.0f}, // Indice 1: Azul
    {1.0f, 1.0f, 0.0f}  // Indice 2: Amarelo
};

int corAtiva = -1;

// Posicoes (X, Y) independentes para cada objeto
float posX_Retangulo = 0.0f, posY_Retangulo = 0.0f, posZ_Retangulo = 0.0f;
float cam_X = 0.0f, cam_Y = 0.0f, cam_Z = 6.0f;

float cam_Rad = 6.0f;     // Distancia da camera
float cam_AngleXZ = 0.0f; // Angulo da camera
float cam_AngleYZ = 0.0f; // Angulo da camera

float angulo = 45.0f;

int larguraJanela = 900;
int alturaJanela = 900;

void ConfiguraVisualizacao()
{
    cam_X = cam_Rad * sin(cam_AngleXZ * PI / 180.0f);
    cam_Z = cam_Rad * cos(cam_AngleXZ * PI / 180.0f);
    cam_Y = cam_Rad * sin(cam_AngleYZ * PI / 180.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        cam_X, cam_Y, cam_Z, // Posicao
        0, 0, 0,             // Alvo referencia
        0, 1, 0              // Vetor UP
    );
}

void ConfiguraProjecao()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(angulo, (GLfloat)larguraJanela / (GLfloat)alturaJanela, 0.4, 500);
}

// Atualizado: adicionado o cálculo/definição de vetores normais
void Face(int anguloRot, float x, float y, float z)
{
    glPushMatrix();
    glRotated(anguloRot, x, y, z);
    glBegin(GL_QUADS);
    
    // A normal aponta para fora no eixo Z local (0, 0, 1)
    glNormal3f(0.0f, 0.0f, 1.0f);
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

    glColor3f(1.0f, 1.0f, 0.0f); // Amarelo
    Face(0, 0, 0, 1);
    glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
    Face(180, 1, 0, 0);
    glColor3f(0.0f, 0.0f, 1.0f); // Azul
    Face(90, 0, 1, 0);
    glColor3f(0.0f, 1.0f, 0.0f); // Verde
    Face(-90, 0, 1, 0);
    glColor3f(0.0f, 1.0f, 1.0f); // Ciano
    Face(90, 1, 0, 0);
    glColor3f(0.2f, 0.6f, 0.3f);
    Face(-90, 1, 0, 0);

    glPopMatrix();
}

void Desenha(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    ConfiguraVisualizacao();

    // Posiciona a luz fixa no mundo 3D (acima da cena: Y = 10)
    GLfloat posicaoLuz[4] = {0.0f, 10.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, posicaoLuz);

    glPushMatrix();
    Retangulo(posX_Retangulo, posY_Retangulo, posZ_Retangulo);
    glTranslatef(1.0f, 1.0f, 0.0f);
    Retangulo(posX_Retangulo, posY_Retangulo, posZ_Retangulo);
    glPopMatrix();

    glutSwapBuffers();
}

void AlteraTamanhoJanela(GLsizei w, GLsizei h)
{
    if (h == 0) h = 1;
    larguraJanela = w;
    alturaJanela = h;

    glViewport(0, 0, w, h);
    ConfiguraProjecao();
}

void Teclado(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);

    switch (key)
    {
        case 'W': case 'w': cam_AngleYZ += 5.0f; break; 
        case 'S': case 's': cam_AngleYZ -= 5.0f; break; 
        case 'D': case 'd': cam_AngleXZ += 5.0f; break; 
        case 'A': case 'a': cam_AngleXZ -= 5.0f; break;
        case 'E': case 'e': cam_Z++; break; 
        case 'Q': case 'q': cam_Z--; break;
    }
    glutPostRedisplay();
}

void GerenciaMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) { }
    glutPostRedisplay();
}

void TeclasEspeciais(int key, int x, int y)
{
    glutPostRedisplay();
}

void Inicializa(void)
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Fundo escuro

    // Parametros da Luz Ciano
	GLfloat luzAmbiente[4]={0.6,0.6,0.6,1.0}; 
	GLfloat luzDifusa[4]={0.7,0.7,0.7,1.0};	   		// "cor" 
	GLfloat luzEspecular[4]={1.0, 1.0, 1.0, 1.0};	// "brilho" 
	GLfloat posicaoLuz[4]={0.0, 50.0, 50.0, 1.0};

    // Propriedades de brilho do material
    GLfloat especularidade[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLint especMaterial = 60;

    glShadeModel(GL_SMOOTH);

    // Configura os materiais
    glMaterialfv(GL_FRONT, GL_SPECULAR, especularidade);
    glMateriali(GL_FRONT, GL_SHININESS, especMaterial);

    // Configura a Luz 0
    glLightfv(GL_LIGHT0, GL_AMBIENT, luzAmbiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luzDifusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luzEspecular);

    // Ativa iluminação e o rastreamento da cor dos materiais via glColor
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Habilita normalizacao automatica das normais apos transformacoes de escala/rotacao
    glEnable(GL_NORMALIZE);

    // Habilita o teste de profundidade (Z-Buffer)
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(5, 5);
    glutInitWindowSize(larguraJanela, alturaJanela);
    glutCreateWindow("Cubo com Iluminacao Ciano Superior");

    glutDisplayFunc(Desenha);
    glutReshapeFunc(AlteraTamanhoJanela);
    glutKeyboardFunc(Teclado);
    glutMouseFunc(GerenciaMouse);
    glutSpecialFunc(TeclasEspeciais);

    Inicializa();

    glutMainLoop();
    return 0;
}