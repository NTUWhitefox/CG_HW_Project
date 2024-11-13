/************************************************************************
     File:        Maze.h

     Author:     
                  Stephen Chenney, schenney@cs.wisc.edu
     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu

     Comment:    
						(c) 2001-2002 Stephen Chenney, University of Wisconsin at Madison

						Class header file for Maze class. Manages the maze.
		

     Platform:    Visio Studio.Net 2003 (converted to 2005)

*************************************************************************/

#ifndef _MAZE_H_
#define _MAZE_H_

#include <FL/math.h> // Use FLTK's math header because it defines M_PI
#include "Cell.h"
#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

//************************************************************************
//
// * A class for exceptions. Used by the constructor to pass file I/O errors
//   back.
//
//************************************************************************
class MazeException {
	private:
		char    *message;

  public:
		MazeException(const char *m = "");
		~MazeException() { delete message; };

		// Return the error message string associated with the exception.
		const char* Message(void) { return message; };
};


//************************************************************************
//
// * The maze consists of cells, separated by edges. NOTE: The maze is defined
//   assuming that z is up, with xy forming the ground plane. This is different
//   to the OpenGL viewing assumption (which has y up and xz in the ground
//   plane). You will have to take this into account when drawing the view.
//   Also, assume that the floor of the maze is at z = -1, and the ceiling is
//   at z = 1.
//
//************************************************************************
const float PI = 3.14159265358979323846f;
//float DEGtoRAD(float DEG) {
//	return DEG * PI / 180.0;
//}
//float RADtoDEG(float RAD) {
//	return RAD * 180.0 / PI;
//}
#define DEGtoRAD(DEG) (float)((DEG) * PI / 180.0)
#define RADtoDEG(RAD) (float)((RAD) * 180.0 / PI)
static float normalizeRad(float rad) {
    rad = fmod(rad, 2 * M_PI);
    if (rad < 0) {
        rad += 2 * M_PI;
    }
    return rad;
}
struct Point3D {
    float x, y, z;
};
struct Point2D {
	float x, y;
	bool operator==(Point2D p) {
		return (fabs(p.x-x) < 0.000001 && fabs(p.y-y) < 0.000001);
	}
};
struct Ray2D{//clipping near is viewer and far is infinity, so two sides of frustum is two rays. 
	Point2D startPoint;
	float angle;
	Ray2D(){};
	Ray2D(Point2D startPoint, Point2D endPoint) {
		this->startPoint = startPoint;
		this->angle = atan2(endPoint.y - startPoint.y, endPoint.x - startPoint.x);
	}
	Ray2D(Point2D startPoint, float angle) {
		this->startPoint = startPoint;
		this->angle = angle;
	}
};
struct ParametricLine {//parametrize ray(0<t<inf) and segment(0<s<1)
	Point2D startPoint;
	float dx, dy;// 
	/*
	x = startPoint.x + t * dx
	y = startPoint.y + t * dy
	*/
	ParametricLine(Ray2D ray) {
		startPoint = ray.startPoint;
		dx = cos(ray.angle);
		dy = sin(ray.angle);
	}
	ParametricLine(Point2D p1, Point2D p2) {
		startPoint = p1;
		dx = p2.x - p1.x;
		dy = p2.y - p1.y;
	}
};
struct Frustum {
	float dx, dy;
	Ray2D leftRay, rightRay;//the frustum here is not necessarily symmetric, so we need two rays to represent it
	Point2D viewerPos;
	Ray2D getLeftRay() {
		return leftRay;
	}
	Ray2D getRightRay() {
		return  rightRay;
	}
	Frustum(){};
	Frustum(Point2D viewerPos, Point2D leftPoint, Point2D rightPoint) {//construct a new frustum from old frustum and two pointshree points
		this->viewerPos = viewerPos;
		dx = (rightPoint.x-viewerPos.x) + (leftPoint.x - viewerPos.x);
		dy = (rightPoint.y - viewerPos.y) + (leftPoint.y - viewerPos.y);
		leftRay = Ray2D(viewerPos, leftPoint);
		rightRay = Ray2D(viewerPos, rightPoint);
		//printf("newDeg %f deg\n", RADtoDEG((normalizeRad(leftRay.angle) + normalizeRad(rightRay.angle))/ 2.0f));
	}
};
struct Matrix4x4 {
    float data[16];

    // Initialize identity matrix
    Matrix4x4() {
        for (int i = 0; i < 16; i++) data[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    // Multiply with another 4x4 matrix
    void multiply(const Matrix4x4& other) {
        float result[16] = {0};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result[i * 4 + j] += data[i * 4 + k] * other.data[k * 4 + j];
        std::copy(result, result + 16, data);
    }

    // Apply the matrix to a 3D point
    Point3D transform(const Point3D& point) const {
        float x = data[0] * point.x + data[4] * point.y + data[8] * point.z + data[12];
        float y = data[1] * point.x + data[5] * point.y + data[9] * point.z + data[13];
        float z = data[2] * point.x + data[6] * point.y + data[10] * point.z + data[14];
        float w = data[3] * point.x + data[7] * point.y + data[11] * point.z + data[15];
        return {x / w, y / w, z / w};
    }
};
struct TransformAPI {
    Matrix4x4 projectionMatrix;
    Matrix4x4 viewMatrix;

    // Set up a perspective matrix
    void Perspective(float fovy, float aspect, float znear, float zfar) {
        float ymax = znear * tan(fovy * PI / 360.0f);
        float xmax = ymax * aspect;
        float depth = zfar - znear;

        projectionMatrix = Matrix4x4(); // Reset to identity
        projectionMatrix.data[0] = znear / xmax;
        projectionMatrix.data[5] = znear / ymax;
        projectionMatrix.data[10] = -(zfar + znear) / depth;
        projectionMatrix.data[11] = -1.0f;
        projectionMatrix.data[14] = -(2 * zfar * znear) / depth;
        projectionMatrix.data[15] = 0.0f;
    }

    // Set up a view matrix
    void LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
        Point3D forward = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
        float forwardMag = sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        forward = {forward.x / forwardMag, forward.y / forwardMag, forward.z / forwardMag};

        Point3D up = {upX, upY, upZ};
        Point3D side = {
            forward.y * up.z - forward.z * up.y,
            forward.z * up.x - forward.x * up.z,
            forward.x * up.y - forward.y * up.x
        };
        float sideMag = sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
        side = {side.x / sideMag, side.y / sideMag, side.z / sideMag};

        up = {
            side.y * forward.z - side.z * forward.y,
            side.z * forward.x - side.x * forward.z,
            side.x * forward.y - side.y * forward.x
        };

        viewMatrix = Matrix4x4(); // Reset to identity
        viewMatrix.data[0] = side.x;
        viewMatrix.data[4] = side.y;
        viewMatrix.data[8] = side.z;

        viewMatrix.data[1] = up.x;
        viewMatrix.data[5] = up.y;
        viewMatrix.data[9] = up.z;

        viewMatrix.data[2] = -forward.x;
        viewMatrix.data[6] = -forward.y;
        viewMatrix.data[10] = -forward.z;

        viewMatrix.data[12] = -(side.x * eyeX + side.y * eyeY + side.z * eyeZ);
        viewMatrix.data[13] = -(up.x * eyeX + up.y * eyeY + up.z * eyeZ);
        viewMatrix.data[14] = forward.x * eyeX + forward.y * eyeY + forward.z * eyeZ;
    }

    // Draw a polygon in screen space given world space points
    void DrawPoly(const std::vector<Point3D>& vertices) {
        glBegin(GL_POLYGON);
        for (const auto& vertex : vertices) {
            Point3D viewTransformed = viewMatrix.transform(vertex);
            Point3D projTransformed = projectionMatrix.transform(viewTransformed);

            // Map the projected 3D point to screen space
            glVertex2f(projTransformed.x, projTransformed.y);
        }
        glEnd();
    }
};
class Maze {

	public:
		// The first constructor takes the number of cells in the x and y 
		// directions, and the cell size in each dimension. This constructor
		// creates a random maze, and returns it.
		Maze(	const int num_x, const int num_y,
				const float size_x, const float size_y);

		// The second constructor takes a maze file name to load. It may throw
		// exceptions of the MazeException class if there is an error.
		Maze(const char *f);

		~Maze(void);

	public:
		// Set the viewer's location 
		void	Set_View_Posn(float x, float y, float z);

		// Set the angle in which the viewer is looking.
		void	Set_View_Dir(const float);

		// Set the horizontal field of view.
		void	Set_View_FOV(const float);

		// Move the viewer's position. This method will do collision detection
		// between the viewer's location and the walls of the maze and prevent
		// the viewer from passing through walls.
		void	Move_View_Posn(const float dx, const float dy, const float dz);

		// Draws the map view of the maze. It is passed the minimum and maximum
		// corners of the window in which to draw.
		void	Draw_Map(int, int, int, int);

		// Draws the viewer's cell and its neighbors in the map view of the maze.
		// It is passed the minimum and maximum corners of the window in which
		// to draw.
		void	Draw_Neighbors(int, int, int, int);

		// Draws the frustum on the map view of the maze. It is passed the
		// minimum and maximum corners of the window in which to draw.
		void	Draw_Frustum(int, int, int, int);
		void    Draw_Intersects(int, int, int, int, bool);

		// Draws the first-person view of the maze. It is passed the focal distance.
		// THIS IS THE FUINCTION YOU SHOULD MODIFY.
		void	Draw_View(const float);
		void    Draw_Cell(Cell* C, Frustum F);//recursive visiblity also called "Cell and Portal visibility" algorithm
		

		// Save the maze to a file of the given name.
		bool	Save(const char*);

		// Functions to convert between degrees and radians.
		static double   To_Radians(double deg) { return deg / 180.0 * M_PI; };
		static double   To_Degrees(double rad) { return rad * 180.0 / M_PI; };
	private:
		// Functions used when creating or loading a maze.

		// Randomly generate the edge's opaque and transparency for an empty maze
		void    Build_Connectivity(const int, const int, const float, const float);
		// Grow a maze by removing candidate edges until all the cells are
		// connected. The edges are not actually removed, they are just made
		// transparent.
		void    Build_Maze(void);
		void    Set_Extents(void);
		void    Find_View_Cell(Cell*);
		void    drawWallFromEdge(Point2D P1, Point2D P2, float color[3]);
		void 	setMatrix();

	private:
		Cell				*view_cell;// The cell that currently contains the view
										  // point. You will need to use this.
		unsigned int    frame_num;	// The frame number we are currently drawing.
											// It isn't necessary, but you might find it
											// helpful for debugging or something.

		static const float	BUFFER;	// The viewer must be at least this far inside
												// an exterior wall of the maze.
												// Not implemented

		float	min_xp;	// The minimum x location of any vertex in the maze.
		float	min_yp;	// The minimum y location of any vertex in the maze.
		float	max_xp;	// The maximum x location of any vertex in the maze.
		float	max_yp;	// The maximum y location of any vertex in the maze.

	public:
		static const char	X; // Used to index into the viewer's position
		static const char	Y;
		static const char	Z;

		int		num_vertices;	// The number of vertices in the maze
		Vertex	**vertices;		// An array of pointers to the vertices.

		int		num_edges;		// The number of edges in the maze.
		Edge		**edges;			// An array of pointers to the edges.

		int		num_cells;     // The number of cells in the maze
		Cell		**cells;       // An array of pointers to the cells.

		float		viewer_posn[3];	// The x,y location of the viewer.
		float		viewer_dir;			// The direction in which the viewer is
											// looking. Measured in degrees about the z
											// axis, in the usual way.
		float		viewer_fov;			// The horizontal field of view, in degrees.
		std::vector<Point2D> wallIntersects;
		std::vector<Point2D> tranIntersects;

		float viewAspect = -1.0f;
		TransformAPI transformAPI;
};


#endif

