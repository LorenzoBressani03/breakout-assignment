// on Linux compile with:   clang main.c -lm -lSDL2 -lGLU -lGL -o main
// on Windows compile with: clang main.c -l SDL2 -l SDL2main -l Shell32 -l glu32 -l opengl32 -o main.exe -Xlinker /subsystem:console

// This is the main SDL include file
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <windows.h>
#endif
// include OpenGL
#include <GL/gl.h>
#include <GL/glu.h>
// include relevant C standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // this and PI are used to create the ball
#include <stdbool.h> // boolean type
#include <time.h> // for random
#define PI 3.14159265359
#define POWERUPNUMBER 2

/* data structures */

typedef struct paddle
{
	double x;
	double y;
	double width;
	double height;
	double speed;
	int lives; //the 3 lives are assigned to the paddle, which represents the player
} paddle;

typedef struct powerup
{
	double x;
	double y;
	double radius;
	double speed;
	bool destroyed; //was the powerup destroyed by the paddle?
} powerup;

typedef struct ball
{
	double x;
	double y;
	double radius;
	double speedX;
	double speedY;
} ball;

typedef struct block
{
	double x;
	double y;
	double width;
	double height;
	bool destroyed;
	double strength; //number of hits needed to destroy a block
	bool power; 
	powerup powerup;
} block;

void initializePaddle(paddle* p, double x, double y, double w, double h, double sp)
{
	p->x = x;
	p->y = y;
	p->width = w;
	p->height = h;
	p->speed = sp;
	p->lives = 3;
}

powerup initializePowerup(double x, double y)
{
	powerup pow;
	pow.x = x;
	pow.y = y;
	pow.radius = 5;
	pow.speed = -60; //powerup falls towards the bottom
	pow.destroyed = false;
	return pow;
}

void initializeBall(ball* b, double x, double y, double r, double sx, double sy)
{
	b->x = x;
	b->y = y;
	b->radius = r;
	b->speedX = sx;
	b->speedY = sy;
}


void initializeBlock(block* bl, double x, double y, double w, double h,int str, bool power)
{
	bl->x = x;
	bl->y = y;
	bl->width = w;
	bl->height = h;
	bl->destroyed = false;
	bl->strength = str;
	bl->power = power;
	if (power) bl->powerup = initializePowerup(x, y);
}

char powerupXpaddle(powerup* pow, paddle* p) { //collision detection
	return (pow->y <= p->y + (p->height / 2.0)) &&
		(pow->y >= p->y - (p->height / 2.0)) &&
		(pow->x >= p->x - (p->width / 2.0)) &&
		(pow->x <= p->x + (p->width / 2.0));
}

void updatePowerup(powerup* pow, paddle* p, double f) {
	if (powerupXpaddle(pow, p) && !pow-> destroyed) {
		pow->destroyed = true;
		p->lives++; // the powerup falls and if its touched by the paddle it gives a bonus life
	}
	pow->y += pow->speed * f;
}

void updatePaddle(paddle* p, double f, int d, int w)
{
	p->x += p->speed * f * (double)d; /* calculate next position */

	/* ensure the paddle does not go beyond the boundaries */
	if (p->x + (p->width / 2.0) >= (double)(w / 2)) p->x = (double)(w / 2) - (p->width / 2.0);
	if (p->x - (p->width / 2.0) <= (double)(w / -2)) p->x = (double)(w / -2) + (p->width / 2.0);
}

char ballXpaddle(ball* b, paddle* p) /* collision detection */
{ 	/* return if the ball has collided with the paddle */
	return (b->y <= p->y + (p->height / 2.0)) &&
		(b->y >= p->y - (p->height / 2.0)) &&
		(b->x >= p->x - (p->width / 2.0)) &&
		(b->x <= p->x + (p->width / 2.0)); /* ball y matches the paddle */
}

char ballXblock(ball* b, block* bl) /* collision detection */
{   /* return if the ball has collided with the block*/
	return (!bl->destroyed) &&
		(b->y - b->radius <= bl->y + (bl->height / 2.0)) &&
		(b->y + b->radius >= bl->y - (bl->height / 2.0)) &&
		(b->x + b->radius >= bl->x - (bl->width / 2)) &&
		(b->x - b->radius <= bl->x + (bl->width / 2));
}


//source from "Davide Bressani" starts here
void changeSpeed(ball* b, paddle* p) { // the ball changes direction based on how it hits the paddle to make it seem more natural, there are 4 cases
	int width = p->width;
	if (b->speedX > 0) {
		if (b->x >= p->x - width / 2 && b->x < p->x) { // 1st case: if the ball arrives from the left and hits the left side of the paddle it bounces back
			b->speedX *= -1;
		}
	}
	else {
		if (b->x <= p->x + width / 2 && b->x > p->x) { // 2nd case: if the ball arrives from the right and hits the right side of the paddle it bounces back
			b->speedX *= -1;
		}
	}
	b->speedY *= -1.0; //in every case the speedY is negative so it bounces back, in the 3rd and 4th case the ball continues travelling with the same speedX
}
//source from "Davide Bressani" ends here
void changeSpeedBlock(ball* b, block* bl) { // the ball hits a block and bounces back
	if ((b->x < bl->x - bl->width / 2 || b->x > bl->x + bl->width / 2 )) { //the ball hits the side of the block and the speedX changes
		b->speedX *= -1.0;
		if(!(b->y - b->radius < bl->y + bl->height / 2 || b->y + b->radius > bl->y - bl->height / 2)) b->speedY *= -1.0;
	}
	else { //the ball hits the bottom or top of the block and speedY changes
		b->speedY *= -1.0;
	}
}

void updateBall(ball* b, double f, paddle* p1, block* bl1, int numberOfBlocks, int w, int h, bool* hit)
{
	bool restart = false;
	/* collision detection & resolution with scene boundaries */
	if ((b->y - b->radius) <= -1.0 * (double)(h/2)) //if the ball hits the bottom boundary the restart is activated, the player loses a life
	{
		restart = true;
	}
	else if ((b->x - b->radius) <= -1.0 * (double)(w / 2))
	{
		b->x = -1.0 * (double)(w / 2) + b->radius; /* ensure the ball does not go beyond the boundaries */
		b->speedX *= -1.0;
	}
	else if ((b->x + b->radius) >= (double)(w / 2))
	{
		b->x = (double)(w / 2) - b->radius; /* ensure the ball does not go beyond the boundaries */
		b->speedX *= -1.0;
	}
	else if ((b->y + b->radius) >= (double)(h / 2))
	{
		b->y = (double)(h / 2) - b->radius; /* ensure the ball does not go beyond the boundaries */
		b->speedY *= -1.0;
	}

	/* update position */
	if (restart) { //if the ball hits the bottom it goes back to the center
		b->x = 0.0;
		b->y = -30.0;
		b->speedX = 60.0;
		b->speedY = 200.0;
		p1->x = 0; //the paddle also goes back to the center
		p1->lives--; //a life is removed
		restart = false;
	}
	else if (!*hit){
		if (ballXpaddle(b, p1))
		{
			*hit = true;
			changeSpeed(b, p1);
		}

		for (int i = 0; i < numberOfBlocks; i++) {
			if (ballXblock(b, bl1))
			{
				*hit = true;
				bl1->strength--;
				if (bl1->strength == 0) bl1->destroyed = true;
				changeSpeedBlock(b, bl1);
				break;
			}
			bl1++;
		}

		b->x += f * b->speedX;
		b->y += f * b->speedY;
	}
	else {
		if (!ballXpaddle(b, p1)) {
			for (int i = 0; i < numberOfBlocks; i++) {
				if (ballXblock(b, bl1)) { 
					*hit = true;
					break;
				}
				else {
					*hit = false;
				}
				bl1++;
			}
		}
		b->x += f * b->speedX;
		b->y += f * b->speedY;
	}
}

void drawBall(ball* b)
{
	GLint matrixmode = 0;
	int i, angle;
	glGetIntegerv(GL_MATRIX_MODE, &matrixmode); /* get current matrix mode */

	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	glPushMatrix(); /* store current modelview matrix */
	glTranslated(b->x, b->y, 0.0); /* move the ball to its correct position */
	//source based on "Eike Anderson"
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.0, 1.0, 0.3);
	glVertex2f(0.0f, 0.0f); /* centre point */
	for (i = 0; i < 60; i++) /* circle */
	{
		angle = 2 * PI * i / 10; /* compute and draw triangles */
		glVertex2f(b->radius * cos(angle), b->radius * sin(angle));
	}
	glVertex2f(b->radius*cos(0.0), b->radius*sin(0.0)); /* to close loop */
	glEnd();

	glPopMatrix(); /* restore previous modelview matrix */
	glMatrixMode(matrixmode); /* set the previous matrix mode */
}

void drawPaddle(paddle* p)
{
	GLint matrixmode = 0;
	glGetIntegerv(GL_MATRIX_MODE, &matrixmode); /* get current matrix mode */

	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	glPushMatrix(); /* store current modelview matrix */
	glTranslated(p->x, p->y, 0.0); /* move the paddle to its correct position */

	glBegin(GL_QUADS); /* draw paddle */
	glColor3f(0.0, 1.0, 0.3);
	glVertex3d(p->width / -2.0, p->height / 2.0, 0.0);
	glVertex3d(p->width / 2.0, p->height / 2.0, 0.0);
	glVertex3d(p->width / 2.0, p->height / -2.0, 0.0);
	glVertex3d(p->width / -2.0, p->height / -2.0, 0.0);
	glEnd();

	glPopMatrix(); /* restore previous modelview matrix */
	glMatrixMode(matrixmode); /* set the previous matrix mode */
}

void drawBlock(block* bl)
{
	GLfloat colourArray[5][3] = { {0.0,0.7,1.0} , {0.0,0.7,0.0}, {1.0,0.8,0.0}, {0.9,0.4,0.0}, {0.9,0.0,0.0}}; //5 different colours based on the strength
	GLint matrixmode = 0;
	int i = bl->strength-1;
	glGetIntegerv(GL_MATRIX_MODE, &matrixmode); /* get current matrix mode */

	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	glPushMatrix(); /* store current modelview matrix */
	if (!bl->destroyed) {
		glTranslated(bl->x, bl->y, 0.0); /* move the block to its correct position */
		glBegin(GL_QUADS); /* draw block */
		glColor3fv(colourArray[i]);
		glVertex3d(bl->width / -2.0, bl->height / 2.0, 0.0);
		glVertex3d(bl->width / 2.0, bl->height / 2.0, 0.0);
		glVertex3d(bl->width / 2.0, bl->height / -2.0, 0.0);
		glVertex3d(bl->width / -2.0, bl->height / -2.0, 0.0);
		glEnd();
	}
	else if (bl->power && !bl->powerup.destroyed){ //draws the powerups
		glTranslated(bl->powerup.x, bl->powerup.y, 0.0); /* move the block to its correct position */
		//source based on "Eike Anderson"
		glBegin(GL_TRIANGLE_FAN);
		glColor3f(1.0, 1.0, 1.0);
		glVertex2f(0.0f, 0.0f); /* centre point */
		for (int i = 0; i < 60; i++) /* circle with 10 segments */
		{
			int angle = 2 * PI * i / 10; /* compute and draw triangles */
			glVertex2f(5 * cos(angle), 5 * sin(angle));
		}
		glVertex2f(5 * cos(0.0), 5 * sin(0.0)); /* to close loop */
		glEnd();
	}

	glPopMatrix(); /* restore previous modelview matrix */
	glMatrixMode(matrixmode); /* set the previous matrix mode */

}

void drawLives(paddle* p, int w, int h) {
	GLint matrixmode = 0;
	glGetIntegerv(GL_MATRIX_MODE, &matrixmode); /* get current matrix mode */

	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	for (int i = 0; i < p->lives; i++) {
		glPushMatrix(); /* store current modelview matrix */
		glTranslated(w / 2 - 20 -(15*i), h / 2 - 20, 0); // lives are shown on the top right
		glBegin(GL_TRIANGLE_FAN);
		glColor3f(1.0, 1.0, 1.0);
		glVertex2f(0.0f, 0.0f); /* centre point */
		for (int i = 0; i < 60; i++) /* circle with 10 segments */
		{
			int angle = 2 * PI * i / 10; /* compute and draw triangles */
			glVertex2f(5 * cos(angle), 5 * sin(angle));
		}
		glVertex2f(5 * cos(0.0), 5 * sin(0.0)); /* to close loop */
		glEnd();
		glPopMatrix(); /* restore previous modelview matrix */
	}
	glMatrixMode(matrixmode); /* set the previous matrix mode */
}

void render(ball* b, paddle* p1, block* bl1, int winWidth, int winHeight, int numberBlocks)
{
	/* Start by clearing the framebuffer (what was drawn before) */
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	/* Set the scene transformations */
	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	glLoadIdentity(); /* Set it to the identity (no transformations) */

	/* draw the objects */
	drawBall(b);
	drawPaddle(p1);
	for (int i = 0; i < numberBlocks; i++) { //draws different blocks
		drawBlock(&bl1[i]);
	}

	drawLives(p1, winWidth, winHeight);

	glFlush();
}



void renderImage(GLuint t) //source from "Eike Anderson" starts here
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	/* Set the scene transformations */
	glMatrixMode(GL_MODELVIEW); /* set the modelview matrix */
	glLoadIdentity(); /* Set it to the identity (no transformations) */	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, t);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-320, 240);
    glTexCoord2f(1, 0); glVertex2f(320, 240);
    glTexCoord2f(1, 1); glVertex2f(320, -240);
    glTexCoord2f(0, 1); glVertex2f(-320, -240);
    glEnd();

    glDisable(GL_TEXTURE_2D);
	glFlush();
}
//source from "Eike Anderson" ends here
GLuint createTexture(const char* path) //source from "Eike Anderson" starts here
{
	GLuint TextureID = 0;
    SDL_Surface* Surface = SDL_LoadBMP(path);
    int mode;
    if(Surface->format->BytesPerPixel == 4) {
		mode = GL_RGBA;
	}
	else mode=GL_RGB;
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, 640, 480, 0, mode, GL_UNSIGNED_BYTE, Surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return TextureID;
}
//source from "Eike Anderson" ends here
//source from "Davide Bressani" starts here
bool appendNoDuplicates(int index, int* array, int element) { //so that the powerups do not end up in the same block
	for (int i = 0; i < index+1; i++) {
		if (element==array[i])return false;
		if (array[i] == 0) {
			array[i] = element;
			break;
		}
	}
	return true;
}
//source from "Davide Bressani" ends here
void init(ball* myB, paddle* p1, int* powerupCoordArray, int numberPowerups, block* blocksArray, int numberBlocks, int blocksRows, int winWidth, int winHeight, Uint32 timer) {
	srand(time(0));
	for (int i = 0; i < numberPowerups; i++) {
		int coord = rand() % (numberBlocks + 1);
		while (!appendNoDuplicates(i, powerupCoordArray, coord)) {
			coord = rand() % (numberBlocks + 1);
		} // this function initializes the levels after a loss or a win
	}

	for (int r = 0; r < blocksRows; r++) { //the placing and spacing between the blocks
		for (int c = 0; c < numberBlocks/blocksRows; c++) {
			block block1;
			int spacing = 8;

			double blockWidth = (winWidth - (spacing * 9)) / 8;
			double blockHeight = 30;
			double x = (-winWidth / 2) + ((c + 1) * spacing + (c + 0.5) * blockWidth);
			double y = (winHeight / 2) - (32 + (r + 0.5) * blockHeight + r * spacing);

			bool spawn = false;
			for (int i = 0; i < numberPowerups; i++) {
				if (powerupCoordArray[i] == 0) break;
				if ((r * numberBlocks / blocksRows) + c == powerupCoordArray[i]-1) {
					spawn = true; //spawns the powerups
					break;
				}
			}
			initializeBlock(&block1, x, y, blockWidth, blockHeight, r+1, spawn);
			blocksArray[(r * numberBlocks / blocksRows) + c] = block1;
		}
	}
	/* Set up the parts of the scene that will stay the same for every frame. */

	glFrontFace(GL_CCW);     /* Enforce counter clockwise face ordering (to determine front and back side) */
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_FLAT); /* enable flat shading - Gouraud shading would be GL_SMOOTH */


	glEnable(GL_DEPTH_TEST);

	/* Set the clear (background) colour */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	/* Set up the camera/viewing volume (projection matrix) */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1.0 * (GLdouble)(winWidth / 2), (GLdouble)(winWidth / 2), -1.0 * (GLdouble)(winHeight / 2), (GLdouble)(winHeight / 2));

	glViewport(0, 0, winWidth, winHeight);

	/* initialize the timer */
	timer = SDL_GetTicks();

	/* initialize objects */
	initializeBall(myB, 0.0, 0.0, 5.0, 60.0, 200.0);
	initializePaddle(p1, 0.0, -200.0, 40, 6, 150.0);
}

int main(int argc, char* argv[])
{
	int shownScreen = 0; //4 different screens, this is the first one for the levels

    //window parameters
	int winPosX = 100;
	int winPosY = 100;
	int winWidth = 640;
	int winHeight = 480;
	int go;

	Uint32 timer = 0; /* animation timer (in milliseconds) */

	paddle p1;
	ball myB;
	bool hit = false;
	block blocksArray[100];
	int numberBlocks;
	int powerupCoordArray[100] = {0}; //chooses 2 random numbers, those are the blocks for the powerups
	int p1dir = 0;

	/* This is our initialisation phase

	   SDL_Init is the main initialisation function for SDL
	   It takes a 'flag' parameter which we use to tell SDL what systems we are going to use
	   Here, we want to initialise everything, so we give it the flag for this.
	   This function also returns an error value if something goes wrong,
	   so we can put this straight in an 'if' statement to check and exit if need be */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) /* alternative: SDL_INIT_EVERYTHING */
	{
		/* Something went very wrong in the initialisation, all we can do is exit */
		perror("Whoops! Something went very wrong, cannot initialise SDL :( - ");
		perror(SDL_GetError());
		return -1;
	}

	/* Now we have got SDL initialised, we are ready to create an OpenGL window! */
	SDL_Window* window = SDL_CreateWindow("Breakout!!!",  /* The first parameter is the window title */
		winPosX, winPosY,
		winWidth, winHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN); /* ensure that OpenGL gets enabled here */
	/* The last parameter lets us specify a number of options.
	   Here, we tell SDL that we want the window to be shown and that it can be resized.
	   You can learn more about SDL_CreateWindow here: https://wiki.libsdl.org/SDL_CreateWindow?highlight=%28\bCategoryVideo\b%29|%28CategoryEnum%29|%28CategoryStruct%29
	   The flags you can pass in for the last parameter are listed here: https://wiki.libsdl.org/SDL_WindowFlags

	   The SDL_CreateWindow function returns an SDL_Window.
	   This is a structure which contains all the data about our window (size, position, etc).
	   We will also need this when we want to draw things to the window.
	   This is therefore quite important we do not lose it! */

	if (window == NULL)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		exit(EXIT_FAILURE); /* crash out if there has been an error */
	}

	/* Use OpenGL 1.5 compatibility */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); /* set up Z-Buffer */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); /* enable double buffering */


	/* SDL_GLContext is the OpenGL rendering context - this is the equivalent to the SDL_Renderer when drawing pixels to the window */
	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (context == NULL)
	{
		printf("OpenGL Rendering Context could not be created! SDL Error: %s\n", SDL_GetError());
		exit(EXIT_FAILURE); /* crash out if there has been an error */
	}

	/* We are now preparing for our main loop.
	   This loop will keep going round until we exit from our program by changing the int 'go' to the value false (0).
	   This loop is an important concept and forms the basis of most SDL programs you will be writing.
	   Within this loop we generally do the following things:
		 * Check for input from the user (and do something about it!)
		 * Update our graphics (if necessary, e.g. for animation)
		 * Draw our graphics
	*/
	go = 1;

    GLuint texture1=createTexture("breakout_menu/levels.bmp");
	GLuint texture2=createTexture("breakout_menu/lost2.bmp");
    GLuint texture3=createTexture("breakout_menu/won.bmp");
    

	while (go)
	{
		Uint32 old = timer;
		timer = SDL_GetTicks();
		SDL_Event incomingEvent;
		switch (shownScreen) //a switch to change between the different screens
		{
		case (0): { //the levels menu, when you click on the buttons it redirects you to a different level
				while (SDL_PollEvent(&incomingEvent))
				{
					switch (incomingEvent.type)
					{
					case SDL_QUIT: {
						go = 0;
						break;
					}
					case SDL_MOUSEBUTTONDOWN: { //the different levels, every level has a different number of blocks
						if (incomingEvent.button.button == SDL_BUTTON_LEFT) {
							if (incomingEvent.button.y > 162 && incomingEvent.button.y < 235) {
								if (incomingEvent.button.x > 112 && incomingEvent.button.x < 352) {
									shownScreen = 1;
									init(&myB, &p1, powerupCoordArray, 2, blocksArray, 24, 3, winWidth, winHeight, timer);
									numberBlocks = 24;
								}
							}
							else if (incomingEvent.button.y > 258 && incomingEvent.button.y < 330) {
							    if (incomingEvent.button.x > 112 && incomingEvent.button.x < 352) {
									shownScreen = 1;
									init(&myB, &p1, powerupCoordArray, 3, blocksArray, 32, 4, winWidth, winHeight, timer);
									numberBlocks = 32;
								}
							}
							else if (incomingEvent.button.y > 357 && incomingEvent.button.y < 428) {
							    if (incomingEvent.button.x > 112 && incomingEvent.button.x < 352) {
									shownScreen = 1;
									init(&myB, &p1, powerupCoordArray, 4, blocksArray, 40, 5, winWidth, winHeight, timer);
									numberBlocks = 40;

								}
							}
						}
						break;
					}
					break;
					}
				}
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				gluOrtho2D(-1.0 * (GLdouble)(winWidth / 2), (GLdouble)(winWidth / 2), -1.0 * (GLdouble)(winHeight / 2), (GLdouble)(winHeight / 2));
				glViewport(0, 0, winWidth, winHeight);
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				renderImage(texture1);
				SDL_GL_SwapWindow(window);
				break;
			}
		case (1): { //the main game

			/* Here we are going to check for any input events.
			Basically when you press the keyboard or move the mouse, the parameters are stored as something called an 'event' or 'message'.
			SDL has a queue of events. We need to check for each event and then do something about it (called 'event handling').
			The SDL_Event is the data type for the event. */

			double fraction = 0.0;

			/* SDL_PollEvent will check if there is an event in the queue - this is the program's 'message pump'.
			If there is nothing in the queue it will not sit and wait around for an event to come along (there are functions which do this,
			and that can be useful too!). Instead for an empty queue it will simply return 'false' (0).
			If there is an event, the function will return 'true' (!=0) and it will fill the 'incomingEvent' we have given it as a parameter with the event data */
			while (SDL_PollEvent(&incomingEvent))
			{
				/* If we get in here, we have an event and need to figure out what to do with it.
				For now, we will just use a switch based on the event's type */
				switch (incomingEvent.type)
				{
				case SDL_QUIT:
					/* The event type is SDL_QUIT.
					This means we have been asked to quit - probably the user clicked on the 'x' at the top right corner of the window.
					To quit we need to set our 'go' variable to false (0) so that we can escape out of the main loop. */
					go = 0;
					break;
				case SDL_KEYDOWN:
					switch (incomingEvent.key.keysym.sym)
					{
					case SDLK_RIGHT:
						p1dir = 1;
						break;
					case SDLK_LEFT:
						p1dir = -1;
						break;
					case SDLK_d:
						p1dir = 1;
						break;
					case SDLK_a:
						p1dir = -1;
						break;
					}
					break;
				case SDL_KEYUP:
					switch (incomingEvent.key.keysym.sym)
					{
					case SDLK_RIGHT:
					case SDLK_LEFT:
						p1dir = 0;
						break;
					case SDLK_d:
					case SDLK_a:
						p1dir = 0;
						break;
					case SDLK_ESCAPE: go = 0;
						break;
				}
				break;

					/* If you want to learn more about event handling and different SDL event types, see:
					  https://wiki.libsdl.org/SDL_Event
					  and also: https://wiki.libsdl.org/SDL_EventType */
				}
			}

			/* update timer */
			fraction = (double)(timer - old) / 1000.0; /* calculate the frametime by finding the difference in ms from the last update/frame and divide by 1000 to get to the fraction of a second */
			

			int blocksDestroyed = 0;
			for (int i = 0; i < numberBlocks; i++) {
				if (blocksArray[i].destroyed) {
					blocksDestroyed++;

					if (blocksArray[i].power) updatePowerup(&(blocksArray[i].powerup), &p1, fraction);
				}
			}
			if (blocksDestroyed == numberBlocks) { //if all the blocks are destroyed the win screen is shown
				shownScreen = 3;
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			}
			/* update positions */
			updatePaddle(&p1, fraction, p1dir, winWidth); /* move paddle */

			updateBall(&myB, fraction, &p1, blocksArray, numberBlocks, winWidth, winHeight, &hit); /* move ball and check collisions with the paddles */
			if (p1.lives == 0) { //if the player finishes his lives the lose screen is shown
				shownScreen = 2;
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			}
			else {
				/* Render our scene. */
				render(&myB, &p1, blocksArray, winWidth, winHeight, numberBlocks);

				/* This does the double-buffering page-flip, drawing the scene onto the screen. */
				SDL_GL_SwapWindow(window);
			}
			break;
			}
		case(2): //the lose screen
			shownScreen = 0;
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			gluOrtho2D(-1.0 * (GLdouble)(winWidth / 2), (GLdouble)(winWidth / 2), -1.0 * (GLdouble)(winHeight / 2), (GLdouble)(winHeight / 2));
			glViewport(0, 0, winWidth, winHeight);
			renderImage(texture2);
			SDL_GL_SwapWindow(window);
			
			SDL_Delay(3000);
			break;
		case(3): //the win screen
			shownScreen = 0;
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(-1.0 * (GLdouble)(winWidth / 2), (GLdouble)(winWidth / 2), -1.0 * (GLdouble)(winHeight / 2), (GLdouble)(winHeight / 2));
			glViewport(0, 0, winWidth, winHeight);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			renderImage(texture3);
			SDL_GL_SwapWindow(window);
			SDL_Delay(3000);
			break;
		break;
		}
	}

	/* If we get outside the main loop, it means our user has requested we exit. */

	/* Our cleanup phase, hopefully fairly self-explanatory ;) */
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

