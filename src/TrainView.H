/************************************************************************
     File:        TrainView.H

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

#pragma once

// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"

class TrainView : public Fl_Gl_Window
{
	public:
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		Pnt3f drawCurve(Pnt3f p0, Pnt3f p1, Pnt3f p2, Pnt3f p3, float t, int line_type);

		void drawTrack(TrainView*, bool doingShadows);

		void toArcLength();

		void drawTrain(TrainView*, bool doingShadows);

		void drawWheel(Pnt3f qt, Pnt3f forward, Pnt3f cross, Pnt3f up, float r, float w);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

	public:
		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube;  // simple - just remember which cube is selected

		int				DIVIDE_LINE = 100;
		float			t_time = 0;
		int				line_type = 1;
		bool			isarclen = true;
		float			arclength = 0;
		float			t_arclength = 0;
		int				smoke_life[50] = { 0 };
		Pnt3f			smoke_pos[50];
		int				smoke_size[50] = { 0 };
		float			physics = 0;

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene
};