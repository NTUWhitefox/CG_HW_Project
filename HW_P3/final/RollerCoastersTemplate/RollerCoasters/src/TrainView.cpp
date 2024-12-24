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

#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"

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

glm::mat4 TrainView::getTransformMatrix(glm::mat4 mat, glm::vec3 pos, glm::vec3 scale, glm::vec3 rotate, float rotate_angle) {
	mat = glm::mat4(1.0f);
	mat = glm::translate(mat, pos);
	mat = glm::scale(mat, scale);
	mat = glm::rotate(mat, glm::radians(rotate_angle), rotate);
	return mat;
}

glm::mat4 billBoardModel(glm::vec3 objectPos, glm::vec3 cameraPos, glm::vec3 cameraUp) {
	//glm::vec3 treePos(200, 20, -200);     // Tree position(200,20,200)
	//glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 lookDir = glm::normalize(cameraPos - objectPos); // Calculate billboard orientation
	glm::vec3 right = glm::normalize(glm::cross(cameraUp, lookDir));
	glm::vec3 newUp = glm::cross(lookDir, right);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, objectPos);
	model[0] = glm::vec4(right, 0.0f);
	model[1] = glm::vec4(newUp, 0.0f);
	model[2] = glm::vec4(lookDir, 0.0f);
    return model;
}
void TrainView::drawModel(Model* model, Shader* shader, int tex_index, GLfloat projection[16], GLfloat view[16], glm::mat4 mat) {
	shader->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "model"), 1, GL_FALSE, glm::value_ptr(mat));
	glUniform1i(glGetUniformLocation(shader->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	glActiveTexture(GL_TEXTURE0);
	model->Draw(*shader);
	glUseProgram(0);
}

void esfera(int radio, bool draw_line) {
	static float sphere_divide = 20.0;
	float px, py, pz;
	int i, j;
	float incO = 2 * M_PI / sphere_divide;
	float incA = M_PI / sphere_divide;
	glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i <= sphere_divide; i++) {
		for (j = 0; j <= sphere_divide; j++) {
			pz = cos(M_PI - (incA * j)) * radio;
			py = sin(M_PI - (incA * j)) * sin(incO * i) * radio;
			px = sin(M_PI - (incA * j)) * cos(incO * i) * radio;
			//printf("%lf%lf%lf\n", px, py, pz);
			glVertex3f(px, py, pz);
			pz = cos(M_PI - (incA * j)) * radio;
			py = sin(M_PI - (incA * j)) * sin(incO * (i + 1)) * radio;
			px = sin(M_PI - (incA * j)) * cos(incO * (i + 1)) * radio;
			glVertex3f(px, py, pz);
		}
	}
	glEnd();
	if (draw_line) {
		glColor3ub(0, 0, 0);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i <= sphere_divide; i++) {
			for (j = 0; j <= sphere_divide; j++) {

				pz = cos(M_PI - (incA * j)) * radio;
				py = sin(M_PI - (incA * j)) * sin(incO * i) * radio;
				px = sin(M_PI - (incA * j)) * cos(incO * i) * radio;
				//printf("%lf%lf%lf\n", px, py, pz);
				glVertex3f(px, py, pz);
				pz = cos(M_PI - (incA * j)) * radio;
				py = sin(M_PI - (incA * j)) * sin(incO * (i + 1)) * radio;
				px = sin(M_PI - (incA * j)) * cos(incO * (i + 1)) * radio;
				glVertex3f(px, py, pz);

			}
		}
		glEnd();
	}
}

void drawTreeRecursive(TrainView* tw, float curLength, int branchNum, float downScaleFactor, glm::vec3 curPos, glm::vec3 curScale, glm::vec3 curRotation, int seed) {
	if (branchNum <= 0 || curLength <= 0) return; // Base case for recursion

	// Create the model matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, curPos);                         // Translate to current position
	model = glm::rotate(model, glm::radians(curRotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X
	model = glm::rotate(model, glm::radians(curRotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y
	model = glm::rotate(model, glm::radians(curRotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z
	model = glm::scale(model, curScale);                          // Scale the cylinder

	// Pass the model matrix to the shader
	glUniformMatrix4fv(glGetUniformLocation(tw->trunkShader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// Draw the current cylinder
	tw->trunkCylinder->Draw(*tw->trunkShader);

	// Calculate the new position (tip of the current cylinder)
	glm::vec3 direction = glm::vec3(0.0f, curLength, 0.0f); // Cylinder extends along the Y-axis
	glm::mat4 rotationMatrix = glm::mat4(1.0f);
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(curRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(curRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(curRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec3 rotatedDirection = glm::vec3(rotationMatrix * glm::vec4(direction, 1.0f));
	glm::vec3 newPos = curPos + rotatedDirection;

	// Update scale and length for the new branches
	curLength *= downScaleFactor;
	glm::vec3 newScale = curScale * downScaleFactor;

	// Generate new branches
	int numBranches = 3; // You can modify this for more branches
	//radom between 30~50;
	srand(seed++);
	float zAngle = rand() % 21 + 30;
	float yAxisStep = 360.0f / numBranches;

	for (int i = 0; i < numBranches; ++i) {
		srand(seed++);
		glm::vec3 newRotation = curRotation;
		// first incline along z axis
		newRotation.z += -zAngle / (numBranches - 1);
        // then rotate along y axis
		newRotation.y += (yAxisStep * i) + rand() % 21 - 10;//plus a random between -10~10

		// Recursive call for each branch
		drawTreeRecursive(tw, curLength, branchNum - 1, downScaleFactor, newPos, newScale, newRotation, seed);
	}
	//esfera(1.0f, true);
}

void drawTree(TrainView* tw, int branchNum, GLfloat projection[16], GLfloat view[16], glm::vec3 my_pos, float lightPosition[4], float lightColor[3], int seed) {
	tw->trunkShader->Use();
	glUniformMatrix4fv(glGetUniformLocation(tw->trunkShader->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(tw->trunkShader->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(tw->trunkShader->Program, "cameraPos"), 1, GL_FALSE, glm::value_ptr(my_pos));
	glUniform3fv(glGetUniformLocation(tw->trunkShader->Program, "lightPos"), 1, lightPosition);
	glUniform3f(glGetUniformLocation(tw->trunkShader->Program, "lightColor"), lightColor[0], lightColor[1], lightColor[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tw->trunk_color);
	glUniform1i(glGetUniformLocation(tw->trunkShader->Program, "albedoMap"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tw->trunk_height);
	glUniform1i(glGetUniformLocation(tw->trunkShader->Program, "heightMap"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tw->trunk_normal);
	glUniform1i(glGetUniformLocation(tw->trunkShader->Program, "normalMap"), 2);
	glm::vec3 curPos = glm::vec3(0.0f, 0.0f, 0.0f); // Initial position of
    glm::vec3 curRotation = glm::vec3(0.0f, 0.0f, 0.0f); // Initial	rotation of the trunk
    glm::vec3 curScale = glm::vec3(3.0f, 6.0f, 3.0f); //
	drawTreeRecursive(tw, 18.0f, branchNum, 0.8f, curPos, curScale, curRotation, seed);
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
			for (int i = 0; i < flowerNum; i++) {
				glm::vec3 position = glm::vec3(rand() % 200 - 100, 0, rand() % 200 - 100);
				flowerPos.push_back(position);
			}
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
		if (!house1) 
            house1 = new Model("./assets/objects/house_001.obj");
		if (!house2) 
			house2 = new Model("./assets/objects/house_002.obj");
		if (!house3) 
			house3 = new Model("./assets/objects/house_003.obj");
		if (fantasyTexture == -1) 
            fantasyTexture = TextureFromFile("./assets/objects/Texture_fantasy.png", ".");
		//if(!ferris)
            //ferris = new Model("./assets/objects/ferris_wheel_low_poly.glb");



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

		if (grass == -1) {
			grass = TextureFromFile("/assets/images/grass-1024x1024.png", ".");
		}

		if (rainSystem == nullptr) {
			rainTexture = TextureFromFile("/assets/images/rain.png", ".");
			rainSystem = new RainSystem(100.0f, 100.0f, 300, 5.0f, rainTexture);
			rainShader = new Shader(
				"./assets/shaders/rain.vert",
				nullptr, nullptr, nullptr,
				"./assets/shaders/rain.frag");
		}

		if (trunkShader == nullptr) {
			trunkCylinder = new Model("./assets/objects/cylinder.obj");
			trunk_color = TextureFromFile("/assets/images/wood_0025_color_1k.jpg", ".");
			trunk_height = TextureFromFile("/assets/images/wood_0025_height_1k.png", ".");
			trunk_normal = TextureFromFile("/assets/images/wood_0025_normal_opengl_1k.png", ".");
			trunkShader = new Shader(
				"./assets/shaders/trunk.vert",
				nullptr, nullptr, nullptr,
				"./assets/shaders/trunk.frag");
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
	}else {
		glEnable(GL_LIGHT3);
	}

	/*
	if (tw->dirlight->value()) {
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float whiteDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float angle = tw->lightanle->value();
		float radius = tw->lightradius->value();
		float strength = tw->brightness->value();
		//lightPosition[0] = 0.0f; lightPosition[1] = 1.0f; lightPosition[2] = 0.0f; lightPosition[3] = 0.0f;
		glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	}
	else {
		glDisable(GL_LIGHT0);
	}
	*/
	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	float lightPosition[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float PI = 3.1415926f;
	float strength = 2.5f;
	float lightColor[3];
	if (tw->pointlight->value()) {
		lightColor[0] = tw->lightR->value(); lightColor[1] = tw->lightG->value(); lightColor[2] = tw->lightB->value();
	}
	else {
		lightColor[0] = 0.8f; lightColor[1] = 0.8f; lightColor[2] = 0.8f;
	}
	if (tw->pointlight->value()) {
		// Enable only the point light
		strength = tw->brightness->value();
		float Ambient[] = { lightColor[0] * strength,  lightColor[1] * strength, lightColor[2] * strength, 1.0f }; // Scale red ambient by strength
		float Diffuse[] = { lightColor[0] * strength, lightColor[1] * strength, lightColor[2] * strength, 1.0f }; // Scale red diffuse by strength
		float angle = tw->lightangle->value();
		float radius = tw->lightradius->value();
		lightPosition[0] = radius * cos(angle);
		lightPosition[1] = 20.0f;
		lightPosition[2] = radius * sin(angle);
		lightPosition[3] = 1.0f; // Positional light
		glEnable(GL_LIGHT1);// Set the point light properties
		glLightfv(GL_LIGHT1, GL_AMBIENT, Ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, Diffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, lightPosition);
		// Disable the other lights
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT2);
	}
	else {
		// Enable the complex light setup
	GLfloat lightPosition1[] = { 0, 1, 1, 0 }; // Directional light
	GLfloat lightPosition2[] = { 1, 0, 0, 0 };
	GLfloat lightPosition3[] = { 0, -1, 0, 0 };
	GLfloat yellowLight[] = { 0.5f, 0.5f, 0.1f, 1.0f };
	GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat blueLight[] = { 0.1f, 0.1f, 0.3f, 1.0f };
	GLfloat grayLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	glEnable(GL_LIGHT0);// Configure the first light
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);
	glEnable(GL_LIGHT1);// Configure the second light
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);
	glLightfv(GL_LIGHT1, GL_AMBIENT, grayLight);
	glEnable(GL_LIGHT2);// Configure the third light
	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);
	// Disable the point light
	glDisable(GL_LIGHT1);
	lightPosition[0] = lightPosition[2] = lightPosition[3] = 0.0f;
	lightPosition[1] = 500.f;//the light simulate light of daytime, which is viewed by shaders as at high postion.
	}

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
	else {
		glUseProgram(0);
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
	

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(500, 50, grass, tw->floornoise->value());


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

	//draw tree model
	//model = getTransformMatrix(model, glm::vec3(0, 0, 0), glm::vec3(2, 2, 2), glm::vec3(0, 1, 0), 0);
	//drawModel(tree, for_model_texture, tree_tex, projection, view, model);

	//draw flower model
	int flowerNum = 25;
	for (int i = 0; i < flowerNum; i++) {
		model = getTransformMatrix(model, flowerPos[i], glm::vec3(5, 5, 5), glm::vec3(0, 1, 0), 0);
        drawModel(flower, for_model_texture, tree_tex, projection, view, model);
	}
	
	drawModel(flower, for_model_texture, tree_tex, projection, view, model);

	model = getTransformMatrix(model, glm::vec3(-100, 0, 70), glm::vec3(10, 10, 10), glm::vec3(0, 1, 0), -90);
	drawModel(house1, for_model_texture, fantasyTexture, projection, view, model);

	model = getTransformMatrix(model, glm::vec3(+100, 0, 30), glm::vec3(10, 10, 10), glm::vec3(0, 1, 0), -90);
	drawModel(house2, for_model_texture, fantasyTexture, projection, view, model);

	model = getTransformMatrix(model, glm::vec3(-60, 0, -110), glm::vec3(10, 10, 10), glm::vec3(0, 1, 0), -90);
	drawModel(house3, for_model_texture, fantasyTexture, projection, view, model);

	

	//draw rock model
	if (tw->centerObject->value() == 2) {
		rockShader->Use();
		model = getTransformMatrix(model, glm::vec3(0, 5, 0), glm::vec3(15, 15, 15), glm::vec3(0, 1, 0), 0);
		glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "projection"), 1, GL_FALSE, projection);
		glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "view"), 1, GL_FALSE, view);
		glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(rockShader->Program, "viewPos"), 1, GL_FALSE, glm::value_ptr(my_pos));
		glUniform3fv(glGetUniformLocation(rockShader->Program, "lightPos"), 1, lightPosition);
		glUniform1f(glGetUniformLocation(rockShader->Program, "lightIntensity"), strength / 5.0f);
		glUniform3f(glGetUniformLocation(rockShader->Program, "lightColor"), lightColor[0], lightColor[1], lightColor[2]);
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
		if (created) {
			randSeed++;
			created = false;
		}
	}
	else if (tw->centerObject->value() == 3) {
		drawTree(this, 7, projection, view, my_pos, lightPosition, lightColor, randSeed);
        created = true;
	} 
	//cout << tw->centerObject->value() << endl;
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
	//model = billBoardModel(glm::vec3(200, 20, -200), my_pos, glm::vec3(0, 1, 0));
	//model = glm::scale(model, glm::vec3(25.0f, 25.0f, 25.0f));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE); // setting start
	billboardTree->Use();
	glUniform1f(glGetUniformLocation(billboardTree->Program, "billboardTree"), billboardTreeTexture);
	glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "projection"), 1, GL_FALSE, projection);
	glBindVertexArray(billboardtree_quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, billboardTreeTexture);

	for (int x = 200; x >= -200; x -= 10) {
		model = billBoardModel(glm::vec3(x, 20, -200), my_pos, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(25.0f, 25.0f, 25.0f));
		glUniformMatrix4fv(glGetUniformLocation(billboardTree->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);// setting end
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	

	//draw billboard tree end
	// ******************************************************************************************************************

	rainSystem->update(1.0f / 30.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	rainSystem->render(view, projection, rainShader);

	//projector disable
	if (tw->projector->value()) {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_R);
		glDisable(GL_TEXTURE_GEN_Q);
		glDisable(GL_TEXTURE_2D);
	}
	

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
						drawWheel(qt0 + cross_t, Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, 
							qt0.y - getFloorHeight(qt0.x + cross_t.x, qt0.z + cross_t.z, tw->floornoise->value()));
						drawWheel(qt0 + cross_t * (-1), Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, 
							qt0.y - getFloorHeight(qt0.x + cross_t.x, qt0.z + cross_t.z, tw->floornoise->value()));
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
					drawWheel(qt0 + cross_t, Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, 
						qt0.y - getFloorHeight(qt0.x + cross_t.x, qt0.z + cross_t.z, tw->floornoise->value()));
					drawWheel(qt0 + cross_t * (-1), Pnt3f(1, 0, 0), Pnt3f(0, -1, 0), Pnt3f(0, 0, 1), 0.5f, 
						qt0.y - getFloorHeight(qt0.x + cross_t.x, qt0.z + cross_t.z, tw->floornoise->value()));
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

	drawModel(capybara, for_model_texture, capybara_tex, projection, glm::value_ptr(view), model);
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
