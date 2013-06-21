#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int w=900, h=850, z=-12;
int x1=0, y1n=0, sudut=0, z1=0;
int arg=0, n=3;
float mult = 1, v = 1.0;
float sx=1, sy=1, sz=1;
float rx=20.0f, ry =50.0f;

float j=0;

GLUquadricObj *quad = gluNewQuadric();

void cube(double x, double y, double z, double w, double angle=0, double yAxis=1)
{
	glPushMatrix();
        glTranslatef(x,y,z);
        glEnable(GL_COLOR_MATERIAL);
        glColor3f(1,1,1);
        glRotatef(angle,0,yAxis,0);
        glutSolidCube(w);
        glDisable(GL_COLOR_MATERIAL);
	glPopMatrix();
}

void menara(double x, double y, double z, double w=0.9)
{
    glPushMatrix();
        glTranslated(x,y,z);
        glRotated(90, -1.0, 0.0, 0.0);
        gluCylinder(quad, w, w, 4, 50, 50);
    glPopMatrix();
}

void kuncup(double x, double y, double z, bool datar=false)
{
    glPushMatrix();
        glTranslated(x,y,z);
        glRotated(90, -1.0, 0.0, 0.0);
        if(datar == false)
            gluCylinder(quad, 0.9, 0.1, 2, 50, 50);
        else
            gluCylinder(quad, 2, 0.9, 1, 50, 50);
    glPopMatrix();
}

void gate()
{
    glPushMatrix();
        glTranslated(0,-0.5,1);
        glRotated(90, -1.0, 0.0, 0.0);
        glutSolidCube(2);
    glPopMatrix();
}


void renderScene(void){
	GLfloat LightPosition[] = {rx, ry, 20.0f, 0.0f};
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0,0,0,0);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glLoadIdentity();
	glTranslatef(0,0,z);
	glRotatef(sudut,x1,y1n,z1);
	glScalef(sx, sy, sz);

	//bagian tembok utama
    cube(0,0,0,3);

    //lantai 1
    cube(0,2,0,3);
    //pagar atas


    //balconi
    for(int i=-1.5; i<2; i++)
    {
        cube(i,1,1.5,0.3);
    }




	//gerbang
	gate();

    //tower kanan
    cube(-3.4,0,1,3,30,1);
    menara(2.4,-1.6,0);
    kuncup(2.4,2.1,0);
    menara(4.4,-1.6,1.8,1.5);
    kuncup(4.4,2.1,1.8,false);

    //tower kiri
    cube(3.4,0,1,3,-30,1);
    menara(-2.4,-1.6,0);
    kuncup(-2.4,2.1,0);
    menara(-4.4,-1.6,1.8,1.5);


	glutSwapBuffers();
}

void kursor(int tombol, int state, int x, int y){
	rx = x-(w/2);
	ry = (h/2)-y;
}

void motion (int x, int y){
	rx = x-(w/2);
	ry = (h/2)-y;
}

void keyboard_gua(unsigned char key, int x, int y){
	if (key =='a') z+=1;
	else if (key == 'd') z-=1;

	else if (key == 'w') {
		x1+=1;
		y1n=0;
		z1=0;
		sudut+=10;
	}
	else if (key == 's') {
		x1-=1;
		y1n=0;
		z1=0;
		sudut+=10;
	}
	else if (key == 'y') {
		y1n=1;
		x1=0;
		z1=0;
		sudut+=-10;
	}
	else if (key == 'q') {
        exit(1);
	}
}

void specialkeyboard_gua(int key, int x, int y){
	switch(key)
	{
	case GLUT_KEY_UP : z+=1;
		break;
	case GLUT_KEY_DOWN : z-=1;
		break;
	case GLUT_KEY_LEFT : x1=0; y1n=1; z1=0; sudut+=-10;
		break;
	case GLUT_KEY_RIGHT : x1=0; y1n=1; z1=0; sudut+=10;
		break;
	}
}

void resize(int w1, int h1){
	 glViewport(0,0,w1,h1);
	 glMatrixMode(GL_PROJECTION);
	 glLoadIdentity();
	 gluPerspective(45.0,(float) w1/(float) h1, 1.0,300.0);
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
}

void timer(int value){
	 glutPostRedisplay();
	 glutTimerFunc(50,timer,0);
}

void init(){
	 GLfloat LightPosition[] = {10.0f, 10.0f, 20.0f, 0.0f};
	 GLfloat LightAmbient[] = {0.0f, 1.0f, 0.0f, 1.0f};
	 GLfloat LightDiffuse[] = {0.7f, 0.7f, 0.7f, 1.0f};
	 GLfloat LightSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
	 GLfloat Shine[] = { 80 };

	 glShadeModel(GL_SMOOTH);
	 glClearColor(0.0f,0.0f,0.0f,0.5f);
	 glClearDepth(1.0f);
	 glEnable(GL_DEPTH_TEST);
	 glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	 glMaterialfv(GL_FRONT, GL_SPECULAR, LightSpecular);
	 glMaterialfv(GL_FRONT, GL_SHININESS, Shine);

	 glEnable(GL_LIGHTING);
	 glEnable(GL_LIGHT0);
	 return;
}

int main (int argc, char **argv){
	 glutInit(&argc, argv);
	 glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	 glutInitWindowPosition(100,100);
	 glutInitWindowSize(w,h);
	 glutCreateWindow("Kings Landing");

	 glutDisplayFunc(renderScene);
	 glutReshapeFunc(resize);
	 glutKeyboardFunc(keyboard_gua);
	 glutSpecialFunc(specialkeyboard_gua);
	 glutMouseFunc(kursor);
	 glutMotionFunc(motion);
	 glutTimerFunc(5,timer,0);
	 init();
	 glutMainLoop();
}
