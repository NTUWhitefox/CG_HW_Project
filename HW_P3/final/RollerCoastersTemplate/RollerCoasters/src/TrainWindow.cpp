/************************************************************************
     File:        TrainWindow.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						this class defines the window in which the project 
						runs - its the outer windows that contain all of 
						the widgets, including the "TrainView" which has the 
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing 
						itself into the FlTk "idle" loop so that we get periodic 
						updates (if we're running the train).


     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <FL/fl.h>
#include <FL/Fl_Box.h>

// for using the real time clock
#include <time.h>

#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"



//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::
TrainWindow(const int x, const int y) 
	: Fl_Double_Window(x,y,1200,900,"Train and Roller Coaster")
//========================================================================
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty=5;			// where the last widgets were drawn

		trainView = new TrainView(5,5,590,590);
		trainView->tw = this;
		trainView->m_pTrack = &m_Track;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600,5,190,900);
		widgets->begin();

		runButton = new Fl_Button(605,pty,60,20,"Run");
		togglify(runButton);

		Fl_Button* fb = new Fl_Button(700,pty,25,20,"@>>");
		fb->callback((Fl_Callback*)forwCB,this);
		Fl_Button* rb = new Fl_Button(670,pty,25,20,"@<<");
		rb->callback((Fl_Callback*)backCB,this);
		
		arcLength = new Fl_Button(730,pty,65,20,"ArcLength");
		togglify(arcLength, 1);
  
		pty+=25;
		speed = new Fl_Value_Slider(655,pty,140,20,"speed");
		speed->range(0,10);
		speed->value(2);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);

		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600,pty,195,20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
        worldCam->type(FL_RADIO_BUTTON);		// radio button
        worldCam->value(1);			// turned on
        worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB,this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
        trainCam->type(FL_RADIO_BUTTON);
        trainCam->value(0);
        trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB,this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
        topCam->type(FL_RADIO_BUTTON);
        topCam->value(0);
        topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB,this);
		camGroup->end();

		pty += 30;

		// browser to select spline types
		// TODO: make sure these choices are the same as what the code supports
		splineBrowser = new Fl_Browser(605,pty,120,75,"Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB,this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->add("Cubic B-Spline");
		splineBrowser->select(2);

		pty += 95;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605,pty,80,20,"Add Point");
		ap->callback((Fl_Callback*)addPointCB,this);
		Fl_Button* dp = new Fl_Button(690,pty,80,20,"Delete Point");
		dp->callback((Fl_Callback*)deletePointCB,this);

		pty += 25;
		// reset the points
		resetButton = new Fl_Button(735,pty,60,20,"Reset");
		resetButton->callback((Fl_Callback*)resetCB,this);
		Fl_Button* loadb = new Fl_Button(605,pty,60,20,"Load");
		loadb->callback((Fl_Callback*) loadCB, this);
		Fl_Button* saveb = new Fl_Button(670,pty,60,20,"Save");
		saveb->callback((Fl_Callback*) saveCB, this);

		pty += 25;
		// roll the points
		Fl_Button* rx = new Fl_Button(605,pty,30,20,"R+X");
		rx->callback((Fl_Callback*)rpxCB,this);
		Fl_Button* rxp = new Fl_Button(635,pty,30,20,"R-X");
		rxp->callback((Fl_Callback*)rmxCB,this);
		Fl_Button* rz = new Fl_Button(670,pty,30,20,"R+Z");
		rz->callback((Fl_Callback*)rpzCB,this);
		Fl_Button* rzp = new Fl_Button(700,pty,30,20,"R-Z");
		rzp->callback((Fl_Callback*)rmzCB,this);

		pty += 30;

		centerObject = new Fl_Browser(605, pty, 190, 50, "centerObject");
		centerObject->type(2);
        centerObject->callback((Fl_Callback*)damageCB, this);
		centerObject->add("None");
		centerObject->add("Rock");
        centerObject->add("Fractal Tree");
		centerObject->select(2);

		pty += 50;

		pointlight = new Fl_Button(605,pty,50,20,"night");
		togglify(pointlight);

		pty += 30;

		lightangle = new Fl_Value_Slider(655,pty,140,20,"lightangle");
		lightangle->range(0, 6.2832);
		lightangle->value(0);
		lightangle->align(FL_ALIGN_LEFT);
		lightangle->type(FL_HORIZONTAL);
		lightangle->callback((Fl_Callback*)damageCB, this);

		pty += 30;

		lightradius = new Fl_Value_Slider(655, pty, 140, 20, "lightradius");
		lightradius->range(0, 50);
		lightradius->value(0);
		lightradius->align(FL_ALIGN_LEFT);
		lightradius->type(FL_HORIZONTAL);
		lightradius->callback((Fl_Callback*)damageCB, this);

		pty += 30;

		brightness = new Fl_Value_Slider(655, pty, 140, 20, "brightness");
		brightness->range(1.0f, 5.0f);
		brightness->value(1.0f);
		brightness->align(FL_ALIGN_LEFT);
		brightness->type(FL_HORIZONTAL);
		brightness->callback((Fl_Callback*)damageCB, this);

		pty += 30;

		lightR = new Fl_Value_Slider(655, pty, 140, 20, "R");
		lightR->range(0.0f, 1.0f);
		lightR->value(1.0f);
		lightR->align(FL_ALIGN_LEFT);
		lightR->type(FL_HORIZONTAL);
		lightR->callback((Fl_Callback*)damageCB, this);
		pty += 30;

		lightG = new Fl_Value_Slider(655, pty, 140, 20, "G");
		lightG->range(0.0f, 1.0f);
		lightG->value(1.0f);
		lightG->align(FL_ALIGN_LEFT);
		lightG->type(FL_HORIZONTAL);
		lightG->callback((Fl_Callback*)damageCB, this);
		pty += 30;

		lightB = new Fl_Value_Slider(655, pty, 140, 20, "B");
		lightB->range(0.0f, 1.0f);
		lightB->value(1.0f);
		lightB->align(FL_ALIGN_LEFT);
		lightB->type(FL_HORIZONTAL);
		lightB->callback((Fl_Callback*)damageCB, this);

		pty += 30;

		floornoise = new Fl_Value_Slider(655,pty,140,20,"noise");
		floornoise->range(0, 100);
		floornoise->value(7);
		floornoise->align(FL_ALIGN_LEFT);
		floornoise->type(FL_HORIZONTAL);
		floornoise->callback((Fl_Callback*)damageCB, this);

		pty += 30;

		physics = new Fl_Button(605,pty,60,20,"Physics");
		togglify(physics);
		projector = new Fl_Button(670,pty,60,20,"Projector");
		togglify(projector);

		pty += 30;

		support = new Fl_Button(605,pty,60,20,"Support");
		togglify(support);
		headlight = new Fl_Button(670,pty,70,20,"Headlight");
		togglify(headlight);
		smoke = new Fl_Button(745,pty,50,20,"Smoke");
		togglify(smoke);

		pty += 30;

		framebuffer = new Fl_Browser(605,pty,190,165,"Frame Buffer Type");
		framebuffer->type(2);		// select
		framebuffer->callback((Fl_Callback*)damageCB, this);
		framebuffer->add("default");
		framebuffer->add("pixel");
		framebuffer->add("offset");
		framebuffer->add("sobel");
		framebuffer->add("toon");
		framebuffer->add("sharpen");
		framebuffer->add("ascii");
		framebuffer->add("motion blur");
		framebuffer->add("fxaa");
		framebuffer->add("chromaticAberration");
		framebuffer->select(1);
		

		// TODO: add widgets for all of your fancier features here
#ifdef EXAMPLE_SOLUTION
		makeExampleWidgets(this,pty);
#endif

		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600,595,200,5);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void (*)(void*))runButtonCB,this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::
togglify(Fl_Button* b, int val)
//========================================================================
{
	b->type(FL_TOGGLE_BUTTON);		// toggle
	b->value(val);		// turned off
	b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB,this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::
damageMe()
//========================================================================
{
	if (trainView->selectedCube >= ((int)m_Track.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::
advanceTrain(float dir)
//========================================================================
{
	//#####################################################################
	// TODO: make this work for your train
	//#####################################################################
	static bool pre_arcLength = arcLength->value();

	if (pre_arcLength != arcLength->value()) {
		if (arcLength->value()) {
			trainView->toArcLength();
			trainView->isarclen = true;
		}
		else {
			trainView->isarclen = false;
		}
	}

	if (arcLength->value()) {
		trainView->t_arclength += dir * speed->value() * 1.0f;
		if (physics->value()) trainView->t_arclength += trainView->physics;
		if (trainView->t_arclength > trainView->arclength) {
			trainView->t_arclength -= trainView->arclength;
		}
		else if (trainView->t_arclength < 0) {
			trainView->t_arclength += trainView->arclength;
		}
	}
	else {
		trainView->t_time += dir * speed->value() * 0.02f;
		if (trainView->t_time > m_Track.points.size()) {
			trainView->t_time -= m_Track.points.size();
		}
	}
	pre_arcLength = arcLength->value();

#ifdef EXAMPLE_SOLUTION
	// note - we give a little bit more example code here than normal,
	// so you can see how this works

	if (arcLength->value()) {
		float vel = ew.physics->value() ? physicsSpeed(this) : dir * (float)speed->value();
		world.trainU += arclenVtoV(world.trainU, vel, this);
	} else {
		world.trainU +=  dir * ((float)speed->value() * .1f);
	}

	float nct = static_cast<float>(world.points.size());
	if (world.trainU > nct) world.trainU -= nct;
	if (world.trainU < 0) world.trainU += nct;
#endif
}