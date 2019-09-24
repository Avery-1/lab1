//Original Author: Gordon Griesel
//Modified by: Avery Raines
//Modified Date: Beginning of Semester thru Sep 23rd, 2019
//
//3350 Fall 2019 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

const int TOTAL_BOXES = 5; //Can be modified to make more boxes if needed
const int MAX_PARTICLES = 20000;
const float GRAVITY = 1.0;

//some structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Particle {
	Shape s;
	Vec velocity;
};

class Global {
public:
	int xres, yres;
	Shape box[TOTAL_BOXES];
	Particle particle[MAX_PARTICLES];
	int n;
	Global();
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
} x11;

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand(time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			check_mouse(&e);
			done = check_keys(&e);
		}
		movement();
		render();
		x11.swapBuffers();
	}
	return 0;
}

//-----------------------------------------------------------------------------
//Global class functions
//-----------------------------------------------------------------------------
Global::Global()
{
	xres = 900;
	yres = 600;
	//define a box shape
	//Array of boxes
    for(int i=0; i < TOTAL_BOXES; i++) {
    box[i].width = 100;
	box[i].height = 10;
    }
	//box 1 placement
	box[0].center.x = 150;
	box[0].center.y = 500;
    //box 2 placement
	box[1].center.x = 250;
	box[1].center.y = 400;
	//box 3 placement
	box[2].center.x = 350;
	box[2].center.y = 300;
    //box 4 placement
	box[3].center.x = 450;
	box[3].center.y = 200;
    //box 5 placement
	box[4].center.x = 550;
	box[4].center.y = 100;
    n = 0;
}

//-----------------------------------------------------------------------------
//X11_wrapper class functions
//-----------------------------------------------------------------------------
X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab 1 - Waterfall Model");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}
//-----------------------------------------------------------------------------

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
    //Allowing the fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void makeParticle(int x, int y)
{
	//Add a particle to the particle system.
	//
	if (g.n >= MAX_PARTICLES)
		return;
	cout << "makeParticle() " << x << " " << y << endl;
	//set position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = ((double)rand()/(double)RAND_MAX) -0.5;
	p->velocity.x = 4.0; //((double)rand()/(double)RAND_MAX) -0.5 + 0.25; 
	++g.n;
}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			int y = g.yres - e->xbutton.y;
			makeParticle(e->xbutton.x, y);
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.
			int y = g.yres - e->xbutton.y;
			for(int i=0; i<10;i++){
			    makeParticle(e->xbutton.x, y);
			}

		}
	}
}

int check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_a:
				//Key A was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void movement()
{
	if (g.n <= 0)
		return;
	for (int i=0; i<g.n;i++){
	Particle *p = &g.particle[i];
	p->s.center.x += p->velocity.x;
	p->s.center.y += p->velocity.y;
	p->velocity.y -= GRAVITY;
	//check for collision with shapes...
	//Shape *s;
    for (int i=0; i < TOTAL_BOXES; i++) {
	Shape *s= &g.box[i];
	if(p->s.center.y < s->center.y + s->height &&
       p->s.center.y > s->center.y - s->height &&
		p->s.center.x > s->center.x - s->width &&
		p->s.center.x < s->center.x + s->width) {
			p->s.center.y = s->center.y + s->height;
	        p->velocity.y = -(p->velocity.y*0.3);
		}
    }
	//check for off-screen
	if (p->s.center.y < 0.0) {
			g.particle[i]= g.particle[g.n-1];
			--g.n;
	}
}
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	//Draw shapes...
	//draw the boxes
	Shape *s;
	float w, h;
	for (int i=0; i < TOTAL_BOXES; i++){
    glColor3ub(90,140,90);
	s = &g.box[i];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	w = s->width;
	h = s->height;
	glBegin(GL_QUADS);
		glVertex2i( w, -h);
		glVertex2i( w,  h);
		glVertex2i(-w,  h);
		glVertex2i(-w, -h);
	glEnd();
	glPopMatrix();
    }
	
    //Draw particles here
    
	for(int i=0; i<g.n; i++){
		//There is at least one particle to draw.
		glPushMatrix();
		glColor3ub(0,60,240);
		Vec *c = &g.particle[i].s.center;
		w = h = 2.2;
		glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
	//
	//Draw your 2D text here
	//Variable created to eaisly use the same color over and over
    const int blue = 0x00ffff;

	//Array of rectangles with the same position as our boxes so we can
	//write text on them.
    Rect re[5];
    re[0].bot = 500 - 5;
    re[0].left = 150 - 33;
    re[0].center = 0; 
    ggprint8b(&re[0], 16, blue, "Requirements");    
    
    re[1].bot = 400 - 5;
    re[1].left = 250 - 17;
    re[1].center = 0; 
    ggprint8b(&re[1], 16, blue, "Design");    

    re[2].bot = 300 - 5;
    re[2].left = 350 - 33;
    re[2].center = 0; 
    ggprint8b(&re[2], 16, blue, "Implementation");    

    re[3].bot = 200 - 5;
    re[3].left = 450 - 33;
    re[3].center = 0; 
    ggprint8b(&re[3], 16, blue, "Testing");    

    re[4].bot = 100 - 5;
    re[4].left = 550 - 33;
    re[4].center = 0; 
    ggprint8b(&re[4], 16, blue, "Maintenance");    
}






