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

// ---------- Real-time clock helper ----------
SYSTEMTIME getCurrentLocalTime()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    return st;
}

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
// A continuous, closed wrist strap surrounding the invisible wrist.
// The watch case sits on top of the loop, just like a real watch worn on a wrist.
void drawStrap()
{
    // Real wristwatch strap geometry:
    // - The two original straight strap halves remain attached to the case.
    // - Their outer/free ends do NOT meet at the center.
    // - A curved section behind the invisible wrist connects those two ends,
    //   forming one continuous closed loop when the watch is rotated.
    const float strapWidth = 0.72f;
    const float strapDepth = 0.20f;
    const float strapCenter = 1.82f;
    const float strapLength = 0.46f;
    const float endY = strapCenter + strapLength * 0.5f; // 2.05

    setMaterial(0.12f, 0.045f, 0.025f, 0.25f, 20.0f);

    // Upper straight strap - unchanged outer end position.
    glPushMatrix();
    glTranslatef(0.0f, strapCenter, -0.05f);
    glScalef(strapWidth, strapLength, strapDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    // Lower straight strap - unchanged outer end position.
    glPushMatrix();
    glTranslatef(0.0f, -strapCenter, -0.05f);
    glScalef(strapWidth, strapLength, strapDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    // ------------------------------------------------------------
    // Closed-loop rear connector.
    // This curved section sits behind the watch/wrist and joins the
    // two outer strap ends. It is intentionally below the watch case
    // so it becomes visible when the watch is rotated to its back.
    // ------------------------------------------------------------
    const int segments = 48;
    const float halfWidth = strapWidth * 0.5f;
    const float halfThickness = strapDepth * 0.5f;
    const float yRadius = endY;
    const float zRadius = 1.35f;
    const float zCenter = -0.10f;

    // Front/back surfaces of the curved leather band.
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float t = PI * static_cast<float>(i) / segments;

        float y = yRadius * cosf(t);
        float z = zCenter - zRadius * sinf(t);

        // Normal in the YZ plane, pointing outward from the ellipse.
        float ny = cosf(t);
        float nz = -sinf(t);

        glNormal3f(0.0f, ny, nz);
        glVertex3f(-halfWidth, y + ny * halfThickness, z + nz * halfThickness);
        glVertex3f(halfWidth, y + ny * halfThickness, z + nz * halfThickness);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float t = PI * static_cast<float>(i) / segments;

        float y = yRadius * cosf(t);
        float z = zCenter - zRadius * sinf(t);

        float ny = cosf(t);
        float nz = -sinf(t);

        glNormal3f(0.0f, -ny, -nz);
        glVertex3f(halfWidth, y - ny * halfThickness, z - nz * halfThickness);
        glVertex3f(-halfWidth, y - ny * halfThickness, z - nz * halfThickness);
    }
    glEnd();

    // Curved outer edges, keeping the rear connector looking like the
    // same leather strap rather than a separate wire or tube.
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float t = PI * static_cast<float>(i) / segments;

        float y = yRadius * cosf(t);
        float z = zCenter - zRadius * sinf(t);

        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-halfWidth, y + cosf(t) * halfThickness,
            z - sinf(t) * halfThickness);
        glVertex3f(-halfWidth, y - cosf(t) * halfThickness,
            z + sinf(t) * halfThickness);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float t = PI * static_cast<float>(i) / segments;

        float y = yRadius * cosf(t);
        float z = zCenter - zRadius * sinf(t);

        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(halfWidth, y - cosf(t) * halfThickness,
            z + sinf(t) * halfThickness);
        glVertex3f(halfWidth, y + cosf(t) * halfThickness,
            z - sinf(t) * halfThickness);
    }
    glEnd();

    // ------------------------------------------------------------
    // Original stitching on the two straight strap sections.
    // ------------------------------------------------------------
    setMaterial(0.72f, 0.48f, 0.20f, 0.15f, 10.0f);

    for (int side = -1; side <= 1; side += 2)
    {
        float x = side * 0.58f;

        for (int i = 0; i < 3; ++i)
        {
            float offset = (i - 1) * 0.13f;

            glPushMatrix();
            glTranslatef(x, strapCenter + offset, 0.08f);
            glScalef(0.025f, 0.06f, 0.025f);
            glutSolidCube(1.0);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(x, -strapCenter - offset, 0.08f);
            glScalef(0.025f, 0.06f, 0.025f);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    }

    // Stitching following the curved rear connector.
    for (int side = -1; side <= 1; side += 2)
    {
        float x = side * 0.31f;

        for (int i = 2; i < segments - 1; i += 3)
        {
            float t = PI * static_cast<float>(i) / segments;
            float y = yRadius * cosf(t);
            float z = zCenter - zRadius * sinf(t);

            glPushMatrix();
            glTranslatef(x, y, z + 0.03f);
            glScalef(0.018f, 0.035f, 0.018f);
            glutSolidCube(1.0);
            glPopMatrix();
        }
    }

    // Original lugs connecting each straight strap to the case.
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
    // Exact 12-position layout: 12 o'clock at +Y, then clockwise every 30 degrees.
    // Hour sticks and minute ticks use the same angular reference, so everything lines up.
    const float z = 0.43f;

    setMaterial(0.84f, 0.86f, 0.89f, 1.0f, 170.0f);

    for (int hour = 0; hour < 12; ++hour)
    {
        const float angleDeg = hour * 30.0f;
        const float angleRad = angleDeg * PI / 180.0f;
        const bool major = (hour % 3 == 0);

        const float innerRadius = major ? 0.93f : 0.97f;
        const float outerRadius = major ? 1.15f : 1.14f;
        const float length = outerRadius - innerRadius;
        const float radius = (innerRadius + outerRadius) * 0.5f;
        const float width = major ? 0.105f : 0.060f;

        const float x = radius * sinf(angleRad);
        const float y = radius * cosf(angleRad);

        glPushMatrix();
        glTranslatef(x, y, z);
        // A marker starts vertical at 12 o'clock and rotates clockwise with the dial.
        glRotatef(-angleDeg, 0.0f, 0.0f, 1.0f);
        glScalef(width, length, 0.055f);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Every minute tick is generated from the same exact 6-degree angular grid.
    // Five-minute positions are slightly stronger; the 12 hour sticks remain clearly visible.
    for (int tick = 0; tick < 60; ++tick)
    {
        const float angleDeg = tick * 6.0f;
        const float angleRad = angleDeg * PI / 180.0f;
        const bool fiveMinute = (tick % 5 == 0);

        setMaterial(
            fiveMinute ? 0.70f : 0.48f,
            fiveMinute ? 0.72f : 0.50f,
            fiveMinute ? 0.76f : 0.53f,
            0.85f,
            105.0f
        );

        const float outerRadius = 1.245f;
        const float innerRadius = fiveMinute ? 1.18f : 1.195f;

        glLineWidth(fiveMinute ? 2.0f : 1.0f);
        glBegin(GL_LINES);
        glVertex3f(
            outerRadius * sinf(angleRad),
            outerRadius * cosf(angleRad),
            0.415f
        );
        glVertex3f(
            innerRadius * sinf(angleRad),
            innerRadius * cosf(angleRad),
            0.415f
        );
        glEnd();
    }

    glLineWidth(1.0f);
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
    SYSTEMTIME st = getCurrentLocalTime();

    glPushMatrix();
    glTranslatef(0.92f, 0.0f, 0.43f);

    // White date disc/window.
    setMaterial(0.92f, 0.91f, 0.86f, 0.35f, 55.0f);
    glScalef(0.25f, 0.16f, 0.045f);
    glutSolidCube(1.0f);

    glDisable(GL_LIGHTING);
    glColor3f(0.06f, 0.06f, 0.065f);

    char dateText[4]{};
    sprintf_s(dateText, "%02d", static_cast<int>(st.wDay));

    // Center the two-digit date inside the window.
    glRasterPos3f(-0.065f, -0.035f, 0.03f);
    for (const char* p = dateText; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);

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
    // Take exactly one Windows time sample so all three hands stay synchronized.
    const SYSTEMTIME st = getCurrentLocalTime();

    const float seconds = static_cast<float>(st.wSecond);
    const float minutes = static_cast<float>(st.wMinute) + seconds / 60.0f;
    const float hours = static_cast<float>(st.wHour % 12) + minutes / 60.0f;

    const float hourAngle = hours * 30.0f;
    const float minuteAngle = minutes * 6.0f;
    const float secondAngle = seconds * 6.0f;

    // Hour hand.
    drawHand(
        0.67f, 0.115f, hourAngle,
        0.78f, 0.80f, 0.83f, 0.49f
    );

    // Minute hand.
    drawHand(
        0.98f, 0.075f, minuteAngle,
        0.86f, 0.87f, 0.89f, 0.51f
    );

    // Traditional ticking seconds hand: exactly one step per real second.
    // This prevents the hand from appearing to race around the dial.
    drawHand(
        1.10f, 0.025f, secondAngle,
        0.68f, 0.045f, 0.035f, 0.54f
    );

    // Seconds-hand counterweight.
    glPushMatrix();
    glRotatef(-secondAngle, 0, 0, 1);
    glTranslatef(0.0f, -0.22f, 0.54f);

    setMaterial(0.65f, 0.04f, 0.03f, 0.9f, 130.0f);
    glScalef(0.04f, 0.22f, 0.025f);
    glutSolidCube(1.0f);

    glPopMatrix();

    // Center jewel/pivot.
    setMaterial(0.80f, 0.58f, 0.16f, 1.0f, 180.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.59f);
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

    drawWatch();

    glutSwapBuffers();
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
    // Keep the scene responsive while the timer below guarantees
    // regular real-time clock/date refreshes.
    glutPostRedisplay();
}


// ---------- Real-time refresh timer ----------
void refreshClock(int)
{
    glutPostRedisplay();
    glutTimerFunc(50, refreshClock, 0); // ~20 FPS refresh; hands still tick in real seconds
}

// ---------- Initialization ----------
void initScene()
{
    // Premium dark studio background instead of the old bright-green test color.
    glClearColor(0.012f, 0.018f, 0.030f, 1.0f);

    initLighting();

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    glutTimerFunc(50, refreshClock, 0);

    glutMainLoop();

    return 0;
}
