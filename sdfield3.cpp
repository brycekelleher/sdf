#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef WIN32
#include "freeglut/include/GL/freeglut.h"
#else
#include <GL/freeglut.h>
#endif

#define PI 3.14159265358979323846f

static char *filename;

static int renderwidth, renderheight;

static float viewangles[2];
static float viewpos[3];

static float viewvector[3];

// Input
typedef struct input_s
{
	int mousepos[2];
	int moused[2];
	bool lbuttondown;
	bool rbuttondown;
	bool keys[256];

} input_t;

static int mousepos[2];
static input_t input;

// ==============================================
// memory allocation

void *Mem_Alloc(int numbytes)
{
	#define MEM_ALLOC_SIZE	16 * 1024 * 1024

	typedef struct memstack_s
	{
		unsigned char mem[MEM_ALLOC_SIZE];
		int allocated;

	} memstack_t;

	static memstack_t memstack;
	unsigned char *mem;
	
	if(memstack.allocated + numbytes > MEM_ALLOC_SIZE)
	{
		printf("Error: Mem: no free space available\n");
		abort();
	}

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	return mem;
}

// ==============================================
// errors and warnings

static void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);

	printf("Error: %s", buffer);
	exit(1);
}

static void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);

	fprintf(stdout, "Warning: %s", buffer);
}

// ==============================================
// vector utils

static void Vec2_Copy(float a[2], float b[2])
{
	a[0] = b[0];
	a[1] = b[1];
}

static void Vec2_Normalize(float *v)
{
	float len, invlen;

	len = sqrtf((v[0] * v[0]) + (v[1] * v[1]));
	invlen = 1.0f / len;

	v[0] *= invlen;
	v[1] *= invlen;
}

// return the circular distance
static float Vec2_Length(float v[2])
{
	return sqrtf((v[0] * v[0]) + (v[1] * v[1]));
}

static float Vec2_Dot(float a[2], float b[2])
{
	return (a[0] * b[0]) + (a[1] * b[1]);
}

static void Plane2d(float abc[3], float a[2], float b[2])
{
	float x0, y0, x1, y1, x, y, l, nx, ny, d;

	x0 = a[0], y0 = a[1];
	x1 = b[0], y1 = b[1];

	x = x1 - x0;
	y = y1 - y0;
	l = sqrt((x * x) + (y * y));

	nx =  y / l;
	ny = -x / l;
	d = -x0 * nx - y0 * ny;

	abc[0] = nx, abc[1] = ny, abc[2] = d;
}

static float PlaneDistance(float abc[3], float xy[2])
{
	float a, b, c, x, y;

	a = abc[0], b = abc[1], c = abc[2];
	x = xy[0], y = xy[1];
	return (a * x) + (b * y) + c;
}

// Called every frame to process the current mouse input state
// We only get updates when the mouse moves so the current mouse
// position is stored and may be used for mulitple frames
static void ProcessInput()
{
	// mousepos has current "frame" mouse pos
	input.moused[0] = mousepos[0] - input.mousepos[0];
	input.moused[1] = mousepos[1] - input.mousepos[1];
	input.mousepos[0] = mousepos[0];
	input.mousepos[1] = mousepos[1];
}

static float CircleDistance(float p[2], float r)
{
	return Vec2_Length(p) - r;
}

#undef min
#define min(a, b) (a < b ? a : b)

#undef max
#define max(a, b) (a > b ? a : b)

static float BoxDistance(float p[2])
{
	float d[2];
	float b[2] = { 1, 0.2 };

	d[0] = abs(p[0]) - b[0];
	d[1] = abs(p[1]) - b[1];

	float d2[2] = { max(d[0], 0.0f), max(d[1], 0.0f) };
	return min(max(d[0], d[1]), 0.0f) + Vec2_Length(d2);
}

#if 0
static float TriangleDistance(float p[2])
{
	static float abc0[3], abc1[3], abc2[3];
	static bool init;

	if (!init)
	{
#if 1
		static float v0[2] = { 0.2f, 0.4f };
		static float v1[2] = { 0.8f, 0.2f };
		static float v2[2] = { 0.5f, 0.8f };
#endif

		Plane2d(abc0, v0, v1);
		Plane2d(abc1, v1, v2);
		Plane2d(abc2, v2, v0);
	}

	float f0 = PlaneDistance(abc0, p);
	float f1 = PlaneDistance(abc1, p);
	float f2 = PlaneDistance(abc2, p);

	return max(f0, max(f1, f2));
}
#endif

static float TriangleDistance(float p[2])
{
	static float abc0[3], abc1[3], abc2[3];
	static float n00[2], n01[2], n10[2], n11[2], n20[2], n21[2];
	static bool init;

	static float v0[2] = { 0.2f, 0.4f };
	static float v1[2] = { 0.8f, 0.2f };
	static float v2[2] = { 0.5f, 0.8f };

	float v0p[2], v1p[2], v2p[2];

	if (!init)
	{

		Plane2d(abc0, v0, v1);
		Plane2d(abc1, v1, v2);
		Plane2d(abc2, v2, v0);

		// calculate positive and negative skewed normals
		n00[0] = -abc2[1], n00[1] =  abc2[0];
		n01[0] =  abc0[1], n01[1] = -abc0[0];
		n10[0] = -abc0[1], n10[1] =  abc0[0];
		n11[0] =  abc1[1], n11[1] = -abc1[0];
		n20[0] = -abc1[1], n20[1] =  abc1[0];
		n21[0] =  abc2[1], n21[1] = -abc2[0];
	}

	v0p[0] = p[0] - v0[0], v0p[1] = p[1] - v0[1];
	v1p[0] = p[0] - v1[0], v1p[1] = p[1] - v1[1];
	v2p[0] = p[0] - v2[0], v2p[1] = p[1] - v2[1];

	if (Vec2_Dot(n00, v0p) > 0.0f && Vec2_Dot(n01, v0p) > 0.0f)
		return Vec2_Length(v0p);
	if (Vec2_Dot(n10, v1p) > 0.0f && Vec2_Dot(n11, v1p) > 0.0f)
		return Vec2_Length(v1p);
	if (Vec2_Dot(n20, v2p) > 0.0f && Vec2_Dot(n21, v2p) > 0.0f)
		return Vec2_Length(v2p);

	float f0 = PlaneDistance(abc0, p);
	float f1 = PlaneDistance(abc1, p);
	float f2 = PlaneDistance(abc2, p);

	return max(f0, max(f1, f2));
}


static float Distance(float p[2])
{
	return TriangleDistance(p);
}

#if 0
static float Distance(float p[2], float r)
{
	return CircleDistance(p, r);
}
#endif

#if 0
static float Distance(float p[2], float r)
{
	float d0, d1;

	float p0[2] = { p[0], p[1] };
	d0 = CircleDistance(p0, r);

	float p1[2] = { p[0] - 1, p[1] };
	d1 = CircleDistance(p1, r);

	return (d0 < d1 ? d0 : d1);
}
#endif

#if 0
static float Distance(float p[2], float r)
{
	return BoxDistance(p);
}
#endif

static void Gradient(float grad[2], float p[2], float r)
{
	float h = 0.01f;
	float dx, dy;

	float p0[2] = { p[0] - h, p[1] };
	float p1[2] = { p[0] + h, p[1] };
	dx = (Distance(p1) - Distance(p0)) / (2 * h);

	float p2[2] = { p[0], p[1] - h };
	float p3[2] = { p[0], p[1] + h };
	dy = (Distance(p3) - Distance(p2)) / (2 * h);

	grad[0] = dx;
	grad[1] = dy;
}

static void DrawCursor()
{
	float xy[2], d, grad[2];

	// convert mouse position from screen to identity
	xy[0] = (float)mousepos[0] / (float)renderwidth;
	xy[1] = 1.0f - ((float)mousepos[1] / (float)renderheight);

	// convert from identity to model pos
	xy[0] = -2 + xy[0] * 4;
	xy[1] = -2 + xy[1] * 4;

	fprintf(stdout, "x, y: %2.2f, %2.2f\n", xy[0], xy[1]);

	d = Distance(xy);
	fprintf(stdout, "distance %f\n", d);

	Gradient(grad, xy, 1.0f);
	Vec2_Normalize(grad);
	fprintf(stdout, "gradient %f, %f\n", grad[0], grad[1]);

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	{
		glVertex2f(xy[0], xy[1]);
		glVertex2f(xy[0] + d * -grad[0], xy[1] + d * -grad[1]);
	}
	glEnd();
}

static unsigned char *BuildTextureData()
{
	unsigned char *data = (unsigned char*)malloc(renderwidth * renderheight * 3);
	for (int y = 0; y < renderheight; y++)
	{
		for (int x = 0; x < renderwidth; x++)
		{
			float xy[2];

			// convert mouse position from screen to identity
			xy[0] = (float)x / (float)renderwidth;
			//xy[1] = 1.0f - ((float)y / (float)renderheight);
			xy[1] = (float)y / (float)renderheight;

			// convert from identity to model pos
			xy[0] = -2 + xy[0] * 4;
			xy[1] = -2 + xy[1] * 4;

			float d = Distance(xy);
			d = max(-1.0f, min(d, 1.0f));
			d *= 50;
			
			unsigned char *t = data + (y * renderwidth * 3) + (x * 3);
			*t++ = max(0, d) + 50;
			*t++ = 0;
			*t++ = max(0, -d) + 50;
		}
	}

	return data;
}

static void DrawField()
{
	static int texw, texh;
	static GLuint texture;

	if (texw != renderwidth || texh != renderheight)
	{
		texw = renderwidth;
		texh = renderheight;

		if (!texture)
			glGenTextures(1, &texture);

		unsigned char *data = BuildTextureData();
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texw, texh, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		free(data);
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	{

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 0.0f);

		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, 0.0f);

		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(0.0f, 1.0f);

		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 1.0f);
		glEnd();
	}

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

static void Draw()
{
	DrawField();

	DrawCursor();
}

static void Thing_Frame()
{
	static float pos[2];
	static bool spawned;

	if(!spawned)
	{
		pos[0] = 1;
		pos[1] = 1;
		spawned = true;
	}

	// distance, normal and tangent
	float d, n[2], t[2];
	d = Distance(pos);
	Gradient(n, pos, 1.0f);
	Vec2_Normalize(n);
	t[0] = -n[1];
	t[1] = n[0];

	// do a distance correction
	pos[0] += d * -n[0];
	pos[1] += d * -n[1];

	// do a move in the tangent direction
	// 2 * pi radians / 60 hz / 10 seconds = 10 seconds to go around the circle
	pos[0] += 0.01f * -t[0];
	pos[1] += 0.01f * -t[1];

	glColor3f(0, 0, 1);
	glPointSize(4.0f);
	glBegin(GL_POINTS);
	{
		glVertex2f(pos[0], pos[1]);
	}
	glEnd();
}

// glut functions
static void DisplayFunc()
{
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_DEPTH_TEST);

	Draw();
	Thing_Frame();

	glutSwapBuffers();
}

static void KeyboardDownFunc(unsigned char key, int x, int y)
{
	input.keys[key] = true;
}

static void KeyboardUpFunc(unsigned char key, int x, int y)
{
	input.keys[key] = false;
}

static void ReshapeFunc(int w, int h)
{
	renderwidth = w;
	renderheight = h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-2, 2, -2, 2, -1, 1);

	glViewport(0, 0, renderwidth, renderheight);
}

static void MouseFunc(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON)
		input.lbuttondown = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		input.rbuttondown = (state == GLUT_DOWN);
}

static void MouseMoveFunc(int x, int y)
{
	mousepos[0] = x;
	mousepos[1] = y;
}

// ticked at 60hz
// split into Frame()
static void TimerFunc(int value)
{
	// standard mouse input
	ProcessInput();

	// kick a display refresh
	glutPostRedisplay();
	glutTimerFunc(16, TimerFunc, 0);
}

static void PrintUsage()
{}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("test");

	glutReshapeFunc(ReshapeFunc);
	glutDisplayFunc(DisplayFunc);
	glutKeyboardFunc(KeyboardDownFunc);
	glutKeyboardUpFunc(KeyboardUpFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MouseMoveFunc);
	glutPassiveMotionFunc(MouseMoveFunc);
	glutTimerFunc(16, TimerFunc, 0);

	glutMainLoop();

	return 0;
}

