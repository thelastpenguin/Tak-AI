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

	constexpr static double MAX_SCORE = 100000000.0;
	constexpr static double WIN_SCORE = MAX_SCORE / 100.0;

	MinmaxPlayer(int depth) : depth(depth) { };

	virtual Board makeAMove(Board board);
	double minmax(Board& board, int depth, Move* result, double alpha = -MAX_SCORE, double beta = MAX_SCORE);
	double scoreBoard(const Board& board);
	double scoreMaterial(const Board& board);
};

#endif
