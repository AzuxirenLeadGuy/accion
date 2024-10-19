#ifndef SCENETYPE_H
#define SCENETYPE_H
#include <stdint.h>

typedef uint8_t error_t;

typedef struct {
	int16_t x, y;
} point2d_t;

struct galton_constants {
	uint8_t max_particles_limit;
	uint8_t max_bouncer_pairs;
	uint8_t particle_speed;
	uint16_t screen_width, screen_height;
	uint16_t particle_spawn_milliseconds;
	uint8_t particle_process_limit;
	uint8_t particle_batch_size_2_power;
};

extern const struct galton_constants galton_constants;

typedef struct {
	int8_t prev_dir;
	int16_t progress;
	uint16_t height;
	int16_t slot;
} galton_particle_t;

typedef struct {
	uint8_t active_particles;
	// galton_particle_t particles[CONSTS__MAX_GALTON_PARTICLES];
	galton_particle_t *particles;
	// point2d_t bouncer_center
	// 	[(CONSTS__BOUNCER_PAIRS * CONSTS__BOUNCER_PAIRS * 2) +
	// 	 CONSTS__BOUNCER_PAIRS];
	point2d_t *bouncer_center;
	// uint64_t result_stack[1 + (2 * CONSTS__BOUNCER_PAIRS)];
	uint64_t *result_stack;
	const uint16_t result_stack_size, bouncer_center_size;
	const uint8_t max_particles, particle_speed;
	const uint16_t particle_dist_limit;
	const uint8_t Cw, Ch;
} galton_board_t;

/**
 * @brief Initializes an existing instance of galton_board
 *
 * @param board The board to init. bouncer_pairs cannot exceed 11
 * @return error_t If the arguments are invalid, returns a positive
 * value, otherwise 0
 */

/**
 * @brief Dynamically allocates an instance of galton board
 *
 * @param bounce_pairs The number of pairs of bouncers to create
 * @param max_particles The maximum number of particles to track at a
 * given instance
 * @return galton_board_t* The pointer to an allocated instance if
 * successful, otherwise returns null(0)
 */
galton_board_t *galton_board__init(
	const uint8_t bounce_pairs,
	const uint8_t max_particles,
	const uint8_t Hc,
	const uint8_t Wc);

/**
 * @brief Resets the count for the galton board
 *
 * @param board The board to reset
 * @return error_t If any error occurs, returns a positive
 * value, otherwise 0
 */
error_t galton_board__reset_count(galton_board_t *board);

/**
 * @brief Add a new ball to the board
 *
 * @param board The board to add the ball at
 * @return error_t If the ball cannot be added, returns a positive
 * value, otherwise 0
 */
error_t galton_board__add(galton_board_t *board);

/**
 * @brief Updates all particles within a galton board
 *
 * @param board The board to update
 * @return error_t If any error occurs, returns a positive value
 * , otherwise 0
 */
error_t galton_board__update(galton_board_t *board);

/**
 * @brief Gets the final point of the galton board particle, provided
 * with the reference
 *
 * @param board The galton board
 * @param idx The particle index
 * @param x The x coordinate of the funnel, which gets updated to the
 * x coordinate of the particle
 * @param y The y coordinate of the funnel, which gets updated to the
 * y coordinate of the particle
 * @return error_t If arguments are valid, returns 0, otherwise a
 * positive integer in case of an error.
 */
error_t galton_board__plot_ball(
	const galton_board_t *board, uint8_t idx, float *x, float *y);

/**
 * @brief Gets the final point of the galton result box, provided
 * with the reference
 *
 * @param board The galton board
 * @param idx The result index
 * @param x The x coordinate of the funnel, which gets updated to the
 * x coordinate of the center of the result box
 * @param y The y coordinate of the funnel, which gets updated to the
 * y coordinate of the center of the result box
 * @return error_t If arguments are valid, returns 0, otherwise a
 * positive integer in case of an error.
 */
error_t galton_board__plot_result_stack(
	const galton_board_t *board, int16_t idx, float *x, float *y);

/**
 * @brief De-allocate all memory resources held by this instance
 * of galton board
 *
 * @param board The board to deallocate
 * @return error_t Returns 0 if function is successful, otherwise
 * returns a positive integer
 */
error_t galton_board__free(galton_board_t *board);

#endif
