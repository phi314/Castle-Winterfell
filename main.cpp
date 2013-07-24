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
#include "color.h"
#endif

using namespace std;

int w=900, h=650, z=-12;
int arg=0, n=3;
float mult = 1, v = 1.0;
float sx=1, sy=1, sz=1;
float rx=20.0f, ry =50.0f;


float rot = 0;

static GLfloat spin = 0.0;
float angle = 0;

static int viewx = 0;
static int viewy = 100;
static int viewz = 500;

/* animation */
int x_tower = 1;
int min_x_tower = 0, max_x_tower=10;

float j=0;

GLUquadricObj *quad = gluNewQuadric();

//texture
GLuint texture[40];


typedef struct ImageTexture ImageTexture; //struktur data untuk

struct ImageTexture {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};


const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 0.0f, 1.0f, 1.0f, 0.0f };

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

// Type Data Terrain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;
Terrain* _terrainJalan;

// Untuk Display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float scale = 1000.0f / max(terrain->width() - 1, terrain->length() - 1);
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


/*
 * Load Texture Bata Hitam
 */
ImageTexture * loadTexture_bata() {
	ImageTexture *image1;
	// alokasi memmory untuk tekstur
	image1 = (ImageTexture *) malloc(sizeof(image1));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!ImageLoad("dinding.bmp", image1)) {
		exit(1);
	}
	return image1;
}

/*
 * Load Texture Wood.bmp
 */
ImageTexture * loadTexture_kayu() {
	ImageTexture *image1;
	// alokasi memmory untuk tekstur
	image1 = (ImageTexture *) malloc(sizeof(image1));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!ImageLoad("wood.bmp", image1)) {
		exit(1);
	}
	return image1;
}

void freetexture(GLuint texture) {
	glDeleteTextures(1, &texture);
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

void square()
{
    glPushMatrix();
        glColor3f(0,0,0);
            glBegin(GL_QUADS);
            glVertex2f(0.2,0.8);
            glVertex2f(-0.2,0.8);
            glVertex2f(-0.2,0.2);
            glVertex2f(0.2,0.2);
            glEnd();
        glColor3f(White);
    glPopMatrix();

}

void wall(float x1,float y1,float z1,float x2,float y2,float z2, int t=0)
{
    glPushMatrix();
     glEnable(GL_TEXTURE_2D);
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
             glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void cube(int t=0)
{
    glPushMatrix();
        glTranslated(0.0,0.0,0.0);
        glScaled(3,3,3);
        wall(0.0,0,0,0.1,0.1,0.1,t);
	glPopMatrix();
}

void ring()
{
//horizontal
    glPushMatrix();
        glTranslated(-3,5.1,0.5);
        glRotated(90,1.0,0.0,0.0);
        glRotated(-35,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(-1.8,5.1,2.2);
        glRotated(90,1.0,0.0,0.0);
        glRotated(-65,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(1.6,5.1,2);
        glRotated(90,1.0,0.0,0.0);
        glRotated(65,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(2.5,5.1,0.3);
        glRotated(90,1.0,0.0,0.0);
        glRotated(35,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,9.8,0.3);
        glRotated(-180,1,0,0);
        glPushMatrix();
            glTranslated(-3,5.1,0.5);
            glRotated(90,1.0,0.0,0.0);
            glRotated(-35,0.0,0.0,1.0);
            glScaled(4.0,20.0,4.0);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();
        glPushMatrix();
            glTranslated(-1.8,5.1,2.2);
            glRotated(90,1.0,0.0,0.0);
            glRotated(-65,0.0,0.0,1.0);
            glScaled(4.0,20.0,4.0);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();
        glPushMatrix();
            glTranslated(1.6,5.1,2);
            glRotated(90,1.0,0.0,0.0);
            glRotated(65,0.0,0.0,1.0);
            glScaled(4.0,20.0,4.0);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();
        glPushMatrix();
            glTranslated(2.5,5.1,0.3);
            glRotated(90,1.0,0.0,0.0);
            glRotated(35,0.0,0.0,1.0);
            glScaled(4.0,20.0,4.0);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();
    glPopMatrix();
}



void balcon_tower()
{
    // balcon tower
    glPushMatrix();
        glColor3f(DimGray);
        glTranslated(0.0,3.3,0.0);
        glScaled(1.0,0.4,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,1,2.5,4,50,50);
    glPopMatrix();
     glPushMatrix();
        glColor3f(DimGray);
        glTranslated(0.0,4.9,0.0);
        glScaled(1.0,0.4,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,2.5,1.2,3,50,50);
    glPopMatrix();
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,6,0.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0,1.2,50,1);
    glPopMatrix();
    //------------------
//top2
    //1
    glPushMatrix();
        glTranslated(-1.3,3.5,0);
        glRotated(45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
     //2
     glPushMatrix();
        glTranslated(1,3.7,0);
        glRotated(-45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //3
     glPushMatrix();
        glTranslated(-0.2,3.7,1);
        glRotated(45,1.0,.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //4
    glPushMatrix();
        glTranslated(-0.2,3.5,-1.1);
        glRotated(-45,1.0,.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //5
    glPushMatrix();
        glTranslated(-1,3.5,0.9);
        glRotated(45,1.0,0.0,1.0);
        glRotated(45,0.0,1.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //6
    glPushMatrix();
        glTranslated(0.9,3.7,0.6);
        glRotated(-45,0.0,1.0,0.0);
        glRotated(-45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();


//------------------
//top3
    //1
    glPushMatrix();
        glTranslated(-2.6,5.1,0);
        glRotated(-45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
     //2
     glPushMatrix();
        glTranslated(2.5,4.8,0);
        glRotated(45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //3
     glPushMatrix();
        glTranslated(-0.2,4.8,2.3);
        glRotated(-45,1.0,0.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //4
    glPushMatrix();
        glTranslated(-0.2,5.1,-2.5);
        glRotated(45,1.0,0.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //5
    glPushMatrix();
        glTranslated(-1.8,5.2,1.6);
        glRotated(-45,1.0,0.0,1.0);
        glRotated(45,0.0,1.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //6
    glPushMatrix();
        glTranslated(1.8,4.8,1.6);
        glRotated(-45,0.0,1.0,0.0);
        glRotated(45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
     //7
    glPushMatrix();
        glTranslated(1.8,4.8,-1.6);
        glRotated(45,0.0,1.0,0.0);
        glRotated(45,0.0,0.0,1.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //8
   glPushMatrix();
        glTranslated(-2.3,4.8,-1.8);
        glRotated(45,0.0,1.0,0.0);
        glRotated(45,1.0,0.0,0.0);
        glScaled(4.0,20.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
//ring center
    ring();

//ring top
    glPushMatrix();
        glScaled(0.5,0.5,0.5);
        glTranslated(0.0,7.2,0.0);
        ring();
    glPopMatrix();
//ring bottom
    glPushMatrix();
        glScaled(0.5,0.5,0.5);
        glTranslated(0.0,2,0.0);
        ring();
    glPopMatrix();
}

void tower_solid()
{
    glPushMatrix();
        glColor3f(Black);
        glTranslated(0.0,0.0,0.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,1.2,1.2,4.5,50,50);
    glPopMatrix();

}

void tower()
{
    // menara polos
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.0,0.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,1.2,1.2,4.5,50,50);
    glPopMatrix();

    balcon_tower();


//------------------
    //1
    glPushMatrix();
        glTranslated(-1.3,0,0);
        glScaled(4.0,40.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //2
    glPushMatrix();
        glTranslated(0.9,0,0);
        glScaled(4.0,40.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //3
    glPushMatrix();
        glTranslated(-0.2,0,0.9);
        glScaled(4.0,40.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //4
    glPushMatrix();
        glTranslated(-0.2,0,-1.3);
        glScaled(4.0,40.0,4.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
    //5
     glPushMatrix();
        glTranslated(-1,0,0.9);
        glScaled(4.0,40.0,4.0);
        glRotated(45,0.0,1.0,0.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();
     //6
     glPushMatrix();
        glTranslated(0.8,0,0.6);
        glScaled(4.0,40.0,4.0);
        glRotated(-45,0.0,1.0,0.0);
        wall(0.0,0,0,0.1,0.1,0.1,1);
    glPopMatrix();

}

void atap()
{
    glPushMatrix();
        glTranslated(0,0.0,0);
        glScaled(3,3,30);
        cube(1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(8,0.0,0);
        glScaled(3,3,30);
        cube(1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,0.0,10);
        glRotated(90,0.0,1.0,0.0);
        glScaled(3,3,30);
        cube(1);
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,0.0,0);
        glRotated(90,0.0,1.0,0.0);
        glScaled(3,3,30);
        cube(1);
    glPopMatrix();
}

void tutup()
{
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,6,0.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0,1.2,50,1);
    glPopMatrix();
}

void rumah_kiri()
{
    glPushMatrix();
        glTranslated(0,0,0);
        glScaled(5,18,8);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(1,0,0);
        glScaled(8,9,10);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-1,0,-2);
        glScaled(16,15,8);
        cube();
    glPopMatrix();
}


void tower_castle()
{
    //-----TOWER--------
// tower

    //tower 1
    glPushMatrix();
        glTranslated(-8,-2,-9);
        glScaled(1.2,1.2,1.2);
        tower();
    glPopMatrix();
    //tower 2
    glPushMatrix();
        glTranslated(6,3,-9);
        glScaled(1.7,1.7,1.7);
        tower();
    glPopMatrix();
    //tower 3
     glPushMatrix();
        glTranslated(-5.5,4,-11);
        glScaled(0.7,1,0.7);
        tower();
    glPopMatrix();
    //tower 4
     glPushMatrix();
        glTranslated(-2,3,-18);
        glScaled(2.5,2,2.5);
        balcon_tower();
    glPopMatrix();
    //tower 5 tengah
     glPushMatrix();
        glTranslated(6,0,-18);
        glScaled(1,1,1);
        tower();
    glPopMatrix();
    //tower 5 tengah
     glPushMatrix();
        glTranslated(17,0,-18);
        glScaled(2,2,2);
        tower();
    glPopMatrix();
    //tower 5 belakang
     glPushMatrix();
        glTranslated(1,12,-28);
        glScaled(2,2,2);
        tower();
    glPopMatrix();
    //tower 5 belakang
     glPushMatrix();
        glTranslated(6,8,-28);
        glScaled(1,1.7,1);
        tower();
    glPopMatrix();
}

/* tenda */


void tiang_tenda(int t=7)
{
    for(int i=0;i<t;i++)
    {
        glPushMatrix();
            glTranslated(0,0,i);
            glScaled(4.0,20.0,4.0);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();
    }
}

void atap_tenda(int solid=1)
{
    glColor3f(White);
    if(solid == 1)
    {
        glPushMatrix();

            glTranslated(0,0,0);
            glRotated(90,0,1,0);
            glRotated(45,1,0,0);
            glScaled(20.0,8.0,3.0);
            square();
        glPopMatrix();
        glPushMatrix();
            glTranslated(9,0,0);
            glRotated(90,0,1,0);
            glRotated(-45,1,0,0);
            glScaled(20.0,8.0,3.0);
            square();
        glPopMatrix();
    }
    else
    {
    glPushMatrix();
        glTranslated(-0.4,2,0);
        glRotated(-45,0,0,1);
        tiang_tenda();
    glPopMatrix();
    glPushMatrix();
        glTranslated(2.6,1.8,0);
        glRotated(45,0,0,1);
        tiang_tenda();
    glPopMatrix();
    }



}

void tenda()
{
    //kiri
    glPushMatrix();
        glTranslated(0,0,0);
        tiang_tenda();
    glPopMatrix();

    //kanan
    glPushMatrix();
        glTranslated(3,0,0);
        tiang_tenda();
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(-0.5,0.5,3.3);
        glScaled(0.5,0.5,1);
        atap_tenda();
    glPopMatrix();

}

void pohon(int c=0)
{
    glPushMatrix();
        if(c==0)
            glColor3f(Green);
        else
            glColor3f(White);
        glTranslated(0.0,0.0,0.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.5,0,4.5,50,50);
    glPopMatrix();
}

void pepohonan()
{
        glPushMatrix();
            glTranslated(70,0,9);
            glScaled(2,2,2);
            pohon();
        glPopMatrix();
        glPushMatrix();
            glTranslated(75,2,1);
            glScaled(2,2,2);
            pohon();
        glPopMatrix();
        glPushMatrix();
            glTranslated(60,0,5);
            glScaled(2,2,2);
            pohon();
        glPopMatrix();
         glPushMatrix();
            glTranslated(70,0,2);
            glScaled(2,2,2);
            pohon();
        glPopMatrix();
}

void tower_turret()
{
    //tower1
    glPushMatrix();
        glTranslated(0,0,0);
        glScaled(3,4,3);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,1.2,0);
        glScaled(0.1,0.2,0.1);
        atap();
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,1.2,0.5);
        glScaled(0.1,0.1,0.1);
        atap_tenda();
    glPopMatrix();
     glPushMatrix();
        glTranslated(0.4,0.8,1.1);
        glScaled(0.2,0.3,0.1);
        square();
    glPopMatrix();

}

void pasukan()
{
    for(int i=0; i<10; i++)
    {
        glPushMatrix();
            glColor3f(White);
            glTranslated(-6,0.0,i);
            glScaled(4,9,2);
            cube();
        glPopMatrix();
    }
}

void castle()
{
     glPushMatrix();
        glTranslated(-6,0.0,-8);
        glScaled(15,10,10);
        cube();
    glPopMatrix();
    //tower 1
    glPushMatrix();
        glTranslated(3,0,-12);
        glScaled(20,20,20);
         cube();
    glPopMatrix();
    //main
    glPushMatrix();
        glTranslated(-7,0.0,-17);
        glScaled(35,20,30);
        cube();
    glPopMatrix();
    //atas kecil
    glPushMatrix();
        glTranslated(-5,3,-19);
        glScaled(10,20,30);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(0,0.0,-20);
        glScaled(20,15,30);
        cube();
    glPopMatrix();
    //kanan belakang
    glPushMatrix();
        glTranslated(3,0.0,-28);
        glScaled(50,30,30);
        cube();
    glPopMatrix();
     glPushMatrix();
        glTranslated(8,6.0,-28);
        glScaled(8,8,8);
        tower_turret();
    glPopMatrix();
    //kiri belakang
    glPushMatrix();
        glTranslated(-5,0.0,-28);
        glScaled(30,43,30);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-5,2.0,-33);
        glScaled(30,43,30);
        cube();
    glPopMatrix();
    //paling belakang
    glPushMatrix();
        glTranslated(-8,0,-38);
        glScaled(30,40,30);
        cube();
    glPopMatrix();


    //atapatap
     glPushMatrix();
        glTranslated(-7.3,6.0,-17);
        glScaled(1.2,1.2,1);
        atap();
    glPopMatrix();
     glPushMatrix();
        glTranslated(2,9.0,-28);
        glScaled(1.8,2,1);
        atap();
    glPopMatrix();

    //jendela
        glPushMatrix();
            glTranslated(0,3.0,-7.9);
            glScaled(1.8,2,1);
            square();
        glPopMatrix();
        //pintu depan
        glPushMatrix();
            glTranslated(6,-0.8,-5.9);
            glScaled(3,2,1);
            square();
        glPopMatrix();
        glPushMatrix();
            glTranslated(4,3.0,-5.9);
            glScaled(1.8,1,1);
            square();
        glPopMatrix();
         glPushMatrix();
            glTranslated(6,3.0,-5.9);
            glScaled(1.8,1,1);
            square();
        glPopMatrix();
        glPushMatrix();
            glTranslated(8,3.0,-5.9);
            glScaled(1.8,1,1);
            square();
        glPopMatrix();

        //pintu kanan
         glPushMatrix();
            glTranslated(11.3,0,-18.9);
            glScaled(10,15,1);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();

         glPushMatrix();
            glTranslated(12,0,3.9);
            glScaled(30,20,1);
            wall(0.0,0,0,0.1,0.1,0.1,1);
        glPopMatrix();

}


void benteng()
{
    //lurus
    glPushMatrix();
        glTranslated(0,0,0);
        glScaled(15,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(4.5,0,0);
        glScaled(15,3,1);
        cube();
    glPopMatrix();
    //kanan
    glPushMatrix();
        glTranslated(8.8,0,0);
        glRotated(70,0,1,0);
        glScaled(10,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(9.9,0,-2.8);
        glRotated(90,0,1,0);
        glScaled(20,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(9.9,0,-9);
        glRotated(100,0,1,0);
        glScaled(8,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(9.6,0,-11.4);
        glRotated(120,0,1,0);
        glScaled(15,3,1);
        cube();
    glPopMatrix();
    //lurus
    glPushMatrix();
        glTranslated(1.5,0,-15);
        glScaled(20,3,1);
        cube();
    glPopMatrix();
    //kiri
    glPushMatrix();
        glTranslated(-1,0,-3);
        glRotated(-70,0,1,0);
        glScaled(10,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-1.2,0,-3);
        glRotated(90,0,1,0);
        glScaled(20,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-0.6,0,-11.6);
        glRotated(-100,0,1,0);
        glScaled(8,3,1);
        cube();
    glPopMatrix();
    glPushMatrix();
        glTranslated(1.6,0,-14.9);
        glRotated(-120,0,1,0);
        glScaled(15,3,1);
        cube();
    glPopMatrix();

    //tower1
    glPushMatrix();
        glTranslated(4,0,-0.2);
        tower_turret();
    glPopMatrix();
     //tower2
    glPushMatrix();
        glTranslated(6,0,-0.2);
        tower_turret();
    glPopMatrix();
}


void renderScene(void){
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);

	glDisable( GL_CULL_FACE );

/*------------
 * TERRAIN
 -------------*/
// tanah merah
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[0]);
        drawSceneTanah(_terrainTanah,  0.1f, 0.1f, 0.1f);
	glPopMatrix();

// air
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[0]);
        drawSceneTanah(_terrainAir,  0.0f, 0.2f, 0.5f);
	glPopMatrix();

	// tanah rumput
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[1]);
        drawSceneTanah(_terrain,  0.9f, 0.9f, 0.9f);
	glPopMatrix();

	// tanah jalan
	glPushMatrix();
        //glBindTexture(GL_TEXTURE_3D, texture[1]);
        //drawSceneTanah(_terrainJalan,  0.4f, 0.2f, 0.1f);
	glPopMatrix();

	glColor3f(1.0f,1.0f,1.0f);


/*------------
 * END TERRAIN
 -------------*/
 glPushMatrix();
     glScaled(3,3,3);
    glTranslated(0,0,50);

/*------------
 * THE VILLAGE
 -------------*/
 //kanan
    glPushMatrix();
        glTranslated(50,0,-35);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
     glPushMatrix();
        glTranslated(80,0,-20);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
    glPushMatrix();
        glTranslated(50,0,-13);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
    glPushMatrix();
        glTranslated(23,0,-38);
        glScaled(1.5,1.8,1.8);
        glRotated(90,0,1,0);
        tenda();
    glPopMatrix();
    //kiri
    glPushMatrix();
        glTranslated(-35,0,-5);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-20,0,-5);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-5,0,-5);
        glScaled(2,2,2);
        tenda();
    glPopMatrix();
/*------------
 * END VILLAGE
 -------------*/

/*------------
 * THE CASTLE
 -------------*/

    glPushMatrix();
        glScaled(3,3,3);
        castle();
    glPopMatrix();
    glPushMatrix();
        glTranslated(-35,0,8);
        glScaled(14,14,9);
        benteng();
    glPopMatrix();
    glPushMatrix();
        glScaled(4,4,4);
        glTranslated(-13,0,-12);
        rumah_kiri();
    glPopMatrix();
    glPushMatrix();
        glScaled(3,3,3);
        tower_castle();
    glPopMatrix();

/*------------
 * END CASTLE
 -------------*/

/*------------
 * THE SCENE
 -------------*/
    glPushMatrix();
        glTranslated(0,0,-60);
        pepohonan();
    glPopMatrix();
    glPushMatrix();
        glTranslated(5,0,-60);
        pepohonan();
    glPopMatrix();
    glPushMatrix();
        glTranslated(4,0,-20);
        pepohonan();
    glPopMatrix();
/*------------
 * END SCENE
 -------------*/

 /*------------
 * THE ARMY
 -------------*/
 glPushMatrix();
        glTranslated(0,0,10);
        //pasukan();
    glPopMatrix();
 /*------------
 * END ARMY
 -------------*/


glPopMatrix();

	glutSwapBuffers();
	glFlush();
    rot++;
	angle++;
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
	if (key == 'a') {
		viewz++;
	}
	if (key == 'd') {
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
	 gluPerspective(45.0,(float) w1/(float) h1, 1.0,1000.0);
	 glMatrixMode(GL_MODELVIEW);

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
	glEnable(GL_CULL_FACE);



    _terrain = loadTerrain("heightmap.bmp", 20);
    _terrainTanah = loadTerrain("heightmapTanah.bmp", 20);
    _terrainAir = loadTerrain("heightmapAir.bmp", 20);
    _terrainJalan = loadTerrain("heightmapJalan.bmp", 12);


    ImageTexture *image1 = loadTexture_bata();
    ImageTexture *image2 = loadTexture_kayu();


	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(5,texture);


    /* texture bata hitam */

	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB,GL_UNSIGNED_BYTE, image1->data);

    /* texture kayu */

	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image2->sizeX, image2->sizeY, 0, GL_RGB,GL_UNSIGNED_BYTE, image2->data);

}

int main (int argc, char **argv){
	 glutInit(&argc, argv);
	 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	 glutInitWindowPosition(100,100);
	 glutInitWindowSize(w,h);
	 glutCreateWindow("Winterfell House of Stark");
	 init();

	 glutDisplayFunc(renderScene);
	 	glutIdleFunc(renderScene);
	 glutReshapeFunc(reshape);
	 glutKeyboardFunc(keyboard);
	 glutSpecialFunc(kibor);
	 glutMouseFunc(kursor);
	 glutMotionFunc(motion);
	 glutTimerFunc(5,timer,0);


    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);


	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);


	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	 glutMainLoop();
}
