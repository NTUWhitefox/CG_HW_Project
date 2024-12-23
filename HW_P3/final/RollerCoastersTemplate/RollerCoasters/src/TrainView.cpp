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
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glu.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#include <random>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "model.h"

#include <array>

#define _USE_MATH_DEFINES
#include <math.h>


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
// * Some utility code here
//========================================================================
unsigned int loadCubemap(vector<const GLchar*> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
std::array<float, 16> perspectiveMatrix(float fov, float aspect, float nearPlane, float farPlane) {
	float f = 1.0f / std::tan(fov * 0.5f * M_PI / 180.0f); // Convert FOV to radians and calculate cotangent
	float nf = 1.0f / (nearPlane - farPlane);

	std::array<float, 16> matrix = {
		f / aspect, 0.0f,  0.0f,                               0.0f,
		0.0f,       f,     0.0f,                               0.0f,
		0.0f,       0.0f,  (farPlane + nearPlane) * nf,       -1.0f,
		0.0f,       0.0f,  (2.0f * farPlane * nearPlane) * nf, 0.0f
	};

	return matrix;
}
void TrainView::set_fbo(int* framebuffer, unsigned int* textureColorbuffer) {

	glGenFramebuffers(1, (GLuint*)framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	// create a color attachment texture
	glGenTextures(1, textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, *textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *textureColorbuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
		if (!for_model) {
			for_model = new
				Shader(
					"./assets/shaders/model_loading.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/model_loading.frag");
		}

		if (!for_model_texture) {
			for_model_texture = new
				Shader(
					"./assets/shaders/model_texture.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/model_texture.frag");
		}

		if (!tree) {
			tree = new Model("./assets/objects/tree.obj");
		}

		if (!flower) {
			flower = new Model("./assets/objects/flower.obj");
		}

		if (tree_tex == -1) {
			tree_tex = TextureFromFile("./assets/objects/texture_gradient.png", ".");
		}

		if (!capybara) {
			capybara = new Model("./assets/objects/capybara.obj");
		}

		if (capybara_tex == -1) {
			capybara_tex = TextureFromFile("./assets/objects/Capybara_Base_color.png", ".");
		}

		if (!this->screen) {
			this->screen = new
				Shader(
					"./assets/shaders/screen.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/screen.frag");

			float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
				// positions   // texCoords
				-1.0f,  1.0f,  0.0f, 1.0f,
				-1.0f, -1.0f,  0.0f, 0.0f,
				 1.0f, -1.0f,  1.0f, 0.0f,

				-1.0f,  1.0f,  0.0f, 1.0f,
				 1.0f, -1.0f,  1.0f, 0.0f,
				 1.0f,  1.0f,  1.0f, 1.0f
			};
			// screen quad VAO
			glGenVertexArrays(1, &screen_quadVAO);
			glGenBuffers(1, &screen_quadVBO);
			glBindVertexArray(screen_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, screen_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			screen->Use();
			glUniform1i(glGetUniformLocation(screen->Program, "screenTexture"), 0);

			glGenFramebuffers(1, &screen_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

			glGenTextures(1, &screen_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

			glGenRenderbuffers(1, &screen_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		if (!this->skybox) {
			this->skybox = new
				Shader(
					"./assets/shaders/skybox.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/skybox.frag");
			float skyboxVertices[] = {
				// positions          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};
			// skybox VAO
			glGenVertexArrays(1, &skyboxVAO);
			glGenBuffers(1, &skyboxVBO);
			glBindVertexArray(skyboxVAO);
			glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			vector<const GLchar*> faces = {
			"./assets/images/right.jpg",
			"./assets/images/left.jpg",
			"./assets/images/top.jpg",
			"./assets/images/bottom.jpg",
			"./assets/images/front.jpg",
			"./assets/images/back.jpg",
			};
			skyBoxCubemapTexture = loadCubemap(faces);
		}
		//Load billboard tree
		if (!this->billboardTree) {
			this->billboardTree = new Shader(
                "./assets/shaders/billboardTree.vert",
				nullptr, nullptr, nullptr,
            "./assets/shaders/billboardTree.frag");
			float quadVertices[] = {
				// Positions         // Texture Coordinates
				-0.5f,  1.0f, 0.0f,  0.0f, 1.0f,  // Top-left
				 0.5f,  1.0f, 0.0f,  1.0f, 1.0f,  // Top-right
				 0.5f, -1.0f, 0.0f,  1.0f, 0.0f,  // Bottom-right
				-0.5f, -1.0f, 0.0f,  0.0f, 0.0f   // Bottom-left
			};
			unsigned int indices[] = {
				0, 1, 2,  // First triangle
				0, 2, 3   // Second triangle
			};
			glGenVertexArrays(1, &billboardtree_quadVAO);
			glGenBuffers(1, &billboardtree_quadVBO);
			glGenBuffers(1, &billboardtree_quadEBO);

			glBindVertexArray(billboardtree_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, billboardtree_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardtree_quadEBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
			// Position Attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			// Texture Coordinate Attribute
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			billboardTreeTexture = TextureFromFile("/assets/images/tree_upsidedown.png", ".");
		}

		//Load projector (project texture with texture matrix)
		if (!projectorShader) {
			this->projectorShader = new Shader(
				"./assets/shaders/projector.vert",
				nullptr, nullptr, nullptr,
				"./assets/shaders/projector.frag"
			);
			projectorTexture = TextureFromFile("/assets/images/earth.png", ".");
		}

		if (framebuffer[0] == -1) {
			for (int i = 0; i < 8; i++) {
				set_fbo(&framebuffer[i], &textureColorbuffer[i]);
				glUniform1i(glGetUniformLocation(screen->Program, ("TextureFBO" + to_string(i + 1)).c_str()), i);
				trans[i] = glm::mat4(1.0f);
			}
		}
		if (!fbo_shader) {
			fbo_shader = new
				Shader(
					"./assets/shaders/fbo.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/fbo.frag");
			//fbo_shader->Use();
			//glUniform1i(glGetUniformLocation(fbo_shader->Program, "texture_diffuse1"), 0);
		}

		if (!rock) {
			rock = new Model("./assets/objects/rock.obj");
			rock_spec = TextureFromFile("/assets/images/rock-spec.png", ".");
			rock_normal = TextureFromFile("/assets/images/rock-norm.png", ".");
			rock_diff = TextureFromFile("/assets/images/rock-diff.png", ".");
			rockShader = new
				Shader(
					"./assets/shaders/rock.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/rock.frag");
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	//######################################################################
	//This is where assets loading end
	//######################################################################

	static int pre_w = w(), pre_h = h();
	// Set up the view port
	glViewport(0, 0, w(), h());
	if (pre_w != w() || pre_h != h()) {
		glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

		glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
		// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	pre_w = w();
	pre_h = h();

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
	float lightPosition[] = {0.0f, 0.0f, 0.0f, 0.0f};
	if (tw->dirlight->value()) {
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float whiteDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		lightPosition[0] = 0.0f; lightPosition[1] = 1.0f; lightPosition[2] = 0.0f; lightPosition[3] = 0.0f;
		//lightPosition = { 0.0f, 1.0f, 0.0f, 0.0f };

		glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	}
	else {
		glDisable(GL_LIGHT0);
	}
	if (tw->pointlight->value()) {
		float yellowAmbient[] = { 0.8f, 0.8f, 0.5f, 1.0f };
		float yellowAmbientDiffuse[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		lightPosition[0] = -2.0f; lightPosition[1] = 2.0f; lightPosition[2] = -5.0f; lightPosition[3] = 1.0f;
		//lightPosition = { -2.0f, 2.0f, -5.0f, 1.0f };
		
		glLightfv(GL_LIGHT1, GL_AMBIENT, yellowAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowAmbientDiffuse);
		glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.05f);
		glLightfv(GL_LIGHT1, GL_POSITION, lightPosition);
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
	/*
	current_trans = glm::make_mat4(projection) * glm::make_mat4(view);

	if (frame < 8) {
		if (frame % 8 != 0) {
			trans[frame % 8] = trans[frame % 8 - 1];
		}
		else {
			trans[0] = current_trans;
		}
	}
	else {

		trans[7] = trans[6];
		trans[6] = trans[5];
		trans[5] = trans[4];
		trans[4] = trans[3];
		trans[3] = trans[2];
		trans[2] = trans[1];
		trans[1] = trans[0];
		trans[0] = current_trans;
	}
	frame++;
	for (int i = 0; i < 8; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[i]);
		draw_scene_model(rocket, true, fbo_shader, trans[i], current_trans);
	}*/

	//view and projection matrix setup
	GLfloat projection[16];
	GLfloat view[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, view);
	glm::mat4 view_without_translate = glm::mat4(glm::mat3(glm::make_mat4(view)));
	glm::mat4 view_inv = glm::inverse(glm::make_mat4(view));
	glm::vec3 my_pos(view_inv[3][0], view_inv[3][1], view_inv[3][2]);
	glm::mat4 model = glm::mat4(1.0f);

	//projector setup start
	if (tw->projector->value()) {
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(0.5f, 0.5f, 0.0f);
		glScalef(0.5f, 0.5f, 1.0f);
		//float scaleX = frustumWidth / textureWidth;
		//float scaleY = frustumHeight / textureHeight;
		//glScalef(scaleX, scaleY, 1.0f);
		glMultMatrixf(projection);
		glMultMatrixf(view);// Apply the camera's view matrix
		glMatrixMode(GL_MODELVIEW);// Return to modelview matrix mode
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, projectorTexture);// Enable texture generation for projection
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
	}
	//projector setup end


	glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);
	// make sure we clear the framebuffer's content
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(500,10);


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

	//draw terrain
	/*
	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	terrain->Draw(*for_model_texture);
	glActiveTexture(GL_TEXTURE0);
	*/

	//draw shark
	/*
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 50, 0));
	model = glm::scale(model, glm::vec3(1, 1, 1));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	shark->Draw(*for_model);
	*/
	
	//draw tree model
	/*
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 0, 0));
	model = glm::scale(model, glm::vec3(2, 2, 2));
	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, tree_tex);
	tree->Draw(*for_model_texture);
	glActiveTexture(GL_TEXTURE0);
	*/

	//draw flower model
	/*
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-15, 0, 0));
	model = glm::scale(model, glm::vec3(5, 5, 5));
	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, tree_tex);
	flower->Draw(*for_model_texture);
	glActiveTexture(GL_TEXTURE0);
	*/

	//draw rock model
	
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-50,  0, 50));
	model = glm::scale(model, glm::vec3(15, 15, 15));
	rockShader->Use();

	glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "view"), 1, GL_FALSE, view);

	glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "viewPos"), 1, GL_FALSE, glm::value_ptr(my_pos));


	if (tw->pointlight->value()) {
		glUniform3fv(glGetUniformLocation(rockShader->Program, "lightPos"), 1, lightPosition);
	}
	else if (tw->dirlight->value()) {
		glUniform3fv(glGetUniformLocation(rockShader->Program, "lightPos"), 1, glm::value_ptr(glm::vec3(-50, 100, 50)));
	}
	else {
		glUniform3fv(glGetUniformLocation(rockShader->Program, "lightPos"), 1, glm::value_ptr(glm::vec3(-50, 100, 50)));
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rock_spec);
	glUniform1i(glGetUniformLocation(rockShader->Program, "specularMap"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rock_normal);
	glUniform1i(glGetUniformLocation(rockShader->Program, "normalMap"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, rock_diff);
	glUniform1i(glGetUniformLocation(rockShader->Program, "diffuseMap"), 2);

	rock->Draw(*rockShader);

	//draw skybox section start
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	skybox->Use();
	glUniform1f(glGetUniformLocation(skybox->Program, "skybox"), skyBoxCubemapTexture);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "model_view"), 1, GL_FALSE, &view_without_translate[0][0]);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxCubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default
	//draw skybox section end
	// ******************************************************************************************************************
	//draw billboard tree start
	
	glm::vec3 treePos(70, 20, 0);     // Tree position
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 lookDir = glm::normalize(my_pos - treePos); // Calculate billboard orientation
	glm::vec3 right = glm::normalize(glm::cross(up, lookDir));
	glm::vec3 newUp = glm::cross(lookDir, right);
	model = glm::mat4(1.0f);
	model = glm::translate(model, treePos);
	model[0] = glm::vec4(right, 0.0f);
	model[1] = glm::vec4(newUp, 0.0f);
	model[2] = glm::vec4(lookDir, 0.0f);
	model = glm::scale(model, glm::vec3(25.0f, 25.0f, 25.0f));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE); // setting start
	billboardTree->Use();
	glUniform1f(glGetUniformLocation(billboardTree->Program, "billboardTree"), billboardTreeTexture);
	glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "projection"), 1, GL_FALSE, projection);
	glm::mat4 billTreeModel = glm::scale(glm::mat4(1.0f), glm::vec3(25, 25, 25));
	glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(billboardtree_quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, billboardTreeTexture);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);// setting end
	glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
	//draw billboard tree end
	// ******************************************************************************************************************

	

	// Draw your scene
	//drawObjects();

	//projector disable
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	glDisable(GL_TEXTURE_2D);
	

	//frame buffer create different view section start
	glBindFramebuffer(GL_FRAMEBUFFER, 0);// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	// clear all relevant buffers
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
	glClear(GL_COLOR_BUFFER_BIT);
	this->screen->Use();
	glUniform1i(glGetUniformLocation(screen->Program, "frame_buffer_type"), tw->framebuffer->value());
	glUniform1f(glGetUniformLocation(screen->Program, "screen_w"), w());
	glUniform1f(glGetUniformLocation(screen->Program, "screen_h"), h());
	glUniform1f(glGetUniformLocation(screen->Program, "t"), 0.0f * 20); // tw->time = 0.0f
	glUniform2f(glGetUniformLocation(screen->Program, "u_texelStep"), 1.0f / w(), 1.0f / h());
	glUniform1i(glGetUniformLocation(screen->Program, "u_showEdges"), 0);
	glUniform1i(glGetUniformLocation(screen->Program, "u_fxaaOn"), 1);
	glUniform1i(glGetUniformLocation(screen->Program, "u_lumaThreshold"), 0.5);
	glUniform1i(glGetUniformLocation(screen->Program, "u_mulReduce"), 8.0f);
	glUniform1i(glGetUniformLocation(screen->Program, "u_minReduce"), 128.0f);
	glUniform1i(glGetUniformLocation(screen->Program, "u_maxSpan"), 8.0f);
	glBindVertexArray(screen_quadVAO);
	glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);	// use the color attachment texture as the texture of the quad plane

	if (tw->framebuffer->value() == 8) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[3]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[4]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[5]);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[6]);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[7]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUseProgram(0);
	//frame buffer create different view section end
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

	// Check whether we use the world cam
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

	// capybara
	Pnt3f capybara_pos = qt + up * 1.2f + forward * (-0.4f);
	float rotation[16] = {
				f.x, f.y, f.z, 0.0,
				c.x, c.y, c.z, 0.0,
				u.x, u.y, u.z, 0.0,
				0.0, 0.0, 0.0, 1.0
	};

	GLfloat projection[16];
	GLfloat view_ptr[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, view_ptr);
	glm::mat4 view = glm::make_mat4(view_ptr);
	view = glm::translate(view, glm::vec3(capybara_pos.x, capybara_pos.y, capybara_pos.z));
	view = view * glm::make_mat4(rotation);
	view = glm::rotate(view, glm::radians(90.0f), glm::vec3(1, 0, 0));
	view = glm::rotate(view, glm::radians(90.0f), glm::vec3(0, 1, 0));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(4, 4, 4));

	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, capybara_tex);
	capybara->Draw(*for_model_texture);
	glActiveTexture(GL_TEXTURE0);

	glUseProgram(0);

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
//========================================================================
//Functions ad by whitefox 
//========================================================================
