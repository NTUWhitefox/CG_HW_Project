/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"


#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0, 0, w(), h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0, 0, .3f, 0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	}
	else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	if (tw->trainCam->value() || !tw->headlight->value()) {
		glDisable(GL_LIGHT3);
	}
	else {
		glEnable(GL_LIGHT3);
	}

	if (tw->dirlight->value()) {
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float whiteDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float position[] = { 1.0f, 1.0f, 0.0f, 0.0f };

		glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, position);
	}
	else {
		glDisable(GL_LIGHT0);
	}
	if (tw->pointlight->value()) {
		float yellowAmbientDiffuse[] = { 1.0f, 1.0f, 0.8f, 1.0f };
		float position[] = { -2.0f, 2.0f, -5.0f, 1.0f };

		glLightfv(GL_LIGHT1, GL_AMBIENT, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowAmbientDiffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, position);
	}
	else {
		glDisable(GL_LIGHT1);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************

	if (!tw->dirlight->value() && !tw->pointlight->value()) {
		GLfloat lightPosition1[] = { 0,1,1,0 }; // {50, 200.0, 50, 1.0};
		GLfloat lightPosition2[] = { 1, 0, 0, 0 };
		GLfloat lightPosition3[] = { 0, -1, 0, 0 };
		GLfloat yellowLight[] = { 0.5f, 0.5f, .1f, 1.0 };
		GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0 };
		GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
		GLfloat grayLight[] = { .3f, .3f, .3f, 1.0 };

		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHT1);

		glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
		glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

		glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);
		glLightfv(GL_LIGHT1, GL_AMBIENT, grayLight);

		glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);
	}


	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(200,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else if (tw->trainCam->value()) {
		float percent = 1.0f / DIVIDE_LINE;
		int i = floor(t_time);
		float t = t_time - i;
		Pnt3f qt = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].pos, m_pTrack->points[i].pos,
			m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos, m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos, t, line_type);
		Pnt3f qt1 = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].pos, m_pTrack->points[i].pos,
			m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos, m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos, t + percent, line_type);
		Pnt3f orient_t = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].orient, m_pTrack->points[i].orient,
			m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient, m_pTrack->points[(i + 2) % m_pTrack->points.size()].orient, t, line_type);

		Pnt3f forward = qt1 + (qt * (-1));
		forward.normalize();
		Pnt3f cross_t = forward * orient_t;
		cross_t.normalize();
		Pnt3f up = -1 * forward * cross_t;
		up.normalize();
		forward = forward * 4.5f;
		up = up * 5.0f;
		Pnt3f pos = qt + up;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60, aspect, 0.01, 200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(pos.x, pos.y, pos.z, pos.x + forward.x, pos.y + forward.y, pos.z + forward.z, up.x, up.y, up.z);

#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

	line_type = tw->splineBrowser->value();
	drawTrack(this, doingShadows);

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value()) {
		drawTrain(this, doingShadows);
	}
}

Pnt3f TrainView::drawCurve(Pnt3f p0, Pnt3f p1, Pnt3f p2, Pnt3f p3, float t, int line_type)
{
	glm::mat4 G(p0.x, p0.y, p0.z, 1,
				p1.x, p1.y, p1.z, 1,
				p2.x, p2.y, p2.z, 1,
				p3.x, p3.y, p3.z, 1);
	glm::mat4 M;
	if (line_type == 1) {
		M = glm::mat4(0, 0, 0, 0,
					  0, 0, 0, 0,
					  0, -1, 1, 0,
					  0, 1, 0, 0);
	}
	else if (line_type == 2) {
		M = glm::mat4(-1, 3, -3, 1,
					  2, -5, 4, -1,
					  -1, 0, 1, 0,
					  0, 2, 0, 0);
		M /= 2;
	}
	else {
		M = glm::mat4(-1, 3, -3, 1,
					  3, -6, 3, 0,
					  -3, 0, 3, 0,
					  1, 4, 1, 0);
		M /= 6;
	}
	glm::vec4 T(t * t * t, t * t, t, 1);
	glm::vec4 p = G * M * T;
	return Pnt3f(p[0], p[1], p[2]);
}

void TrainView::drawTrack(TrainView*, bool doingShadows)
{
	float percent = 1.0f / DIVIDE_LINE;
	bool calctrainpos = false;
	int sleepercount = 0;
	arclength = 0.0f;

	// Variables with m meaning previous state, which are used to fill the gap
	Pnt3f qt1m = drawCurve(m_pTrack->points[m_pTrack->points.size() - 2].pos, m_pTrack->points[m_pTrack->points.size() - 1].pos, m_pTrack->points[0].pos, m_pTrack->points[1].pos, 1 - percent, line_type);
	Pnt3f qt = drawCurve(m_pTrack->points[m_pTrack->points.size() - 1].pos, m_pTrack->points[0].pos, m_pTrack->points[1].pos, m_pTrack->points[2].pos, 0, line_type);
	Pnt3f orient_tm = drawCurve(m_pTrack->points.back().orient, m_pTrack->points.front().orient, m_pTrack->points[1].orient, m_pTrack->points[2].orient, 0, line_type);
	orient_tm.normalize();
	Pnt3f cross_tm = (qt + qt1m * (-1)) * orient_tm;
	cross_tm.normalize();
	cross_tm = cross_tm * 2.5f;

	for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
		
		// pos
		Pnt3f cp_pos_p0 = m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
		Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;
		Pnt3f cp_pos_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos;
		// orient
		Pnt3f cp_orient_p0 = m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
		Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;
		Pnt3f cp_orient_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].orient;
		
		float t = 0;
		for (size_t j = 0; j < DIVIDE_LINE; j++) {
			Pnt3f qt0 = qt;
			t += percent;
			qt = drawCurve(cp_pos_p0, cp_pos_p1, cp_pos_p2, cp_pos_p3, t, line_type);
			Pnt3f qt1 = qt;
			
			// cross
			Pnt3f orient_t = drawCurve(cp_orient_p0, cp_orient_p1, cp_orient_p2, cp_orient_p3, t, line_type);
			orient_t.normalize();
			Pnt3f forward = (qt1 + qt0 * (-1));
			arclength += sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
			forward.normalize();
			forward = forward * 2.0f;
			Pnt3f cross_t = forward * orient_t;
			cross_t.normalize();
			cross_t = cross_t * 2.5f;

			glLineWidth(3);
			glBegin(GL_LINES);
			if (!doingShadows)
				glColor3ub(32, 32, 64);
			glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
			glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
			glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
			glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
			// Below drawing functions are used to fill the gap
			glVertex3f(qt1m.x + cross_tm.x, qt1m.y + cross_tm.y, qt1m.z + cross_tm.z);
			glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
			glVertex3f(qt1m.x - cross_tm.x, qt1m.y - cross_tm.y, qt1m.z - cross_tm.z);
			glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
			glEnd();
			// Draw sleeper and support stuctures
			if (tw->arcLength->value()) {
				if (arclength > sleepercount * 8.0f) {
					if (!doingShadows) glColor3ub(125, 80, 0);
					glBegin(GL_QUADS);
					glVertex3f(qt1.x + 2 * cross_t.x, qt1.y + 2 * cross_t.y, qt1.z + 2 * cross_t.z);
					glVertex3f(qt1.x - 2 * cross_t.x, qt1.y - 2 * cross_t.y, qt1.z - 2 * cross_t.z);
					glVertex3f(qt1.x - 2 * cross_t.x + forward.x, qt1.y - 2 * cross_t.y + forward.y, qt1.z - 2 * cross_t.z + forward.z);
					glVertex3f(qt1.x + 2 * cross_t.x + forward.x, qt1.y + 2 * cross_t.y + forward.y, qt1.z + 2 * cross_t.z + forward.z);
					glEnd();
					sleepercount++;
					if (sleepercount % 5 == 0 && tw->support->value()) {
						if (!doingShadows) glColor3ub(255, 100, 150);
						drawWheel(qt0 + cross_t, Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, qt0.y);
						drawWheel(qt0 + cross_t * (-1), Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, qt0.y);
					}
				}
			}
			else {
				if (j % 10 == 2) {
					if (!doingShadows) glColor3ub(125, 80, 0);
					glBegin(GL_QUADS);
					glVertex3f(qt1.x + 2 * cross_t.x, qt1.y + 2 * cross_t.y, qt1.z + 2 * cross_t.z);
					glVertex3f(qt1.x - 2 * cross_t.x, qt1.y - 2 * cross_t.y, qt1.z - 2 * cross_t.z);
					glVertex3f(qt1.x - 2 * cross_t.x + forward.x, qt1.y - 2 * cross_t.y + forward.y, qt1.z - 2 * cross_t.z + forward.z);
					glVertex3f(qt1.x + 2 * cross_t.x + forward.x, qt1.y + 2 * cross_t.y + forward.y, qt1.z + 2 * cross_t.z + forward.z);
					glEnd();
				}
				if (j % 50 == 2 && tw->support->value()) {
					if (!doingShadows) glColor3ub(255, 100, 150);
					drawWheel(qt0 + cross_t, Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, qt0.y);
					drawWheel(qt0 + cross_t * (-1), Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, qt0.y);
				}
			}
			if (!calctrainpos && arclength > t_arclength && isarclen) {
				calctrainpos = true;
				t_time = i + t;
				physics = (qt0.y - qt1.y) * 2.0f;
			}
			cross_tm = cross_t;
			qt1m = qt0;
		}
	}
}

void TrainView::toArcLength() {
	int point_size = m_pTrack->points.size();
	float percent = 1.0f / DIVIDE_LINE;
	int time_i = floor(t_time);
	float time_t = t_time - time_i;
	t_arclength = 0.0f;
	Pnt3f qt0 = drawCurve(m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, m_pTrack->points[2].pos, 0.0f, line_type);
	for (int i = 0; i <= time_i; i++) {
		ControlPoint& p0 = m_pTrack->points[(point_size + i - 1) % point_size];
		ControlPoint& p1 = m_pTrack->points[i];
		ControlPoint& p2 = m_pTrack->points[(i + 1) % point_size];
		ControlPoint& p3 = m_pTrack->points[(i + 2) % point_size];
		float t = 0;
		float t_DIVIDE_LINE = i != time_i ? DIVIDE_LINE : time_t * DIVIDE_LINE;
		for (int j = 0; j < t_DIVIDE_LINE; j++) {
			Pnt3f qt1 = drawCurve(p0.pos, p1.pos, p2.pos, p3.pos, t, line_type);
			Pnt3f forward = (qt1 + qt0 * (-1));
			t_arclength += sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
			qt0 = qt1;
			t += percent;
		}
	}
}

void TrainView::drawTrain(TrainView*, bool doingShadows)
{
	float percent = 1.0f / DIVIDE_LINE;
	int i = floor(t_time);
	float t = t_time - i;
	Pnt3f qt = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].pos, m_pTrack->points[i].pos,
		m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos, m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos, t, line_type);
	Pnt3f qt1 = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].pos, m_pTrack->points[i].pos,
		m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos, m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos, t + percent, line_type);
	Pnt3f orient_t = drawCurve(m_pTrack->points[(m_pTrack->points.size() + i - 1) % m_pTrack->points.size()].orient, m_pTrack->points[i].orient,
		m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient, m_pTrack->points[(i + 2) % m_pTrack->points.size()].orient, t, line_type);

	Pnt3f forward = qt1 + (qt * (-1));
	forward.normalize();
	Pnt3f cross_t = forward * orient_t;
	cross_t.normalize();
	Pnt3f up = -1 * forward * cross_t;
	up.normalize();
	forward = forward * 4.5f;
	cross_t = cross_t * 2.5f;
	up = up * 8.0f;

	if (!doingShadows) glColor3ub(200, 120, 30);
	// back
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x * 0.2f, qt.y - forward.y - cross_t.y + up.y * 0.2f, qt.z - forward.z - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x * 0.2f, qt.y - forward.y + cross_t.y + up.y * 0.2f, qt.z - forward.z + cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x, qt.y - forward.y + cross_t.y + up.y, qt.z - forward.z + cross_t.z + up.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x, qt.y - forward.y - cross_t.y + up.y, qt.z - forward.z - cross_t.z + up.z);
	glEnd();
	// front back
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f - cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f - cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f + cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f + cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x, qt.y + forward.y * 0.2f + cross_t.y + up.y, qt.z + forward.z * 0.2f + cross_t.z + up.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x, qt.y + forward.y * 0.2f - cross_t.y + up.y, qt.z + forward.z * 0.2f - cross_t.z + up.z);
	glEnd();
	// front front
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.2f, qt.y + forward.y - cross_t.y + up.y * 0.2f, qt.z + forward.z - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.2f, qt.y + forward.y + cross_t.y + up.y * 0.2f, qt.z + forward.z + cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.6f, qt.y + forward.y + cross_t.y + up.y * 0.6f, qt.z + forward.z + cross_t.z + up.z * 0.6f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.6f, qt.y + forward.y - cross_t.y + up.y * 0.6f, qt.z + forward.z - cross_t.z + up.z * 0.6f);
	glEnd();
	// left back
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x * 0.2f, qt.y - forward.y - cross_t.y + up.y * 0.2f, qt.z - forward.z - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x, qt.y - forward.y - cross_t.y + up.y, qt.z - forward.z - cross_t.z + up.z);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x, qt.y + forward.y * 0.2f - cross_t.y + up.y, qt.z + forward.z * 0.2f - cross_t.z + up.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x * 0.2f, qt.y + forward.y * 0.2f - cross_t.y + up.y * 0.2f, qt.z + forward.z * 0.2f - cross_t.z + up.z * 0.2f);
	glEnd();
	// right back
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x * 0.2f, qt.y - forward.y + cross_t.y + up.y * 0.2f, qt.z - forward.z + cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x, qt.y - forward.y + cross_t.y + up.y, qt.z - forward.z + cross_t.z + up.z);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x, qt.y + forward.y * 0.2f + cross_t.y + up.y, qt.z + forward.z * 0.2f + cross_t.z + up.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x * 0.2f, qt.y + forward.y * 0.2f + cross_t.y + up.y * 0.2f, qt.z + forward.z * 0.2f + cross_t.z + up.z * 0.2f);
	glEnd();
	// left front
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x * 0.2f, qt.y + forward.y * 0.2f - cross_t.y + up.y * 0.2f, qt.z + forward.z * 0.2f - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f - cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f - cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.6f, qt.y + forward.y - cross_t.y + up.y * 0.6f, qt.z + forward.z - cross_t.z + up.z * 0.6f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.2f, qt.y + forward.y - cross_t.y + up.y * 0.2f, qt.z + forward.z - cross_t.z + up.z * 0.2f);
	glEnd();
	// right front
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x * 0.2f, qt.y + forward.y * 0.2f + cross_t.y + up.y * 0.2f, qt.z + forward.z * 0.2f + cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f + cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f + cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.6f, qt.y + forward.y + cross_t.y + up.y * 0.6f, qt.z + forward.z + cross_t.z + up.z * 0.6f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.2f, qt.y + forward.y + cross_t.y + up.y * 0.2f, qt.z + forward.z + cross_t.z + up.z * 0.2f);
	glEnd();
	// bottom
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x * 0.2f, qt.y - forward.y - cross_t.y + up.y * 0.2f, qt.z - forward.z - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.2f, qt.y + forward.y - cross_t.y + up.y * 0.2f, qt.z + forward.z - cross_t.z + up.z * 0.2f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.2f, qt.y + forward.y + cross_t.y + up.y * 0.2f, qt.z + forward.z + cross_t.z + up.z * 0.2f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x * 0.2f, qt.y - forward.y + cross_t.y + up.y * 0.2f, qt.z - forward.z + cross_t.z + up.z * 0.2f);
	glEnd();
	// top back
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - cross_t.x + up.x, qt.y - forward.y - cross_t.y + up.y, qt.z - forward.z - cross_t.z + up.z);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x, qt.y + forward.y * 0.2f - cross_t.y + up.y, qt.z + forward.z * 0.2f - cross_t.z + up.z);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x, qt.y + forward.y * 0.2f + cross_t.y + up.y, qt.z + forward.z * 0.2f + cross_t.z + up.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x - forward.x + cross_t.x + up.x, qt.y - forward.y + cross_t.y + up.y, qt.z - forward.z + cross_t.z + up.z);
	glEnd();
	// top front
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x * 0.2f - cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f - cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f - cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(qt.x + forward.x - cross_t.x + up.x * 0.6f, qt.y + forward.y - cross_t.y + up.y * 0.6f, qt.z + forward.z - cross_t.z + up.z * 0.6f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(qt.x + forward.x + cross_t.x + up.x * 0.6f, qt.y + forward.y + cross_t.y + up.y * 0.6f, qt.z + forward.z + cross_t.z + up.z * 0.6f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(qt.x + forward.x * 0.2f + cross_t.x + up.x * 0.6f, qt.y + forward.y * 0.2f + cross_t.y + up.y * 0.6f, qt.z + forward.z * 0.2f + cross_t.z + up.z * 0.6f);
	glEnd();
	Pnt3f f = forward, c = cross_t, u = up;
	f.normalize();
	c.normalize();
	u.normalize();
	if (!doingShadows) glColor3ub(80, 40, 0);
	drawWheel(qt + forward * 0.6f + cross_t * (-1) + up * 0.2f, f, c * (-1), u, 1.5f, 0.5f);
	drawWheel(qt + forward * 0.6f + cross_t + up * 0.2f, f, c, u, 1.5f, 0.5f);
	drawWheel(qt + forward * (-0.6f) + cross_t * (-1) + up * 0.2f, f, c * (-1), u, 1.5f, 0.5f);
	drawWheel(qt + forward * (-0.6f) + cross_t + up * 0.2f, f, c, u, 1.5f, 0.5f);
	drawWheel(qt + forward * 0.6f + up * 0.6f, f, u, c, 0.8f, 3.0f);

	// smoke
	if (tw->smoke->value()) {
		srand(t_time * 1000);
		for (int i = 0; i < 50; i++) {
			if (smoke_life[i] == 0 && rand() % 50 == 0) {
				smoke_life[i] = rand() % 15 + 50;
				smoke_pos[i] = qt + forward * 0.6f + up;
				smoke_size[i] = rand() % 100 + 100;
			}
			if (smoke_life[i] <= 0) smoke_life[i] = 0;
			else {
				if (!doingShadows) glColor3ub(100, 100, 100);
				drawWheel(smoke_pos[i] + Pnt3f(0, 0, smoke_size[i] * -0.005f), Pnt3f(1, 0, 0), Pnt3f(0, 0, 1), Pnt3f(0, 1, 0), 
					smoke_size[i] * 0.005f, smoke_size[i] * 0.01f);
				smoke_life[i]--;
				smoke_pos[i].y += (rand() % 2 + 1) * 0.1f;
				smoke_size[i] += (rand() % 3) * smoke_life[i] / 25;
			}
		}
	}

	// headlight
	if (tw->headlight->value()) {
		Pnt3f head = qt + forward + up * 0.3f;
		float ambient[] = { 0.8f, 0.8f, 0.5f, 1.0f };
		float diffuse[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		float position[] = { head.x, head.y, head.z, 1.0f };
		glLightfv(GL_LIGHT3, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT3, GL_POSITION, position);

		float direction[] = { f.x, f.y, f.z };
		glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, direction);
		float spotCutOff = 45; // angle of the cone light emitted by the spot
		glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, spotCutOff);

		glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 15.0f); // the concentration of the light
		glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 0.1f); //light attenuation (1.0f: no attenuation)
		glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.02f);
		glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.0f);
	}
}

void TrainView::drawWheel(Pnt3f qt, Pnt3f forward, Pnt3f cross, Pnt3f up, float r, float w)
{
	float x = 0.0f, y = 0.0f, theta = 0.0f, t = 0.1f, PI = 3.1415926f;
	// back circle
	glBegin(GL_POLYGON);
	theta = 0.0f;
	while (theta < 2 * PI) {
		x = r * cos(theta);
		y = r * sin(theta);
		glVertex3f(qt.x + x * forward.x + y * up.x, qt.y + x * forward.y + y * up.y, qt.z + x * forward.z + y * up.z);
		theta += t;
	}
	glVertex3f(qt.x + r * forward.x, qt.y + r * forward.y, qt.z + r * forward.z);
	glEnd();
	// front circle
	glBegin(GL_POLYGON);
	theta = 0.0f;
	while (theta < 2 * PI) {
		x = r * cos(theta);
		y = r * sin(theta);
		glVertex3f(qt.x + x * forward.x + y * up.x + w * cross.x, qt.y + x * forward.y + y * up.y + w * cross.y, qt.z + x * forward.z + y * up.z + w * cross.z);
		theta += t;
	}
	glVertex3f(qt.x + r * forward.x + w * cross.x, qt.y + r * forward.y + w * cross.y, qt.z + r * forward.z + w * cross.z);
	glEnd();
	// tube
	glBegin(GL_QUAD_STRIP);
	theta = 0.0f;
	while (theta < 2 * PI) {
		x = r * cos(theta);
		y = r * sin(theta);
		glVertex3f(qt.x + x * forward.x + y * up.x, qt.y + x * forward.y + y * up.y, qt.z + x * forward.z + y * up.z);
		glVertex3f(qt.x + x * forward.x + y * up.x + w * cross.x, qt.y + x * forward.y + y * up.y + w * cross.y, qt.z + x * forward.z + y * up.z + w * cross.z);
		theta += t;
	}
	glVertex3f(qt.x + r * forward.x, qt.y + r * forward.y, qt.z + r * forward.z);
	glVertex3f(qt.x + r * forward.x + w * cross.x, qt.y + r * forward.y + w * cross.y, qt.z + r * forward.z + w * cross.z);
	glEnd();
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}