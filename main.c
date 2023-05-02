#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "raylib.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_COLUMNS 20
#define CROSSHAIR "+"
#define CROSSHAIR_SIZE 28
#define CROSSHAIR_COLOR DARKGREEN

// [0] para leitura, [1] para escrita
int pipe_p1[2];
int pipe_p2[2];

typedef struct {
	int id;
	Camera3D camera;
	bool dead;
} Player;

Player p, ep;
float heights[MAX_COLUMNS] = { 0 };
Vector3 positions[MAX_COLUMNS] = { 0 };
Color colors[MAX_COLUMNS] = { 0 };

void *fetchEnemyPlayer(void *rid);
void processPlayer(int id, Player *p, Player *ep, pthread_t *enemyReceiver, pid_t write_id, pid_t read_id);

int main(void) {
	if(pipe(pipe_p1) < 0 ||  pipe(pipe_p2) < 0) {
		fprintf(stderr, "Erro no procedimento pipe\n");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < MAX_COLUMNS; i++) {
		positions[i] = (Vector3){ GetRandomValue(-15, 15), heights[i]/2.0f, GetRandomValue(-15, 15) };
		heights[i] = GetRandomValue(1, 12);
		if(GetRandomValue(1, 2) == 1) {
			colors[i] = YELLOW;
		} else {
			colors[i] = SKYBLUE;
		}
	}

	pthread_t enemyReceiver;
	pid_t pid = fork();
	if(pid > 0) {
		processPlayer(1, &p, &ep, &enemyReceiver, pipe_p1[1], pipe_p2[0]);
	} else if(pid == 0) {
		processPlayer(2, &p, &ep, &enemyReceiver, pipe_p2[1], pipe_p1[0]);
	} else {
		fprintf(stderr, "Erro no procedimento fork\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void *fetchEnemyPlayer(void *rid) {
	int read_id = *(int*)rid;
	while(!ep.dead) {
		if(read(read_id, &ep, sizeof(Player)) != sizeof(Player)) {
			fprintf(stderr, "\nFalha ao ler player no pipe %d\n", read_id);
		}
	}
	return EXIT_SUCCESS;
}

void processPlayer(int id, Player *p, Player *ep, pthread_t *enemyReceiver, pid_t write_id , pid_t read_id) {
	p->id = id;
	p->dead = 0;
	p->camera.position = (Vector3) { -5.0, 3.0, 0.0 }; if(id == 2) { p->camera.position.x *= -1; }
	p->camera.target = (Vector3) { 5.0, 3.0, 0.0 }; if(id == 2) { p->camera.target.x *= - 1; }
	p->camera.up = (Vector3) { 0.0, 1.0, 0.0 };
	p->camera.fovy = 60.0;
	p->camera.projection = CAMERA_PERSPECTIVE;

	if(pthread_create(enemyReceiver, NULL, fetchEnemyPlayer, &read_id) != 0) {
		fprintf(stderr, "Erro ao criar thread\n");
		exit(EXIT_FAILURE);
	}

	SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(WIDTH, HEIGHT, TextFormat("Player %d", id));
	SetTargetFPS(60);

	switch(id) {
		case 1:
			SetWindowPosition(0, GetScreenHeight() / 2);
			break;
		case 2:
			SetWindowPosition(GetScreenWidth() / 2 + WIDTH, GetScreenHeight() / 2);
			break;
	}

	bool cursorDisabled = false;
	while(!WindowShouldClose()) {
		if(cursorDisabled)
			UpdateCamera(&p->camera, CAMERA_FIRST_PERSON);

		if(IsKeyPressed(KEY_SPACE)) {
			DisableCursor();
			cursorDisabled = true;
		} else if(IsKeyPressed(KEY_LEFT_ALT)) {
			EnableCursor();
			cursorDisabled = false;
		}

		Vector3 pos = (Vector3){ ep->camera.position.x, ep->camera.position.y - 3, ep->camera.position.z };
		int crosshairPos = MeasureText(CROSSHAIR, 28);

		if(!ep->dead) {
			if(write(write_id, p, sizeof(Player)) != sizeof(Player)) {
				fprintf(stderr, "\nErro ao escrever para o player %d\n", (id == 1 ? 2 : 1));
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);
		if(!cursorDisabled)
			DrawText("Pressione espaço para interagir", 10, 10, 16, GREEN);
		else
			DrawText("Pressione alt esq. para liberar o cursor", 10, 10, 16, GREEN);
		BeginMode3D(p->camera);
		DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY);

		if(!ep->dead) {
			DrawCylinder(pos, 1, 1, 3, 100, PINK);
		}

		for (int i = 0; i < MAX_COLUMNS; i++)
		{
			DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
			DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, WHITE);
		}
		EndMode3D();

		DrawText("+", WIDTH / 2 - crosshairPos / 2, HEIGHT / 2 - crosshairPos, CROSSHAIR_SIZE, CROSSHAIR_COLOR);
		EndDrawing();
	}

	p->dead = 1;
	write(write_id, p, sizeof(Player));
	pthread_cancel(*enemyReceiver);
	close(write_id);
	close(read_id);
	exit(EXIT_SUCCESS);
}
