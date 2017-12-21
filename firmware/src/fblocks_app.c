/*
 * This file is part of the 1Bitsy 1UP retro inspired game console project.
 *
 * Copyright (C) 2017 Nicolas Schodet <nico@ni.fr.eu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gamepad.h"
#include "lcd.h"
#include "math-util.h"
#include "systick.h"
#include "text.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "assets/assets.h"

#define FBLOCKS_COLS 10
#define FBLOCKS_ROWS 22
#define FBLOCKS_VISIBLE_ROWS 20

struct fblocks_shape_t {
	int rows, cols;
	uint8_t grid[4][4];
};

struct fblocks_piece_t {
	int shape_count;
	struct fblocks_shape_t shapes[4];
};

#define FBLOCKS_PIECES 7

static const struct fblocks_piece_t fblocks_pieces[FBLOCKS_PIECES] = {
	/* {{{ */
	{ 2, {
			{ 1, 4, {
					{ 2, 2, 2, 2 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 4, 1, {
					{ 2, 0, 0, 0 },
					{ 2, 0, 0, 0 },
					{ 2, 0, 0, 0 },
					{ 2, 0, 0, 0 },
				} },
		} },
	{ 1, {
			{ 2, 2, {
					{ 3, 3, 0, 0 },
					{ 3, 3, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	{ 2, {
			{ 2, 3, {
					{ 0, 4, 4, 0 },
					{ 4, 4, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 4, 0, 0, 0 },
					{ 4, 4, 0, 0 },
					{ 0, 4, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	{ 2, {
			{ 2, 3, {
					{ 8, 8, 0, 0 },
					{ 0, 8, 8, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 0, 8, 0, 0 },
					{ 8, 8, 0, 0 },
					{ 8, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	{ 4, {
			{ 2, 3, {
					{ 5, 5, 5, 0 },
					{ 5, 0, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 5, 5, 0, 0 },
					{ 0, 5, 0, 0 },
					{ 0, 5, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 2, 3, {
					{ 0, 0, 5, 0 },
					{ 5, 5, 5, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 5, 0, 0, 0 },
					{ 5, 0, 0, 0 },
					{ 5, 5, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	{ 4, {
			{ 2, 3, {
					{ 6, 6, 6, 0 },
					{ 0, 0, 6, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 0, 6, 0, 0 },
					{ 0, 6, 0, 0 },
					{ 6, 6, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 2, 3, {
					{ 6, 0, 0, 0 },
					{ 6, 6, 6, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 6, 6, 0, 0 },
					{ 6, 0, 0, 0 },
					{ 6, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	{ 4, {
			{ 2, 3, {
					{ 7, 7, 7, 0 },
					{ 0, 7, 0, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 0, 7, 0, 0 },
					{ 7, 7, 0, 0 },
					{ 0, 7, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 2, 3, {
					{ 0, 7, 0, 0 },
					{ 7, 7, 7, 0 },
					{ 0, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
			{ 3, 2, {
					{ 7, 0, 0, 0 },
					{ 7, 7, 0, 0 },
					{ 7, 0, 0, 0 },
					{ 0, 0, 0, 0 },
				} },
		} },
	/* }}} */
};

#define FBLOCKS_SCREEN_AREA_X (5 * 12)
#define FBLOCKS_SCREEN_AREA_Y (0 * 12)
#define FBLOCKS_SCREEN_AREA_W (FBLOCKS_COLS * 12)
#define FBLOCKS_SCREEN_AREA_H (FBLOCKS_VISIBLE_ROWS * 12)
#define FBLOCKS_SCREEN_PREVIEW_X (16 * 12)
#define FBLOCKS_SCREEN_PREVIEW_Y (1 * 12)
#define FBLOCKS_SCREEN_PREVIEW_W (5 * 12)
#define FBLOCKS_SCREEN_PREVIEW_H (3 * 12)
#define FBLOCKS_SCREEN_SCORE_X (16 * 12)
#define FBLOCKS_SCREEN_SCORE_Y (5 * 12)
#define FBLOCKS_SCREEN_SCORE_W (7 * 12)
#define FBLOCKS_SCREEN_SCORE_H (2 * 12)

static const uint8_t fblocks_screen[LCD_HEIGHT / 12][LCD_WIDTH / 12 + 1] = {
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

#define FBLOCKS_START_FALL_SPEED 800
#define FBLOCKS_AUTO_REPEAT_SPEED 100
#define FBLOCKS_PREVIEW_COUNT 1

struct fblocks_game_t;

typedef bool (*fblocks_animation_t)(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis);

struct fblocks_game_t {
	/* Playing area, row 0 is at bottom of screen. */
	uint8_t area[FBLOCKS_ROWS][FBLOCKS_COLS];
	/* Current score. */
	int score;
	/* Score to obtain to reach the next level. */
	int next_level_score;
	/* Current level. */
	int level;
	/* Current piece and shape. */
	const struct fblocks_piece_t *piece;
	const struct fblocks_shape_t *shape;
	/* Base of current piece position. */
	int row, col;
	/* Speed of fall. */
	int fall_speed;
	/* Date of last fall action. */
	uint32_t last_fall_millis;
	/* Date of last move action. */
	uint32_t last_move_millis;
	/* Previous gamepad state. */
	uint16_t last_gamepad;
	/* Animation. */
	fblocks_animation_t animation;
	uint32_t animation_start_millis;
	/* Bag of random pieces. */
	unsigned int bag;
	/* Future pieces. */
	const struct fblocks_piece_t *preview[FBLOCKS_PREVIEW_COUNT];
	/* Full row being removed or -1. */
	int full_row;
	/* You lose. */
	bool over;
};

static struct fblocks_game_t game;

/* Initialise/reset game. */
void fblocks_init(void) {
	memset(game.area, 0, sizeof(game.area));
	game.score = 0;
	game.next_level_score = 0;
	game.level = 1;
	game.piece = NULL;
	game.fall_speed = FBLOCKS_START_FALL_SPEED;
	game.last_fall_millis = system_millis;
	game.last_move_millis = system_millis;
	game.last_gamepad = 0;
	game.animation = NULL;
	game.animation_start_millis = 0;
	game.bag = 0;
	game.full_row = -1;
	game.over = true;
}

/* Undraw shape in area. */
static void fblocks_unblit(struct fblocks_game_t *game) {
	const struct fblocks_shape_t *shape = game->shape;
	int row = game->row, col = game->col;
	for (int r = 0; r < shape->rows; r++) {
		for (int c = 0; c < shape->cols; c++) {
			if (shape->grid[r][c]) {
				game->area[row + r][col + c] = 0;
			}
		}
	}
}

/* Draw shape in area. */
static void fblocks_blit(struct fblocks_game_t *game) {
	const struct fblocks_shape_t *shape = game->shape;
	int row = game->row, col = game->col;
	for (int r = 0; r < shape->rows; r++) {
		for (int c = 0; c < shape->cols; c++) {
			int s = shape->grid[r][c];
			if (s) {
				game->area[row + r][col + c] = s;
			}
		}
	}
}

/* Return true if there is a collision between play area and shape. */
static bool fblocks_test(struct fblocks_game_t *game,
		const struct fblocks_shape_t *shape, int row, int col) {
	for (int r = 0; r < shape->rows; r++) {
		for (int c = 0; c < shape->cols; c++) {
			int s = shape->grid[r][c];
			if (s && game->area[row + r][col + c]) {
				return true;
			}
		}
	}
	return false;
}

/* Remove a row. */
static void fblocks_remove_row(struct fblocks_game_t *game, int row) {
	for (int r = row; r < FBLOCKS_ROWS - 1; r++) {
		for (int c = 0; c < FBLOCKS_COLS; c++) {
			game->area[r][c] = game->area[r + 1][c];
		}
	}
	for (int c = 0; c < FBLOCKS_COLS; c++) {
		game->area[FBLOCKS_ROWS - 1][c] = 0;
	}
}

/* Find first full row or return -1. */
static int fblocks_full_row(struct fblocks_game_t *game) {
	for (int r = 0; r < FBLOCKS_ROWS; r++) {
		bool full = true;
		for (int c = 0; full && c < FBLOCKS_COLS; c++) {
			if (!game->area[r][c]) {
				full = false;
			}
		}
		if (full) {
			return r;
		}
	}
	return -1;
}

/* Choose a random piece from the bag. */
static int fblocks_random(struct fblocks_game_t *game) {
	if (!game->bag) {
		game->bag = (1 << FBLOCKS_PIECES) - 1;
	}
	int piece = rand() % FBLOCKS_PIECES;
	while (!(game->bag & (1 << piece))) {
		piece = (piece + 1) % FBLOCKS_PIECES;
	}
	game->bag &= ~(1 << piece);
	return piece;
}

static bool fblocks_animation_full_row(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis);

static bool fblocks_animation_game_over(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis);

/* Apply rules. */
static void fblocks_rules(struct fblocks_game_t *game) {
	/* Restart? */
	if (game->over) {
		fblocks_init();
		for (int i = 0; i < FBLOCKS_PREVIEW_COUNT; i++) {
			game->preview[i] = &fblocks_pieces[fblocks_random(game)];
		}
		game->next_level_score = 10;
		game->over = false;
	}
	/* Remove full row after animation. */
	if (game->full_row != -1) {
		game->score++;
		if (game->score >= game->next_level_score) {
			game->level++;
			game->next_level_score += game->level * 10;
			game->fall_speed = game->fall_speed * 7 / 10;
		}
		fblocks_remove_row(game, game->full_row);
		game->full_row = -1;
	}
	/* New piece? */
	if (!game->piece) {
		/* Test for full rows. */
		game->full_row = fblocks_full_row(game);
		if (game->full_row != -1) {
			game->animation = fblocks_animation_full_row;
			game->animation_start_millis = system_millis;
		} else {
			/* New piece. */
			game->piece = game->preview[0];
			for (int i = 0; i < FBLOCKS_PREVIEW_COUNT - 1; i++) {
				game->preview[i] = game->preview[i + 1];
			}
			game->preview[FBLOCKS_PREVIEW_COUNT - 1] =
				&fblocks_pieces[fblocks_random(game)];
			game->shape = &game->piece->shapes[0];
			game->row = FBLOCKS_VISIBLE_ROWS - game->shape->rows;
			game->col = (FBLOCKS_COLS - game->shape->cols) / 2;
			game->last_fall_millis = system_millis;
			/* Game over? */
			if (fblocks_test(game, game->shape, game->row, game->col)) {
				game->over = true;
				game->animation = fblocks_animation_game_over;
				game->animation_start_millis = system_millis;
			}
			fblocks_blit(game);
		}
	}
}

/* Game & control logic. */
void fblocks_animate(void) {
	/* Animation running? */
	if (game.animation) {
		return;
	}
	/* Apply rules. */
	fblocks_rules(&game);
	if (!game.piece) {
		return;
	}
	/* Key? */
	uint16_t gamepad = gamepad_get();
	uint16_t gamepad_new = gamepad & (gamepad ^ game.last_gamepad);
	uint16_t gamepad_old = gamepad & game.last_gamepad;
	bool repeat_move = (int) (system_millis - game.last_move_millis
			- FBLOCKS_AUTO_REPEAT_SPEED) > 0;
	int new_col = game.col;
	int rotate = 0;
	if ((gamepad_new & GAMEPAD_BLEFT)
			|| ((gamepad_old & GAMEPAD_BLEFT) && repeat_move)) {
		new_col--;
	}
	if ((gamepad_new & GAMEPAD_BRIGHT)
			|| ((gamepad_old & GAMEPAD_BRIGHT) && repeat_move)) {
		new_col++;
	}
	if (gamepad_new & GAMEPAD_BA) {
		rotate--;
	}
	if (gamepad_new & GAMEPAD_BB) {
		rotate++;
	}
	game.last_gamepad = gamepad;
	/* Rotate. */
	if (rotate) {
		/* Compute rotation. */
		const struct fblocks_shape_t *new_shape = game.shape + rotate;
		if (new_shape == &game.piece->shapes[-1]) {
			new_shape = &game.piece->shapes[game.piece->shape_count - 1];
		} else if (new_shape == &game.piece->shapes[game.piece->shape_count]) {
			new_shape = &game.piece->shapes[0];
		}
		/* Compute new position. */
		int new_row = game.row;
		new_row -= (new_shape->rows - game.shape->rows) / 2;
		new_row = CLAMP(0, FBLOCKS_ROWS - new_shape->rows, new_row);
		int new_col = game.col;
		new_col -= (new_shape->cols - game.shape->cols) / 2;
		new_col = CLAMP(0, FBLOCKS_COLS - new_shape->cols, new_col);
		/* Is there room to rotate. */
		fblocks_unblit(&game);
		if (!fblocks_test(&game, new_shape, new_row, new_col)) {
			/* OK, there is room. */
			game.shape = new_shape;
			game.row = new_row;
			game.col = new_col;
		}
		fblocks_blit(&game);
	/* Move. */
	} else if (new_col != game.col && new_col >= 0
			&& new_col + game.shape->cols <= FBLOCKS_COLS) {
		/* Is there room to move? */
		fblocks_unblit(&game);
		if (!fblocks_test(&game, game.shape, game.row, new_col)) {
			/* OK, there is room. */
			game.col = new_col;
			game.last_move_millis = system_millis;
		}
		fblocks_blit(&game);
	}
	/* Faster down? */
	bool drop = gamepad & GAMEPAD_BDOWN;
	int speed = game.fall_speed;
	if (drop) {
		speed /= 20;
	}
	/* Fall? */
	if ((int) (system_millis - game.last_fall_millis - speed) > 0) {
		/* Erase previous position, test new one... */
		fblocks_unblit(&game);
		if (game.row == 0
				|| fblocks_test(&game, game.shape, game.row - 1, game.col)) {
			/* Revert, there is no room! */
			fblocks_blit(&game);
			game.piece = NULL;
		} else {
			/* OK use new position. */
			game.row--;
			fblocks_blit(&game);
		}
		game.last_fall_millis = system_millis;
	}
}

/* Draw one tile from the pixmap. */
static void fblocks_draw_tile(gfx_pixslice *slice, int id, size_t x, size_t y) {
	gfx_pixslice tile;
	gfx_init_pixslice(&tile, (void *) &fblocks_pixmap[id * 12][0],
			0, 0, 12, 12, 12);
	gfx_ipoint p = { .x = x, .y = y };
	gfx_copy_pixslice(slice, &tile, p);
}

/* Default area drawing. */
static bool fblocks_animation_default(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis) {
	for (int r = 0; r < FBLOCKS_VISIBLE_ROWS; r++) {
		int y = FBLOCKS_SCREEN_AREA_Y + (FBLOCKS_VISIBLE_ROWS - r - 1) * 12;
		if (y >= slice->y - 12 && y < slice->y + slice->h) {
			for (int c = 0; c < FBLOCKS_COLS; c++) {
				int x = FBLOCKS_SCREEN_AREA_X + c * 12;
				int s = game->area[r][c];
				if (s) {
					fblocks_draw_tile(slice, s, x, y);
				}
			}
		}
	}
	return false;
}

/* Drawing when a full row is being removed. */
static bool fblocks_animation_full_row(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis) {
	int collapse = millis < 500 ? millis * 12 / 500 : 12;
	for (int r = 0; r < FBLOCKS_VISIBLE_ROWS; r++) {
		int y = FBLOCKS_SCREEN_AREA_Y + (FBLOCKS_VISIBLE_ROWS - r - 1) * 12;
		if (r > game->full_row)
			y += collapse;
		if (y >= slice->y - 12 && y < slice->y + slice->h) {
			for (int c = 0; c < FBLOCKS_COLS; c++) {
				int x = FBLOCKS_SCREEN_AREA_X + c * 12;
				int s = game->area[r][c];
				fblocks_draw_tile(slice, s, x, y);
			}
		}
	}
	return millis >= 800;
}

/* Game over drawing. */
static bool fblocks_animation_game_over(struct fblocks_game_t *game,
		gfx_pixslice *slice, int millis) {
	float anim = millis / 3000.0f;
	if (anim < 1.0f) {
		float radius = (1.0f - anim);
		float angle = anim * 6.0f * M_PI;
		float ca = cos(angle);
		float sa = sin(angle);
		const int cy = FBLOCKS_SCREEN_AREA_Y + FBLOCKS_SCREEN_AREA_H / 2;
		const int cx = FBLOCKS_SCREEN_AREA_X + FBLOCKS_SCREEN_AREA_W / 2;
		for (int r = 0; r < FBLOCKS_ROWS; r++) {
			int y = FBLOCKS_SCREEN_AREA_Y + (FBLOCKS_VISIBLE_ROWS - r - 1) * 12;
			for (int c = 0; c < FBLOCKS_COLS; c++) {
				int s = game->area[r][c];
				if (s) {
					int x = FBLOCKS_SCREEN_AREA_X + c * 12;
					int dx = x + 6 - cx;
					int dy = y + 6 - cy;
					int rx = cx + radius * (dx * ca - dy * sa) - 6;
					int ry = cy + radius * (dx * sa + dy * ca) - 6;
					fblocks_draw_tile(slice, s, rx, ry);
				}
			}
		}
	} else {
		text_draw_str16(slice, "Game Over",
				(FBLOCKS_SCREEN_AREA_X + FBLOCKS_SCREEN_AREA_W / 2) / 8 - 5,
				(FBLOCKS_SCREEN_AREA_Y + FBLOCKS_SCREEN_AREA_H / 2) / 16, 0x0000);
	}
	return anim > 2.0f;
}

static bool fblocks_render_slice(gfx_pixslice *slice) {
	/* Background. */
	for (size_t y = slice->y / 12; y <= (slice->y + slice->h) / 12; y++) {
		for (size_t x = 0; x <= slice->w / 12 + 1; x++) {
			fblocks_draw_tile(slice, fblocks_screen[y][x], x * 12, y * 12);
		}
	}
	/* Preview. */
	if (slice->y < FBLOCKS_SCREEN_PREVIEW_Y + FBLOCKS_SCREEN_PREVIEW_H && game.preview[0]) {
		const struct fblocks_shape_t *shape = &game.preview[0]->shapes[0];
		int sx = FBLOCKS_SCREEN_PREVIEW_X + FBLOCKS_SCREEN_PREVIEW_W / 2
			- shape->cols * 12 / 2;
		int sy = FBLOCKS_SCREEN_PREVIEW_Y + FBLOCKS_SCREEN_PREVIEW_H / 2
			- shape->rows * 12 / 2;
		for (int r = 0; r < shape->rows; r++) {
			int dy = (shape->rows - r - 1) * 12;
			for (int c = 0; c < shape->cols; c++) {
				int s = shape->grid[r][c];
				if (s) {
					fblocks_draw_tile(slice, s, sx + c * 12, sy + dy);
				}
			}
		}
	}
	/* Score. */
	if (slice->y < FBLOCKS_SCREEN_SCORE_Y + FBLOCKS_SCREEN_SCORE_H) {
		char score[11];
		sprintf(score, "%d", game.score);
		int score_len = strlen(score);
		text_draw_str16(slice, score,
			(FBLOCKS_SCREEN_SCORE_X + FBLOCKS_SCREEN_SCORE_W) / 8 - score_len - 1,
			(FBLOCKS_SCREEN_SCORE_Y + FBLOCKS_SCREEN_SCORE_H / 2) / 16, 0x0000);
	}
	/* Area. */
	fblocks_animation_t animation = game.animation ? game.animation
		: fblocks_animation_default;
	return animation(&game, slice,
			system_millis - game.animation_start_millis);
}

void fblocks_render(void) {
	size_t h;
	bool animation_stop = false;
	for (size_t y = 0; y < LCD_HEIGHT; y += h) {
		h = MIN(LCD_MAX_SLICE_ROWS, LCD_HEIGHT - y);
		gfx_pixslice *slice = lcd_alloc_pixslice(0, y, LCD_WIDTH, h);
		animation_stop = fblocks_render_slice(slice);
		lcd_send_pixslice(slice);
	}
	if (animation_stop) {
		game.animation = NULL;
	}
}
