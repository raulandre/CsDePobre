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
	pipe(pipe_p1); pipe(pipe_p2);

	for (int i = 0; i < MAX_COLUMNS; i++)
	{
		heights[i] = (float)GetRandomValue(1, 12);
		positions[i] = (Vector3){ (float)GetRandomValue(-15, 15), heights[i]/2.0f, (float)GetRandomValue(-15, 15) };
		colors[i] = (Color){ GetRandomValue(20, 255), GetRandomValue(10, 55), 30, 255 };
	}

	pthread_t enemyReceiver;
	pid_t pid = fork();
	if(pid > 0) {
		processPlayer(1, &p, &ep, &enemyReceiver, pipe_p1[1], pipe_p2[0]);
	} else if(pid == 0) {
		processPlayer(2, &p, &ep, &enemyReceiver, pipe_p2[1], pipe_p1[0]);
	}

	return EXIT_SUCCESS;
}

void *fetchEnemyPlayer(void *rid) {
	int read_id = *(int*)rid;
	while(1) {
		read(read_id, &ep, sizeof(Player)); 
	}
	return EXIT_SUCCESS;
}

void processPlayer(int id, Player *p, Player *ep, pthread_t *enemyReceiver, pid_t write_id , pid_t read_id) {
	p->id = id;
	p->dead = 0;
	p->camera.position = (Vector3) { -5.0, 3.0, 0.0 };
	p->camera.target = (Vector3) { 5.0, 3.0, 0.0 };
	p->camera.up = (Vector3) { 0.0, 1.0, 0.0 };
	p->camera.fovy = 60.0;
	p->camera.projection = CAMERA_PERSPECTIVE;

	pthread_create(enemyReceiver, NULL, fetchEnemyPlayer, &read_id);

	InitWindow(WIDTH, HEIGHT, TextFormat("Player %d", id));
	SetTargetFPS(60);
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

		write(write_id, p, sizeof(Player));
		BeginDrawing();
		ClearBackground(SKYBLUE);
		BeginMode3D(p->camera);
		DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY);
		if(!ep->dead)
			DrawCylinder(pos, 1, 1, 3, 100, PINK);
		for (int i = 0; i < MAX_COLUMNS; i++)
		{
			DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
			DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, MAROON);
		}
		EndMode3D();
			DrawText("+", WIDTH / 2 - crosshairPos, HEIGHT / 2 - crosshairPos, CROSSHAIR_SIZE, CROSSHAIR_COLOR);
		EndDrawing();
	}

	p->dead = 1;
	write(write_id, p, sizeof(Player));
	pthread_cancel(*enemyReceiver);
	close(write_id);
	close(read_id);
}
