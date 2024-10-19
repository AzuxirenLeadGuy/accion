#include "galton_board.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>

void cycle_color(Color *cd) {
	Color c = *cd;
	if (c.a == 255) {
		c.b--;
		if (c.b == 0) {
			c.a = 254;
		}

	} else if (c.a == 254) {
		c.g--;
		if (c.g == 0) {
			c.a = 253;
		}

	} else if (c.a == 253) {
		c.b++;
		if (c.b == 255) {
			c.a = 252;
		}

	} else if (c.a == 252) {
		c.r--;
		if (c.r == 0) {
			c.a = 251;
		}

	} else if (c.a == 251) {
		c.g++;
		if (c.g == 255) {
			c.a = 250;
		}
	} else if (c.a == 250) {
		c.r++;
		if (c.r == 255) {
			c.a = 255;
		}
	}
	*cd = c;
}

int main(void) {

	const int unit = 48;
	galton_board_t *board = galton_board__init(5, 255, unit, unit);

	if (board == 0) {
		return -1;
	}

	uint8_t state = 0;

	int return_error;

	InitWindow(
		galton_constants.screen_width,
		galton_constants.screen_height,
		"Raylib project: accion");

	SetTargetFPS(60);

	Color c = {255, 255, 255, 255};

	const point2d_t particle_spawn_point = {
		galton_constants.screen_width / 2,
		galton_constants.screen_height / 20};

	const point2d_t funnel_origin = {
		particle_spawn_point.x, particle_spawn_point.y / 2};

	// DrawPoly(center, 3, 3, 0, RED);
	const float reset_timer_value =
		galton_constants.particle_spawn_milliseconds / 1000.0f;
	float current_timer = 1;

	const uint16_t ball_batch_size =
		1 << galton_constants.particle_batch_size_2_power;
	uint32_t i;
	uint32_t batch_remaining = ball_batch_size, max_this_instance;

	char remain_text[128];

	while (!WindowShouldClose()) {

		// Turn timer and spawn a new particle (if not exceeded
		// MAX_GALTON_PARTICLES)

		current_timer -= GetFrameTime();
		if (state == 0) { // batch_remaining > 0
			if (current_timer <= 0) {
				if (galton_board__add(board) != 0) {
					current_timer = 0;
				} else {
					batch_remaining--;
					current_timer = reset_timer_value;
					if (batch_remaining == 0) {
						state = 1;
					}
				}
			}
		} else if (state == 1) {
			// batch_remaining == 0, active_particles > 0
			if (board->active_particles == 0) {
				current_timer = reset_timer_value * 8;
				state = 2;
			}

		} else if (state >= 2 && current_timer <= 0) {
			state = 0;
			galton_board__reset_count(board);
			current_timer = reset_timer_value;
			batch_remaining = ball_batch_size;
		}

		// Update particles
		return_error = galton_board__update(board);
		if (return_error != 0) {
			CloseWindow();
			break;
		}
		max_this_instance = 0;
		for (i = 0; i < board->result_stack_size; i++) {
			if (board->result_stack[i] > max_this_instance) {
				max_this_instance = board->result_stack[i];
			}
		}
		sprintf(remain_text, "Remaining: %d", batch_remaining);
		ClearBackground(WHITE);
		BeginDrawing();

		// Draw title
		DrawText("Galton Board", 0, 0, unit, c);
		DrawText(
			remain_text,
			funnel_origin.x + unit,
			0,
			unit / 2.0,
			BLACK);

		// Draw funnel
		Vector2 pt = {funnel_origin.x, funnel_origin.y};
		DrawPoly(pt, 3, unit / 2.0, 90, YELLOW);

		// Draw particles
		for (i = 0; i < board->active_particles; i++) {
			Vector2 pt = (Vector2){
				particle_spawn_point.x, particle_spawn_point.y};
			return_error =
				galton_board__plot_ball(board, i, &pt.x, &pt.y);
			if (return_error != 0) {
				CloseWindow();
				break;
			}

			DrawCircle(pt.x, pt.y, unit / 4.0, RED);
		}

		// Draw each bouncer
		float unit_bc = unit / 4.0;
		for (i = 0; i < board->bouncer_center_size; i++) {
			pt.x =
				particle_spawn_point.x + board->bouncer_center[i].x;
			pt.y = particle_spawn_point.y +
				   board->bouncer_center[i].y + unit_bc / 2;
			DrawPoly(pt, 3, unit_bc, -90, GRAY);
		}

		for (i = 0; i < board->result_stack_size; i++) {
			Vector2 pt = (Vector2){
				particle_spawn_point.x, particle_spawn_point.y};
			return_error = galton_board__plot_result_stack(
				board, i, &pt.x, &pt.y);
			float bar_length = board->result_stack[i],
				  width = unit / 2.5;
			bar_length /= max_this_instance;
			bar_length *= galton_constants.screen_height - pt.y;
			pt.x -= width / 2;
			DrawRectangleV(
				pt, (Vector2){.x = width, .y = bar_length}, GREEN);

			if (state != 0) {
				sprintf(remain_text, "%lu", board->result_stack[i]);
				DrawText(
					remain_text,
					pt.x,
					pt.y,
					unit / 1.5,
					(Color){128, 0, 64, 255});
			}
		}

		EndDrawing();
		cycle_color(&c);
	}
	return_error += galton_board__free(board);
	printf("Returned with error %d\n\n", return_error);
	return return_error;
}
