//
//  gl_frontEnd.cpp
//
//  Created by Jean-Yves Hervé on 2023-03-24.
//

#include <cstring>
#include <cstdlib>
#include <cstdio>
//
#include "gl_frontEnd.h"


//---------------------------------------------------------------------------
//  Private functions' prototypes
//---------------------------------------------------------------------------

void myResize(int w, int h);
void displayTextualInfo(const char* infoStr, int x, int y, int isLarge);
void myMouse(int b, int s, int x, int y);
void myGridPaneMouse(int b, int s, int x, int y);
void myStatePaneMouse(int b, int s, int x, int y);
void myKeyboard(unsigned char c, int x, int y);
void myMenuHandler(int value);
void mySubmenuHandler(int colorIndex);
void myTimerFunc(int val);
//
//	implemented in main.cpp
void cleanupAndQuit(void);
void faster(void);
void slower(void);

//---------------------------------------------------------------------------
//  Defined in main.c --> don't touch
//---------------------------------------------------------------------------

extern const int MAX_NUM_THREADS;
extern unsigned int rule;
extern unsigned int colorMode;

//---------------------------------------------------------------------------
//  Interface constants
//---------------------------------------------------------------------------

//	I like to setup my meny item indices as enumerated values, but really
//	regular int constants would do the job just fine.

enum MenuItemID {	SEPARATOR = -1,
					//
					QUIT_MENU = 0,
					OTHER_MENU_ITEM
};

const char* MAIN_MENU_ITEM_STR[] = {	"Quit",			//	QUIT_MENU
										"Something"		//	OTHER_MENU_ITEM
};

enum FirstSubmenuItemID {	FIRST_SUBMENU_ITEM = 11,
							SECOND_SUBMENU_ITEM
};

#define SMALL_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_12
#define LARGE_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_18
// const int SMALL_FONT_HEIGHT = 12;
// const int LARGE_FONT_HEIGHT = 18;
const int TEXT_PADDING = 0;
const float kTextColor[4] = {1.f, 1.f, 1.f, 1.f};

//	Predefine some colors for "age"-based rendering of the cells
GLfloat cellColor[NB_COLORS][4] = {	{0.f, 0.f, 0.f, 1.f},	//	BLACK_COL
									{1.f, 1.f, 1.f, 1.f},	//	WHITE_COL,
									{0.f, 0.f, 1.f, 1.f},	//	BLUE_COL,
									{0.f, 1.f, 0.f, 1.f},	//	GREEN_COL,
									{1.f, 1.f, 0.f, 1.f},	//	YELLOW_COL,
									{1.f, 0.f, 0.f, 1.f}};	//	RED_COL
	

//	Initial position of the window
const int   INIT_WIN_X = 100,
            INIT_WIN_Y = 40;

//	Wow!  gcc on Linux is really dumb!  The code below doesn't even compile there.
//const int	GRID_PANE_WIDTH = 600,
//			GRID_PANE_HEIGHT = GRID_PANE_WIDTH,	//	--> claims GRID_PANE_WIDTH not constant!
//			STATE_PANE_WIDTH = 300,
//			STATE_PANE_HEIGHT = GRID_PANE_HEIGHT,
//			H_PADDING = 5,
//			WINDOW_WIDTH = GRID_PANE_WIDTH + STATE_PANE_WIDTH + H_PADDING,
//			WINDOW_HEIGHT = GRID_PANE_HEIGHT;
//	(sigh!)	This completely negates the point of using constants for this kind of setup.
//	No wonder most Linux apps suck so hard.
const int GRID_PANE_WIDTH = 800;
const int GRID_PANE_HEIGHT = 700;
const int STATE_PANE_WIDTH = 300;
const int STATE_PANE_HEIGHT = 700;
const int H_PADDING = 0;
const int WINDOW_WIDTH = 1100;
const int WINDOW_HEIGHT = 700;


//---------------------------------------------------------------------------
//  File-level global variables
//---------------------------------------------------------------------------

void (*gridDisplayFunc)(void);
void (*stateDisplayFunc)(void);

//	We use a window split into two panes/subwindows.  The subwindows
//	will be accessed by an index.
int	GRID_PANE = 0,
	STATE_PANE = 1;
int	gMainWindow,
	gSubwindow[2];

int drawGridLines = 0;

//---------------------------------------------------------------------------
//	Drawing functions
//---------------------------------------------------------------------------


//	This is the function that does the actual grid drawing
void drawGrid(unsigned int** grid, unsigned int numRows, unsigned int numCols)
{
	const float	DH = (1.f * GRID_PANE_WIDTH) / numCols,
				DV = (1.f * GRID_PANE_HEIGHT) / numRows;
	
	//	Display the grid as a series of quad strips
	for (unsigned int i=0; i<numRows; i++)
	{
		glBegin(GL_QUAD_STRIP);
			for (unsigned int j=0; j<numCols; j++)
			{
				
				glColor4fv(cellColor[grid[i][j]]);

				glVertex2f(j*DH, i*DV);
				glVertex2f(j*DH, (i+1)*DV);
				glVertex2f((j+1)*DH, i*DV);
				glVertex2f((j+1)*DH, (i+1)*DV);
			}
		glEnd();
	}

	if (drawGridLines)
	{
		//	Then draw a grid of lines on top of the squares
		glColor4f(0.5f, 0.5f, 0.5f, 1.f);
		glBegin(GL_LINES);
			//	Horizontal
			for (unsigned int i=0; i<= numRows; i++)
			{
				glVertex2f(0, i*DV);
				glVertex2f(GRID_PANE_WIDTH, i*DV);
			}
			//	Vertical
			for (unsigned int j=0; j<= numCols; j++)
			{
				glVertex2f(j*DH, 0);
				glVertex2f(j*DH, GRID_PANE_HEIGHT);
			}
		glEnd();
	}
}



void displayTextualInfo(const char* infoStr, int xPos, int yPos, int isLarge)
{
    //-----------------------------------------------
    //  0.  get current material properties
    //-----------------------------------------------
    float oldAmb[4], oldDif[4], oldSpec[4], oldShiny;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
    glGetMaterialfv(GL_FRONT, GL_SHININESS, &oldShiny);

    glPushMatrix();

    //-----------------------------------------------
    //  1.  Build the string to display <-- parameter
    //-----------------------------------------------
    unsigned int infoLn = (unsigned int) strlen(infoStr);

    //-----------------------------------------------
    //  2.  Determine the string's length (in pixels)
    //-----------------------------------------------
    int textWidth = 0;
    for (unsigned int k=0; k<infoLn; k++)
	{
		if (isLarge)
			textWidth += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		else
			textWidth += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		
    }
	//  add a few pixels of padding
    textWidth += 2*TEXT_PADDING;
    (void) textWidth;
	
    //-----------------------------------------------
    //  4.  Draw the string
    //-----------------------------------------------    
    glColor4fv(kTextColor);
    int x = xPos;
    for (unsigned int k=0; k<infoLn; k++)
    {
        glRasterPos2i(x, yPos);
		if (isLarge)
		{
			glutBitmapCharacter(LARGE_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		}
		else
		{
			glutBitmapCharacter(SMALL_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		}
	}

    //-----------------------------------------------
    //  5.  Restore old material properties
    //-----------------------------------------------
	glMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, oldShiny);  
    
    //-----------------------------------------------
    //  6.  Restore reference frame
    //-----------------------------------------------
    glPopMatrix();
}



void drawState(unsigned int numLiveThreads)
{
	const int H_PAD = STATE_PANE_WIDTH / 16;
	const int TOP_LEVEL_TXT_Y = 4*STATE_PANE_HEIGHT / 5;

	//	Build, then display text info for the red, green, and blue tanks
	char infoStr[256];
	//	display info about number of live threads
	sprintf(infoStr, "Live Threads: %d", numLiveThreads);
	displayTextualInfo(infoStr, H_PAD, TOP_LEVEL_TXT_Y, 1);
}


//	This callback function is called when the window is resized
//	(generally by the user of the application).
//	It is also called when the window is created, why I placed there the
//	code to set up the virtual camera for this application.
//
void myResize(int w, int h)
{
	if ((w != WINDOW_WIDTH) || (h != WINDOW_HEIGHT))
	{
		glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	}
	else
	{
		glutPostRedisplay();
	}
}


void myDisplay(void)
{
    glutSetWindow(gMainWindow);

    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

	gridDisplayFunc();
	stateDisplayFunc();
	
    glutSetWindow(gMainWindow);	
}

//	This function is called when a mouse event occurs just in the tiny
//	space between the two subwindows.
//
void myMouse(int button, int state, int x, int y)
{
	(void) button;
	(void) state;
	(void) x;
	(void) y;
	
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the grid pane
//
void myGridPaneMouse(int button, int state, int x, int y)
{
	(void) state;
	(void) x;
	(void) y;
	
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


//	This callback function is called when a keyboard event occurs
//
void myKeyboard(unsigned char c, int x, int y)
{
	(void) x;
	(void) y;
	
	bool ok = true;
	
	switch (c)
	{
		//	'ESC' --> exit the application
		case 27:
			cleanupAndQuit();
			break;

		//	spacebar --> resets the grid
		case ' ':
			resetGrid();
			break;

		//	'+' --> increase simulation speed
		case '+':
			faster();
			break;

		//	'-' --> reduce simulation speed
		case '-':
			slower();
			break;

		//	'1' --> apply Rule 1 (Game of Life: B23/S3)
		case '1':
			rule = GAME_OF_LIFE_RULE;
			break;

		//	'2' --> apply Rule 2 (Coral: B3_S45678)
		case '2':
			rule = CORAL_GROWTH_RULE;
			break;

		//	'3' --> apply Rule 3 (Amoeba: B357/S1358)
		case '3':
			rule = AMOEBA_RULE;
			break;

		//	'4' --> apply Rule 4 (Maze: B3/S12345)
		case '4':
			rule = MAZE_RULE;
			break;

		//	'c' --> toggles on/off color mode
		//	'b' --> toggles off/on color mode
		case 'c':
		case 'b':
			colorMode = !colorMode;
			break;

		//	'l' --> toggles on/off grid line rendering
		case 'l':
			drawGridLines = !drawGridLines;
			break;
		default:
			ok = false;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
	
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


//	This function is called when a mouse event occurs in the state pane
void myStatePaneMouse(int button, int state, int x, int y)
{
	(void) state;
	(void) x;
	(void) y;
	
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


void myTimerFunc(int value)
{
	//	value not used.  Warning suppression
	(void) value;

	//	Re-prime the timer
	glutTimerFunc(10, myTimerFunc, 0);

	//  possibly I do something to update the state information displayed
    //	in the "state" pane
	
	//	This call must **DEFINITELY** go away.  After you have properly multithreaded
	//	the code, the processing threads will run without having to be called within
	//	the rendering loop (of the main thread)
 	//  generationVII();
	
	//	And finally I perform the rendering
	//	This is not the way it should be done, but it seems that Apple is
	//	not happy with having marked glut as deprecated.  They are doing
	//	things to make it break
    //glutPostRedisplay();
    myDisplay();
}

void myMenuHandler(int choice)
{
	switch (choice)
	{
		//	Exit/Quit
		case QUIT_MENU:
			exit(0);
			break;
		
		//	Do something
		case OTHER_MENU_ITEM:
			break;
		
		default:	//	this should not happen
			break;
	
	}

    glutPostRedisplay();
}


void initializeFrontEnd(int argc, char** argv, void (*gridDisplayCB)(void),
						void (*stateDisplayCB)(void))
{

	//	Initialize glut and create a new window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);


	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(INIT_WIN_X, INIT_WIN_Y);
	gMainWindow = glutCreateWindow("Programming Assignment 04 -- Cellular Automaton -- CSC 412 - Spring 2018");
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	
	//	set up the callbacks for the main window
	glutDisplayFunc(myDisplay);
	glutReshapeFunc(myResize);
	glutMouseFunc(myMouse);
	glutTimerFunc(20, myTimerFunc, 0);

	gridDisplayFunc = gridDisplayCB;
	stateDisplayFunc = stateDisplayCB;
	
	//	create the two panes as glut subwindows
	gSubwindow[GRID_PANE] = glutCreateSubWindow(gMainWindow,
												0, 0,							//	origin
												GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glViewport(0, 0, GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, GRID_PANE_WIDTH, 0.0f, GRID_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(gridDisplayCB);
	
	
	glutSetWindow(gMainWindow);
	gSubwindow[STATE_PANE] = glutCreateSubWindow(gMainWindow,
												GRID_PANE_WIDTH + H_PADDING, 0,	//	origin
												STATE_PANE_WIDTH, STATE_PANE_HEIGHT);
    glViewport(0, 0, STATE_PANE_WIDTH, STATE_PANE_WIDTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, STATE_PANE_WIDTH, 0.0f, STATE_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(stateDisplayCB);
}
