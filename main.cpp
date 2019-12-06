#include <SDL.h>
//#include <SDL_opengl.h>
#include <GL/GL.h>
#include <GL/GLU.h>
#include <stdio.h>
#include <string>

#include "main.h"
#include "wavefront.h"
#include "font.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//Starts up SDL, creates window, and initializes OpenGL
bool init();

//Initializes matrices and clear color
bool initGL();

//Input handler
void handleKey( unsigned char key, int x, int y );

//Per frame update
void update(int elapsed);

//Renders quad to the screen
void render();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

//Render flag
bool gRenderQuad = true;

WavefrontModel *trenchModel;
WavefrontModel *tumtumModel;

class Camera camera;

Camera::Camera() {
	from.x = -200;
	from.y = 50;
	from.x = 25;
	to.x = 0;
	to.y = 0;
	to.z = 0;
	up.x = 0;
	up.y = 1;
	up.z = 0;
	width = 640;
	height = 480;
	hudDepth = 0;
	FILE *file = fopen("screensize.txt", "r");
	if(file) {
		if(fscanf(file, "%d %d", &width, &height)) {
		}
		fclose(file);
		if(width < 640) width = 640;
		if(height < 480) height = 480;
	}
}

ScePspFMatrix4 view;

void Camera::reposition()
{
	glLoadIdentity();
	gluLookAt(from.x, from.y, from.z, to.x, to.y, to.z, up.x, up.y, up.z);
 glGetFloatv(GL_MODELVIEW_MATRIX, (float *)&view);
}

void Camera::hudBegin()
{
	hudDepth++;
	if(hudDepth == 1) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, width, 0, height);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(0, (float)height, 0);
		glScalef(1, -1, 1);
		glDisable(GL_DEPTH_TEST);
	}
}

void Camera::hudEnd()
{
	if(hudDepth < 1) {
		printf("Assert failed: hud depth already ended.\n");
		return;
	}
	if(hudDepth == 1) {
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);	// just to be a good neighbour.
	}
	hudDepth--;
}


bool init()
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO|SDL_INIT_JOYSTICK	) < 0 )
    {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Use OpenGL 2.1
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );

        //Create window
        gWindow = SDL_CreateWindow( "Vast Space War", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        }
		else
        {
            //Create context
            gContext = SDL_GL_CreateContext( gWindow );
            if( gContext == NULL )
            {
                printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            }
            else
            {
                //Use Vsync
                if( SDL_GL_SetSwapInterval( 1 ) < 0 )
                {
                    printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
                }

                //Initialize OpenGL
                if( !initGL() )
                {
                    printf( "Unable to initialize OpenGL!\n" );
                    success = false;
                }
            }
        }
    }

    return success;
}

bool initGL()
{
    bool success = true;
    GLenum error = GL_NO_ERROR;

    //Initialize Projection Matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR )
    {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }

    //Initialize Modelview Matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR )
    {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }
	//Initialize clear color
    glClearColor( 0.f, 0.f, 0.f, 1.f );

    //Check for error
    error = glGetError();
    if( error != GL_NO_ERROR )
    {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        success = false;
    }

    return success;
}

void handleKey( unsigned char key, int x, int y )
{
    //Toggle quad
    if( key == 'q' )
    {
        gRenderQuad = !gRenderQuad;
    }
}

void update(int elapsed)
{
    //No per frame update needed
}

int oldTime;
int oldElapsed;
float oldFps;

void render()
{
    //Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera.reposition();

    drawWavefront(trenchModel);
    drawWavefront(tumtumModel);


    int newTime = SDL_GetTicks();
    int elapsed = newTime - oldTime;
    oldTime = newTime;
    if(elapsed < 1) elapsed = 1;
    if(elapsed > 1000) elapsed = 1000 / 60;
    float fps = 1.0f / (elapsed / 1000.0f); // instantanious frame rate

    camera.hudBegin();
    char buf[32];
    sprintf(buf, "FPS: %.3f (%d ms)", fps, elapsed);
    if(oldElapsed < elapsed - 1 || oldElapsed > elapsed + 1) {
        oldElapsed = elapsed;
        oldFps = fps;
    } else {
        // smoothed a bit.
        sprintf(buf, "FPS: %.3f (%d ms)", oldFps, oldElapsed);
    }
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    SDL_Rect position = {camera.width - 160, camera.height - 80, 0, 0};
    SDL_Color color = {255, 255, 255};
    for(int y=0;y<=camera.height;y+=18) {
        position.y=y;
        Font.drawMessage(buf, FONT_BODY, color, &position);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    camera.hudEnd();

}

int main(int argc,char **argv)
{
	if (!init()) return 10;
	if (!initGL()) return 20;
	atexit(SDL_Quit);

	printf("%d joysticks detected\n",SDL_NumJoysticks());
	SDL_Joystick *joy=0;
	for(int i=0;i<SDL_NumJoysticks();i++) {
		joy=SDL_JoystickOpen(i);
		printf("Active joystick %d: ",i);
		printf("Button Count: %d, ",SDL_JoystickNumButtons(joy));
		printf("Axis Count: %d, ",SDL_JoystickNumAxes(joy));
		printf("Hat Count: %d\n",SDL_JoystickNumHats(joy));
	}

	camera.width=640;
	camera.height=480;

	initImage();

	trenchModel = loadWavefront("utrench");
	tumtumModel = loadWavefront("tumtum");

    //SDL_Surface *icon = SDL_LoadBMP("data/icon.bmp");
    //if(icon) SDL_WM_SetIcon(icon, 0);

    glViewport(0, 0, camera.width, camera.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClearColor(0.7f, 0.9f, 1.0f, 0.0f);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ar = (float)camera.width / camera.height;
    gluPerspective(45, ar, 2.0, 4000.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    const GLfloat light_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);


	int done=0;
	while(!done) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {

			case SDL_QUIT:
				done=1;
			case SDL_KEYUP:
				if( event.key.keysym.sym==27) done=1;
				else if( event.key.keysym.sym=='q') {
					int x=0,y=0;
					SDL_GetMouseState(&x,&y);
					handleKey( 'q', x,y);
				}
				break;
			case SDL_JOYBUTTONDOWN:
				printf("Stick %d button %d down!\n",event.jbutton.which,event.jbutton.button);
				break;
			case SDL_JOYBUTTONUP:
				printf("Stick %d button %d up!\n",event.jbutton.which,event.jbutton.button);
				break;
			case SDL_JOYAXISMOTION:
				printf("Stick %d Axis %d Motion %d\n",event.jaxis.which,event.
	jaxis.axis,event.jaxis.value);

                if(event.jaxis.axis==0) camera.from.x=event.jaxis.value/100.0f;
                if(event.jaxis.axis==1) camera.from.z=event.jaxis.value/100.0f;
                if(event.jaxis.axis==3) camera.from.y=event.jaxis.value/100.0f;
				break;
			case SDL_JOYHATMOTION:
				printf("Stick %d hat %d Motion %d\n",event.jhat.which,event.jhat.hat,event.jhat.value);
				break;
			default:
				break;

			}

		}
		render();
		SDL_GL_SwapWindow( gWindow);
		update(100);
		SDL_Delay(100);

	}

	return 0;
}
