#define NOMINMAX
#include <windows.h>
#include <GL/freeglut.h>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>

// ============================================================
// REALISTIC 3D WRISTWATCH - GLUT / Fixed Function OpenGL
// Controls:
//   Arrow keys       Rotate watch
//   W / S            Rotate vertically
//   A / D            Rotate horizontally
//   + / -            Zoom
//   Left mouse       Zoom in
//   Right mouse      Zoom out
//   Q / ESC          Quit
// ============================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float rotationX = 18.0f;
float rotationY = -22.0f;
float scale = 1.0f;

const float PI = 3.14159265358979323846f;
const float CASE_R = 1.62f;
const float DIAL_R = 1.38f;

// ---------- Materials ----------
void setMaterial(float r, float g, float b,
    float spec = 0.6f, float shine = 80.0f,
    float alpha = 1.0f)
{
    GLfloat ambient[] = { r * 0.35f, g * 0.35f, b * 0.35f, alpha };
    GLfloat diffuse[] = { r, g, b, alpha };
    GLfloat specular[] = { spec, spec, spec, alpha };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine);
}

void setColor(float r, float g, float b)
{
    glColor3f(r, g, b);
}

// ---------- Lighting ----------
void initLighting()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    // Main studio/key light
    GLfloat keyPos[] = { 3.5f, 4.5f, 6.0f, 1.0f };
    GLfloat keyDiffuse[] = { 1.0f, 0.96f, 0.90f, 1.0f };
    GLfloat keySpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, keyPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, keyDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, keySpec);

    // Fill light
    GLfloat fillPos[] = { -4.0f, 1.5f, 3.0f, 1.0f };
    GLfloat fillDiffuse[] = { 0.38f, 0.45f, 0.55f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, fillPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiffuse);

    // Rim light
    GLfloat rimPos[] = { 0.0f, -4.0f, 2.5f, 1.0f };
    GLfloat rimDiffuse[] = { 0.55f, 0.60f, 0.75f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, rimPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, rimDiffuse);

    GLfloat globalAmbient[] = { 0.055f, 0.055f, 0.065f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
}

// ---------- Utility primitives ----------
void drawCylinder(float radius, float depth, int slices = 64)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -depth * 0.5f);
    gluCylinder(q, radius, radius, depth, slices, 2);

    // Back cap
    glPushMatrix();
    glRotatef(180.0f, 1, 0, 0);
    gluDisk(q, 0.0, radius, slices, 1);
    glPopMatrix();

    // Front cap
    glTranslatef(0.0f, 0.0f, depth);
    gluDisk(q, 0.0, radius, slices, 1);

    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawDisc(float radius, float z, int slices = 96)
{
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(0, 0, z);
    gluDisk(q, 0.0, radius, slices, 2);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawRing(float innerRadius, float outerRadius, float z, int segments = 96)
{
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float a = 2.0f * PI * i / segments;
        float c = cosf(a);
        float s = sinf(a);

        glNormal3f(0, 0, 1);
        glVertex3f(innerRadius * c, innerRadius * s, z);
        glVertex3f(outerRadius * c, outerRadius * s, z);
    }
    glEnd();
}

// ---------- Watch strap ----------
void drawStrap()
{
    // Upper and lower leather straps.
    // The watch face remains in the XY plane; strap extends along Y.
    setMaterial(0.12f, 0.045f, 0.025f, 0.25f, 20.0f);

    // Upper strap
    glPushMatrix();
    glTranslatef(0.0f, 2.25f, -0.05f);
    glScalef(0.72f, 1.55f, 0.20f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Lower strap
    glPushMatrix();
    glTranslatef(0.0f, -2.25f, -0.05f);
    glScalef(0.72f, 1.55f, 0.20f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Leather edge strips / stitching
    setMaterial(0.72f, 0.48f, 0.20f, 0.15f, 10.0f);

    for (int side = -1; side <= 1; side += 2)
    {
        float x = side * 0.58f;

        for (int i = -5; i <= 5; ++i)
        {
            float y = i * 0.23f;

            glPushMatrix();
            glTranslatef(x, y + (i > 0 ? 2.0f : -2.0f), 0.08f);
            glScalef(0.025f, 0.06f, 0.025f);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    }

    // Lugs connecting strap to case
    setMaterial(0.42f, 0.44f, 0.47f, 0.9f, 110.0f);

    for (int y = -1; y <= 1; y += 2)
    {
        glPushMatrix();
        glTranslatef(0.0f, y * 1.55f, 0.0f);
        glScalef(1.0f, 0.18f, 0.28f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

// ---------- Watch case ----------
void drawCase()
{
    // Main case
    setMaterial(0.42f, 0.44f, 0.47f, 1.0f, 120.0f);
    drawCylinder(CASE_R, 0.42f, 96);

    // Dark case side accent
    setMaterial(0.16f, 0.17f, 0.19f, 0.75f, 90.0f);
    glPushMatrix();
    glTranslatef(0, 0, -0.04f);
    drawCylinder(1.54f, 0.46f, 96);
    glPopMatrix();

    // Polished outer bezel
    setMaterial(0.68f, 0.70f, 0.73f, 1.0f, 150.0f);
    glPushMatrix();
    glTranslatef(0, 0, 0.24f);
    drawCylinder(1.55f, 0.16f, 96);
    glPopMatrix();

    // Inner bezel ring
    setMaterial(0.18f, 0.19f, 0.21f, 0.85f, 110.0f);
    glPushMatrix();
    glTranslatef(0, 0, 0.33f);
    drawRing(1.37f, 1.50f, 0.0f);
    glPopMatrix();
}

// ---------- Textured-looking sunburst dial ----------
void drawDial()
{
    // Dark dial base
    setMaterial(0.018f, 0.025f, 0.035f, 0.55f, 90.0f);
    drawDisc(DIAL_R, 0.36f);

    // Radial sunburst lines using subtle metallic variation
    glDisable(GL_LIGHTING);

    for (int i = 0; i < 72; ++i)
    {
        float a = 2.0f * PI * i / 72.0f;
        float c = cosf(a);
        float s = sinf(a);

        float shade = (i % 2 == 0) ? 0.055f : 0.025f;
        glColor3f(shade, shade * 1.15f, shade * 1.45f);

        glBegin(GL_TRIANGLES);
        glVertex3f(0, 0, 0.365f);
        glVertex3f(DIAL_R * c, DIAL_R * s, 0.365f);
        glVertex3f(DIAL_R * cosf(a + 0.012f),
            DIAL_R * sinf(a + 0.012f), 0.365f);
        glEnd();
    }

    glEnable(GL_LIGHTING);

    // Fine inner chapter ring
    setMaterial(0.32f, 0.34f, 0.37f, 0.8f, 100.0f);
    drawRing(1.22f, 1.235f, 0.375f);
}

// ---------- Hour markers ----------
void drawHourMarkers()
{
    setMaterial(0.78f, 0.80f, 0.83f, 1.0f, 150.0f);

    for (int i = 0; i < 12; ++i)
    {
        float angle = i * 30.0f * PI / 180.0f;

        // 12, 3, 6, 9 get larger markers
        bool major = (i % 3 == 0);

        float radius = major ? 1.08f : 1.13f;
        float x = radius * sinf(angle);
        float y = radius * cosf(angle);

        glPushMatrix();
        glTranslatef(x, y, 0.42f);

        // Rotate the rectangular marker tangentially
        glRotatef(-i * 30.0f, 0, 0, 1);

        glScalef(major ? 0.10f : 0.055f,
            major ? 0.22f : 0.16f,
            0.055f);

        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Minute markers
    setMaterial(0.52f, 0.54f, 0.57f, 0.75f, 90.0f);

    for (int i = 0; i < 60; ++i)
    {
        if (i % 5 == 0) continue;

        float angle = i * 6.0f * PI / 180.0f;
        float r1 = 1.22f;
        float r2 = (i % 5 == 0) ? 1.17f : 1.185f;

        glBegin(GL_LINES);
        glVertex3f(r1 * sinf(angle), r1 * cosf(angle), 0.39f);
        glVertex3f(r2 * sinf(angle), r2 * cosf(angle), 0.39f);
        glEnd();
    }
}

// ---------- Dial text ----------
void drawText(float x, float y, float z, const char* text, void* font)
{
    glRasterPos3f(x, y, z);
    for (const char* p = text; *p; ++p)
        glutBitmapCharacter(font, *p);
}

void drawDialDetails()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.72f, 0.74f, 0.77f);

    // Brand
    drawText(-0.35f, 0.55f, 0.43f, "AURORA",
        GLUT_BITMAP_HELVETICA_12);

    // Automatic text
    glColor3f(0.42f, 0.44f, 0.47f);
    drawText(-0.28f, -0.48f, 0.43f, "AUTOMATIC",
        GLUT_BITMAP_HELVETICA_10);

    glEnable(GL_LIGHTING);
}

// ---------- Date window ----------
void drawDateWindow()
{
    // At 3 o'clock
    glPushMatrix();
    glTranslatef(0.92f, 0.0f, 0.43f);

    setMaterial(0.88f, 0.87f, 0.82f, 0.4f, 50.0f);
    glScalef(0.25f, 0.16f, 0.045f);
    glutSolidCube(1.0);

    glDisable(GL_LIGHTING);
    glColor3f(0.08f, 0.08f, 0.08f);
    glRasterPos3f(-0.035f, -0.035f, 0.03f);

    time_t now = time(nullptr);
    struct tm localTime {};
    localtime_s(&localTime, &now);

    char dateText[3];
    sprintf_s(dateText, "%02d", localTime.tm_mday);

    for (int i = 0; dateText[i]; ++i)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, dateText[i]);

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ---------- Watch hands ----------
void drawHand(float length, float width, float angle,
    float r, float g, float b, float z)
{
    glPushMatrix();

    glRotatef(-angle, 0, 0, 1);
    glTranslatef(0, length * 0.5f, z);

    setMaterial(r, g, b, 1.0f, 150.0f);

    glScalef(width, length, 0.045f);
    glutSolidCube(1.0);

    glPopMatrix();
}

void drawClockHands()
{
    time_t now = time(nullptr);

    // Millisecond precision for smooth sweeping
    struct tm localTime {};
    localtime_s(&localTime, &now);

    SYSTEMTIME st;
    GetLocalTime(&st);

    float seconds = localTime.tm_sec + st.wMilliseconds / 1000.0f;
    float minutes = localTime.tm_min + seconds / 60.0f;
    float hours = (localTime.tm_hour % 12) + minutes / 60.0f;

    float hourAngle = hours * 30.0f;
    float minuteAngle = minutes * 6.0f;
    float secondAngle = seconds * 6.0f;

    // Hour hand
    drawHand(0.67f, 0.115f, hourAngle,
        0.78f, 0.80f, 0.83f, 0.49f);

    // Minute hand
    drawHand(0.98f, 0.075f, minuteAngle,
        0.86f, 0.87f, 0.89f, 0.51f);

    // Red-tipped sweeping second hand
    drawHand(1.10f, 0.025f, secondAngle,
        0.68f, 0.045f, 0.035f, 0.54f);

    // Counterweight for second hand
    glPushMatrix();
    glRotatef(-secondAngle, 0, 0, 1);
    glTranslatef(0, -0.22f, 0.54f);

    setMaterial(0.65f, 0.04f, 0.03f, 0.9f, 130.0f);
    glScalef(0.04f, 0.22f, 0.025f);
    glutSolidCube(1.0);

    glPopMatrix();

    // Center jewel
    setMaterial(0.80f, 0.58f, 0.16f, 1.0f, 180.0f);
    glPushMatrix();
    glTranslatef(0, 0, 0.59f);
    glutSolidSphere(0.065f, 32, 32);
    glPopMatrix();
}

// ---------- Sapphire crystal ----------
void drawCrystal()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    setMaterial(0.72f, 0.84f, 0.95f, 1.0f, 180.0f, 0.14f);

    glPushMatrix();
    glTranslatef(0, 0, 0.66f);
    drawCylinder(1.39f, 0.045f, 96);
    glPopMatrix();

    // Diagonal highlight across crystal
    glDisable(GL_LIGHTING);
    glColor4f(0.9f, 0.95f, 1.0f, 0.11f);

    glBegin(GL_QUADS);
    glVertex3f(-1.25f, 0.75f, 0.69f);
    glVertex3f(-0.95f, 0.98f, 0.69f);
    glVertex3f(1.20f, -0.72f, 0.69f);
    glVertex3f(0.90f, -0.95f, 0.69f);
    glEnd();

    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ---------- Crown ----------
void drawCrown()
{
    setMaterial(0.52f, 0.54f, 0.57f, 1.0f, 140.0f);

    glPushMatrix();
    glTranslatef(CASE_R + 0.13f, 0.0f, 0.0f);
    glRotatef(90.0f, 0, 1, 0);

    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, 0.13f, 0.13f, 0.18f, 32, 2);
    gluDeleteQuadric(q);

    // Crown ridges
    for (int i = 0; i < 10; ++i)
    {
        glPushMatrix();
        glRotatef(i * 36.0f, 1, 0, 0);
        glTranslatef(0, 0.13f, 0);
        glScalef(0.02f, 0.03f, 0.12f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glPopMatrix();
}

// ---------- Bezel screws ----------
void drawScrews()
{
    setMaterial(0.25f, 0.27f, 0.30f, 1.0f, 180.0f);

    const float radius = 1.48f;

    for (int i = 0; i < 4; ++i)
    {
        float a = (45.0f + i * 90.0f) * PI / 180.0f;

        glPushMatrix();
        glTranslatef(radius * cosf(a), radius * sinf(a), 0.38f);
        glutSolidSphere(0.035f, 20, 20);
        glPopMatrix();
    }
}

// ---------- Complete watch ----------
void drawWatch()
{
    drawStrap();
    drawCase();
    drawDial();
    drawHourMarkers();
    drawDialDetails();
    drawDateWindow();
    drawClockHands();
    drawScrews();
    drawCrown();
    drawCrystal();
}

// ---------- Display ----------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        0.0, 0.0, 7.0,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    );

    glScalef(scale, scale, scale);

    glRotatef(rotationX, 1, 0, 0);
    glRotatef(rotationY, 0, 1, 0);

    // TEST TRIANGLE - REMOVE LATER
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex3f(-2.0f, -2.0f, 0.0f);
        glVertex3f(2.0f, -2.0f, 0.0f);
        glVertex3f(0.0f, 2.0f, 0.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    drawWatch();

    glutSwapBuffers();

    static bool firstFrame = true;
    if (firstFrame) {
        printf("First frame rendered!\n");
        firstFrame = false;
    }
}

// ---------- Interaction ----------
void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
    case 'q':
    case 'Q':
    case 27:
        exit(0);

    case '+':
    case '=':
        scale = std::min(scale + 0.08f, 2.5f);
        break;

    case '-':
    case '_':
        scale = std::max(scale - 0.08f, 0.45f);
        break;

    case 'w':
    case 'W':
        rotationX -= 5.0f;
        break;

    case 's':
    case 'S':
        rotationX += 5.0f;
        break;

    case 'a':
    case 'A':
        rotationY -= 5.0f;
        break;

    case 'd':
    case 'D':
        rotationY += 5.0f;
        break;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    switch (key)
    {
    case GLUT_KEY_UP:
        rotationX -= 5.0f;
        break;

    case GLUT_KEY_DOWN:
        rotationX += 5.0f;
        break;

    case GLUT_KEY_LEFT:
        rotationY -= 5.0f;
        break;

    case GLUT_KEY_RIGHT:
        rotationY += 5.0f;
        break;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int, int)
{
    if (state != GLUT_DOWN) return;

    if (button == GLUT_LEFT_BUTTON)
        scale = std::min(scale + 0.10f, 2.5f);

    if (button == GLUT_RIGHT_BUTTON)
        scale = std::max(scale - 0.10f, 0.45f);

    glutPostRedisplay();
}

// ---------- Projection ----------
void reshape(int width, int height)
{
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        42.0f,
        static_cast<float>(width) / height,
        0.1f,
        100.0f
    );

    glMatrixMode(GL_MODELVIEW);
}

// ---------- Continuous animation ----------
void idle()
{
    glutPostRedisplay();
}

// ---------- Initialization ----------
void initScene()
{
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // Bright green to test rendering

    initLighting();

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

// ---------- Main ----------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(1000, 800);
    glutCreateWindow("AURORA - Realistic 3D Luxury Watch");

    initScene();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    glutMainLoop();

    return 0;
}
