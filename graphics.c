#include "lib.h"
#include "graphics.h"

void init_SDL() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(800, 600, 32, SDL_OPENGL);
    SDL_WM_SetCaption("Simulation Drone 3D", NULL);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    gluPerspective(60.0, 800.0/600.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);  // fond bleu foncé
}

void update_camera(const Drone *drone,
                   float distance_derriere,
                   float hauteur_cam,
                   float target_offset_y)
{
    // 1. on est bien en MODELVIEW
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 2. calcul du yaw en radians
    float yaw_rad = drone->rot[1] * (M_PI / 180.0f);
    float fx = sinf(yaw_rad), fz = cosf(yaw_rad);

    // 3. position de la caméra
    float camX = drone->pos.x- fx * distance_derriere;
    float camY = drone->pos.y + hauteur_cam;
    float camZ = drone->pos.z - fz * distance_derriere;

    // 4. point visé (on ajoute target_offset_y, typiquement négatif)
    float tgtX = drone->pos.x + fx;
    float tgtY = drone->pos.y + target_offset_y;
    float tgtZ = drone->pos.z + fz;

    // 5. on appelle gluLookAt
    gluLookAt(
        camX,    camY,    camZ,
        tgtX,    tgtY,    tgtZ,
        0.0f,    1.0f,    0.0f
    );
}

void render_drone(Drone *drone, float armLength, float armRadius, float diskRadius) {
    // Sauve l'état de la matrice
    glPushMatrix();

    // 1) Déplace le drone à sa position
    glTranslatef(drone->pos.x,
                 drone->pos.y,
                 drone->pos.z);

    // 2) Applique les rotations dans l'ordre yaw → pitch → roll
    //    pour qu'on tourne d'abord autour de l'axe Y (lacet),
    //    puis X (tangage), puis Z (roulis).
    glRotatef(drone->rot[1], 0.0f, 1.0f, 0.0f);  // yaw
    glRotatef(drone->rot[0], 1.0f, 0.0f, 0.0f);  // pitch
    glRotatef(drone->rot[2], 0.0f, 0.0f, 1.0f);  // roll

    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);

    // 3) On dessine les bras et les disques aux extrémités
    GLUquadric *q = gluNewQuadric();

    // -- Bras sur l’axe X
    glColor3f(0.6f, 0.6f, 0.6f);
    glPushMatrix();
      glTranslatef(-armLength/2, 0.0f, 0.0f);
      glRotatef(90.0f, 0, 1, 0);
      gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // -- Bras sur l’axe Z
    glPushMatrix();
      glTranslatef(0.0f, 0.0f, -armLength/2);
      glRotatef(90.0f, 0, 0, 1);
      gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // -- 4 disques (hélices) aux extrémités
    glColor3f(1.0f, 0.2f, 0.2f);
    const float half = armLength / 2;
    for (int i = 0; i < 4; ++i) {
        // positions : ( +X, 0, 0 ), ( -X, 0, 0 ), (0, 0, +Z), (0, 0, -Z)
        float x = (i==0 ?  half : (i==1 ? -half : 0.0f));
        float z = (i>=2 ? (i==2 ?  half : -half) : 0.0f);

        glPushMatrix();
          glTranslatef(x, 0.1f, z);
          glRotatef(-90.0f, 1, 0, 0);
          gluDisk(q, 0.0, diskRadius, 16, 1);
        glPopMatrix();
    }

    gluDeleteQuadric(q);

    // 4) Restaure la matrice
    glPopMatrix();
}

void render_ground() {
    // Couleur du sol
    glColor3f(0.0f, 0.6f, 0.0f);  // vert
    glBegin(GL_QUADS);
    glVertex3f(-50.0f, 0.0f, -50.0f);
    glVertex3f( 50.0f, 0.0f, -50.0f);
    glVertex3f( 50.0f, 0.0f,  50.0f);
    glVertex3f(-50.0f, 0.0f,  50.0f);
    glEnd();

    // Grille noir
    glColor3f(0.0f, 0.0f, 0.0f);  // noir
    glBegin(GL_LINES);
    for (float i = -50.0f; i <= 50.0f; i += 1.0f) {
        // lignes parallèles à X
        glVertex3f(i, 0.01f, -50.0f);
        glVertex3f(i, 0.01f,  50.0f);

        // lignes parallèles à Z
        glVertex3f(-50.0f, 0.01f, i);
        glVertex3f( 50.0f, 0.01f, i);
    }
    glEnd();
}



void affichage(Drone * drone) {
    glLoadIdentity();
    update_camera(drone,
                  /*distance*/         10.0f,
                  /*hauteur_cam*/       5.0f,
                  /*target_offset_y*/  -1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_ground();
    render_drone(drone, 3.0f, 0.1f, 0.5f);
    SDL_GL_SwapBuffers();
}