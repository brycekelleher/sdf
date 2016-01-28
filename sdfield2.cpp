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

static float renderwidth, renderheight;

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

#undef min
#define min(a, b) (a < b ? a : b)

#undef max
#define max(a, b) (a > b ? a : b)

static float TriangleDistance(float p[2])
{
	float d1[2], d2[2];
	static float a = 0.707106781f;

	d1[0] = p[0] - 1;
	d1[1] = p[1];
	d2[0] = p[0];
	d2[1] = p[1] - 1;

	// check the corners
	if(p[0] < 0.0f && p[1] < 0.0f)
		return Vec2_Length(p);
	if(d1[0] > 0.0f && d1[1] <= d1[0])
		return Vec2_Length(d1);
	if(d2[1] > 0.0f && d2[1] >= d2[0])
		return Vec2_Length(d2);

	// check the edges
	if(p[1] < 0.0f)
		return -p[1];
	if(p[0] < 0.0f)
		return -p[0];
	if((a * p[0] + a * p[1]) - a > 0.0f)
		return (a * p[0] + a * p[1]) - a;

	return max(max(-p[0], -p[1]), (a * p[0] + a * p[1]) - a);
}

static float CircleDistance(float p[2], float r)
{
	return Vec2_Length(p) - r;
}

static float BoxDistance(float p[2])
{
	float d[2];
	float b[2] = { 1, 1 };

	d[0] = abs(p[0]) - b[0];
	d[1] = abs(p[1]) - b[1];

#if 0
	// this code works but is probably the same as
	if(d[0] <= 0.0f && d[1] <= 0.0f)
	{
		// inside box (returns negative)
		return max(d[0], d[1]);
	}
	else if(d[0] <= 0.0f)
	{
		// 
		return d[1];
	}
	else if(d[1] <= 0.0f)
	{
		return d[0];
	}
	else
	{
		return Vec2_Length(d);
	}
#endif

	float d2[2] = { max(d[0], 0.0f), max(d[1], 0.0f) };
	return min(max(d[0], d[1]), 0.0f) + Vec2_Length(d2);
}

// -----------------------------------------------------------------
// triangle distance

static float VertexDistance(float a[2], float b[2])
{
	float v[2] = { b[0] - a[0], b[1] - a[1] };
	return Vec2_Length(v);
}

static float EdgeDistance(float v0[2], float v1[2], float p[2])
{
	float e[2] = { v1[0] - v0[0], v1[1] - v0[1] };
	Vec2_Normalize(e);
	float n[2] = { e[1], -e[0] };

	return (n[0] * p[0]) + (n[1] * p[1]) - (n[0] * v0[0]) - (n[1] * v0[1]);
}

static float TriangleArea(float v0[2], float v1[2], float v2[2])
{
	float x0 = v0[0];
	float y0 = v0[1];
	float x1 = v1[0];
	float y1 = v1[1];
	float x2 = v2[0];
	float y2 = v2[1];

	return 0.5f * ((x0 * y1) - (x0 * y2) - (x1 * y0) + (x1 * y2 ) + (x2 * y0) - (x2 * y1));
}

#if 0
static void MakeEdgeVector(float e[2], float v0[2], float v1[2])
{
	float e[2] = { v1[0] - v0[0], v1[1] - v0[1] };
	Vec2_Normalize(e);
}

static void MakeEdgePlane(float e[2], float d[2], float v0[2], float v1[2])
{
	float e[2] = { v1[0] - v0[0], v1[1] - v0[1] };
	Vec2_Normalize(e);
	
	d[0] = -(e[0] * v0[0]) - (e[1] * v0[1]);
	d[1] = -(e[0] * v1[0]) - (e[1] * v1[1]);
}

static float PlaneDistance(float n[2], float d, float p[2])
{
	return (n[0] * p[0]) + (n[1] * p[1]) + d;
}

static float TriangleDistance(float v0[2], float v1[2], float v2[2], float p[2])
{
	float	areas[3];
	int	signs[2];

	float	e[3][2];
	float	d[3][2];

	MakeEdgePlane(e[0], d[0], v0, v1);
	MakeEdgePlane(e[1], d[1], v1, v2);
	MakeEdgePlane(e[2], d[2], v2, v0);

	// figure out we're in a vertex region
	{
	PlaneDistance(e[0], d[0][0]), PlaneDistance(	

	}
	p[0] = -1;
	p[1] = 1.5;

	areas[0] = -TriangleArea(v0, v1, p);
	areas[1] = -TriangleArea(v1, v2, p);
	areas[2] = -TriangleArea(v2, v0, p);

	signs[0] = signs[1] = 0;
	for(int i = 0; i < 3; i++)
	{
		if(areas[i] <= 0.0f)
			signs[0]++;
		else
			signs[1]++;
	}

	// if all are negative then inside of triangle
	if(!signs[0] == 3)
	{
		return 0.0f;
	}

	// if one is positive then we're closer to an edge
	if(signs[1] == 1)
	{
		// take the max of the distance to all edges
		printf("edge\n");
		return max(EdgeDistance(v0, v1, p), max(EdgeDistance(v1, v2, p), EdgeDistance(v2, v0, p)));
	}
	
	// two are positive then we're closer to a vertex
	if(signs[1] == 2)
	{
		// take the min of the the distance to all vertices
		printf("vertex\n");
		return min(VertexDistance(v0, p), min(VertexDistance(v1, p), VertexDistance(v2, p)));
	}

	return 0.0f;
}
#endif

#if 0
static float TriangleDistance(float v0[2], float v1[2], float v2[2], float p[2])
{
	float	areas[3];
	int	signs[2];

	p[0] = -1;
	p[1] = 1.5;

	areas[0] = -TriangleArea(v0, v1, p);
	areas[1] = -TriangleArea(v1, v2, p);
	areas[2] = -TriangleArea(v2, v0, p);

	signs[0] = signs[1] = 0;
	for(int i = 0; i < 3; i++)
	{
		if(areas[i] <= 0.0f)
			signs[0]++;
		else
			signs[1]++;
	}

	// if all are negative then inside of triangle
	if(!signs[0] == 3)
	{
		return 0.0f;
	}

	// if one is positive then we're closer to an edge
	if(signs[1] == 1)
	{
		// take the max of the distance to all edges
		printf("edge\n");
		return max(EdgeDistance(v0, v1, p), max(EdgeDistance(v1, v2, p), EdgeDistance(v2, v0, p)));
	}
	
	// two are positive then we're closer to a vertex
	if(signs[1] == 2)
	{
		// take the min of the the distance to all vertices
		printf("vertex\n");
		return min(VertexDistance(v0, p), min(VertexDistance(v1, p), VertexDistance(v2, p)));
	}

	return 0.0f;
}
#endif

// a line from 0,0 to 1,0
static float LineDistance(float p[2])
{
	float d;

	if(p[0] >= 0.0f && p[0] <= 1.0f)
	{	
		d = p[1];
	}
	else
	{
		if(p[0] < 0.0f)
		{
			float p2[2] = { p[0], p[1] };
			d = Vec2_Length(p2);
		}
		else
		{
			float p2[2] = { p[0] - 1, p[1] };
			d =  Vec2_Length(p2);
		}

		//if(p[1] < 0.0f)
		//	d = -d;
	}

	return d;
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
	return BoxDistance(p);
}
#endif

static float Distance(float p[2], float r)
{
	return TriangleDistance(p);
}

#if 0
static float Distance(float p[2], float r)
{
	float d0, d1, d2;

	float p0[2] = { p[0], p[1] };
	d0 = CircleDistance(p0, r);

	float p1[2] = { p[0] - 1, p[1] - 1 };
	d1 = CircleDistance(p1, r);

	float p2[2] = { p[0] - 1, p[1] + 1 };
	d2 = CircleDistance(p2, r);

	return min(d2, min(d1, d0));
}
#endif

#if 0
static float Distance(float p[2], float r)
{
	float d0, d1, d2;

#if 0
	float vertices[3][2] = 
	{
		{ -1, -1 },
		{  1, -1 },
		{ 0, 1 }
	};
#endif

	float vertices[3][2] = 
	{
		{ 0, 0 },
		{ 1, 0 },
		{ 0, 1 }
	};

	return TriangleDistance(vertices[0], vertices[1], vertices[2], p);
}
#endif

static void Gradient(float grad[2], float p[2], float r)
{
	float h = 0.01f;
	float dx, dy;

	float p0[2] = { p[0] - h, p[1] };
	float p1[2] = { p[0] + h, p[1] };
	dx = (Distance(p1, r) - Distance(p0, r)) / (2 * h);

	float p2[2] = { p[0], p[1] - h };
	float p3[2] = { p[0], p[1] + h };
	dy = (Distance(p3, r) - Distance(p2, r)) / (2 * h);

	grad[0] = dx;
	grad[1] = dy;
}

static void Draw()
{
	float xy[2], d, grad[2];

	// convert mouse position from screen to identity
	xy[0] = (float)mousepos[0] / (float)renderwidth;
	xy[1] = 1.0f - ((float)mousepos[1] / (float)renderheight);

	// convert from identity to screen pos
	xy[0] = -2 + xy[0] * 4;
	xy[1] = -2 + xy[1] * 4;

	fprintf(stdout, "x, y: %2.2f, %2.2f\n", xy[0], xy[1]);

	d = Distance(xy, 1.0f);
	//fprintf(stdout, "distance %f\n", d);

	Gradient(grad, xy, 1.0f);
	Vec2_Normalize(grad);
	//fprintf(stdout, "gradient %f, %f\n", grad[0], grad[1]);

	if(d > 0)
		glColor3f(1, 0, 0);
	else
		glColor3f(0, 0, 1);

	glBegin(GL_LINES);
	{
		glVertex2f(xy[0], xy[1]);
		glVertex2f(xy[0] + d * -grad[0], xy[1] + d * -grad[1]);
	}
	glEnd();
}

static void Thing_Frame()
{
	static float pos[2];
	static bool spawned;

	if(!spawned)
	{
		pos[0] = 2;
		pos[1] = 2;
		spawned = true;
	}

	// distance, normal and tangent
	float d, n[2], t[2];
	d = Distance(pos, 1.0f);
	Gradient(n, pos, 1.0f);
	Vec2_Normalize(n);
	t[0] = -n[1];
	t[1] = n[0];

	// do a distance correction
	pos[0] += (d - 0.01f) * -n[0];
	pos[1] += (d - 0.01f) * -n[1];

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

