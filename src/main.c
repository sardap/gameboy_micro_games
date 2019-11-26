#include <gb/gb.h>
#include <stdio.h>
#include <rand.h>
#include "tile_map.c"
#include "types.h"

#define MAX_X 160
#define MAX_Y 144
#define MIDDLE_X MAX_X / 2
#define MIDDLE_Y MAX_Y / 2

#define DEVIL_CHANGE_MOVE_DIRECTION_DELAY 120
#define SHOOTING_TIME 60
#define INVINCIBILITY_TIME 60 // 3 secs

// ENEMY_NUMBERS
#define NUMBER_OF_DEVILS 2
#define NUMBER_OF_SMALL_ENEMIES 5

// TILES
#define BLANK_TILE 7

#define PLAYER_IDLE_TILE 0
#define PLAYER_SHOOT_TILE 1
#define PLAYER_DAMAGE_TILE 2

#define HEALTH_TILE 8

#define DEVIL_ENEMY_IDLE_TOP 4
#define DEVIL_ENEMY_IDLE_BOTTOM 5
#define DEVIL_ENEMY_SHOOT_BOTTOM 6

#define SMALL_ENEMY_IDLE 3

// SPRITES
#define PLAYER_SPRITE 0
#define HEALTH_SPRITE_START PLAYER_SPRITE + 1
#define HEALTH_SPRITE_END HEALTH_SPRITE_START + 2
#define DEVIL_SPRITE_START HEALTH_SPRITE_END + 1
#define DEVIL_SPRITE_END DEVIL_SPRITE_START + (NUMBER_OF_DEVILS * 4)
#define SMALL_ENEMY_SPRITE_START (DEVIL_SPRITE_END + 1)
#define SMALL_ENEMY_SPRITE_END SMALL_ENEMY_SPRITE_START + (NUMBER_OF_SMALL_ENEMIES * 2)

#define CHANGE_PLAYER_TO_IDLE_SPRITE set_sprite_tile(PLAYER_SPRITE, 0)
#define CHANGE_PLAYER_TO_SHOOT_SPRITE set_sprite_tile(PLAYER_SPRITE, 1)

#define initialise_8x8_sprite(ID, TILE, X, Y) {\
		set_sprite_tile(ID, TILE); \
		move_sprite(ID, X, Y); \
	}

#define initialise_player_sprite() { \
	initialise_8x8_sprite(0, PLAYER_IDLE_TILE, MIDDLE_X, MIDDLE_Y); \
}

#define move_16x16_sprite(STARTING_ID, X, Y) { \
		move_sprite(STARTING_ID, X, Y); \
		move_sprite(STARTING_ID + 1, X + 8, Y); \
		move_sprite(STARTING_ID + 2, X, Y + 8); \
		move_sprite(STARTING_ID + 3, X + 8, Y + 8); \
	}

#define move_16x8_sprite(STARTING_ID, X, Y) { \
		move_sprite(STARTING_ID, X, Y); \
		move_sprite(STARTING_ID + 1, X + 8, Y); \
	}

#define devil_laugh(STARTING_ID) { \
		set_sprite_tile(STARTING_ID + 2, DEVIL_ENEMY_SHOOT_BOTTOM); \
		set_sprite_tile(STARTING_ID + 3, DEVIL_ENEMY_SHOOT_BOTTOM); \
		set_sprite_prop(STARTING_ID + 3, S_FLIPX); \
	}

#define initialise_small_enemy_sprite(STARTING_ID) { \
		set_sprite_tile(STARTING_ID, SMALL_ENEMY_IDLE); \
		\
		set_sprite_tile(STARTING_ID + 1, SMALL_ENEMY_IDLE); \
		set_sprite_prop(STARTING_ID + 1, S_FLIPX); \
	}

#define initialise_devil_enemy_sprite(STARTING_ID) { \
		set_sprite_tile(STARTING_ID, DEVIL_ENEMY_IDLE_TOP); \
		\
		set_sprite_tile(STARTING_ID + 1, DEVIL_ENEMY_IDLE_TOP); \
		set_sprite_prop(STARTING_ID + 1, S_FLIPX); \
		\
		set_sprite_tile(STARTING_ID + 2, DEVIL_ENEMY_IDLE_BOTTOM); \
		\
		set_sprite_tile(STARTING_ID + 3, DEVIL_ENEMY_IDLE_BOTTOM); \
		set_sprite_prop(STARTING_ID + 3, S_FLIPX); \
	}

#define change_direction(X, Y) { \
		X += rand_uint8_in_range(-1, 1); \
		Y += rand_uint8_in_range(-1, 1); \
	}

#define rand_uint8_in_range(MIN, MAX) \
	(uint_8)(rand() % (MAX + 1 - MIN) + MIN) 

typedef struct {
	uint_8 x;
	uint_8 y;
	uint_16 shooting;
	uint_16 invincibility_time_left;
	int_8 health;
} EntData;

typedef struct {
	uint_8 x;
	uint_8 y;
	int_8 vel_x;
	int_8 vel_y;
} EnemyData;

typedef enum {
	PLAYER_STATE_NOTHING,
	PLAYER_STATE_TAKING_DAMAGE
} PlayerState;

void initlaise_game();

EntData _player_data;
EnemyData _devils[NUMBER_OF_DEVILS];
EnemyData _small_enemies[NUMBER_OF_SMALL_ENEMIES];

uint_16 _seed;
uint_16 _devil_move_delay;
uint_16 _devil_change_direction;

PlayerState _player_state;

void initalise_seed() {
  while(!joypad()){_seed++; if(_seed>=255)_seed=1;}  /* creates a seed value for the random number                                                      generator from the time taken to press the                                                      first key  */
  waitpadup();
  initrand(_seed); /* init the random number generator with our seed value */
}

void player_die() {
	initlaise_game();
}

void beep() {
	NR10_REG = 0x38U;
	NR11_REG = 0x70U;
	NR12_REG = 0xE0U;
	NR13_REG = 0x0AU;
	NR14_REG = 0xC6U;

	NR51_REG |= 0x11;
}

void player_take_damage() {
	int i;

	if(_player_state == PLAYER_STATE_TAKING_DAMAGE){
		return;
	}

	beep();

	set_sprite_tile(PLAYER_SPRITE, PLAYER_DAMAGE_TILE);
	_player_state = PLAYER_STATE_TAKING_DAMAGE;
	_player_data.invincibility_time_left = INVINCIBILITY_TIME;

	_player_data.health--;
	for(i = 0; i < 3 - _player_data.health; i++) {
		set_sprite_tile(HEALTH_SPRITE_END - i, BLANK_TILE);
	}

	if(_player_data.health <= 0) {
		player_die();
	}
}

void process_collisions() {

}

void update_devils(EnemyData* devils, EntData* player) {
	int i;
	uint_8 player_right = _player_data.x + 8;
	uint_8 player_bottom = _player_data.y + 8;

	_devil_change_direction--;

	for(i = 0; i < NUMBER_OF_DEVILS; i++) {
		if(_devil_change_direction - (NUMBER_OF_DEVILS - i) == 0) {
			change_direction(devils[i].vel_x, devils[i].vel_y);
		}

		devils[i].x += devils[i].vel_x;
		devils[i].y += devils[i].vel_y;
		move_16x16_sprite(DEVIL_SPRITE_START + (i * 4), devils[i].x, devils[i].y);

		if(
			_player_data.x < _devils[i].x + 16 && 
			_player_data.y < _devils[i].y + 16 &&
			_devils[i].x < player_right &&
			_devils[i].y < player_bottom
		) {
			player_take_damage();
		}
	}

	if(_devil_change_direction == 0) {
		_devil_change_direction = DEVIL_CHANGE_MOVE_DIRECTION_DELAY;
	}
}

void update_small_enemies(EnemyData* small_enemies, EntData* player) {
	int i;
	uint_8 player_right = _player_data.x + 8;
	uint_8 player_bottom = _player_data.y + 8;

	for(i = 0; i < NUMBER_OF_SMALL_ENEMIES; i++) {
		small_enemies[i].x += small_enemies[i].vel_x;
		small_enemies[i].y += small_enemies[i].vel_y;
		move_16x8_sprite(SMALL_ENEMY_SPRITE_START + (i * 2), small_enemies[i].x, small_enemies[i].y);

		if(small_enemies[i].x > 200) {
			small_enemies[i].vel_x = 1;
			small_enemies[i].vel_y = rand_uint8_in_range(-1, 1);
		} else if(small_enemies[i].x > MAX_X) {
			small_enemies[i].vel_x = -1;
			small_enemies[i].vel_y = rand_uint8_in_range(-1, 1);
		} else if(small_enemies[i].y > 200) {
			small_enemies[i].vel_x = rand_uint8_in_range(-1, 1);
			small_enemies[i].vel_y = 1;
		} else if(small_enemies[i].y > MAX_Y) {
			small_enemies[i].vel_x = rand_uint8_in_range(-1, 1);
			small_enemies[i].vel_y = -1;
		}

		if(
			_player_data.x < _small_enemies[i].x + 16 && 
			_player_data.y < _small_enemies[i].y + 8 &&
			_small_enemies[i].x < player_right &&
			_small_enemies[i].y < player_bottom
		) {
			player_take_damage();
		}

	}
}

void update_player(EntData* player_data) {
	if(player_data->shooting) {
		player_data->shooting--;
		if(player_data->shooting == 0) {
			CHANGE_PLAYER_TO_IDLE_SPRITE;
		}
	}
}

void shoot(EntData* player_data) {
	CHANGE_PLAYER_TO_SHOOT_SPRITE;
	player_data->shooting = SHOOTING_TIME;
}

void player_moved(EntData* player_data) {
	move_sprite(PLAYER_SPRITE, player_data->x, player_data->y);
}

void process_movement(EntData* player_data) {
	if(joypad() & J_RIGHT) {
		player_data->x++;
		player_moved(player_data);
	}
	
	if(joypad() & J_LEFT) {
		player_data->x--;
		player_moved(player_data);
	}

	if(joypad() & J_UP) {
		player_data->y--;
		player_moved(player_data);
	}

	if(joypad() & J_DOWN) {
		player_data->y++;
		player_moved(player_data);
	}
}
 
void process_input(EntData* player_data) {
	process_movement(player_data);

	if(joypad() & J_A && !player_data->shooting) {
		shoot(player_data);
	}
}

void initialise_sound() {
	NR50_REG = 0xFF;
    NR51_REG = 0xFF;
    NR52_REG = 0x80;
}

void initlaise_game() {
	int i;

	_player_data.x = MIDDLE_X;
	_player_data.y = MIDDLE_Y;
	_player_data.shooting = 0;
	_player_data.health = 3;
	_player_data.invincibility_time_left = INVINCIBILITY_TIME;
	_player_state = PLAYER_STATE_TAKING_DAMAGE;

	_devil_move_delay = 2;

	initialise_sound();

	SPRITES_8x8;
	set_sprite_data(0, 0, tile_map);
	initialise_player_sprite();
	
	for(i = 0; i < 3; i++) {
		initialise_8x8_sprite(HEALTH_SPRITE_START + i, HEALTH_TILE, 10 + (i * 9), 18);
	}

	for(i = 0; i < NUMBER_OF_DEVILS; i++) {
		initialise_devil_enemy_sprite(DEVIL_SPRITE_START + (i * 4));
	}

	for(i = 0; i < NUMBER_OF_SMALL_ENEMIES; i++) {
		initialise_small_enemy_sprite(SMALL_ENEMY_SPRITE_START + (i * 2));
	}

	SHOW_SPRITES;
	
	initalise_seed();

	for(i = 0; i < NUMBER_OF_DEVILS; i++) {
		_devils[i].x = rand_uint8_in_range(0, MAX_X);
		_devils[i].y = rand_uint8_in_range(0, MAX_Y);
		_devils[i].vel_x = rand_uint8_in_range(-1, 1);
		_devils[i].vel_y = rand_uint8_in_range(-1, 1);
	}

	for(i = 0; i < NUMBER_OF_SMALL_ENEMIES; i++) {
		_small_enemies[i].x = rand_uint8_in_range(0, MAX_X);
		_small_enemies[i].y = rand_uint8_in_range(0, MAX_Y);
		_small_enemies[i].vel_x = rand_uint8_in_range(-1, 1);
		_small_enemies[i].vel_y = rand_uint8_in_range(-1, 1);
		move_16x8_sprite(SMALL_ENEMY_SPRITE_START + (i * 2), _small_enemies[i].x, _small_enemies[i].y);
	}

	_devil_change_direction = DEVIL_CHANGE_MOVE_DIRECTION_DELAY;
}

void main() {
	initlaise_game();

	while(1) {
		process_input(&_player_data);
		update_player(&_player_data);

		_devil_move_delay--;
		if(_devil_move_delay == 0) {
			update_devils(_devils, &_player_data);
			_devil_move_delay = 5;
		}
		update_small_enemies(_small_enemies, &_player_data);

		if(_player_state == PLAYER_STATE_TAKING_DAMAGE) {
			_player_data.invincibility_time_left--;

			if(_player_data.invincibility_time_left == 0) {
				set_sprite_tile(PLAYER_SPRITE, PLAYER_IDLE_TILE);
				_player_state = PLAYER_STATE_NOTHING;
			} else if(_player_data.invincibility_time_left % 2 == 0) {
				set_sprite_tile(PLAYER_SPRITE, BLANK_TILE);
			}
			else {
				set_sprite_tile(PLAYER_SPRITE, PLAYER_DAMAGE_TILE);
			}
			
		} else {
			process_collisions();
		}
		

		wait_vbl_done();
	}
}
