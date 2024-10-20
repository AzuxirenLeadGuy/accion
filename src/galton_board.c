#include "galton_board.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ALLOC_ARRAY(T, count) (T *)calloc(count, sizeof(T))
#define ASSIGN_CONST(var, T, val) *(T *)(&var) = val

const struct galton_constants galton_constants = {
	.max_particles_limit = 255,
	.max_bouncer_pairs = 11,
	.particle_speed = 12,
	.screen_width = 1200,
	.screen_height = 900,
	.particle_spawn_milliseconds = 300,
	.particle_process_limit = 0xff,
	.particle_batch_size_2_power = 8};

point2d_t get_position_base(
	const int16_t slot,
	const uint16_t depth,
	const uint16_t Ch,
	const uint16_t Cw) {
	return (point2d_t){.x = (slot * Cw) >> 1, .y = depth * Ch};
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
	float *x,
	float *y,
	const int16_t slot,
	const uint16_t depth,
	const uint16_t Ch,
	const uint16_t Cw,
	const int8_t prev_dir,
	float progress) {
	if (progress < 0 || progress > 1) {
		return 5;
	}
	point2d_t base = get_position_base(slot, depth, Ch, Cw);
	*y += base.y;
	*x += base.x;
	*y -= progress * Ch;
	*x -= prev_dir * progress * progress * Cw / 2;
	return 0;
}

galton_board_t *galton_board__init(
	const uint8_t bounce_pairs,
	const uint8_t max_particles,
	const uint8_t Hc,
	const uint8_t Wc) {
	if (bounce_pairs > galton_constants.max_bouncer_pairs) {
		return 0;
	} else if (max_particles > galton_constants.max_particles_limit) {
		return 0;
	} else if (Hc == 0 || Wc == 0) {
		return 0;
	}
	srand(time(0));
	galton_board_t *bd =
		(galton_board_t *)malloc(sizeof(galton_board_t));
	bd->active_particles = 0;

	ASSIGN_CONST(bd->max_particles, uint8_t, max_particles);
	uint16_t px = 1 + (2 * bounce_pairs);
	ASSIGN_CONST(bd->result_stack_size, uint16_t, px);
	ASSIGN_CONST(
		bd->bouncer_center_size, uint16_t, px * bounce_pairs);
	ASSIGN_CONST(bd->Ch, uint8_t, Hc);
	ASSIGN_CONST(bd->Cw, uint8_t, Wc);
	ASSIGN_CONST(
		bd->particle_dist_limit,
		uint16_t,
		galton_constants.particle_process_limit);

	bd->particles = ALLOC_ARRAY(galton_particle_t, max_particles);

	bd->result_stack = ALLOC_ARRAY(uint64_t, bd->result_stack_size);

	bd->bouncer_center =
		ALLOC_ARRAY(point2d_t, bd->bouncer_center_size);

	for (int idx = 0, k, h = px - 1; h > 0; h--) {
		k = 1;
		if ((h & 1) != 0) { // odd
			bd->bouncer_center[idx++] =
				get_position_base(0, h, Hc, Wc);
			k = 2;
		}
		while (k < h) {
			bd->bouncer_center[idx++] =
				get_position_base(k, h, Hc, Wc);
			bd->bouncer_center[idx++] =
				get_position_base(-k, h, Hc, Wc);
			k += 2;
		}
	}

	return bd;
}

error_t galton_board__free(galton_board_t *board) {
	if (board == 0)
		return 1;
	free(board->bouncer_center);
	free(board->particles);
	free(board->result_stack);
	free(board);
	return 0;
}

error_t galton_board__reset_count(galton_board_t *board) {
	if (board == 0)
		return 1;
	for (int i = 0; i < board->result_stack_size; i++) {
		board->result_stack[i] = 0;
	}
	board->active_particles = 0;
	return 0;
}

error_t galton_board__add(galton_board_t *board) {
	if (board == 0)
		return 2;
	else if (board->active_particles == board->max_particles)
		return 1;
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
		galton_particle_t *p = board->particles + idx;
		p->progress -= galton_constants.particle_speed;
		if (p->progress > 0) {
			idx++;
		} else if (p->height == board->result_stack_size) {
			int slot_bin = p->slot;
			slot_bin = slot_bin + board->result_stack_size - 1;
			if ((slot_bin & 1) == 1)
				return 4;
			slot_bin = slot_bin / 2;
			if (slot_bin < 0)
				return 5;
			else if (slot_bin >= board->result_stack_size)
				return 6;
			board->result_stack[slot_bin]++;
			board->active_particles--;
			board->particles[idx] =
				board->particles[board->active_particles];
		} else {
			p->height++;
			p->progress = galton_constants.particle_process_limit;
			p->prev_dir = ((rand()) & 1) == 1 ? 1 : -1;
			p->slot = p->slot + p->prev_dir;
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
