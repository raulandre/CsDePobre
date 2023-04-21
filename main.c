#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "raylib.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_COLUMNS 20

// [0] para leitura, [1] para escrita
int pipe_p1[2];
int pipe_p2[2];

typedef struct {
	int id;
	Camera3D camera;
	bool dead;
} Player;
Player p, ep;

void *fetchEnemyPlayer() {
	while(1) {
		if(p.id == 1) {
			read(pipe_p2[0], &ep, sizeof(Player)); //Player 2 escreve em pipe_p1[1]
		} else if(p.id == 2) {
			read(pipe_p1[0], &ep, sizeof(Player)); //Player 1 escreve em pipe_p2[1]
		}
	}
	return NULL;
}

int main(void) {
	pipe(pipe_p1); pipe(pipe_p2);
	float heights[MAX_COLUMNS] = { 0 };
	Vector3 positions[MAX_COLUMNS] = { 0 };
	Color colors[MAX_COLUMNS] = { 0 };

	for (int i = 0; i < MAX_COLUMNS; i++)
	{
		heights[i] = (float)GetRandomValue(1, 12);
		positions[i] = (Vector3){ (float)GetRandomValue(-15, 15), heights[i]/2.0f, (float)GetRandomValue(-15, 15) };
		colors[i] = (Color){ GetRandomValue(20, 255), GetRandomValue(10, 55), 30, 255 };
	}

	pthread_t enemyReceiver;
	pid_t pid = fork();
	if(pid > 0) {
		p.id = 1;
		p.dead = 0;
		p.camera.position = (Vector3) { -5.0, 3.0, 0.0 };
		p.camera.target = (Vector3) { 5.0, 3.0, 0.0 };
		p.camera.up = (Vector3) { 0.0, 1.0, 0.0 };
		p.camera.fovy = 60.0;
		p.camera.projection = CAMERA_PERSPECTIVE;

		pthread_create(&enemyReceiver, NULL, fetchEnemyPlayer, &ep);

		InitWindow(WIDTH, HEIGHT, "Player 1");
		SetTargetFPS(60);
		while(!WindowShouldClose()) {
			UpdateCamera(&p.camera, CAMERA_FIRST_PERSON);
			if(IsKeyPressed(KEY_SPACE)) {
				DisableCursor();
			} else if(IsKeyPressed(KEY_LEFT_ALT)) {
				EnableCursor();
			}
			write(pipe_p1[1], &p, sizeof(Player));
			BeginDrawing();
			ClearBackground(SKYBLUE);
			BeginMode3D(p.camera);
			DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY);
			DrawSphere(ep.camera.position, 1, PINK);
			for (int i = 0; i < MAX_COLUMNS; i++)
			{
				DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
				DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, MAROON);
			}
			EndMode3D();
			EndDrawing();
		}

		pthread_join(enemyReceiver, NULL);
	} else if(pid == 0) {
		p.id = 2;
		p.dead = 0;
		p.camera.position = (Vector3) { 5.0, 3.0, 0.0 };
		p.camera.target = (Vector3) { -5.0, 3.0, 0.0 };
		p.camera.up = (Vector3) { 0.0, 1.0, 0.0 };
		p.camera.fovy = 60.0;
		p.camera.projection = CAMERA_PERSPECTIVE;

		pthread_create(&enemyReceiver, NULL, fetchEnemyPlayer, &ep);

		InitWindow(WIDTH, HEIGHT, "Player 2");
		SetTargetFPS(60);
		while(!WindowShouldClose()) {
			UpdateCamera(&p.camera, CAMERA_FIRST_PERSON);
			if(IsKeyPressed(KEY_SPACE)) {
				DisableCursor();
			} else if(IsKeyPressed(KEY_LEFT_ALT)) {
				EnableCursor();
			}
			write(pipe_p2[1], &p, sizeof(Player));
			BeginDrawing();
			ClearBackground(SKYBLUE);
			BeginMode3D(p.camera);
			DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY);
			DrawSphere(ep.camera.position, 1, PINK);
			for (int i = 0; i < MAX_COLUMNS; i++)
			{
				DrawCube(positions[i], 2.0f, heights[i], 2.0f, colors[i]);
				DrawCubeWires(positions[i], 2.0f, heights[i], 2.0f, MAROON);
			}
			EndMode3D();
			EndDrawing();
		}

		pthread_join(enemyReceiver, NULL);
	}

	return EXIT_SUCCESS;
}
