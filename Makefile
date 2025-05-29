# --------------------------------------------------------------------
# Configuration
CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -Imap
LDFLAGS  := -lm -lmingw32 -lSDLmain -lSDL -lSDL_gfx -lopengl32 -lglu32 -lSDL_ttf

# Sources principales
ROOT_SRCS := drone.c input.c main.c math3d.c pid.c map/map_perlin.c test.c

# Objets
ROOT_OBJS := $(ROOT_SRCS:.c=.o)
OBJS      := $(ROOT_OBJS)

# Nom de l'exécutable
TARGET := droneSim.exe

.PHONY: all clean

# cible par défaut
all: $(TARGET)

# règle de création de l'exécutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# règle générique pour les .c à la racine
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# nettoyage
clean:
	@rm -f $(OBJS) $(TARGET) *.exe

# gcc main.c drone.c map/map_perlin.c input.c math3d.c pid.c -o droneSim.exe -lmingw32 -lSDLmain -lSDL -lSDL_gfx -lopengl32 -lglu32 -lm -lSDL_ttf