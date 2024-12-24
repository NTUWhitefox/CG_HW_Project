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
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glu.h>

#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include <vector>
#include <tuple>


// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;
class RainSystem;


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
#include <AL/alut.h>
// this uses the old ArcBall Code
#include "Utilities/ArcBallCam.H"
#include "Utilities/Pnt3f.H"

using std::vector;
using std::tuple;
class Model;

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

		void set_fbo(int* framebuffer, unsigned int* textureColorbuffer);

		glm::mat4 getTransformMatrix(glm::mat4 mat, glm::vec3 pos, glm::vec3 scale, glm::vec3 rotate, float rotate_angle);

		void drawModel(Model* model, Shader* shader, int tex_index, GLfloat projection[16], GLfloat view[16], glm::mat4 mat);

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
		float 			t_delta_time = 0;
		int				line_type = 1;
		bool			isarclen = true;
		float			arclength = 0;
		float			t_arclength = 0;
		int				smoke_life[50] = { 0 };
		Pnt3f			smoke_pos[50];
		int				smoke_size[50] = { 0 };
		float			physics = 0;
		int				framebuffer[8] = { -1 };
		unsigned int	textureColorbuffer[8];
		glm::mat4		current_trans = glm::mat4(1.0f);
		glm::mat4		trans[8];
		int				frame = 0;

		TrainWindow*	tw;				// The parent of this display window
		CTrack*			m_pTrack;		// The track of the entire scene

		//model
		Model* tree = nullptr;
		Model* flower = nullptr;
		int	tree_tex = -1;//tree and flower texture
		Model* capybara = nullptr;
		int	capybara_tex = -1;
		Shader* for_model = nullptr;
		Shader* for_model_texture = nullptr;
		//skybox
		GLuint skyBoxCubemapTexture;
		GLuint skyboxVAO, skyboxVBO;
		Shader* skybox = nullptr;
		//billboard tree
		Shader* billboardTree = nullptr;
		GLuint billboardTreeTexture;
		GLuint billboardtree_quadVAO, billboardtree_quadVBO, billboardtree_quadEBO;
		//flash light (project image with texture matrix)
		GLuint projectorTexture;
		Shader* projectorShader = nullptr;
		//framebuffer
		Shader* screen = nullptr;
		Shader* fbo_shader = nullptr;//for motion blur
		GLuint screen_framebuffer;
		GLuint screen_textureColorbuffer;
		GLuint screen_rbo;
		GLuint screen_quadVAO, screen_quadVBO;

		//rock
		Model* rock = nullptr;
		GLuint rock_diff;
		GLuint rock_spec;
		GLuint rock_normal;
		Shader* rockShader = nullptr;

		//grasstexture for floor
        GLuint grass = -1;

		//rain
		RainSystem* rainSystem = nullptr;
		Shader* rainShader = nullptr;
		GLuint rainTexture;

		//fractal tree
		Model* trunkCylinder = nullptr;
		Shader* trunkShader = nullptr;
		GLuint trunk_color;
		GLuint trunk_height;
		GLuint trunk_normal;
};

struct RainParticle {
	glm::vec3 position; // Position of the raindrop
	float lifetime;     // Lifetime of the particle (not used here but useful for effects)
};

class RainSystem {
private:
	unsigned int VAO, VBO, instanceVBO;
	GLuint rainTexture;
	std::vector<glm::vec3> rainPositions;
	float areaWidth, areaDepth;
	float rainSpeed;
public:
	RainSystem(float areaWidth, float areaDepth, int numParticles, float rainSpeed, GLuint rainTexture)
		: areaWidth(areaWidth), areaDepth(areaDepth), rainSpeed(rainSpeed), rainTexture(rainTexture) {
		// Load rain texture
		// Generate random initial positions for raindrops
		for (int i = 0; i < numParticles; ++i) {
			rainPositions.push_back(glm::vec3(
				randomRange(-areaWidth / 2.0f, areaWidth / 2.0f),
				randomRange(30.0f, 50.0f), // Start above the ground
				randomRange(-areaDepth / 2.0f, areaDepth / 2.0f)
			));
		}


		// Create quad for raindrop rendering
		float quadVertices[] = {
			-0.02f, 0.0f, 0.0f, 0.0f, 1.0f,
			 0.02f, 0.0f, 0.0f, 1.0f, 1.0f,
			-0.02f, -0.1f, 0.0f, 0.0f, 0.0f,
			 0.02f, -0.1f, 0.0f, 1.0f, 0.0f
		};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &instanceVBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		// Vertex positions
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Texture coordinates
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Instance data
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferData(GL_ARRAY_BUFFER, rainPositions.size() * sizeof(glm::vec3), rainPositions.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1); // Tell OpenGL this is instanced data

		glBindVertexArray(0);
	}

	~RainSystem() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &instanceVBO);
	}

	void update(float deltaTime) {
		for (auto& pos : rainPositions) {
			pos.y -= rainSpeed * deltaTime; // Move downward

			// Reset raindrop if it falls below ground level
			if (pos.y < 0.0f) {
				pos.y = randomRange(5.0f, 10.0f); // Reset to above the scene
				pos.x = randomRange(-areaWidth / 2.0f, areaWidth / 2.0f);
				pos.z = randomRange(-areaDepth / 2.0f, areaDepth / 2.0f);
			}
			//std::cout << pos.x << " " << pos.y << " " << pos.z << ";";
		}//std::cout << std::endl;

		// Update instance VBO with new positions
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, rainPositions.size() * sizeof(glm::vec3), rainPositions.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void render(GLfloat viewMatrix[], GLfloat projMatrix[], Shader* rainShader) {
		rainShader->Use();
		//glm::mat4 model = glm::mat4(1.0f);
		//model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(-50, 0, 50));
		//model = glm::scale(model, glm::vec3(1, 1, 1));
		glUniformMatrix4fv(glGetUniformLocation(rainShader->Program, "viewMatrix"), 1, GL_FALSE, viewMatrix);
		glUniformMatrix4fv(glGetUniformLocation(rainShader->Program, "projMatrix"), 1, GL_FALSE, projMatrix);
		glUniform1f(glGetUniformLocation(rainShader->Program, "scaleFactor"), 5.0f); // Adjust scaleFactor
		//glUniformMatrix4fv(glGetUniformLocation(rainShader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform1i(glGetUniformLocation(rainShader->Program, "rainTexture"), 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rainTexture);

		glBindVertexArray(VAO);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rainPositions.size());
		glBindVertexArray(0);

		glUseProgram(0);
	}

private:
	float randomRange(float min, float max) {
		return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
	}
};