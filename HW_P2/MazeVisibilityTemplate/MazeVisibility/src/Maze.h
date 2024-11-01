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
float DEGtoRAD(float DEG) {
	return DEG * PI / 180.0;
}
float RADtoDEG(float RAD) {
	return RAD * 180.0 / PI;
}
struct Point2D {
	float x, y;
};
struct Ray2D{//clipping near is viewer and far is infinity, so two sides of frustum is two rays. 
	Point2D startPoint;
	float angle;
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
	float rotationDeg, FOV;//in degree
	float focalDistance;
	Point2D viewerPos;
	Ray2D getLeftRay() {
		return  {viewerPos, DEGtoRAD(rotationDeg - FOV / 2)};
	}
	Ray2D getRightRay() {
		return  {viewerPos, DEGtoRAD(rotationDeg + FOV / 2)};
	}
	Frustum(Point2D viewerPos, Point2D leftPoint, Point2D rightPoint, float focalDistance) {//construct a new frustum from three points
		this->viewerPos = viewerPos;
		this->focalDistance = focalDistance;
		this->FOV = RADtoDEG(atan2(leftPoint.y - viewerPos.y, leftPoint.x - viewerPos.x) - atan2(rightPoint.y - viewerPos.y, rightPoint.x - viewerPos.x));
		this->rotationDeg = RADtoDEG(atan2(leftPoint.y - viewerPos.y, leftPoint.x - viewerPos.x));
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
};


#endif

