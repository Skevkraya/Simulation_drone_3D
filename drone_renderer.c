#define GL_SILENCE_DEPRECATION 
#include <SDL/SDL_gfxPrimitives.h>
#include "lib.h"
#include "drone.h"
#include "input.h"
#include "pid.h"
#include "math3d.h" 


// --- Fonctions de dessin des formes primitives ---

void drawCylinder(GLUquadric *q, float radius, float height) {
    gluCylinder(q, radius, radius, height, 16, 1);
}

// un disque 
void drawDisk(GLUquadric *q, float innerRadius, float outerRadius) {
    gluDisk(q, innerRadius, outerRadius, 16, 1);
}

void drawAxes(float length) {
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(length, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, length, 0); 
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, length); 
    glEnd();
}
// --- Fonctions de dessin des composants du drone ---
void drawPropellerBlade(GLUquadric *q, float length, float width) {
    // Couleurs corrigées pour être dans la plage [0.0, 1.0]
    glColor3f(0.2f, 0.5f, 0.9f); 

    glBegin(GL_QUADS);
        // Face supérieure de la pale
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-width / 2, 0.0f, 0.0f);
        glVertex3f(width / 2, 0.0f, 0.0f);
        glVertex3f(width / 2, length, 0.0f);
        glVertex3f(-width / 2, length, 0.0f);

        // Face inférieure (légèrement décalée pour l'épaisseur)
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-width / 2, 0.0f, -0.005f);
        glVertex3f(width / 2, 0.0f, -0.005f);
        glVertex3f(width / 2, length, -0.005f);
        glVertex3f(-width / 2, length, -0.005f);
    glEnd();

    // Petits côtés
    glBegin(GL_QUAD_STRIP);
        glNormal3f(1.0f, 0.0f, 0.0f); //  côté droit
        glVertex3f(width / 2, 0.0f, 0.0f);
        glVertex3f(width / 2, 0.0f, -0.005f);
        glVertex3f(width / 2, length, 0.0f);
        glVertex3f(width / 2, length, -0.005f);
        
        glNormal3f(-1.0f, 0.0f, 0.0f); // côté gauche
        glVertex3f(-width / 2, length, 0.0f);
        glVertex3f(-width / 2, length, -0.005f);
        glVertex3f(-width / 2, 0.0f, 0.0f);
        glVertex3f(-width / 2, 0.0f, -0.005f);
    glEnd();
}
void drawMotor(GLUquadric *q, float motorRadius, float bladeLength, float bladeWidth, float currentBladeAngle) {
    glPushMatrix();
        float motorHeight = motorRadius * 0.8f;
        
       
        glColor3f(0.3f, 0.3f, 0.3f); // Gris foncé
        
        drawCylinder(q, motorRadius, motorHeight); 
        
        // Dessus du moteur (disque)
        glPushMatrix();
            glTranslatef(0, 0, motorHeight); 
            drawDisk(q, 0.0f, motorRadius); 
        glPopMatrix();

        // Base de l'hélice 
        glPushMatrix();
            glTranslatef(0.0f, 0.0f, motorHeight + 0.01f); 
            glColor3f(0.7f, 0.0f, 0.0f); 
            drawCylinder(q, motorRadius * 0.4f, motorRadius * 0.2f); 
        glPopMatrix();

        // Hélices
        glPushMatrix(); 
            glTranslatef(0.0f, 0.0f, motorHeight + 0.01f + motorRadius * 0.2f); 
            glRotatef(currentBladeAngle, 0, 0, 1);

            // Première pale
            glPushMatrix();
                glTranslatef(-bladeWidth / 2, 0.0f, 0.0f); 
                drawPropellerBlade(q, bladeLength, bladeWidth);
            glPopMatrix();

            // Deuxième pale (opposée)
            glPushMatrix();
                glRotatef(180, 0, 0, 1); 
                glTranslatef(-bladeWidth / 2, 0.0f, 0.0f); 
                drawPropellerBlade(q, bladeLength, bladeWidth);
            glPopMatrix();
        glPopMatrix();
    glPopMatrix();
}


void drawFoot(GLUquadric *q, float radius, float height) {
    glColor3f(0.4f, 0.4f, 0.4f); 
    glPushMatrix();
       
        glTranslatef(0.0f, 0.0f, -height); 
        drawCylinder(q, radius, height);
        
        gluSphere(q, radius * 1.5f, 8, 8); 
    glPopMatrix();
}


// Fonction pour dessiner le corps principal du drone
void drawDroneBody(GLUquadric *q, float length, float width, float height) {
    glColor3f(0.1f, 0.1f, 0.1f); // Couleur principale (noir mat)

    glPushMatrix();
        glScalef(length, width, height); // Ces paramètres mettent à l'échelle les coordonnées ci-dessous

        glBegin(GL_QUADS);
           //le haut
            glNormal3f(0.0f, 0.0f, 1.0f); 
            glVertex3f(-0.5f, -0.3f, 0.5f);  
            glVertex3f(0.5f, -0.3f, 0.5f);   
            glVertex3f(0.5f, 0.3f, 0.5f);    
            glVertex3f(-0.5f, 0.3f, 0.5f); 


           // le bas
           glNormal3f(0.0f, 0.0f, -1.0f); 
           glVertex3f(-0.5f, -0.3f, -0.5f); 
           glVertex3f(-0.5f, 0.3f, -0.5f); 
           glVertex3f(0.5f, 0.3f, -0.5f);   
           glVertex3f(0.5f, -0.3f, -0.5f);  

            // arrière
            glNormal3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.5f, -0.3f, 0.5f);  
            glVertex3f(0.5f, -0.3f, -0.5f);  
            glVertex3f(0.5f, 0.3f, -0.5f);   
            glVertex3f(0.5f, 0.3f, 0.5f);  
            // face avant
            glNormal3f(-1.0f, 0.0f, 0.0f);
            glVertex3f(-0.5f, -0.3f, 0.5f);  
            glVertex3f(-0.5f, 0.3f, 0.5f);   
            glVertex3f(-0.5f, 0.3f, -0.5f); 
            glVertex3f(-0.5f, -0.3f, -0.5f); 
             // face gauche (fermé)
             glNormal3f(0.0f, 1.0f, 0.0f);
             glVertex3f(-0.5f, 0.3f, 0.5f);   
             glVertex3f(0.5f, 0.3f, 0.5f);   
             glVertex3f(0.5f, 0.3f, -0.5f);   
             glVertex3f(-0.5f, 0.3f, -0.5f); 

           // face droite  
           glNormal3f(0.0f, -1.0f, 0.0f);
           glVertex3f(-0.5f, -0.3f, 0.5f);  
           glVertex3f(0.5f, -0.3f, 0.5f);  
           glVertex3f(0.5f, -0.3f, -0.5f);  
           glVertex3f(-0.5f, -0.3f, -0.5f); 
        glEnd();
    glPopMatrix();

    // Capteur avant (rouge)
    glPushMatrix();
        
        glTranslatef(0.0f, 0.0f, 0.5f * height + 0.05f); 
        glColor3f(1.0f, 0.0f, 0.0f); 
        gluSphere(q, 0.05f, 8, 8);
    glPopMatrix();

    // Antennes 
    glColor3f(0.3f, 0.3f, 0.3f); 
    glPushMatrix();
       
        glTranslatef(-0.2f * length, 0.3f * width + 0.1f, 0.0f); 
        glRotatef(45.0f, 1.0f, 0.0f, 0.0f); 
        drawCylinder(q, 0.01f, 0.3f);
    glPopMatrix();
    glPushMatrix();
        // Antenne droite
        glTranslatef(0.2f * length, 0.3f * width + 0.1f, 0.0f);
        glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
        drawCylinder(q, 0.01f, 0.3f);
    glPopMatrix();
}
// --- Fonction principale de rendu du drone ---
void renderDrone(Drone *drone, float armLength, float armRadius) {
    glPushMatrix(); 
    GLUquadric* quad = gluNewQuadric();
  
    glTranslatef(drone->pos.x, drone->pos.y, drone->pos.z);
    glRotatef(drone->rot[2], 0, 0, 1); // Lacet global du drone
    glRotatef(drone->rot[1], 0, 1, 0); // Tangage global du drone
    glRotatef(drone->rot[0], 1, 0, 0); // Roulis global du drone

    glPushMatrix();
      
        drawDroneBody(quad, 1.5f, 0.5f, 0.5f);

    glPopMatrix();

    // 3. Dessiner les bras du drone
    glColor3f(0.6f, 0.6f, 0.6f);

    // Bras le long de l'axe X (avant/arrière)
    glPushMatrix();
        glRotatef(90, 0, 1, 0); 
        glTranslatef(0, 0, armLength / 2.0f);
        drawCylinder(quad, armRadius,  armLength);
    glPopMatrix();

    // Bras le long de l'axe Y (gauche/droite)
    glPushMatrix();
        glRotatef(90, -1, 0, 0);
        glTranslatef(0, 0, armLength / 2.0f);
        drawCylinder(quad, armRadius, armLength);
    glPopMatrix();

    float footHeight = 0.2f;
    float footRadius = 0.02f;
    float footSpread = 0.05f; 

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();

            glPushMatrix();
                glTranslatef(footSpread, 0.0f, 0.0f); // Décalage latéral du pied
                drawFoot(quad, footRadius, footHeight);
            glPopMatrix();

            
            glPushMatrix();
                glTranslatef(-footSpread, 0.0f, 0.0f); 
                drawFoot(quad, footRadius, footHeight);
            glPopMatrix();

        glPopMatrix(); 
    }
    
    glPopMatrix(); 
}


