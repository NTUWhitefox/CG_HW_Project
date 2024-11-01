/************************************************************************
     File:        Maze.cpp

     Author:
                  Stephen Chenney, schenney@cs.wisc.edu
     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu

     Comment:
                                                (c) 2001-2002 Stephen Chenney, University of Wisconsin at Madison

                                                Class header file for Maze class. Manages the maze.


     Platform:    Visio Studio.Net 2003 (converted to 2005)

*************************************************************************/

#include "Maze.h"

#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vector>

const char Maze::X = 0;
const char Maze::Y = 1;
const char Maze::Z = 2;

const float Maze::BUFFER = 0.1f;

//**********************************************************************
//
// * Constructor for the maze exception
//======================================================================
MazeException::
    MazeException(const char *m)
//======================================================================
{
    message = new char[strlen(m) + 4];
    strcpy(message, m);
}

//**********************************************************************
//
// * Constructor to create the default maze
//======================================================================
Maze::
    Maze(const int nx, const int ny, const float sx, const float sy)
//======================================================================
{
    // Build the connectivity structure.
    Build_Connectivity(nx, ny, sx, sy);

    // Make edges transparent to create a maze.
    Build_Maze();

    // Set the extents of the maze
    Set_Extents();

    // Default values for the viewer.
    viewer_posn[X] = viewer_posn[Y] = viewer_posn[Z] = 0.0;
    viewer_dir = 0.0;
    viewer_fov = 45.0;

    // Always start on the 0th frame.
    frame_num = 0;
}

//**********************************************************************
//
// * Construtor to read in precreated maze
//======================================================================
Maze::
    Maze(const char *filename)
//======================================================================
{
    char err_string[128];
    FILE *f;
    int i;

    // Open the file

    if (!(f = fopen(filename, "r")))
        throw new MazeException("Maze: Couldn't open file");
    // Get the total number of vertices
    if (fscanf(f, "%d", &num_vertices) != 1)
        throw new MazeException("Maze: Couldn't read number of vertices");

    // Read in each vertices
    vertices = new Vertex *[num_vertices];
    for (i = 0; i < num_vertices; i++) {
        float x, y;
        if (fscanf(f, "%g %g", &x, &y) != 2) {
            sprintf(err_string, "Maze: Couldn't read vertex number %d", i);
            throw new MazeException(err_string);
        }
        vertices[i] = new Vertex(i, x, y);
    }

    // Get the number of edges
    if (fscanf(f, "%d", &num_edges) != 1)
        throw new MazeException("Maze: Couldn't read number of edges");

    // read in all edges
    edges = new Edge *[num_edges];
    for (i = 0; i < num_edges; i++) {
        int vs, ve, cl, cr, o;
        float r, g, b;
        if (fscanf(f, "%d %d %d %d %d %g %g %g",
                   &vs, &ve, &cl, &cr, &o, &r, &g, &b) != 8) {
            sprintf(err_string, "Maze: Couldn't read edge number %d", i);
            throw new MazeException(err_string);
        }
        edges[i] = new Edge(i, vertices[vs], vertices[ve], r, g, b);
        edges[i]->Add_Cell((Cell *)cl, Edge::LEFT);
        edges[i]->Add_Cell((Cell *)cr, Edge::RIGHT);
        edges[i]->opaque = o ? true : false;
    }

    // Read in the number of cells
    if (fscanf(f, "%d", &num_cells) != 1)
        throw new MazeException("Maze: Couldn't read number of cells");

    // Read in all cells
    cells = new Cell *[num_cells];
    for (i = 0; i < num_cells; i++) {
        int epx, epy, emx, emy;
        if (fscanf(f, "%d %d %d %d", &epx, &epy, &emx, &emy) != 4) {
            sprintf(err_string, "Maze: Couldn't read cell number %d", i);
            throw new MazeException(err_string);
        }
        cells[i] = new Cell(i, epx >= 0 ? edges[epx] : NULL,
                            epy >= 0 ? edges[epy] : NULL,
                            emx >= 0 ? edges[emx] : NULL,
                            emy >= 0 ? edges[emy] : NULL);
        if (cells[i]->edges[0]) {
            if (cells[i]->edges[0]->neighbors[0] == (Cell *)i)
                cells[i]->edges[0]->neighbors[0] = cells[i];
            else if (cells[i]->edges[0]->neighbors[1] == (Cell *)i)
                cells[i]->edges[0]->neighbors[1] = cells[i];
            else {
                sprintf(err_string,
                        "Maze: Cell %d not one of edge %d's neighbors",
                        i, cells[i]->edges[0]->index);
                throw new MazeException(err_string);
            }
        }

        if (cells[i]->edges[1]) {
            if (cells[i]->edges[1]->neighbors[0] == (Cell *)i)
                cells[i]->edges[1]->neighbors[0] = cells[i];
            else if (cells[i]->edges[1]->neighbors[1] == (Cell *)i)
                cells[i]->edges[1]->neighbors[1] = cells[i];
            else {
                sprintf(err_string,
                        "Maze: Cell %d not one of edge %d's neighbors",
                        i, cells[i]->edges[1]->index);
                throw new MazeException(err_string);
            }
        }
        if (cells[i]->edges[2]) {
            if (cells[i]->edges[2]->neighbors[0] == (Cell *)i)
                cells[i]->edges[2]->neighbors[0] = cells[i];
            else if (cells[i]->edges[2]->neighbors[1] == (Cell *)i)
                cells[i]->edges[2]->neighbors[1] = cells[i];
            else {
                sprintf(err_string,
                        "Maze: Cell %d not one of edge %d's neighbors",
                        i, cells[i]->edges[2]->index);
                throw new MazeException(err_string);
            }
        }
        if (cells[i]->edges[3]) {
            if (cells[i]->edges[3]->neighbors[0] == (Cell *)i)
                cells[i]->edges[3]->neighbors[0] = cells[i];
            else if (cells[i]->edges[3]->neighbors[1] == (Cell *)i)
                cells[i]->edges[3]->neighbors[1] = cells[i];
            else {
                sprintf(err_string,
                        "Maze: Cell %d not one of edge %d's neighbors",
                        i, cells[i]->edges[3]->index);
                throw new MazeException(err_string);
            }
        }
    }

    if (fscanf(f, "%g %g %g %g %g",
               &(viewer_posn[X]), &(viewer_posn[Y]), &(viewer_posn[Z]),
               &(viewer_dir), &(viewer_fov)) != 5)
        throw new MazeException("Maze: Error reading view information.");

    // Some edges have no neighbor on one side, so be sure to set their
    // pointers to NULL. (They were set at -1 by the save/load process.)
    for (i = 0; i < num_edges; i++) {
        if (edges[i]->neighbors[0] == (Cell *)-1)
            edges[i]->neighbors[0] = NULL;
        if (edges[i]->neighbors[1] == (Cell *)-1)
            edges[i]->neighbors[1] = NULL;
    }

    fclose(f);

    Set_Extents();

    // Figure out which cell the viewer is in, starting off by guessing the
    // 0th cell.
    Find_View_Cell(cells[0]);

    frame_num = 0;
}

//**********************************************************************
//
// * Destructor must free all the memory allocated.
//======================================================================
Maze::
    ~Maze(void)
//======================================================================
{
    int i;

    for (i = 0; i < num_vertices; i++)
        delete vertices[i];
    delete[] vertices;

    for (i = 0; i < num_edges; i++)
        delete edges[i];
    delete[] edges;

    for (i = 0; i < num_cells; i++)
        delete cells[i];
    delete[] cells;
}

//**********************************************************************
//
// * Randomly generate the edge's opaque and transparency for an empty maze
//======================================================================
void Maze::
    Build_Connectivity(const int num_x, const int num_y,
                       const float sx, const float sy)
//======================================================================
{
    int i, j, k;
    int edge_i;

    // Ugly code to allocate all the memory for a new maze and to associate
    // edges with vertices and faces with edges.

    // Allocate and position the vertices.
    num_vertices = (num_x + 1) * (num_y + 1);
    vertices = new Vertex *[num_vertices];
    k = 0;
    for (i = 0; i < num_y + 1; i++) {
        for (j = 0; j < num_x + 1; j++) {
            vertices[k] = new Vertex(k, j * sx, i * sy);
            k++;
        }
    }

    // Allocate the edges, and associate them with their vertices.
    // Edges in the x direction get the first num_x * ( num_y + 1 ) indices,
    // edges in the y direction get the rest.
    num_edges = (num_x + 1) * num_y + (num_y + 1) * num_x;
    edges = new Edge *[num_edges];
    k = 0;
    for (i = 0; i < num_y + 1; i++) {
        int row = i * (num_x + 1);
        for (j = 0; j < num_x; j++) {
            int vs = row + j;
            int ve = row + j + 1;
            edges[k] = new Edge(k, vertices[vs], vertices[ve],
                                rand() / (float)RAND_MAX * 0.5f + 0.25f,
                                rand() / (float)RAND_MAX * 0.5f + 0.25f,
                                rand() / (float)RAND_MAX * 0.5f + 0.25f);
            k++;
        }
    }

    edge_i = k;
    for (i = 0; i < num_y; i++) {
        int row = i * (num_x + 1);
        for (j = 0; j < num_x + 1; j++) {
            int vs = row + j;
            int ve = row + j + num_x + 1;
            edges[k] = new Edge(k, vertices[vs], vertices[ve],
                                rand() / (float)RAND_MAX * 0.5f + 0.25f,
                                rand() / (float)RAND_MAX * 0.5f + 0.25f,
                                rand() / (float)RAND_MAX * 0.5f + 0.25f);
            k++;
        }
    }

    // Allocate the cells and associate them with their edges.
    num_cells = num_x * num_y;
    cells = new Cell *[num_cells];
    k = 0;
    for (i = 0; i < num_y; i++) {
        int row_x = i * (num_x + 1);
        int row_y = i * num_x;
        for (j = 0; j < num_x; j++) {
            int px = edge_i + row_x + 1 + j;
            int py = row_y + j + num_x;
            int mx = edge_i + row_x + j;
            int my = row_y + j;
            cells[k] = new Cell(k, edges[px], edges[py], edges[mx], edges[my]);
            edges[px]->Add_Cell(cells[k], Edge::LEFT);
            edges[py]->Add_Cell(cells[k], Edge::RIGHT);
            edges[mx]->Add_Cell(cells[k], Edge::RIGHT);
            edges[my]->Add_Cell(cells[k], Edge::LEFT);
            k++;
        }
    }
}

//**********************************************************************
//
// * Add edges from cell to the set that are available for removal to
//   grow the maze.
//======================================================================
static void
Add_To_Available(Cell *cell, int *available, int &num_available)
//======================================================================
{
    int i, j;

    // Add edges from cell to the set that are available for removal to
    // grow the maze.

    for (i = 0; i < 4; i++) {
        Cell *neighbor = cell->edges[i]->Neighbor(cell);

        if (neighbor && !neighbor->counter) {
            int candidate = cell->edges[i]->index;
            for (j = 0; j < num_available; j++)
                if (candidate == available[j]) {
                    printf("Breaking early\n");
                    break;
                }
            if (j == num_available) {
                available[num_available] = candidate;
                num_available++;
            }
        }
    }

    cell->counter = 1;
}

//**********************************************************************
//
// * Grow a maze by removing candidate edges until all the cells are
//   connected. The edges are not actually removed, they are just made
//   transparent.
//======================================================================
void Maze::
    Build_Maze()
//======================================================================
{
    Cell *to_expand;
    int index;
    int *available = new int[num_edges];
    int num_available = 0;
    int num_visited;
    int i;

    srand(time(NULL));

    // Choose a random starting cell.
    index = (int)floor((rand() / (float)RAND_MAX) * num_cells);
    to_expand = cells[index];
    Add_To_Available(to_expand, available, num_available);
    num_visited = 1;

    // Join cells up by making edges opaque.
    while (num_visited < num_cells && num_available > 0) {
        int ei;

        index = (int)floor((rand() / (float)RAND_MAX) * num_available);
        to_expand = NULL;

        ei = available[index];

        if (edges[ei]->neighbors[0] &&
            !edges[ei]->neighbors[0]->counter)
            to_expand = edges[ei]->neighbors[0];
        else if (edges[ei]->neighbors[1] &&
                 !edges[ei]->neighbors[1]->counter)
            to_expand = edges[ei]->neighbors[1];

        if (to_expand) {
            edges[ei]->opaque = false;
            Add_To_Available(to_expand, available, num_available);
            num_visited++;
        }

        available[index] = available[num_available - 1];
        num_available--;
    }

    for (i = 0; i < num_cells; i++)
        cells[i]->counter = 0;
}

//**********************************************************************
//
// * Go through all the vertices looking for the minimum and maximum
//   extents of the maze.
//======================================================================
void Maze::
    Set_Extents(void)
//======================================================================
{
    int i;

    min_xp = vertices[0]->posn[Vertex::X];
    max_xp = vertices[0]->posn[Vertex::X];
    min_yp = vertices[0]->posn[Vertex::Y];
    max_yp = vertices[0]->posn[Vertex::Y];
    for (i = 1; i < num_vertices; i++) {
        if (vertices[i]->posn[Vertex::X] > max_xp)
            max_xp = vertices[i]->posn[Vertex::X];
        if (vertices[i]->posn[Vertex::X] < min_xp)
            min_xp = vertices[i]->posn[Vertex::X];
        if (vertices[i]->posn[Vertex::Y] > max_yp)
            max_yp = vertices[i]->posn[Vertex::Y];
        if (vertices[i]->posn[Vertex::Y] < min_yp)
            min_yp = vertices[i]->posn[Vertex::Y];
    }
}

//**********************************************************************
//
// * Figure out which cell the view is in, using seed_cell as an
//   initial guess. This procedure works by repeatedly checking
//   whether the viewpoint is in the current cell. If it is, we're
//   done. If not, Point_In_Cell returns in new_cell the next cell
//   to test. The new cell is the one on the other side of an edge
//   that the point is "outside" (meaning that it might be inside the
//   new cell).
//======================================================================
void Maze::
    Find_View_Cell(Cell *seed_cell)
//======================================================================
{
    Cell *new_cell;

    //
    while (!(seed_cell->Point_In_Cell(viewer_posn[X], viewer_posn[Y],
                                      viewer_posn[Z], new_cell))) {
        if (new_cell == 0) {
            // The viewer is outside the top or bottom of the maze.
            throw new MazeException("Maze: View not in maze\n");
        }

        seed_cell = new_cell;
    }

    view_cell = seed_cell;
}

//**********************************************************************
//
// * Move the viewer's position. This method will do collision detection
//   between the viewer's location and the walls of the maze and prevent
//   the viewer from passing through walls.
//======================================================================
void Maze::
    Move_View_Posn(const float dx, const float dy, const float dz)
//======================================================================
{
    Cell *new_cell;
    float xs, ys, zs, xe, ye, ze;

    // Move the viewer by the given amount. This does collision testing to
    // prevent walking through walls. It also keeps track of which cells the
    // viewer is in.

    // Set up a line segment from the start to end points of the motion.
    xs = viewer_posn[X];
    ys = viewer_posn[Y];
    zs = viewer_posn[Z];
    xe = xs + dx;
    ye = ys + dy;
    ze = zs + dz;

    // Fix the z to keep it in the maze.
    if (ze > 1.0f - BUFFER)
        ze = 1.0f - BUFFER;
    if (ze < BUFFER - 1.0f)
        ze = BUFFER - 1.0f;

    // Clip_To_Cell clips the motion segment to the view_cell if the
    // segment intersects an opaque edge. If the segment intersects
    // a transparent edge (through which it can pass), then it clips
    // the motion segment so that it _starts_ at the transparent edge,
    // and it returns the cell the viewer is entering. We keep going
    // until Clip_To_Cell returns NULL, meaning we've done as much of
    // the motion as is possible without passing through walls.
    while ((new_cell = view_cell->Clip_To_Cell(xs, ys, xe, ye, BUFFER)))
        view_cell = new_cell;

    // The viewer is at the end of the motion segment, which may have
    // been clipped.
    viewer_posn[X] = xe;
    viewer_posn[Y] = ye;
    viewer_posn[Z] = ze;
}

//**********************************************************************
//
// * Set the viewer's location
//======================================================================
void Maze::
    Set_View_Posn(float x, float y, float z)
//======================================================================
{
    // First make sure it's in some cell.
    // This assumes that the maze is rectangular.
    if (x < min_xp + BUFFER)
        x = min_xp + BUFFER;
    if (x > max_xp - BUFFER)
        x = max_xp - BUFFER;
    if (y < min_yp + BUFFER)
        y = min_yp + BUFFER;
    if (y > max_yp - BUFFER)
        y = max_yp - BUFFER;
    if (z < -1.0f + BUFFER)
        z = -1.0f + BUFFER;
    if (z > 1.0f - BUFFER)
        z = 1.0f - BUFFER;

    viewer_posn[X] = x;
    viewer_posn[Y] = y;
    viewer_posn[Z] = z;

    // Figure out which cell we're in.
    Find_View_Cell(cells[0]);
}

//**********************************************************************
//
// * Set the angle in which the viewer is looking.
//======================================================================
void Maze::
    Set_View_Dir(const float d)
//======================================================================
{
    viewer_dir = d;
}

//**********************************************************************
//
// * Set the horizontal field of view.
//======================================================================
void Maze::
    Set_View_FOV(const float f)
//======================================================================
{
    viewer_fov = f;
}

//**********************************************************************
//
// * Draws the map view of the maze. It is passed the minimum and maximum
//   corners of the window in which to draw.
//======================================================================
void Maze::
    Draw_Map(int min_x, int min_y, int max_x, int max_y)
//======================================================================
{
    int height;
    float scale_x, scale_y, scale;
    int i;

    // Figure out scaling factors and the effective height of the window.
    scale_x = (max_x - min_x - 10) / (max_xp - min_xp);
    scale_y = (max_y - min_y - 10) / (max_yp - min_yp);
    scale = scale_x > scale_y ? scale_y : scale_x;
    height = (int)ceil(scale * (max_yp - min_yp));

    min_x += 5;
    min_y += 5;

    // Draw all the opaque edges.
    for (i = 0; i < num_edges; i++)
        if (edges[i]->opaque) {
            float x1, y1, x2, y2;

            x1 = edges[i]->endpoints[Edge::START]->posn[Vertex::X];
            y1 = edges[i]->endpoints[Edge::START]->posn[Vertex::Y];
            x2 = edges[i]->endpoints[Edge::END]->posn[Vertex::X];
            y2 = edges[i]->endpoints[Edge::END]->posn[Vertex::Y];

            fl_color((unsigned char)floor(edges[i]->color[0] * 255.0),
                     (unsigned char)floor(edges[i]->color[1] * 255.0),
                     (unsigned char)floor(edges[i]->color[2] * 255.0));
            fl_line_style(FL_SOLID);
            fl_line(min_x + (int)floor((x1 - min_xp) * scale),
                    min_y + height - (int)floor((y1 - min_yp) * scale),
                    min_x + (int)floor((x2 - min_xp) * scale),
                    min_y + height - (int)floor((y2 - min_yp) * scale));
        }
}

/*
Whitefox's personal space: my own function is written here: start
*/

struct Point3D {
    float x, y, z;
};

Point2D RaySegIntersect(const Ray2D ray, const Point2D P1, const Point2D P2, bool &isIntersect) {
    // check parallel
    ParametricLine l1(ray), l2(P1, P2);
    if (fabs(l1.dx * l2.dy - l2.dx * l1.dy) <= 0.000001) {
        isIntersect = false;
        return {-1e9, -1e9};
    }
    float t = (l2.dx * (l1.startPoint.y - l2.startPoint.y) - l2.dy * (l1.startPoint.x - l2.startPoint.x)) / (l2.dy * l1.dx - l2.dx * l1.dy);
    float s = (l1.dx * (l1.startPoint.y - l2.startPoint.y) - l1.dy * (l1.startPoint.x - l2.startPoint.x)) / (l2.dy * l1.dx - l2.dx * l1.dy);
    if (t >= 0 && s >= 0 && s <= 1) {
        isIntersect = true;
        return {l1.startPoint.x + t * l1.dx, l1.startPoint.y + t * l1.dy};
    } else {
        isIntersect = false;
        return {-1e9, -1e9};
    }
}

float sideTest(ParametricLine line, Point2D point) {  // left: 1, right: -1, on: 0
    return line.dx * (point.y - line.startPoint.y) - line.dy * (point.x - line.startPoint.x);
}
float frontTest(ParametricLine line, Point2D point) {  // front: 1, back: -1, on: 0
    return (point.x - line.startPoint.x) * line.dx + (point.y - line.startPoint.y) * line.dy;
}
bool clipToFrustum(const Edge *e, Frustum f, Point2D &resP1, Point2D &resP2, bool debugMode) {
    Point2D p1 = {e->endpoints[0]->posn[0], e->endpoints[0]->posn[1]};
    Point2D p2 = {e->endpoints[1]->posn[0], e->endpoints[1]->posn[1]};
    bool isIntersectL = 0, isIntersectR = 0;
    // Define left and right rays from the frustum parameters
    Ray2D leftRay = f.getLeftRay();
    Ray2D rightRay = f.getRightRay();
    ParametricLine leftPara = ParametricLine(leftRay);
    ParametricLine rightPara = ParametricLine(rightRay);
    // check inside outside by cross product
    if (debugMode) {
        // printf("wut1 %f, %f, %f, %f\n",sideTest(leftPara, p1), sideTest(rightPara, p1), frontTest(leftPara, p1), frontTest(rightPara, p1));
        // printf("wut2 %f, %f, %f, %f\n",sideTest(leftPara, p2), sideTest(rightPara, p2), frontTest(leftPara, p2), frontTest(rightPara, p2));
    }
    bool p1Inside = sideTest(leftPara, p1) * sideTest(rightPara, p1) < 0 && frontTest(leftPara, p1) > 0 && frontTest(rightPara, p1) > 0;
    bool p2Inside = sideTest(leftPara, p2) * sideTest(rightPara, p2) < 0 && frontTest(leftPara, p2) > 0 && frontTest(rightPara, p2) > 0;
    Point2D intersectL = RaySegIntersect(leftRay, p1, p2, isIntersectL);
    Point2D intersectR = RaySegIntersect(rightRay, p1, p2, isIntersectR);
    // if(debugMode) printf("edgeN: %d, p1Inside: %d, p2Inside: %d, isIntersectL: %d, isIntersectR: %d\n",e->index, p1Inside, p2Inside, isIntersectL, isIntersectR);

    if (p1Inside && p2Inside) {
        resP1 = p1;
        resP2 = p2;
        return true;
    }
    if (!p1Inside && !p2Inside) {
        // edge may across the frustum
        if (isIntersectL && isIntersectR) {
            // if(debugMode) printf("case2\n");
            resP1 = intersectL;
            resP2 = intersectR;
            return true;
        }
        // if(debugMode) printf("case3\n");
        return false;
    }
    if (p1Inside && !p2Inside) {
        // if(debugMode) printf("case4\n");
        resP1 = p1;
        if (isIntersectL)
            resP2 = intersectL;
        else
            resP2 = intersectR;
        return true;
    }
    if (!p1Inside && p2Inside) {
        // if(debugMode) printf("case5\n");
        resP1 = p2;
        if (isIntersectR)
            resP2 = intersectR;
        else
            resP2 = intersectL;
        return true;
    }
    return false;
}
bool framePeriod(int frameCnt, int perFrame) {
    return (frameCnt % perFrame) == 0;
}
void MatrixMultiply(const float m1[16], const float m2[16], float result[16]) {
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) {
            result[row * 4 + col] = 0.0f;
            for (int k = 0; k < 4; ++k) result[row * 4 + col] += m1[row * 4 + k] * m2[k * 4 + col];
        }
}
void viewBound(float *matrix, float left, float right, float bottom, float top, float znear, float zfar) {  // used by perspective matrix
    float temp, temp2, temp3, temp4;
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zfar - znear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zfar) / temp4;
    matrix[15] = 0.0;
}
void PerspectiveMatrix(float matrix[16], float fovyInDegrees, float aspectRatio, float znear, float zfar) {
    float ymax = znear * tanf(fovyInDegrees * M_PI / 360.0);
    float xmax = ymax * aspectRatio;
    viewBound(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}
#define square(x) ((x) * (x))
void NormalizeVector(float x[3]) {
    float Norm = sqrt(square(x[0]) + square(x[1]) + square(x[2]));
    x[0] /= Norm;
    x[1] /= Norm;
    x[2] /= Norm;
}

void ComputeNormalOfPlane(float result[3], float A[3], float B[3]) {
    result[0] = A[1] * B[2] - A[2] * B[1];
    result[1] = A[2] * B[0] - A[0] * B[2];
    result[2] = A[0] * B[1] - A[1] * B[0];
}

void LookAtMatrix(float matrix[16], float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
    float forward[3] = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
    NormalizeVector(forward);
    float up[3] = {upX, upY, upZ};
    float side[3];
    ComputeNormalOfPlane(side, forward, up);
    NormalizeVector(side);
    ComputeNormalOfPlane(up, side, forward);
    matrix[0] = side[0];
    matrix[4] = side[1];
    matrix[8] = side[2];
    matrix[12] = 0.0f;
    matrix[1] = up[0];
    matrix[5] = up[1];
    matrix[9] = up[2];
    matrix[13] = 0.0f;
    matrix[2] = -forward[0];
    matrix[6] = -forward[1];
    matrix[10] = -forward[2];
    matrix[14] = 0.0f;
    matrix[3] = matrix[7] = matrix[11] = 0.0f;
    matrix[15] = 1.0f;

    // Apply translation to move the world to the eye position
    float translation[16] = {
        1, 0, 0, -eyeX,
        0, 1, 0, -eyeY,
        0, 0, 1, -eyeZ,
        0, 0, 0, 1};
    MatrixMultiply(matrix, translation, matrix);
}
float projectionMatrix[16];
float viewMatrix[16];
float combinedMatrix[16];
void Maze::drawWallFromEdge(Point2D P1, Point2D P2, float color[3], Frustum frustum) {  //
    // In world space the wall is (P1.x,P1.y,-1),(P1.x,P1.y,1), (P2.x,P2.y,1),(P2.x,P2.y,-1), which is a rectangle
    // viewer's z coordinate is always 0
    // Draw in perspective projection matrix
    Point2D points[4] = {{P1.x, P1.y}, {P1.x, P1.y + 1}, {P2.x, P2.y + 1}, {P2.x, P2.y}};
    glColor3fv(color);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 4; ++i) {
        float worldCoord[4] = {points[i].x, points[i].y, i < 2 ? -1.0f : 1.0f, 1.0f};
        float screenCoord[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        // Apply transformation
        for (int j = 0; j < 4; ++j) {
            screenCoord[j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                screenCoord[j] += combinedMatrix[j * 4 + k] * worldCoord[k];
            }
        }
        // Normalize screen coordinates
        if (screenCoord[3] != 0.0f) {
            screenCoord[0] /= screenCoord[3];
            screenCoord[1] /= screenCoord[3];
        }
        // Convert 3D point to 2D and draw
		printf("Screen coordinates: (%f, %f)\n", screenCoord[0], screenCoord[1]);
        glVertex2f(screenCoord[0], screenCoord[1]);
    }
    glEnd();
    /*
    glColor3fv(color);
    glBegin(GL_POLYGON);
            glVertex3f(P1.x, P1.y, -1);
            glVertex3f(P1.x, P1.y, 1);
            glVertex3f(P2.x, P2.y, 1);
            glVertex3f(P2.x, P2.y, -1);
    glEnd();
    */
}
void Maze::setMatrix(Frustum frustum) {
	Point3D viewerPos = {frustum.viewerPos.x, frustum.viewerPos.y, 0};
    float rotationRad = DEGtoRAD(frustum.rotationDeg);
    float verticalFOV = this->viewer_fov / viewAspect;

    // now this is hand written version instead of using gluPerspective and gluLookAt
    PerspectiveMatrix(projectionMatrix, verticalFOV, viewAspect, 0.001f, 1000.0f);
    LookAtMatrix(viewMatrix, viewerPos.x, viewerPos.y, 0.0f, viewerPos.x + cos(rotationRad), viewerPos.y + sin(rotationRad), 0.0f, 0.0f, 0.0f, 1.0f);
    MatrixMultiply(projectionMatrix, viewMatrix, combinedMatrix);

	for(int i = 0; i < 16; i++) printf("%f ", projectionMatrix[i]);
	printf(" proj\n");
	for(int i = 0; i < 16; i++) printf("%f ", viewMatrix[i]);
	printf(" view\n");
	for(int i = 0; i < 16; i++) printf("%f ", combinedMatrix[i]);
	printf(" comb\n");
	/*
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // Reset projection matrix
        gluPerspective(verticalFOV, viewAspect, 0.001f, 1000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(viewerPos.x, viewerPos.y, 0.0f, viewerPos.x + cos(rotationRad), viewerPos.y + sin(rotationRad), 0.0f, 0.0f, 0.0f, 1.0f);
    */
}
int callCnt = 0;
void Maze::Draw_Cell(Cell *C, Frustum F) {
    if (C->counter == frame_num) return;
    // printf("call count: %d\n", callCnt++);
    // printf("frustum info, viewerPos: (%f, %f), rotationDeg: %f, FOV: %f\n",F.viewerPos.x, F.viewerPos.y, F.rotationDeg, F.FOV);
    C->counter = frame_num;  // used to prevent drawing repeatly in the same frame

	setMatrix(F);
    for (int i = 0; i < 4; i++) {
        Edge *E = C->edges[i];
        if (E->opaque) {
            Point2D P1, P2;
            if (clipToFrustum(E, F, P1, P2, 0)) drawWallFromEdge(P1, P2, E->color, F);
        } else {
            Point2D P1, P2;
            if (clipToFrustum(E, F, P1, P2, 0)) {
                Frustum newF(F.viewerPos, P1, P2, F);  // construct new frustum from the viewer's pos, P1, P2
                Draw_Cell(E->Neighbor(C), newF);
            }
        }
    }
}
/*
Whitefox's personal space: my own function is written here: end
*/
//**********************************************************************
//
// * Draws the first-person view of the maze. It is passed the focal distance.
//   THIS IS THE FUINCTION YOU SHOULD MODIFY.
//======================================================================
/*################################################################### TO DO later
        Draw Cell(cell C frustum F){
                for each cell edge E{
                        if E is opaque{
                                E' = clip E to F draw E'
                                draw E'

                        }
                        if E is transparent{
                                E' = clip E to F
                                F' = F restricted to E'
                                Draw_Cell(neighbor(C,E),F')
                        }
                }
        }
        ###################################################################*/
void Maze::
    Draw_View(const float focal_dist)
//======================================================================
{
    frame_num++;
    Frustum viewerFrustum;
    viewerFrustum.viewerPos = {this->viewer_posn[X], this->viewer_posn[Y]};
    viewerFrustum.rotationDeg = this->viewer_dir;
    viewerFrustum.leftRay = {viewerFrustum.viewerPos, DEGtoRAD(this->viewer_dir - this->viewer_fov / 2)};
    viewerFrustum.rightRay = {viewerFrustum.viewerPos, DEGtoRAD(this->viewer_dir + this->viewer_fov / 2)};
    bool isSet = 0;
    callCnt == 0;
    Draw_Cell(this->view_cell, viewerFrustum);
}

//**********************************************************************
//
// * Draws the frustum on the map view of the maze. It is passed the
//   minimum and maximum corners of the window in which to draw.
//======================================================================
void Maze::
    Draw_Frustum(int min_x, int min_y, int max_x, int max_y)
//======================================================================
{
    int height;
    float scale_x, scale_y, scale;
    float view_x, view_y;

    // Draws the view frustum in the map. Sets up all the same viewing
    // parameters as draw().
    scale_x = (max_x - min_x - 10) / (max_xp - min_xp);
    scale_y = (max_y - min_y - 10) / (max_yp - min_yp);
    scale = scale_x > scale_y ? scale_y : scale_x;
    height = (int)ceil(scale * (max_yp - min_yp));

    min_x += 5;
    min_y += 5;

    view_x = (viewer_posn[X] - min_xp) * scale;
    view_y = (viewer_posn[Y] - min_yp) * scale;
    fl_line(min_x + (int)floor(view_x +
                               cos(To_Radians(viewer_dir + viewer_fov / 2.0)) * scale),
            min_y + height -
                (int)floor(view_y +
                           sin(To_Radians(viewer_dir + viewer_fov / 2.0)) *
                               scale),
            min_x + (int)floor(view_x),
            min_y + height - (int)floor(view_y));
    fl_line(min_x + (int)floor(view_x +
                               cos(To_Radians(viewer_dir - viewer_fov / 2.0)) *
                                   scale),
            min_y + height -
                (int)floor(view_y + sin(To_Radians(viewer_dir - viewer_fov / 2.0)) *
                                        scale),
            min_x + (int)floor(view_x),
            min_y + height - (int)floor(view_y));
}

//**********************************************************************
//
// * Draws the viewer's cell and its neighbors in the map view of the maze.
//   It is passed the minimum and maximum corners of the window in which
//   to draw.
//======================================================================
void Maze::
    Draw_Neighbors(int min_x, int min_y, int max_x, int max_y)
//======================================================================
{
    int height;
    float scale_x, scale_y, scale;
    int i, j;

    // Draws the view cell and its neighbors in the map. This works
    // by drawing just the neighbor's edges if there is a neighbor,
    // otherwise drawing the edge. Every edge is shared, so drawing the
    // neighbors' edges also draws the view cell's edges.

    scale_x = (max_x - min_x - 10) / (max_xp - min_xp);
    scale_y = (max_y - min_y - 10) / (max_yp - min_yp);
    scale = scale_x > scale_y ? scale_y : scale_x;
    height = (int)ceil(scale * (max_yp - min_yp));

    min_x += 5;
    min_y += 5;

    for (i = 0; i < 4; i++) {
        Cell *neighbor = view_cell->edges[i]->Neighbor(view_cell);

        if (neighbor) {
            for (j = 0; j < 4; j++) {
                Edge *e = neighbor->edges[j];

                if (e->opaque) {
                    float x1, y1, x2, y2;

                    x1 = e->endpoints[Edge::START]->posn[Vertex::X];
                    y1 = e->endpoints[Edge::START]->posn[Vertex::Y];
                    x2 = e->endpoints[Edge::END]->posn[Vertex::X];
                    y2 = e->endpoints[Edge::END]->posn[Vertex::Y];

                    fl_color((unsigned char)floor(e->color[0] * 255.0),
                             (unsigned char)floor(e->color[1] * 255.0),
                             (unsigned char)floor(e->color[2] * 255.0));
                    fl_line_style(FL_SOLID);
                    fl_line(min_x + (int)floor((x1 - min_xp) * scale),
                            min_y + height - (int)floor((y1 - min_yp) * scale),
                            min_x + (int)floor((x2 - min_xp) * scale),
                            min_y + height - (int)floor((y2 - min_yp) * scale));
                }
            }
        } else {
            Edge *e = view_cell->edges[i];

            if (e->opaque) {
                float x1, y1, x2, y2;

                x1 = e->endpoints[Edge::START]->posn[Vertex::X];
                y1 = e->endpoints[Edge::START]->posn[Vertex::Y];
                x2 = e->endpoints[Edge::END]->posn[Vertex::X];
                y2 = e->endpoints[Edge::END]->posn[Vertex::Y];

                fl_color((unsigned char)floor(e->color[0] * 255.0),
                         (unsigned char)floor(e->color[1] * 255.0),
                         (unsigned char)floor(e->color[2] * 255.0));
                fl_line_style(FL_SOLID);
                fl_line(min_x + (int)floor((x1 - min_xp) * scale),
                        min_y + height - (int)floor((y1 - min_yp) * scale),
                        min_x + (int)floor((x2 - min_xp) * scale),
                        min_y + height - (int)floor((y2 - min_yp) * scale));
            }
        }
    }
}

//**********************************************************************
//
// * Save the maze to a file of the given name.
//======================================================================
bool Maze::
    Save(const char *filename)
//======================================================================
{
    FILE *f = fopen(filename, "w");
    int i;

    // Dump everything to a file of the given name. Returns false if it
    // couldn't open the file. True otherwise.

    if (!f) {
        return false;
    }

    fprintf(f, "%d\n", num_vertices);
    for (i = 0; i < num_vertices; i++)
        fprintf(f, "%g %g\n", vertices[i]->posn[Vertex::X],
                vertices[i]->posn[Vertex::Y]);

    fprintf(f, "%d\n", num_edges);
    for (i = 0; i < num_edges; i++)
        fprintf(f, "%d %d %d %d %d %g %g %g\n",
                edges[i]->endpoints[Edge::START]->index,
                edges[i]->endpoints[Edge::END]->index,
                edges[i]->neighbors[Edge::LEFT] ? edges[i]->neighbors[Edge::LEFT]->index : -1,
                edges[i]->neighbors[Edge::RIGHT] ? edges[i]->neighbors[Edge::RIGHT]->index : -1,
                edges[i]->opaque ? 1 : 0,
                edges[i]->color[0], edges[i]->color[1], edges[i]->color[2]);

    fprintf(f, "%d\n", num_cells);
    for (i = 0; i < num_cells; i++)
        fprintf(f, "%d %d %d %d\n",
                cells[i]->edges[0] ? cells[i]->edges[0]->index : -1,
                cells[i]->edges[1] ? cells[i]->edges[1]->index : -1,
                cells[i]->edges[2] ? cells[i]->edges[2]->index : -1,
                cells[i]->edges[3] ? cells[i]->edges[3]->index : -1);

    fprintf(f, "%g %g %g %g %g\n",
            viewer_posn[X], viewer_posn[Y], viewer_posn[Z],
            viewer_dir, viewer_fov);

    fclose(f);

    return true;
}
