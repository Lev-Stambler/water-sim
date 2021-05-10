#include <GL/glut.h>
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
 
char title[] = "Ocean";
int refreshMills = 15;
int N = 64;
GLfloat *vertices, *normals;
GLuint *indices;
GLfloat *d, *b, *s;
int m1[] = {-1, 1, 0, 0};
float k = 0.8f;
float p1 = 0.2f;
float p2 = 0.0f;
int count=0;

void cross(GLfloat *a, GLfloat *b, GLfloat *r1, GLfloat *r2, GLfloat *r3){
  float x = (a[1]*b[2])-(a[2]*b[1]);
  float y = (a[2]*b[0])-(a[0]*b[2]);
  float z = (a[0]*b[1])-(a[1]*b[0]);
  r1[0]+=x; r2[0]+=x; r3[0]+=x;
  r1[1]+=y; r2[1]+=y; r3[1]+=y;
  r1[2]+=z; r2[2]+=z; r3[2]+=z;
}

void addNormals(){
  for (int x=0; x<N; x++){
    for (int z=0; z<N; z++){
      int i = x+z*(N+1);
      int a = i;
      int b = i+N+1;
      int c = i+1;
      cross(vertices+3*a, vertices+3*b, normals+3*a, normals+3*b, normals+3*c);
      cross(vertices+3*b, vertices+3*c, normals+3*a, normals+3*b, normals+3*c);
      cross(vertices+3*c, vertices+3*a, normals+3*a, normals+3*b, normals+3*c);
      a = i+1;
      b = i+N+1;
      c = i+N+2;
      cross(vertices+3*a, vertices+3*b, normals+3*a, normals+3*b, normals+3*c);
      cross(vertices+3*b, vertices+3*c, normals+3*a, normals+3*b, normals+3*c);
      cross(vertices+3*c, vertices+3*a, normals+3*a, normals+3*b, normals+3*c);
    }
  }
}

void setups(){
  vertices = (GLfloat *)malloc(4 * (N+1) * (N+1) * 3);
  normals = (GLfloat *)malloc(4 * (N+1) * (N+1) * 3);
  //printf("%d %d\n", 4 * (N+1) * (N+1) * 3, 4 * 3 * N * N * 2);
  for (int x=0; x<=N; x++){
    for (int z=0; z<=N; z++){
      int i = x+z*(N+1);
      vertices[3*i] = (x-(N/2))/4.0f;
      vertices[3*i+1] = 1.0f;
      //vertices[3*i+1] = 2.0+(rand() % 10001) / 10000.0;
      //vertices[3*i+1] += 1.6+0.6*sin(x/3.0f)+0.5*sin(z/3.0f);
      vertices[3*i+2] = (z-(N/2))/4.0f;
      normals[3*i] = 0.0f;
      normals[3*i+1] = 0.0f;
      normals[3*i+2] = 0.0f;
    }
  }
  //vertices[300*3+1]=15.0f;
  //vertices[301*3+1]=15.0f;
  //addNormals();
  
  indices = (GLuint *)malloc(4 * 3 * N * N * 2);
  for (int x=0; x<N; x++){
    for (int z=0; z<N; z++){
      int i = x+z*N;
      uint i2 = x+z*(N+1);
      indices[6*i] = i2;
      indices[6*i+1] = i2+N+1;
      indices[6*i+2] = i2+1;
      indices[6*i+3] = i2+1;
      indices[6*i+4] = i2+N+1;
      indices[6*i+5] = i2+N+2;
    }
  }
  b = (GLfloat *)malloc(4 * (N+1) * (N+1));
  for (int i=0; i<(N+1)*(N+1); i++)
    b[i]=0.1f;
  d = (GLfloat *)malloc(4 * (N+1) * (N+1));
  for (int i=0; i<(N+1)*(N+1); i++)
    d[i]=vertices[3*i+1]-b[i];
  s = (GLfloat *)malloc(4 * (N+1) * (N+1) * 4);
  for (int i=0; i<(N+1)*(N+1); i++){
    s[4*i]=0.0f; s[4*i+1]=0.0f;
    s[4*i+2]=0.0f; s[4*i+3]=0.0f;
  }
}

int ind1(int dir){
  return m1[dir];
}
int ind2(int dir){
  return m1[3-dir];
}

void update(){
  int N2 = N+1;
  if (count % 2 == 0){
    count = 0;
    int x = rand()%(N-1)+1;
    int z = rand()%(N-1)+1;
    d[x+z*N2]+=2;
    d[(x+1)+z*N2]+=1;
    d[x+(z+1)*N2]+=1;
    d[(x-1)+z*N2]+=1;
    d[x+(z-1)*N2]+=1;
    vertices[3*(x+z*N2)+1]+=2;
    vertices[3*((x+1)+z*N2)+1]+=1;
    vertices[3*(x+(z+1)*N2)+1]+=1;
    vertices[3*((x-1)+z*N2)+1]+=1;
    vertices[3*(x+(z-1)*N2)+1]+=1;
  }
  count += 1;
  for (int x=0; x<N2; x++){
    for (int z=0; z<N2; z++){
      int i=x+z*N2;
      for (int dir=0; dir<4; dir++){
        int x2 = x+ind1(dir);
        int z2 = z+ind2(dir);
        int i2=x2+z2*N2;
        float outflow = 0.0f;
        if (x2>=0 && x2<N2 && z2>=0 && z2<N2){
          float r = vertices[3*i+1]-vertices[3*i2+1];
          if (r>0.0f) outflow=r;
        }
        s[4*i+dir] = k * s[4*i+dir] + p1 * outflow;
        if (s[4*i+dir]!=s[4*i+dir]){
          printf("err\n");
          exit(0);
        }
        //printf("%d %d %d %d %f\n",x,z,x2,z2,s[4*i+dir]);
      }
    }
  }
  for (int x=0; x<N2; x++){
    for (int z=0; z<N2; z++){
      int i=x+z*N2;
      float r = s[4*i]+s[4*i+1]+s[4*i+2]+s[4*i+3];
      if (d[i] < r){
        s[4*i] *= d[i] / r;
        s[4*i+1] *= d[i] / r;
        s[4*i+2] *= d[i] / r;
        s[4*i+3] *= d[i] / r;
        if (s[4*i]!=s[4*i]){
          printf("%f %f\n",d[i],r);
        }
        r = s[4*i]+s[4*i+1]+s[4*i+2]+s[4*i+3];
      }
      if (r < p2){
        s[4*i] = 0.0f; s[4*i+1] = 0.0f;
        s[4*i+2] = 0.0f; s[4*i+3] = 0.0f;
      }
    }
  }
  for (int x=0; x<N2; x++){ //1,2,3,0
    for (int z=0; z<N2; z++){
      int i=x+z*N2;
      float r = -s[4*i]-s[4*i+1]-s[4*i+2]-s[4*i+3];
      if (x>0){
        int i2 = (x-1)+z*N2;
        r += s[4*i2+1];
      }
      if (z>0){
        int i2 = x+(z-1)*N2;
        r += s[4*i2+2];
      }
      if (x<N){
        int i2 = (x+1)+z*N2;
        r += s[4*i2+0];
      }
      if (z<N){
        int i2 = x+(z+1)*N2;
        r += s[4*i2+3];
      }
      d[i]+=r;
      vertices[3*i+1]=b[i]+d[i];
      normals[3*i] = 0.0f;
      normals[3*i+1] = 0.0f;
      normals[3*i+2] = 0.0f;
    }
  }
  addNormals();
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
  update();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);

  glColor3f(0.0f, 0.0f, 1.0f);

  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, -24);
  glRotatef(30, 1.0f, 0.0f, 0.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vertices);
  glNormalPointer(GL_FLOAT, 3*4, normals);
  glDrawElements(GL_TRIANGLES, N * N * 3 * 2, GL_UNSIGNED_INT, indices);
  glDisableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  glutSwapBuffers();
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
