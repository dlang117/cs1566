#include <glut.h>
#include <vector>

#ifndef _MY_CAMERA_H_
#define _MY_CAMERA_H_

class myCamera
{
    public:
        myCamera() {};
        myCamera(float, float, float, float, float, float, float, float, float, int);
        ~myCamera();
        int type;
        GLfloat pos[3];
        GLfloat look[3];
        GLfloat up[3];
        GLfloat u[3];
        GLfloat v[3];
        GLfloat w[3];
        GLfloat N[4][4];
        GLfloat glN[16];

        GLdouble T[16];
        GLdouble R[16];
        GLdouble S[16];
		GLdouble M[16];

        GLdouble fovy;
        GLdouble aspect;
        GLdouble zNear;
        GLdouble zFar;

		GLdouble left;
		GLdouble right;
		GLdouble bottom;
		GLdouble top;

        GLfloat dot_product(GLfloat[3], GLfloat[3]);
        GLfloat* scalar_mult(float, GLfloat[3]);
        GLfloat* vector_subtract(GLfloat[3], GLfloat[3]);
        GLfloat* cross_product(GLfloat[3], GLfloat[3]);
        GLfloat* normalize(GLfloat[3]);

        void generateT();
        void generateR();
        void generateS();
		void generateM();

		void moveAlongU(float);
		void moveAlongV(float);
		void moveAlongW(float);
		void moveAlongX(float);
		void moveAlongY(float);
		void moveAlongZ(float);

		void moveOrigin(float);

		void adjustNear(float);
		void adjustFar(float);

		void RotateU(float);
		void RotateV(float);
		void RotateW(float);

		void LookLeftRight(float);
		void MoveLeftRight(float);
		void MoveForwardBack(float);

        void zoomIn();
        void zoomOut();

        void increaseHeight();
        void decreaseHeight();

        void increaseAspect();
        void decreaseAspect();

		void switchType();
        void printInfo();
};

#endif
