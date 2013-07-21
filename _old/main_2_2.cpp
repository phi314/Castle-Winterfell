#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

using namespace std;

int w=900, h=850, z=-12;
int arg=0, n=3;
float mult = 1, v = 1.0;
float sx=1, sy=1, sz=1;
float rx=20.0f, ry =50.0f;

static GLfloat spin = 0.0;
float angle = 0;

static int viewx = 0;
static int viewy = 20;
static int viewz = 40;

/* animation */
int x_tower = 1;
int min_x_tower = 0, max_x_tower=10;

float j=0;

GLUquadricObj *quad = gluNewQuadric();

//texture



typedef struct ImageTexture ImageTexture; //struktur data untuk

struct ImageTexture {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};


const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

unsigned int LoadTextureFromBmpFile(char *filename);


/*
 * Class Terrain
 */
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
/*
 * end class terrain
 */

/*
 * Loads a terrain from a heightmap.  The heights of the terrain range from
 * -height / 2 to height / 2.
 * load terain di procedure inisialisasi
 */
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

/*
 * Load gambar BMP
 */
int ImageLoad(char *filename, ImageTexture *imageTex) {
	FILE *file;
	unsigned long size; // ukuran image dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in image

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
	// mencari file header bmp
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&imageTex->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	//printf("Width of %s: %lu\n", filename, image->sizeX);
	// membaca nilai height
	if ((i = fread(&imageTex->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, image->sizeY);
	//menghitung ukuran image(asumsi 24 bits or 3 bytes per pixel).

	size = imageTex->sizeX * imageTex->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	imageTex->data = (char *) malloc(size);
	if (imageTex->data == NULL) {
		printf("Error allocating memory for color-corrected image data");
		return 0;
	}
	if ((i = fread(imageTex->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = imageTex->data[i];
		imageTex->data[i] = imageTex->data[i + 2];
		imageTex->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}


GLuint texture[10];
/*
 * Load Texture
 */
GLuint loadtextures(const char *filename, int width, int height) {
	GLuint texture;

	unsigned char *data;
	FILE *file;

	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;

	data = (unsigned char *) malloc(width * height * 3);
	fread(data, width * height * 3, 1, file);

	fclose(file);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(  GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB,
			GL_UNSIGNED_BYTE, data);

	data = NULL;

	return texture;
}

void freetexture(GLuint texture) {
	glDeleteTextures(1, &texture);
}

// Type Data Terrain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;

// Untuk Display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

/*
 * Membuat kotak
 * x = koordinat x
 * y = koordinat y
 * z = koordinat z
 * w = ukuran kotak
 * angle = rotasi
 * yAxis = merotasi pada sumbu y
 */
void cube(double x, double y, double z, double w, double angle=0, double yAxis=1)
{
	glPushMatrix();

        glTranslatef(x,y,z);
        glColor3f(1,1,1);
        glRotatef(angle,0,yAxis,0);
        glutSolidCube(w);
        //glDisable(GL_COLOR_MATERIAL);
	glPopMatrix();
}

/*
 * Membuat menara
 * x = koordinat x
 * y = koordinat y
 * z = koordinat z
 * w = ukuran atas dan bawah menara
 */
void menara(double x, double y, double z, double w=0.9)
{
    glPushMatrix();
        glTranslated(x,y,z);
        glRotated(90, -1.0, 0.0, 0.0);
        gluCylinder(quad, w, w, 4, 50, 50);
    glPopMatrix();
}

/*
 * Membuat kuncup
 * x = koordinat x
 * y = koordinat y
 * z = koordinat z
 * datar = bagian bawah datar
 */
void kuncup(double x, double y, double z, double w = 0.9)
{
    glPushMatrix();
        glTranslated(x,y,z);
        glRotated(90, -1.0, 0.0, 0.0);
        gluCylinder(quad, w, 0.1, 2, 50, 50);
    glPopMatrix();
}

/*
 * Membuat gerbang
 * x = koordinat x
 * y = koordinat y
 * z = koordinat z
 */
void gate()
{
    glPushMatrix();
        glTranslated(0,3,1);
        glRotated(90, -1.0, 0.0, 0.0);
        glutSolidCube(2);
    glPopMatrix();
}

void wall(float x1,float y1,float z1,float x2,float y2,float z2, int t=0)
{
    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D,texture[t]);
            glBegin(GL_QUADS); // Start drawing a quad primitive
	//depan
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x1,y1,z1);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x2,y1,z1);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x2,y2,z1);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x1,y2,z1);
	//atas
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x1,y2,z1);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x2,y2,z1);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x2,y2,z2);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x1,y2,z2);
	//belakang
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x1,y2,z2);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x2,y2,z2);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x2,y1,z2);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x1,y1,z2);
	//bawah
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x1,y1,z2);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x2,y1,z2);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x2,y1,z1);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x1,y1,z1);
	//samping kiri
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x1,y1,z1);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x1,y2,z1);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x1,y2,z2);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x1,y1,z2);
	//samping kanan
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x2,y1,z1);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x2,y2,z1);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x2,y2,z2);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x2,y1,z2);



            glEnd();
    glPopMatrix();
}


void kampung()
{
    cube(3,0,12,0.5,30,1); // rumah 1
    cube(2,0,13,0.5,60,1); // rumah 1
    cube(2.6,0,12,0.5); // rumah 1
    cube(3,0,12.4,0.5); // rumah 1
    cube(3,0,13.8,0.5); // rumah 1
    cube(-3,0,12,0.5,30,1); // rumah 1
    cube(-2,0,13,0.5,60,1); // rumah 1
    cube(-2.6,0,12,0.5); // rumah 1
    cube(-3,0,12.4,0.5); // rumah 1
    cube(-3,0,13.8,0.5); // rumah 1
}

void prajurit()
{
    cube(8,0,12,0.1);
}

/*
 *
 */


void renderScene(void){
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);

/*------------
 * TERRAIN
 -------------*/
// tanah rumput
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[1]);
        drawSceneTanah(_terrain, 0.3f, 0.9f, 0.0f);
	glPopMatrix();

// tanah merah
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[0]);
        drawSceneTanah(_terrainTanah, 0.7f, 0.2f, 0.1f);
	glPopMatrix();

// air
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[0]);
        drawSceneTanah(_terrainAir, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

	glColor3f(1,1,1);
/*------------
 * END TERRAIN
 -------------*/



/*------------
 * THE CASTLE
 -------------*/

glScaled(1.5,1.5,1.5);
//front-side
    wall(0,0,0,5,5,-5); //width tower 1
    wall(5,-4,-1,10,4,-2);// gerbang

    wall(5,-4,0,10,4,-2,0.2);// pintu

    wall(10,0,0,15,5,-5); //width tower 2
    wall(15,-4,-1,20,3,-2);// the wall 1
    wall(0,-4,-1,-5,3,-2);// the wall 2




/*------------
 * END CASTLE
 -------------*/
glScaled(1,1,1);
 kampung();

 prajurit();


	glutSwapBuffers();
	glFlush();
}


/*
 * User Input
 */

void kursor(int tombol, int state, int x, int y){
	rx = x-(w/2);
	ry = (h/2)-y;
}

void motion (int x, int y){
	rx = x-(w/2);
	ry = (h/2)-y;
}

static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;
	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {
		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
}

/*
 *  End User Input
 */

void reshape(int w1, int h1){
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

void init(void){

    glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_TEXTURE_2D);



    _terrain = loadTerrain("heightmap.bmp", 13);
    _terrainTanah = loadTerrain("heightmapTanah.bmp", 13);
    _terrainAir = loadTerrain("heightmapAir.bmp", 13);

    texture[0] = loadtextures("water.bmp",256,256);
    texture[1] = loadtextures("water.bmp",256,256);


}

int main (int argc, char **argv){
	 glutInit(&argc, argv);
	 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	 glutInitWindowPosition(100,100);
	 glutInitWindowSize(w,h);
	 glutCreateWindow("Winterfell");
	 init();

	 glutDisplayFunc(renderScene);
	 glutReshapeFunc(reshape);
	 glutKeyboardFunc(keyboard);
	 glutSpecialFunc(kibor);
	 glutMouseFunc(kursor);
	 glutMotionFunc(motion);
	 glutTimerFunc(5,timer,0);

    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);


	 glutMainLoop();
}
