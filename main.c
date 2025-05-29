#define SDL_MAIN_HANDLED
#include "lib.h"
#include "drone.h"
#include "input.h"
#include "./map/main/map_main.h"
#include "pid.h"

// gcc main.c drone.c graphics.c input.c  -o drone3D.exe -lmingw32 -lSDLmain -lSDL -lSDL_gfx -lm -lopengl32 -lglu32

int running = 1;
int main(int argc, char *argv[]) {
    // Pour la console de debug sur Windows
    #ifdef _WIN32
    AllocConsole();
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
    freopen("CON", "r", stdin);
    #endif
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_OPENGL);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.2f, 0.4f, 1.0f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_pos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    srand(time(NULL));
    Drone drone;
    init_drone(&drone);
    init_pids();
    if (!menu(&drone)) {
    fprintf(stderr, "Erreur lors du chargement/génération du chunk. Quitte.\n");
    return EXIT_FAILURE;
    }
    printf("ok avant perlin \n");
    init_perlin();
    printf("ok après perlin \n");
    
    // État initial du drone
    printf("ok après memset \n");

    control_init();

    double dt = 5e-2; // 0.05s
    SDL_EnableKeyRepeat(200, SDL_DEFAULT_REPEAT_INTERVAL);
    while(running) {
        SDL_event_listener(&drone, dt);
        drone_update(&drone, dt);
        updateChunks();
    }

    SDL_Delay(1000);

    SDL_Quit();
    exit(EXIT_SUCCESS);
}
