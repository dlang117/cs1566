/**************************************************************************
File: glmain.c
Does: basic lighting and modeling for cs1566 hw4 Modeler
Author: Steven Lauck, based on some hwa
Date: 01/08/09
**************************************************************************/
#include <stdlib.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "glmain.h"

#define my_assert(X,Y) ((X)?(void) 0:(printf("error:%s in %s at %d", Y, __FILE__, __LINE__), myabort()))

#define min(a,b) ((a) < (b)? a:b)
#define FALSE 0 
#define TRUE  1
#define MAX_LIGHTS  8
#define NUM_OBJECTS 8

using namespace std;

string spec_filename;

typedef struct _Object {
  int sid;

  // example object storage arrays for vertex and normals
  GLfloat vertices_cube_smart[8][4];
  GLfloat normals_cube_smart[8][3];
 
  GLfloat shine;
  GLfloat emi[4];
  GLfloat amb[4];
  GLfloat diff[4];
  GLfloat spec[4];

  GLfloat translate[4];
  GLfloat scale[4];
  GLfloat rotate[4];

}OBJECT;

typedef struct _CAM{
  GLfloat pos[4];
  GLfloat at[4];
  GLfloat up[4];

  GLfloat dir[4];
}CAM;

typedef struct _LITE{
  GLfloat amb[4];
  GLfloat diff[4];
  GLfloat spec[4];
  GLfloat pos[4];
  GLfloat dir[3];
  GLfloat angle;
}LITE;

GLfloat colors [][3] = {
  {0.0, 0.0, 0.0},  /* black   */
  {1.0, 0.0, 0.0},  /* red     */
  {1.0, 1.0, 0.0},  /* yellow  */
  {1.0, 0.0, 1.0},  /* magenta */
  {0.0, 1.0, 0.0},  /* green   */
  {0.0, 1.0, 1.0},  /* cyan    */
  {0.0, 0.0, 1.0},  /* blue    */
  {0.5, 0.5, 0.5},  /* 50%grey */
  {1.0, 1.0, 1.0}   /* white   */
};

GLfloat vertices_axes[][4] = {
	{0.0, 0.0, 0.0, 1.0},  /* origin */ 
	{5.0, 0.0, 0.0, 1.0},  /* maxx */ 
	{0.0, 5.0, 0.0, 1.0}, /* maxy */ 
	{0.0, 0.0, 5.0, 1.0}  /* maxz */ 

};

void make_cube_smart(OBJECT *po, double size );

void real_translation(OBJECT *po, GLfloat x, GLfloat y, GLfloat z);
void real_scaling(OBJECT *po, GLfloat sx, GLfloat sy, GLfloat sz);
void real_rotation(OBJECT *po, GLfloat deg, GLfloat x, GLfloat y, GLfloat z);

OBJECT my_objects[NUM_OBJECTS];
LITE my_lights[MAX_LIGHTS];
int num_objects;
int  num_lights;

myObject my_scene_objects[NUM_OBJECTS];
myObject* cone;
myCamera* my_camera;
// camera variables
CAM my_cam;
GLfloat camx, camy, camz;
GLfloat atx, aty, atz;
GLfloat upx, upy, upz;

vector<myObject> int_objects;

//TRUE or FALSE
int firstPersonView;

int crt_render_mode;
int crt_shape, crt_rs, crt_vs;
int crt_transform;

int curr_mode = NO_MODE;

bool move_cone_pos_x = false;
bool move_cone_neg_x = false;
bool move_cone_pos_y = false;
bool move_cone_neg_y = false;
bool move_cone_pos_z = false;
bool move_cone_neg_z = false;
bool turn_cone_left = false;
bool turn_cone_right = false;


bool move_cam_forward = false;
bool move_cam_back = false;
bool move_cam_left = false;
bool move_cam_right = false;

float ray_length = 1000;
float ray_start[4] = {0,0,0,1};
float ray_dir[4] = {0,0,0,1};
bool show_ray = false;

int window_w = 700;
int window_h = 700;

bool show_normals = false;
bool lock_pointer = false;

int curr_mouse_x = 0;
int curr_mouse_y = 0;

float window_center_y = float(window_h)/2.0;
float window_center_x = float(window_w)/2.0;

void myabort(void)
{
  abort();
  exit(1); /* exit so g++ knows we don't return. */
} 

int main(int argc, char** argv)
{ 
  setbuf(stdout, NULL);   /* for writing to stdout asap */
  glutInit(&argc, argv);

  my_setup(argc, argv);  
  glut_setup();
  gl_setup();

  glutMainLoop();
  return(0);
}


void glut_setup (){

  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  
  glutInitWindowSize(window_w, window_h);
  glutInitWindowPosition(400,20);
  glutCreateWindow("CS1566 Project 5");

  /* set up callback functions */
  glutDisplayFunc(my_display);
  glutReshapeFunc(my_reshape);
  glutPassiveMotionFunc(my_mouse_move);
  glutMouseFunc(my_mouse);
  glutMotionFunc(my_mouse_drag);
  glutKeyboardFunc(my_keyboard);
  glutKeyboardUpFunc( my_keyboard_up );	
  glutIdleFunc( my_idle );	
  glutTimerFunc( 1.0, my_TimeOut, 0 );/* schedule a my_TimeOut call with the ID 0 in 20 seconds from now */

  return;
}

void gl_setup(void)
{

  // enable depth handling (z-buffer)
  glEnable(GL_DEPTH_TEST);

  // enable auto normalize
  glEnable(GL_NORMALIZE);

  // define the background color 
  glClearColor(0,0,0,1);

  glMatrixMode(GL_PROJECTION) ;
  glLoadIdentity() ;
  //glOrtho(stuff);
  gluPerspective( 40, 1.0, 1, 200.0);
  glMatrixMode(GL_MODELVIEW) ;
  glLoadIdentity() ;  // init modelview to identity

  // toggle to smooth shading (instead of flat)
  glShadeModel(GL_SMOOTH); 
  lighting_setup();

  return ;
}

void my_setup(int argc, char **argv){

  firstPersonView = 1;
  num_objects = num_lights = 0;

  // initialize global shape defaults and mode for drawing
  crt_render_mode = GL_POLYGON; // GL_LINE_LOOP;
  crt_shape = 0;

  crt_rs = 40;
  crt_vs = 40;
 
  //If you want to allow the user to type in the spec file
  //then modify the following code.
  //Otherwise, the program will attempt to load the file as specified
  //on the command line:
  //EX: ./glmain spec3

  cone = new myObject(CONE);
  cone->TranslateObjectTo(-5, 0, 0);

  // my_assert(argc >1, "need to supply a spec file");
  // read_spec(argv[1]);
  cout << "Please enter the path to the spec file: " << flush;
  string infilename;  
  while (true)
    {
        ifstream infile;
        getline( cin, infilename );
        infile.open( infilename.c_str() );
        if (infile.good())
        {
            break;        
        }
        cout << "Invalid file. Please enter a valid input file name> " << flush;
    }

  read_spec((char *) infilename.c_str());
  return;
}

void parse_floats(char *buffer, GLfloat nums[]) {
  int i;
  char *ps;

  ps = strtok(buffer, " ");
  for (i=0; ps; i++) {
    nums[i] = atof(ps);
    ps = strtok(NULL, " ");
    //printf("read %f ",nums[i]);
  }

}

void parse_obj(char *buffer){
  OBJECT *po;
  char *pshape, *pshine, *pemi, *pamb, *pdiff, *pspec, *ptranslate, *pscale, *protate;


  my_assert ((num_objects < NUM_OBJECTS), "too many objects");
  po = &my_objects[num_objects++];

  pshape  = strtok(buffer, " ");
  //printf("pshape is %s\n",pshape);

  ptranslate    = strtok(NULL, "()");  strtok(NULL, "()");
  pscale        = strtok(NULL, "()");  strtok(NULL, "()"); 
  protate       = strtok(NULL, "()");  strtok(NULL, "()");  

  pshine  = strtok(NULL, "()");strtok(NULL, "()");
  //printf("pshine is %s\n",pshine);
 
  pemi    = strtok(NULL, "()");  strtok(NULL, "()"); 
  pamb    = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdiff   = strtok(NULL, "()");  strtok(NULL, "()"); 
  pspec   = strtok(NULL, "()");  strtok(NULL, "()"); 


  po->sid  = atoi(pshape);
  po->shine = atof(pshine);

  parse_floats(ptranslate, po->translate);
  parse_floats(pscale, po->scale);
  parse_floats(protate, po->rotate);

  parse_floats(pemi, po->emi);
  parse_floats(pamb, po->amb);
  parse_floats(pdiff, po->diff);
  parse_floats(pspec, po->spec);

  //printf("po->sid = %d\n",po->sid);
  myObject * obj1 = new myObject(po->sid);
  
  obj1->ScaleObject(po->scale[0], po->scale[1], po->scale[2]);
  obj1->RotateObject(po->rotate[0], 1, 0, 0);
  obj1->RotateObject(po->rotate[1], 0, 1, 0);
  obj1->RotateObject(po->rotate[2], 0, 0, 1);
  obj1->TranslateObject(po->translate[0], po->translate[1], po->translate[2]);

  my_scene_objects[num_objects-1] = *obj1;

  delete obj1;

  printf("read object\n");
}

void parse_camera(char *buffer){
  CAM *pc;
  char *ppos, *plook, *pup;

  pc = &my_cam;

  strtok(buffer, "()");
  ppos  = strtok(NULL, "()");  strtok(NULL, "()"); 
  plook  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pup  = strtok(NULL, "()");  strtok(NULL, "()"); 

  parse_floats(ppos, pc->pos);
  parse_floats(plook, pc->at);
  parse_floats(pup, pc->up);

  pc->at[0] += pc->pos[0];
  pc->at[1] += pc->pos[1];
  pc->at[2] += pc->pos[2];

  pc->dir[0] = pc->at[0] - pc->pos[0];
  pc->dir[1] = pc->at[1] - pc->pos[1];
  pc->dir[2] = pc->at[2] - pc->pos[2];
  normalize(pc->dir);

  my_camera = new myCamera(pc->pos[0], pc->pos[1], pc->pos[2],
            pc->dir[0], pc->dir[1], pc->dir[2], pc->up[0], pc->up[1], pc->up[2], PERSP);

  printf("read camera\n");
}

void parse_light(char *buffer){
  LITE *pl;
  char *pamb, *pdiff, *pspec, *ppos, *pdir, *pang;
  my_assert ((num_lights < MAX_LIGHTS), "too many lights");
  pl = &my_lights[++num_lights];

  strtok(buffer, "()");
  pamb  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdiff = strtok(NULL, "()");  strtok(NULL, "()"); 
  pspec = strtok(NULL, "()");  strtok(NULL, "()"); 
  ppos  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pdir  = strtok(NULL, "()");  strtok(NULL, "()"); 
  pang  = strtok(NULL, "()");

  parse_floats(pamb, pl->amb);
  parse_floats(pdiff, pl->diff);
  parse_floats(pspec, pl->spec);
  parse_floats(ppos, pl->pos);
  if (pdir) {
    parse_floats(pdir, pl->dir);
    pl->angle = atof(pang);
    //printf("angle %f\n", pl->angle);
  }
  else
    pl->dir[0]= pl->dir[1]= pl->dir[2] =0;
  printf("read light\n");

}

/* assuming the spec is going to be properly written
   not error-checking here */
void read_spec(char *fname) {
  char buffer[300];
  FILE *fp;

  fp = fopen(fname, "r");
  my_assert(fp, "can't open spec");
  while(!feof(fp))
  {
    buffer[0] = '#';
    fgets(buffer, 300, fp);
    //printf("read line: %s\n", buffer);
    switch (buffer[0]) {
    case '#':
      break;
    case '1': //cube
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
   	  //read in the obj
	  parse_obj(buffer);
 	  break;
	//etc

    case 'l':
      parse_light(buffer);
      break;

    case 'c':
      parse_camera(buffer);
      break;

    default:
      break;
    }
  }
}

void lighting_setup ()
{
  int i;
  GLfloat globalAmb[]     = {.1, .1, .1, .1};

  // create flashlight
  GLfloat amb[] = {0.2, 0.2, 0.2, 1.0};
  GLfloat dif[] = {0.5, 0.5, 0.5, 1.0};
  GLfloat spec[] = {0.7, 0.7, 0.7, 1.0};

  //enable lighting
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);

  // reflective propoerites -- global ambiant light
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

  // this was for the flashlights

  glLightfv(GL_LIGHT0, GL_POSITION, my_camera->pos);
  glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, my_camera->look);

  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
  glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
  glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0);

  glEnable(GL_LIGHT0);

  // setup properties of lighting
  for (i=1; i<=num_lights; i++) {
    glEnable(GL_LIGHT0+i);
    glLightfv(GL_LIGHT0+i, GL_AMBIENT, my_lights[i].amb);
    glLightfv(GL_LIGHT0+i, GL_DIFFUSE, my_lights[i].diff);
    glLightfv(GL_LIGHT0+i, GL_SPECULAR, my_lights[i].spec);
    glLightfv(GL_LIGHT0+i, GL_POSITION, my_lights[i].pos);
    if ((my_lights[i].dir[0] > 0) ||  (my_lights[i].dir[1] > 0) ||  (my_lights[i].dir[2] > 0)) {
      glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, my_lights[i].angle);
      glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, my_lights[i].dir);
    }
  }

}

void my_reshape(int w, int h) {
  // ensure a square view port
  glViewport(0,0,min(w,h),min(w,h)) ;
  return ;
}

void my_keyboard( unsigned char key, int x, int y )
{
	switch( key )
	{
		case 'l':
		case 'L':
			move_cone_pos_x = true;
			break;
		case 'j':
		case 'J':
			move_cone_neg_x = true;
			break;
		case 'i':
		case 'I':
			move_cone_neg_z = true;
			break;
		case 'k':
		case 'K':
			move_cone_pos_z = true;
			break;
		case 'o':
		case 'O':
			move_cone_pos_y = true;
			break;
		case 'p':
		case 'P':
			move_cone_neg_y = true;
			break;
		case 'n':
		case 'N':
			turn_cone_left = true;
			break;
		case 'm':
		case 'M':
			turn_cone_right = true;
			break;
        case '`':
            if (lock_pointer == true)
            {
                lock_pointer = false;
				glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
            }
            else
            {
                lock_pointer = true;
				glutSetCursor(GLUT_CURSOR_NONE);
            }
            break;
        case 'a':
		case 'A':
            move_cam_left = true;
			break;
		case 'd':
		case 'D':
            move_cam_right = true;
			break;
		case 'w':
		case 'W':
			if (curr_mode == TRANSLATION_MODE)
			{
				float t = -1;
				if (key == 'W')
				{
					t = 1;
				}
				my_camera->moveAlongW(t);
				glutPostRedisplay();
			}
			else
			{
				move_cam_forward = true;
			}
			break;
		case 's':
		case 'S':
            move_cam_back = true;
			break;
        case 'h':
            my_camera->decreaseHeight();
            glutPostRedisplay();
            break;
        case 'H':
            my_camera->increaseHeight();
            glutPostRedisplay();
            break;
		case 'c':
		case 'C':
			my_camera->switchType();
			glutPostRedisplay();
			break;
		/*case 'x':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongX(-1); break;
				case ROTATION_MODE: my_camera->RotateU(-5); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case 'X':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongX(1); break;
				case ROTATION_MODE: my_camera->RotateU(5); break;
				default: break;
			}
			glutPostRedisplay();
			break;*/
		case 'y':
			my_camera->moveAlongY(1); break;
			glutPostRedisplay();
			break;
		case 'Y':
			my_camera->moveAlongY(-1); break;
			glutPostRedisplay();
			break;
		/*case 'z':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongZ(-1); break;
				case ROTATION_MODE: my_camera->RotateW(-5); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case 'Z':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongZ(1); break;
				case ROTATION_MODE: my_camera->RotateW(5); break;
				default: break;
			}
			glutPostRedisplay();
			break;*/
		case 'u':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongU(-1); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case 'U':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongU(1); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case 'v':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongV(-1); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case 'V':
			switch (curr_mode)
			{
				case TRANSLATION_MODE: my_camera->moveAlongV(1); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case '+':
			switch (curr_mode)
			{
				case NEAR_ADJUST: my_camera->adjustNear(2); break;
				case FAR_ADJUST: my_camera->adjustFar(2); break;
                case ZOOM_MODE: my_camera->zoomIn(); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case '-':
			switch (curr_mode)
			{
				case NEAR_ADJUST: my_camera->adjustNear(-2); break;
				case FAR_ADJUST: my_camera->adjustFar(-2); break;
				case ZOOM_MODE: my_camera->zoomOut(); break;
				default: break;
			}
			glutPostRedisplay();
			break;
		case '<':
            my_camera->decreaseAspect();
            glutPostRedisplay();
            break;
		case '>':
            my_camera->increaseAspect();
            glutPostRedisplay();
			break;
		/*case 'T':
		case 't':
			curr_mode = TRANSLATION_MODE;
			break;*/
		/*case 'R':
		case 'r':
			curr_mode = ROTATION_MODE;
			break;*/
		/*case 'N':
		case 'n':
			curr_mode = NEAR_ADJUST;
			break;
		case 'F':
		case 'f':
			curr_mode = FAR_ADJUST;
			break;*/
		case 'b':
		case 'B':
			if (show_normals == true)
			{
				show_normals = false;
			}
			else
			{
				show_normals = true;
			}
			glutPostRedisplay();
			break;
		/*case 'L':
		case 'l':
			curr_mode = ZOOM_MODE;
			break;*/
		case 'q': 
		case 'Q':
			exit(0) ;
			break ;	
		default: break;
	}
  
	return ;
}

void my_keyboard_up( unsigned char key, int x, int y ) {
	switch( key )
	{
		case 'i':
		case 'I':
			move_cone_neg_z = false;
            break;
		case 'k':
		case 'K':
			move_cone_pos_z = false;
            break;
		case 'j':
		case 'J':
			move_cone_neg_x = false;
            break;
		case 'l':
		case 'L':
			move_cone_pos_x = false;
            break;
		case 'o':
		case 'O':
			move_cone_pos_y = false;
            break;
		case 'p':
		case 'P':
			move_cone_neg_y = false;
            break;
		case 'n':
		case 'N':
			turn_cone_left = false;
			break;
		case 'm':
		case 'M':
			turn_cone_right = false;
			break;

		case 'w':
		case 'W':
			move_cam_forward = false;
            break;
		case 'a':
		case 'A':
			move_cam_left = false;
            break;
		case 's':
		case 'S':
			move_cam_back = false;
            break;
		case 'd':
		case 'D':
			move_cam_right = false;
            break;

        /*case 'F':
		case 'f':
		case 'N':
		case 'n':
		case 'T':
		case 't':
		case 'R':
		case 'r':
			crt_transform = NO_MODE; break;*/
        default: break;
	}
}

void my_mouse_drag(int x, int y)
{
	curr_mouse_x = x;
	curr_mouse_y = y;
}

void my_mouse_move(int x, int y)
{
	curr_mouse_x = x;
	curr_mouse_y = y;
}

void mouse_adjust()
{
	float diff_x = (curr_mouse_x - window_center_x)/5.0;
	float diff_y = (curr_mouse_y - window_center_y)/5.0;

	my_camera->LookLeftRight(diff_x);
    my_camera->RotateU(diff_y);

	glutWarpPointer(window_center_x, window_center_y);

	glutPostRedisplay();

}

int my_raytrace_cube(myObject *o, float result[3])
{
	float* new_p = applyMatrixToVector(ray_start, o->ctmi);
	float* new_dir = applyMatrixToVector(ray_dir, o->diri);

	int num_hits = 0;
	float t;
	float r[3];
	if (new_dir[0] != 0)
	{
		// left
		t = (-0.5 - new_p[0])/new_dir[0];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[1] <= 0.5 && r[1] >= -0.5 && r[2] <= 0.5 && r[2] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
		}
		// right
		t = (-new_p[0] + 0.5)/new_dir[0];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[1] <= 0.5 && r[1] >= -0.5 && r[2] <= 0.5 && r[2] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
		}

		if (num_hits == 2)
		{
			delete[] new_p;
			delete[] new_dir;

			if (result[1] < 0 && result[2] >= 0)
			{
				result[0] = 2;
			}
			else if (result[1] >= 0 && result[2] <0)
			{
				result[0] = 2;
			}
			else if (result[1] == result[2])
			{
				result[0] = 0;
			}
			else if (result[1] != result[2])
			{
				result[0] = 1;
			}
			return num_hits;
		}
	}

	if (new_dir[1] != 0)
	{
		// bottom
		t = (-0.5 - new_p[1])/new_dir[1];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[0] <= 0.5 && r[0] >= -0.5 && r[2] <= 0.5 && r[2] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
			if (num_hits == 2)
			{
				delete[] new_p;
				delete[] new_dir;

				if (result[1] < 0 && result[2] >= 0)
				{
					result[0] = 2;
				}
				else if (result[1] >= 0 && result[2] <0)
				{
					result[0] = 2;
				}
				else if (result[1] == result[2])
				{
					result[0] = 0;
				}
				else if (result[1] != result[2])
				{
					result[0] = 1;
				}
				return num_hits;
			}
		}
		// top
		t = (-new_p[1] + 0.5)/new_dir[1];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[0] <= 0.5 && r[0] >= -0.5 && r[2] <= 0.5 && r[2] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
			if (num_hits == 2)
			{
				delete[] new_p;
				delete[] new_dir;

				if (result[1] < 0 && result[2] >= 0)
				{
					result[0] = 2;
				}
				else if (result[1] >= 0 && result[2] <0)
				{
					result[0] = 2;
				}
				else if (result[1] == result[2])
				{
					result[0] = 0;
				}
				else if (result[1] != result[2])
				{
					result[0] = 1;
				}
				return num_hits;
			}
		}

	}

	if (new_dir[2] != 0)
	{
		// back
		t = (-0.5 - new_p[2])/new_dir[2];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[0] <= 0.5 && r[0] >= -0.5 && r[1] <= 0.5 && r[1] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
			if (num_hits == 2)
			{
				delete[] new_p;
				delete[] new_dir;

				if (result[1] < 0 && result[2] >= 0)
				{
					result[0] = 2;
				}
				else if (result[1] >= 0 && result[2] <0)
				{
					result[0] = 2;
				}
				else if (result[1] == result[2])
				{
					result[0] = 0;
				}
				else if (result[1] != result[2])
				{
					result[0] = 1;
				}
				return num_hits;
			}
		}

		// front
		t = (-new_p[2] + 0.5)/new_dir[2];
		r[0] = new_p[0] + t * new_dir[0];
		r[1] = new_p[1] + t * new_dir[1];
		r[2] = new_p[2] + t * new_dir[2];

		if (r[0] <= 0.5 && r[0] >= -0.5 && r[1] <= 0.5 && r[1] >= -0.5)
		{
			num_hits += 1;
			result[num_hits] = t;
			if (num_hits == 2)
			{
				delete[] new_p;
				delete[] new_dir;

				if (result[1] < 0 && result[2] >= 0)
				{
					result[0] = 2;
				}
				else if (result[1] >= 0 && result[2] <0)
				{
					result[0] = 2;
				}
				else if (result[1] == result[2])
				{
					result[0] = 0;
				}
				else if (result[1] != result[2])
				{
					result[0] = 1;
				}
				return num_hits;
			}
		}

	}
	
	return num_hits;
}

int my_raytrace_sphere(myObject* o, float result[3])
{
	float* new_p = applyMatrixToVector(ray_start, o->ctmi);
	float* new_dir = applyMatrixToVector(ray_dir, o->diri);

	new_p[3] = 0;
	new_dir[3] = 0;

	float a = dotprod(new_dir, new_dir);
	float b = 2*dotprod(new_p, new_dir);
	float c = dotprod(new_p, new_p) - 1;

	/*float a = (pow(new_p[0],2) + pow(new_p[1],2) + pow(new_p[2],2) - 1);
	float b = 2*(new_p[0]*new_dir[0] + new_p[1]*new_dir[1] + new_p[2]*new_dir[2]);
	float c = (pow(new_dir[0], 2) + pow(new_dir[1], 2) + pow(new_dir[2], 2));
*/
	float q = 0;

	delete[] new_p;
	delete[] new_dir;

	float t_test = b*b - 4*a*c;
	if (t_test < 0)
	{
		return 0;
	}
	else
	{
		float t1 = (-b + sqrt(b*b - 4*a*c))/(2*a);
		float t2 = (-b - sqrt(b*b - 4*a*c))/(2*a);
		float type = 0;
		if (t1 < 0 && t2 >=0)
		{
			type = 1;
			result[0] = type;
			result[1] = t2;
			result[2] = t2;
		}
		else if (t1 >= 0 && t2 <0)
		{
			type = 1;
			result[0] = type;
			result[1] = t1;
			result[2] = t1;
		}
		else if (t1 == t2)
		{
			type = 1;
			result[0] = type;
			result[1] = t1;
			result[2] = t2;
		}
		else if (t1 != t2)
		{
			type = 2;
			result[0] = type;
			result[1] = t1;
			result[2] = t2;
		}
		return 1;
	}

	return 0;
}

int my_raytrace_cylinder(myObject* o, float result[3])
{
	float* new_p = applyMatrixToVector(ray_start, o->ctmi);
	float* new_dir = applyMatrixToVector(ray_dir, o->diri);

	new_p[3] = 0;
	new_dir[3] = 0;

    float dx = new_dir[0];
    float dy = new_dir[1];
    float dz = new_dir[2];

    float px = new_p[0];
    float py = new_p[1];
    float pz = new_p[2];

	float a = pow(dx, 2) + pow(dz, 2);
	float b = 2*(px*dx + pz*dz);
    float c = pow(px,2) + pow(pz, 2) - pow(o->ray, 2);

	float t_test = b*b - 4*a*c;
	if (t_test < 0)
	{
        delete[] new_p;
	    delete[] new_dir;
		return 0;
	}
	else
	{
        bool hit1 = false;
        bool hit2 = false;
		float t1 = (-b + sqrt(b*b - 4*a*c))/(2*a);
		float t2 = (-b - sqrt(b*b - 4*a*c))/(2*a);

        float y1 = new_p[1]+t1*new_dir[1];
        float y2 = new_p[1]+t2*new_dir[1];

		float type = 0;

        if ((y1 >= 0 && y1 <= 1) && t1 >= 0)
        {
            hit1 = true;
            result[0] = 1;
            result[1] = t1;
            result[2] = t1;
        }
		
        if ((y2 >= 0 && y2 <= 1) && t2 >= 0)
        {
            hit2 = true;
            if (hit1)
            {
                result[0] = 2;
                if (t2 < t1)
                {
                    result[1] = t2;
                    result[2] = t1;
                }
                else
                {
                    result[2] = t2;
                }
            }
            else
            {
                result[0] = 1;
                result[1] = result[2] = t2;
            }
        }
        
        
        if (hit1 || hit2)
        {
            delete[] new_p;
	        delete[] new_dir;
		    return 1;
        }
	}

    delete[] new_p;
	delete[] new_dir;
	return 0;
}

void my_raytrace(int mousex, int mousey)
{
	int hit = 0;
	myObject *o;

	int_objects.clear();
	// first we need to get the modelview matrix, the projection matrix, and the viewport
	/*glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);*/

	// gluUnProject with a Z value of 1 will find the point on the far clipping plane
	// corresponding the the mouse click. This is not the same as the vector
	// representing the click.
	/*gluUnProject(mousex, mousey, 1.0, modelViewMatrix, projMatrix, viewport, &clickPoint[0], &clickPoint[1], &clickPoint[2]);*/

	// Now we need a vector representing the click. It should start at the camera
	// position. We can subtract the click point, we will get the vector

	/* code for finding direction vector, set rayStart and rayDirection */

	// now go through the shapes and see if there is a hit

	float result[3];
	for (int i = 0; i < num_objects; i++)
	{
		o = &my_scene_objects[i];
		hit = 0;
		switch (o->type)
		{
			case CUBE:
				hit = my_raytrace_cube(o, result);
				break;
			case SPHERE:
				hit = my_raytrace_sphere(o, result);
				break;
			case CYLINDER:
				hit = my_raytrace_cylinder(o, result);
				break;
			default: break;
		}

		// found intersection
		if (hit)
		{
            printf("Hit\n");
			float type = result[0];
			float x,y,z;
			if (type == 0 || type == 2)
			{
				x = ray_start[0]+result[1]*ray_dir[0];
				y = ray_start[1]+result[1]*ray_dir[1];
				z = ray_start[2]+result[1]*ray_dir[2];
				myObject s = myObject(SPHERE);
				s.ScaleObject(0.1, 0.1, 0.1);
				s.TranslateObjectTo(x, y, z);
				s.setColor(0, 1, 0);
				int_objects.push_back(s);
			}
			else if (type == 1)
			{
				x = ray_start[0]+result[1]*ray_dir[0];
				y = ray_start[1]+result[1]*ray_dir[1];
				z = ray_start[2]+result[1]*ray_dir[2];
				myObject s = myObject(SPHERE);
				s.ScaleObject(0.1, 0.1, 0.1);
				s.TranslateObjectTo(x, y, z);
				s.setColor(0, 1, 0);
				int_objects.push_back(s);

				x = ray_start[0]+result[2]*ray_dir[0];
				y = ray_start[1]+result[2]*ray_dir[1];
				z = ray_start[2]+result[2]*ray_dir[2];
				s = myObject(SPHERE);
				s.ScaleObject(0.1, 0.1, 0.1);
				s.TranslateObjectTo(x, y, z);
				s.setColor(0, 1, 0);
				int_objects.push_back(s);
			}
		}
	}
}

void my_mouse(int button, int state, int mousex, int mousey)
{
	//printf("curr_mouse: %d, %d\n", mousex, mousey);
  switch( button )
  {
	
  case GLUT_LEFT_BUTTON:
    if( state == GLUT_DOWN )
	{	
		/*for (int i = 0; i < num_objects; i++)
		{
			o = &my_scene_objects[i];
			o->ApplyCTMi();
			o->ApplyCTM();
		}*/
		set_ray(cone->center[0], cone->center[1], cone->center[2], cone->u[0], cone->u[1], cone->u[2]);
		my_raytrace(mousex, mousey);
		show_ray = true;
		//glutPostRedisplay();
    }
    break ;
  case GLUT_RIGHT_BUTTON:
    if( state == GLUT_DOWN )
	{	
		show_ray = false;
		//glutPostRedisplay();
    }
    break ;
  /*case GLUT_RIGHT_BUTTON:
    if ( state == GLUT_DOWN ) {
    }
    
    if( state == GLUT_UP ) {
    }
    break ;*/
  default: break;
  }
  
	
	return;
}

float dotprod(float v1[], float v2[]) {
  float tot = 0;
  int i;
  for (i=0; i<4; i++)
    tot += v1[i]*v2[i];
  return tot;
}

void normalize(GLfloat *p) { 
  double d=0.0;
  int i;
  for(i=0; i<3; i++) d+=p[i]*p[i];
  d=sqrt(d);
  if(d > 0.0) for(i=0; i<3; i++) p[i]/=d;
}

void cross(GLfloat *a, GLfloat *b, GLfloat *c, GLfloat *d) { 
  d[0]=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
  d[1]=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
  d[2]=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
  normalize(d);
}

/***********************************
 FUNCTION: draw_axes
 ARGS: none
 RETURN: none
 DOES: draws main X, Y, Z axes
************************************/
void draw_axes( void ) {
  glLineWidth( 5.0 );

  glDisable(GL_LIGHTING);

  glBegin(GL_LINES); 
  {
    glColor3fv(colors[1]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[1]);
		
    glColor3fv(colors[4]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[2]);
		
    glColor3fv(colors[6]);
    glVertex4fv(vertices_axes[0]);
    glVertex4fv(vertices_axes[3]);
  }
  glEnd();
  glLineWidth( 1.0 );

  glEnable(GL_LIGHTING);
	
}

void set_ray(float start_x, float start_y, float start_z, float dir_x, float dir_y, float dir_z)
{
	ray_start[0] = start_x;
	ray_start[1] = start_y;
	ray_start[2] = start_z;

	ray_dir[0] = dir_x;
	ray_dir[1] = dir_y;
	ray_dir[2] = dir_z;
}

float* applyMatrixToVector(float vec[4], float mat[4][4])
{
	float *new_vec = new float[4];
	for (int i = 0; i < 4; i++)
	{
		float curr_sum = 0;
		for (int j = 0; j < 4; j++)
        {
			curr_sum += mat[i][j]*vec[j];
		}
		new_vec[i] = curr_sum;
	}
	return new_vec;
}

void draw_ray()
{
	float end_x = ray_start[0] + ray_length*ray_dir[0];
	float end_y = ray_start[1] + ray_length*ray_dir[1];
	float end_z = ray_start[2] + ray_length*ray_dir[2];
	glColor3f(1.0, 1.0, 1.0);
	glLineWidth( 3.0 );
	glBegin(GL_LINES);
	glVertex4f(ray_start[0], ray_start[1], ray_start[2], 1);
	glVertex4f(end_x, end_y, end_z, 1);
	glEnd();

	glLineWidth( 1.0 );
}

void draw_objects() {
    int i;
    for(i=0; i<num_objects; i++)
    {
        OBJECT *cur;
        myObject *o;
        cur = &my_objects[i];
        o = &my_scene_objects[i];

        glMaterialfv(GL_FRONT, GL_AMBIENT, cur->amb);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, cur->diff);
        glMaterialfv(GL_FRONT, GL_SPECULAR, cur->spec);
        glMaterialfv(GL_FRONT, GL_SHININESS, &cur->shine);
        //glMaterialfv(GL_FRONT, GL_EMISSION, cur->emi);

        o->draw(crt_render_mode);
		if (show_normals)
		{
			o->draw_normals();
		}
    }

	for (i = 0; (unsigned) i < int_objects.size(); i++)
	{
		int_objects[i].draw(crt_render_mode);
	}

	/*for (vector<myObject>::iterator iter = int_objects.begin(); iter < int_objects.end(); iter++)
	{

	}*/

	cone->draw(crt_render_mode);
	if (show_normals)
	{
		cone->draw_normals();
	}

}

void my_display() {

  // clear all pixels, reset depth 
  glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT );
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // using my camera matrices and loading them on the stack
  glMultMatrixd(my_camera->R);
  glMultMatrixd(my_camera->T);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // only use Mpp for perspective camera
  if (my_camera->type == PERSP)
  {
	  glMultMatrixd(my_camera->M);
  }
  glMultMatrixd(my_camera->S);

  glMatrixMode(GL_MODELVIEW);
  
  //draw the objects
  draw_axes();

  draw_objects();

  if (show_ray)
  {
	  draw_ray();
  }

  // this buffer is ready
  glutSwapBuffers();
}

void my_idle(void) {
  //EC idea: Make the flashlight flicker a bit (random flicker strength) when the user is idle.
  return ;
}

void my_TimeOut(int id)
{
    // For some unknown reason, this timer function will not work unless I put a print statement in it.
    // haven't been able to figure out why that's the case
    printf("");
    if (lock_pointer == true)
    {
	    mouse_adjust();
    }

	// moving the cone
    float move_amount = 0.05;
    if (move_cone_neg_x == true)
    {
        cone->TranslateObject(-move_amount, 0, 0);
    }
    if (move_cone_pos_x == true)
    {
        cone->TranslateObject(move_amount, 0, 0);
    }
    if (move_cone_neg_y == true)
    {
        cone->TranslateObject(0, -move_amount, 0);
    }
    if (move_cone_pos_y == true)
    {
        cone->TranslateObject(0, move_amount, 0);
    }
    if (move_cone_neg_z == true)
    {
        cone->TranslateObject(0, 0, -move_amount);
    }
    if (move_cone_pos_z == true)
    {
        cone->TranslateObject(0, 0, move_amount);
    }
	if (turn_cone_left)
	{
		float x = cone->center[0];
		float y = cone->center[1];
		float z = cone->center[2];
		cone->TranslateObjectToOrigin();
		cone->RotateObjectAboutCentralAxis(0.2);
		cone->TranslateObjectTo(x, y, z);
	}
	if (turn_cone_right)
	{
		float x = cone->center[0];
		float y = cone->center[1];
		float z = cone->center[2];
		cone->TranslateObjectToOrigin();
		cone->RotateObjectAboutCentralAxis(-0.2);
		cone->TranslateObjectTo(x, y, z);
	}

	// moving the camera
	if (move_cam_forward)
	{
		my_camera->MoveForwardBack(-move_amount);
	}
	if (move_cam_back)
	{
		my_camera->MoveForwardBack(move_amount);
	}
	if (move_cam_left)
	{
		my_camera->MoveLeftRight(-move_amount);
	}
	if (move_cam_right)
	{
		my_camera->MoveLeftRight(move_amount);
	}
	glutPostRedisplay();
	glutTimerFunc(0.2, my_TimeOut, 0);
	return ;
}