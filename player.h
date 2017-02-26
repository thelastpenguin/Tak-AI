#ifndef __PLAYER_H_
#define __PLAYER_H_

#include "board.h"

struct Player {
	virtual Board makeAMove(Board board) = 0;
};

struct HumanPlayer : public Player {
	virtual Board makeAMove(Board board);
};

struct MinmaxPlayer : public Player {
	int depth;

	MinmaxPlayer(int depth) : depth(depth) { };

	virtual Board makeAMove(Board board);
	double minmax(Board& board, int depth, Move* result);
	double scoreBoard(const Board& board);
	double scoreMaterial(const Board& board);
};

#endif
