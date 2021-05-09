#include <GL/glut.h>
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
 
char title[] = "3D Shapes with animation";
GLfloat angle = 0.0f;
int refreshMills = 15;
int N = 32;
GLfloat *vertices;
GLfloat *normals;
GLuint *indices;

typedef struct vec3{
  GLfloat x;
  GLfloat y;
  GLfloat z;
}vec3_t;

void cross(GLfloat *a, GLfloat *b, GLfloat *r1, GLfloat *r2, GLfloat *r3){
  float x = (a[1]*b[2])-(a[2]*b[1]);
  float y = (a[2]*b[0])-(a[0]*b[2]);
  float z = (a[0]*b[1])-(a[1]*b[0]);
  r1[0]+=x; r2[0]+=x; r3[0]+=x;
  r1[1]+=y; r2[1]+=y; r3[1]+=y;
  r1[2]+=z; r2[2]+=z; r3[2]+=z;
}

void setups(){
  vertices = (GLfloat *)malloc(4 * (N+1) * (N+1) * 3);
  normals = (GLfloat *)malloc(4 * (N+1) * (N+1) * 3);
  for (int x=0; x<=N; x++){
    for (int z=0; z<=N; z++){
      int i = x+z*(N+1);
      vertices[3*i] = (x-(N/2))/2.0f;
      //vertices[3*i+1] = 0;
      //vertices[3*i+1] = (rand() % 10001) / 10000.0;
      vertices[3*i+1] = 0.5*sin(x/2.0f)+0.5*sin(z/2.0f);
      vertices[3*i+2] = (z-(N/2))/2.0f;
      /*
      if (i==0)
          printf("%f %f\n",vertices[3*i],vertices[3*i+2]);
      if (z==N & x==N)
          printf("%f %f\n",vertices[3*i],vertices[3*i+2]);
      */
      normals[3*i] = 0;
      normals[3*i+1] = 0;
      normals[3*i+2] = 0;
    }
  }
  //vertices[30*3+1]=1;
  //vertices[31*3+1]=1;

  for (int x=0; x<N; x++){
    for (int z=0; z<N; z++){
      int i = x+z*(N+1);
      int a = i;
      int b = i+N+1;
      int c = i+1;
      cross(vertices+3*a, vertices+3*b, normals+3*a,
            normals+3*b, normals+3*c);
      cross(vertices+3*b, vertices+3*c, normals+3*a,
            normals+3*b, normals+3*c);
      cross(vertices+3*c, vertices+3*a, normals+3*a,
            normals+3*b, normals+3*c);

      a = i+1;
      b = i+N+1;
      c = i+N+2;
      cross(vertices+3*a, vertices+3*b, normals+3*a,
            normals+3*b, normals+3*c);
      cross(vertices+3*b, vertices+3*c, normals+3*a,
            normals+3*b, normals+3*c);
      cross(vertices+3*c, vertices+3*a, normals+3*a,
            normals+3*b, normals+3*c);
    }
  }
  
  for (int x=0; x<=N; x++){
    for (int z=0; z<=N; z++){
      int i = x+z*(N+1);
      printf("%d, %d, %d: %f, %f, %f\n", x, z, i,
             normals[3*i],
             normals[3*i+1],
             normals[3*i+2]);
    }
  }
  
  indices = (GLuint *)malloc(4 * 3 * N * N * 2);
  for (int x=0; x<N; x++){
    for (int z=0; z<N; z++){
      int i = x+z*N;
      uint i2 = x+z*(N+1);
      indices[6*i] = i2;
      indices[6*i+1] = i2+(uint)N+1;
      indices[6*i+2] = i2+1;
      indices[6*i+3] = i2+1;
      indices[6*i+4] = i2+(uint)N+1;
      indices[6*i+5] = i2+(uint)N+2;
    }
  }
}
 
void initGL() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glShadeModel(GL_SMOOTH);

  GLfloat ambientColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat diffuseColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat specularColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat lightPosition[] = {1.0f, 1.0f, 1.0f, 0.0f};
  GLfloat mat_specular[]   = { 0.8f, 0.8f, 0.8f, 1.0f };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambientColor);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseColor);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specularColor);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
  glMateriali(GL_FRONT, GL_SHININESS, 100);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
}
 
void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  glColor3f(0.0f, 0.0f, 1.0f);

  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -32.0f);
  glRotatef(-angle, 1.0f, 0.1f, 0.1f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vertices);
  glNormalPointer(GL_FLOAT, 3*4, normals);
  glDrawElements(GL_TRIANGLES, N * N * 3 * 2, GL_UNSIGNED_INT, indices);
  glDisableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  glutSwapBuffers();

  angle -= 0.9f;
}

void timer(int value) {
  glutPostRedisplay();
  glutTimerFunc(refreshMills, timer, 0);
}

void reshape(GLsizei width, GLsizei height) {
  if (height == 0) height = 1;
  GLfloat aspect = (GLfloat)width / (GLfloat)height;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  gluPerspective(45.0f, aspect, 0.1f, 100.0f);
}

int main(int argc, char** argv) {
  setups();
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE);
  glutInitWindowSize(640, 480);
  glutInitWindowPosition(50, 50);
  glutCreateWindow(title);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  initGL();
  glutTimerFunc(0, timer, 0);
  glutMainLoop();
  return 0;
}
