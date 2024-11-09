#include "galton_board.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ALLOC_ARRAY(T, count) (T *)calloc(count, sizeof(T))
#define ASSIGN_CONST(var, T, val) *(T *)(&var) = val

const struct galton_constants galton_constants = {
	.max_particles_limit = 255,
	.max_bouncer_pairs = 11,
	.particle_speed = 16,
	.screen_width = 1200,
	.screen_height = 900,
	.particle_spawn_milliseconds = 300,
	.particle_process_limit = 0xff,
	.particle_batch_size_2_power = 8};

point2d_t get_position_base(
	const int16_t slot,
	const uint16_t depth,
	const uint16_t C_h,
	const uint16_t C_w) {
	return (point2d_t){.x = (slot * C_w) >> 1, .y = depth * C_h};
}

/**
 * @brief Get the position object
 *
 * @param x The x-coordinate of spawn point, which gets updated to
 * final position
 * @param y The y-coordinate of spawn point, which gets updated to
 * final position
 * @param slot The integer value of x-coordinate
 * @param depth The positive integer value of y-coordinate
 * @param Ch The constant of height
 * @param Cw The constant of width
 * @param prev_dir The direction in which the particle is turning
 * @param progress The progress made till reaching dest from the
 * source. Must be in [0,1]
 * @return error_t returns 0 if no errors are present, otherwise a
 * positive value is returned.
 */
error_t get_position(
	float *x_f,
	float *y_f,
	const int16_t slot,
	const uint16_t depth,
	const uint16_t C_h,
	const uint16_t C_w,
	const int8_t prev_dir,
	float progress) {
	if (progress < 0 || progress > 1) {
		return 5;
	}
	point2d_t base = get_position_base(slot, depth, C_h, C_w);
	*y_f += base.y;
	*x_f += base.x;
	*y_f -= progress * C_h;
	*x_f -= prev_dir * progress * progress * C_w / 2;
	return 0;
}

galton_board_t *galton_board__init(
	const uint8_t bounce_pairs,
	const uint8_t max_particles,
	const uint8_t Hc,
	const uint8_t Wc) {
	if (bounce_pairs > galton_constants.max_bouncer_pairs) {
		return 0;
	}
	if (max_particles > galton_constants.max_particles_limit) {
		return 0;
	}
	if (H_c == 0 || W_c == 0) {
		return 0;
	}
	srand((uint32_t)time(0));
	galton_board_t *bounds =
		(galton_board_t *)malloc(sizeof(galton_board_t));
	bounds->active_particles = 0;

	ASSIGN_CONST(bounds->max_particles, uint8_t, max_particles);
	uint16_t pos_x = 1 + (2 * bounce_pairs);
	ASSIGN_CONST(bounds->result_stack_size, uint16_t, pos_x);
	ASSIGN_CONST(
		bounds->bouncer_center_size, uint16_t, pos_x * bounce_pairs);
	ASSIGN_CONST(bounds->Ch, uint8_t, H_c);
	ASSIGN_CONST(bounds->Cw, uint8_t, W_c);
	ASSIGN_CONST(
		bounds->particle_dist_limit,
		uint16_t,
		galton_constants.particle_process_limit);

	bounds->particles = ALLOC_ARRAY(galton_particle_t, max_particles);

	bounds->result_stack =
		ALLOC_ARRAY(uint64_t, bounds->result_stack_size);

	bounds->bouncer_center =
		ALLOC_ARRAY(point2d_t, bounds->bouncer_center_size);

	for (int idx = 0, k, height = pos_x - 1; height > 0; height--) {
		k = 1;
		if ((height & 1) != 0) { // odd
			bounds->bouncer_center[idx++] =
				get_position_base(0, (uint16_t)height, H_c, W_c);
			k = 2;
		}
		while (k < height) {
			bounds->bouncer_center[idx++] = get_position_base(
				(int16_t)k, (uint16_t)height, H_c, W_c);
			bounds->bouncer_center[idx++] = get_position_base(
				(int16_t)-k, (uint16_t)height, H_c, W_c);
			k += 2;
		}
	}

	return bounds;
}

error_t galton_board__free(galton_board_t *board) {
	if (board == 0) {
		return 1;
	}
	free(board->bouncer_center);
	free(board->particles);
	free(board->result_stack);
	free(board);
	return 0;
}

error_t galton_board__reset_count(galton_board_t *board) {
	if (board == 0) {
		return 1;
	}
	for (int i = 0; i < board->result_stack_size; i++) {
		board->result_stack[i] = 0;
	}
	board->active_particles = 0;
	return 0;
}

error_t galton_board__add(galton_board_t *board) {
	if (board == 0) {
		return 2;
	}
	if (board->active_particles == board->max_particles) {
		return 1;
	}
	board->particles[board->active_particles] = (galton_particle_t){
		.progress = galton_constants.particle_process_limit,
		.prev_dir = 0,
		.height = 1,
		.slot = 0,
	};
	board->active_particles++;
	return 0;
}

error_t galton_board__update(galton_board_t *board) {
	if (board == 0) {
		return 2;
	}
	int idx = 0;
	while (idx < board->active_particles) {
		galton_particle_t *pnt = board->particles + idx;
		pnt->progress = (int16_t)(pnt->progress -
								  galton_constants.particle_speed);
		if (pnt->progress > 0) {
			idx++;
		} else if (pnt->height == board->result_stack_size) {
			int slot_bin = pnt->slot;
			slot_bin = slot_bin + board->result_stack_size - 1;
			if ((slot_bin & 1) == 1) {
				return 4;
			}
			slot_bin = slot_bin / 2;
			if (slot_bin < 0) {
				return 3;
			}
			if (slot_bin >= board->result_stack_size) {
				return 2;
			}
			board->result_stack[slot_bin]++;
			board->active_particles--;
			board->particles[idx] =
				board->particles[board->active_particles];
		} else {
			pnt->height++;
			pnt->progress = galton_constants.particle_process_limit;
			pnt->prev_dir = ((rand()) & 1) == 1 ? 1 : -1;
			pnt->slot = (int16_t)(pnt->slot + pnt->prev_dir);
			idx++;
		}
	}
	return 0;
}

error_t galton_board__plot_ball(
	const galton_board_t *board, uint8_t idx, float *x, float *y) {

	if (board == 0) {
		return 2;
	} else if (idx >= board->active_particles) {
		return 1;
	}

	galton_particle_t *pt = &board->particles[idx];
	float dp = pt->progress / (float)board->particle_dist_limit;

	return get_position(
		x,
		y,
		pt->slot,
		pt->height,
		board->Ch,
		board->Cw,
		pt->prev_dir,
		dp);

	return 0;
}

error_t galton_board__plot_result_stack(
	const galton_board_t *board, int16_t idx, float *x, float *y) {

	if (board == 0) {
		return 2;
	} else if (idx >= board->result_stack_size) {
		return 1;
	}

	point2d_t pt = get_position_base(
		1 + (idx * 2) - board->result_stack_size,
		board->result_stack_size,
		board->Ch,
		board->Cw);
	*x += pt.x;
	*y += pt.y;
	return 0;
}
