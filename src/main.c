#include "galton_board.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>

const uint8_t cmax = 255;

void cycle_color(Color *color_p) {
	Color color_v = *color_p;
	if (color_v.a == cmax) {
		color_v.b--;
		if (color_v.b == 0) {
			color_v.a = cmax - 1;
		}

	} else if (color_v.a == cmax - 1) {
		color_v.g--;
		if (color_v.g == 0) {
			color_v.a = cmax - 2;
		}

	} else if (color_v.a == cmax - 2) {
		color_v.b++;
		if (color_v.b == cmax) {
			color_v.a = cmax - 3;
		}

	} else if (color_v.a == cmax - 3) {
		color_v.r--;
		if (color_v.r == 0) {
			color_v.a = cmax - 4;
		}

	} else if (color_v.a == cmax - 4) {
		color_v.g++;
		if (color_v.g == cmax) {
			color_v.a = cmax - 4 - 1;
		}
	} else if (color_v.a == cmax - 4 - 1) {
		color_v.r++;
		if (color_v.r == cmax) {
			color_v.a = cmax;
		}
	}
	*color_p = color_v;
}

int main(void) {

	const uint8_t unit = 48;
	galton_board_t *board =
		galton_board__init(4 + 1, cmax, unit, unit);

	if (board == 0) {
		return -1;
	}

	uint8_t state = 0;

	int return_error;

	InitWindow(
		galton_constants.screen_width,
		galton_constants.screen_height,
		"Raylib project: accion");

	SetTargetFPS(4 * 3 * 2 * (4 + 1));

	Color back_color = {cmax / 2, cmax / 2, cmax / 2, cmax};

	const point2d_t particle_spawn_point = {
		(int16_t)(galton_constants.screen_width / 2),
		(int16_t)(galton_constants.screen_height / 20)};

	const point2d_t funnel_origin = {
		(int16_t)(particle_spawn_point.x),
		(int16_t)(particle_spawn_point.y / 2)};

	// DrawPoly(center, 3, 3, 0, RED);
	const float reset_timer_value =
		(float)galton_constants.particle_spawn_milliseconds / 1000.0F;
	float current_timer = 1;

	const uint16_t ball_batch_size =
		(uint16_t)(1 << galton_constants.particle_batch_size_2_power);
	uint32_t idx;
	uint32_t batch_remaining = ball_batch_size;
	uint32_t max_this_instance;

	char remain_text[cmax / 2];

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
				current_timer = reset_timer_value * 4 + 4;
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
		for (idx = 0; idx < board->result_stack_size; idx++) {
			if (board->result_stack[idx] > max_this_instance) {
				max_this_instance =
					(uint32_t)(board->result_stack[idx]);
			}
		}
		sprintf(remain_text, "Remaining: %d", batch_remaining);
		ClearBackground((Color){cmax, cmax, cmax, cmax});
		BeginDrawing();

		// Draw title
		DrawText("Galton Board", 0, 0, unit, back_color);
		DrawText(
			remain_text, funnel_origin.x + unit, 0, unit / 2, BLACK);

		// Draw funnel
		Vector2 point_v = {funnel_origin.x, funnel_origin.y};
		const float right_angle = 90;
		const float halver = 0.5F;
		DrawPoly(
			point_v, 3, (float)unit * halver, right_angle, WHITE);

		// Draw particles
		for (idx = 0; idx < board->active_particles; idx++) {
			Vector2 point_u = (Vector2){
				particle_spawn_point.x, particle_spawn_point.y};
			return_error = galton_board__plot_ball(
				board, (uint8_t)idx, &point_u.x, &point_u.y);
			if (return_error != 0) {
				CloseWindow();
				break;
			}

			DrawCircle(
				(int)point_u.x,
				(int)point_u.y,
				(float)unit * halver * halver,
				RED);
		}

		// Draw each bouncer
		float unit_bc = (float)unit * halver * halver;
		for (idx = 0; idx < board->bouncer_center_size; idx++) {
			point_v.x = (float)particle_spawn_point.x +
						(float)board->bouncer_center[idx].x;
			point_v.y = (float)particle_spawn_point.y +
						(float)board->bouncer_center[idx].y +
						unit_bc / 2;
			DrawPoly(point_v, 3, unit_bc, -right_angle, BLACK);
		}

		for (idx = 0; idx < board->result_stack_size; idx++) {
			Vector2 point_v = (Vector2){
				particle_spawn_point.x, particle_spawn_point.y};
			return_error = galton_board__plot_result_stack(
				board, (int16_t)idx, &point_v.x, &point_v.y);
			float bar_length = (float)board->result_stack[idx];
			float width = (float)unit * halver;
			bar_length /= (float)max_this_instance;
			bar_length *=
				(float)galton_constants.screen_height - point_v.y;
			point_v.x -= width / 2;
			DrawRectangleV(
				point_v,
				(Vector2){.x = width, .y = bar_length},
				GREEN);

			if (state != 0) {
				sprintf(remain_text, "%lu", board->result_stack[idx]);
				DrawText(
					remain_text,
					(int)point_v.x,
					(int)point_v.y,
					(int)(unit / 2),
					(Color){cmax / 2, 0, 1 << (3 + 3), cmax});
			}
		}

		EndDrawing();
		cycle_color(&back_color);
	}
	return_error += galton_board__free(board);
	printf("Returned with error %d\n\n", return_error);
	return return_error;
}
