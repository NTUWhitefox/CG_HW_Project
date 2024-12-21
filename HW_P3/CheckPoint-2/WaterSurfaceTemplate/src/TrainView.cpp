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
				add_drop(8.0f, 1.0f);
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
		/*
		section start
		*/
		if (!this->sinwave) {
			this->sinwave = new
				Shader(
					"./assets/shaders/sinwave.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/sinwave.frag");
		}

		if (!this->height_map) {
			this->height_map = new
				Shader(
					"./assets/shaders/height_map.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/height_map.frag");
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
			cubemapTexture = loadCubemap(faces);
		}

		if (!this->tiles) {
			this->tiles = new
				Shader(
					"./assets/shaders/tiles.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/tiles.frag");
			float tilesVertices[] = {
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

				//-1.0f,  1.0f, -1.0f,���n�e����γ�
				// 1.0f,  1.0f, -1.0f,
				// 1.0f,  1.0f,  1.0f,
				// 1.0f,  1.0f,  1.0f,
				//-1.0f,  1.0f,  1.0f,
				//-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};
			// skybox VAO
			glGenVertexArrays(1, &tilesVAO);
			glGenBuffers(1, &tilesVBO);
			glBindVertexArray(tilesVAO);
			glBindBuffer(GL_ARRAY_BUFFER, tilesVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(tilesVertices), &tilesVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			vector<const GLchar*> faces = {
			"./assets/images/tiles.jpg",
			"./assets/images/tiles.jpg",
			"./assets/images/tiles.jpg",
			"./assets/images/tiles.jpg",
			"./assets/images/tiles.jpg",
			"./assets/images/tiles.jpg",
			};
			tiles_cubemapTexture = loadCubemap(faces);

		}

		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
		//this->commom_matrices->size = 2 * sizeof(glm::mat4);
		//glGenBuffers(1, &this->commom_matrices->ubo);
		//glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
		//glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
		//glBindBuffer(GL_UNIFORM_BUFFER, 0);

		
		if (!wave) {
			//wave = new Model("backpack/backpack.obj");
			//wave = new Model("assets/water/cube.obj");
			//wave = new Model("water/water.obj");
			wave = new Model("assets/water/water_bunny.obj");
			//wave = new Model("water/plane.obj");

			for (int i = 0; i < 200; i++) {
				string num = to_string(i);
				if (num.size() == 1) {
					num = "00" + num;
				}
				else if (num.size() == 2) {
					num = "0" + num;
				}
				wave->add_height_map_texture((num + ".png").c_str(), "assets/height_map");
				//wave->add_height_map_texture((num + ".png").c_str(), "Images/height map2");
			}
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
		/*
		section end
		*/
		if (!this->shader)
			this->shader = new
			Shader(
				PROJECT_DIR "/assets/shaders/simple.vert",
				nullptr, nullptr, nullptr, 
				PROJECT_DIR "/assets/shaders/simple.frag");
		if (!interactive_frame) {
			this->interactive_frame = new
				Shader(
					"./assets/shaders/interactive_frame.vert",
					nullptr, nullptr, nullptr,
					"./assets/shaders/interactive_frame.frag");

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
			glGenVertexArrays(1, &interactive_quadVAO);
			glGenBuffers(1, &interactive_quadVBO);
			glBindVertexArray(interactive_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, interactive_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			// framebuffer configuration
			glGenFramebuffers(1, &interactive_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, interactive_framebuffer);

			glGenTextures(1, &interactive_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, interactive_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, interactive_textureColorbuffer, 0);

			glGenRenderbuffers(1, &interactive_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, interactive_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, interactive_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
			this->commom_matrices->size = 2 * sizeof(glm::mat4);
			glGenBuffers(1, &this->commom_matrices->ubo);
			glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
			glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

		if (!this->plane) {
			GLfloat  vertices[] = {
				-0.5f ,0.0f , -0.5f,
				-0.5f ,0.0f , 0.5f ,
				0.5f ,0.0f ,0.5f ,
				0.5f ,0.0f ,-0.5f };
			GLfloat  normal[] = {
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 1.0f, 0.0f };
			GLfloat  texture_coordinate[] = {
				0.0f, 0.0f,
				1.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f };
			GLuint element[] = {
				0, 1, 2,
				0, 2, 3, };

			this->plane = new VAO;
			this->plane->element_amount = sizeof(element) / sizeof(GLuint);
			glGenVertexArrays(1, &this->plane->vao);
			glGenBuffers(3, this->plane->vbo);
			glGenBuffers(1, &this->plane->ebo);

			glBindVertexArray(this->plane->vao);

			// Position attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Normal attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);

			// Texture Coordinate attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(2);

			//Element attribute
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

			// Unbind VAO
			glBindVertexArray(0);
		}

		if (!this->texture)
			this->texture = new Texture2D(PROJECT_DIR "/assets/images/church.png");

		// Disabled play sound
		if (!interactive_frame) {
			this->interactive_frame = new
				Shader(
					"./Codes/shaders/interactive_frame.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/interactive_frame.frag");

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
			glGenVertexArrays(1, &interactive_quadVAO);
			glGenBuffers(1, &interactive_quadVBO);
			glBindVertexArray(interactive_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, interactive_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			// framebuffer configuration
			glGenFramebuffers(1, &interactive_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, interactive_framebuffer);

			glGenTextures(1, &interactive_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, interactive_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, interactive_textureColorbuffer, 0);

			glGenRenderbuffers(1, &interactive_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, interactive_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, interactive_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		if (tiles_tex == -1) {
			tiles_tex = TextureFromFile("assets/images/tiles.jpg", ".");
			//tiles_tex = TextureFromFile("Images/church.png",".");
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

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
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {1, 0, 0, 0};
	GLfloat lightPosition3[]	= {0, -1, 0, 0};
	GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

	//********************************************************************* 
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	//drawFloor(200,10); 


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

	Shader* choose_wave;
	if (tw->waveBrowser->value() == 1) {
		choose_wave = sinwave;
	}
	else if (tw->waveBrowser->value() == 2) {
		choose_wave = height_map;
	}
	else {
		choose_wave = height_map;
	}
	choose_wave->Use();

	//glUniform1i(glGetUniformLocation(choose_wave->Program, "toon_open"), 1);
	//setUBO();
	//glBindBufferRange(
		//GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

	wave->height_map_index = tw->height_map_index;//int <- float

	GLfloat projection[16];
	GLfloat view[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, view);
	glm::mat4 view_without_translate = glm::mat4(glm::mat3(glm::make_mat4(view)));
	glm::mat4 view_inv = glm::inverse(glm::make_mat4(view));
	glm::vec3 my_pos(view_inv[3][0], view_inv[3][1], view_inv[3][2]);
	//cout << my_pos[0] << ' ' << my_pos[1] << ' ' << my_pos[2] << endl;

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 0, 0));
	model = glm::scale(model, glm::vec3(scaleValue, scaleValue, scaleValue));

	glUniform1f(glGetUniformLocation(choose_wave->Program, "amplitude"),0.15f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "interactive_amplitude"), 0.15f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "wavelength"), 1);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "interactive_wavelength"), 1.0f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "time"), tw->time);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "speed"), 1.0f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "interactive_speed"),1.0f);


	glUniform1f(glGetUniformLocation(choose_wave->Program, "Eta"), 1);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "ratio_of_reflect_refract"), tw->ratio_of_reflect_refract->value());

	GLfloat translation_and_scale[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, translation_and_scale);

	glUniform3f(glGetUniformLocation(choose_wave->Program, "viewPos"), my_pos[0], my_pos[1], my_pos[2]);
	glUniformMatrix4fv(glGetUniformLocation(choose_wave->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(choose_wave->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(choose_wave->Program, "model"), 1, GL_FALSE, &model[0][0]);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "material.diffuse"), 0.0f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "material.specular"), 1.0f);
	glUniform1f(glGetUniformLocation(choose_wave->Program, "material.shininess"), 16.0f);

	//glUniform1f(glGetUniformLocation(choose_wave->Program, "dir_open"), tw->dir_L->value());
	//glUniform1f(glGetUniformLocation(choose_wave->Program, "point_open"), tw->point_L->value());
	//glUniform1f(glGetUniformLocation(choose_wave->Program, "spot_open"), tw->spot_L->value());
	//glUniform1f(glGetUniformLocation(choose_wave->Program, "reflect_open"), tw->reflect->value());
	//glUniform1f(glGetUniformLocation(choose_wave->Program, "refract_open"), tw->refract->value());
	//	glUniform1f(glGetUniformLocation(choose_wave->Program, "flat_shading"), tw->height_map_flat->value());

	glUniform1f(glGetUniformLocation(choose_wave->Program, "skybox"), cubemapTexture);


	//dir_light(choose_wave);
	//point_light(choose_wave);
	//spot_light(choose_wave, glm::normalize(glm::vec3(0, 0, 0) - my_pos));

	glUniform2f(glGetUniformLocation(choose_wave->Program, "drop_point"), -1, -1);


	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tiles_tex);
	glUniform1i(glGetUniformLocation(choose_wave->Program, "tiles"), 2);
	wave->Draw_wave(*choose_wave, tw->waveBrowser->value());

	
	for (int i = 0; i < all_drop.size(); i++) {
		if (tw->time - all_drop[i].time > all_drop[i].keep_time) {
			all_drop.erase(all_drop.begin() + i);
			i--;
			continue;
		}
		glUniform2f(glGetUniformLocation(choose_wave->Program, "drop_point"), all_drop[i].point.x, all_drop[i].point.y);
		glUniform1f(glGetUniformLocation(choose_wave->Program, "drop_time"), all_drop[i].time);
		glUniform1f(glGetUniformLocation(choose_wave->Program, "interactive_radius"), all_drop[i].radius);
		wave->Draw_wave(*choose_wave, tw->waveBrowser->value());
	}
	
	glEnable(GL_CULL_FACE);
	glm::mat4 tiles_model = glm::scale(glm::mat4(1.0f), glm::vec3(scaleValue, scaleValue, scaleValue));

	tiles->Use();
	glUniform1f(glGetUniformLocation(tiles->Program, "tiles"), tiles_cubemapTexture);
	glUniformMatrix4fv(glGetUniformLocation(tiles->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(tiles->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(tiles->Program, "model"), 1, GL_FALSE, &tiles_model[0][0]);
	// tile cube
	glBindVertexArray(tilesVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tiles_cubemapTexture);

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default

	glDisable(GL_CULL_FACE);

	glDepthFunc(GL_LEQUAL);
	skybox->Use();
	glUniform1f(glGetUniformLocation(skybox->Program, "skybox"), cubemapTexture);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "model_view"), 1, GL_FALSE, &view_without_translate[0][0]);
	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default




	// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	// clear all relevant buffers
	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
	//glClear(GL_COLOR_BUFFER_BIT);
	//this->screen->Use();
	glUniform1i(glGetUniformLocation(screen->Program, "frame_buffer_type"), 1);
	glUniform1f(glGetUniformLocation(screen->Program, "screen_w"), w());
	glUniform1f(glGetUniformLocation(screen->Program, "screen_h"), h());
	glUniform1f(glGetUniformLocation(screen->Program, "t"), tw->time * 20);
	glBindVertexArray(screen_quadVAO);
	glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
	//glDrawArrays(GL_TRIANGLES, 0, 6);
	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);

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
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}
void TrainView::add_drop(float radius, float keep_time) {
	glBindFramebuffer(GL_FRAMEBUFFER, interactive_framebuffer);
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	// make sure we clear the framebuffer's content
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	interactive_frame->Use();

	GLfloat View[16];
	GLfloat Projection[16];

	glGetFloatv(GL_PROJECTION_MATRIX, Projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, View);

	//transformation matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 0, 0));
	model = glm::scale(model, glm::vec3(scaleValue, scaleValue, scaleValue));

	glUniformMatrix4fv(glGetUniformLocation(interactive_frame->Program, "projection"), 1, GL_FALSE, Projection);
	glUniformMatrix4fv(glGetUniformLocation(interactive_frame->Program, "view"), 1, GL_FALSE, View);
	glUniformMatrix4fv(glGetUniformLocation(interactive_frame->Program, "model"), 1, GL_FALSE, &model[0][0]);
	wave->Draw_wave(*interactive_frame, tw->waveBrowser->value());

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glm::vec3 uv;
	glReadPixels(Fl::event_x(), h() - Fl::event_y(), 1, 1, GL_RGB, GL_FLOAT, &uv[0]);

	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	if (uv.b != 1.0) {
		cout << "drop : " << uv.x << ' ' << uv.y << endl;

		all_drop.push_back(Drop(glm::vec2(uv.x, uv.y), tw->time, radius, keep_time));
	}
	else {
		//drop_point.x = -1.0f;
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

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
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

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	//HMatrix view_matrix; 
	//this->arcball.getMatrix(view_matrix);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}