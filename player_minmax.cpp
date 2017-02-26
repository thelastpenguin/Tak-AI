#include <algorithm>
#include <iostream>

#include "player.h"


int cutoffs = 0;
double MinmaxPlayer::minmax(Board& board, int depth, Move* result) {
	int winner = board.getWinner();
	if (winner != 0) return winner * 1000000.0 * (depth + 1);
	if (depth == 0) return scoreBoard(board);

	std::vector<Move> moves = board.get_moves();

	if (board.playerTurn > 0) {
		double max = -std::numeric_limits<double>::max();
		for (Move& move : moves) {
			move.apply(board);
			double score = this->minmax(board, depth - 1, nullptr);
			move.revert(board);

			// if (score > alpha) alpha = score;
			// if (beta <= alpha) {
			// 	cutoffs++;
			// 	break;
			// }

	        if (score > max || (score == max && rand() % 2 == 0)) {
	            if (result)
	                *result = move;
	            max = score;
	        }
	    }
		return max;
	} else {
		double min = std::numeric_limits<double>::max();
		for (Move& move : moves) {
	        move.apply(board);
	        double score = this->minmax(board, depth - 1, nullptr);
	        move.revert(board);

			// if (score < beta) beta = score;
			// if (beta <= alpha) {
			// 	cutoffs ++;
			// 	break ;
			// }

	        if (score < min || (score == min && rand() % 2 == 0)) {
	            if (result)
	                *result = move;
	            min = score;
	        }
	    }
		return min;
	}
}

Board MinmaxPlayer::makeAMove(Board board) {
	Move move;
	int lastCutoffs = cutoffs;
	double score = minmax(board, depth, &move);
	std::cout << "AI Player generated move with score: " << score << std::endl;
	std::cout << "\tcutoffs: " << cutoffs - lastCutoffs << std::endl;
	move.apply(board);
	std::cout << "board material score: " << scoreMaterial(board) << std::endl;
	std::cout << "move: " << move.toString() << std::endl;
	return board;
}

double MinmaxPlayer::scoreBoard(const Board& board) {
	double mat =  this->scoreMaterial(board);

	int horDjkWhite;
	int vrtDjkWhite;
	int horDjkBlack;
	int vrtDjkBlack;

	board.getDjikstraScore(1, &horDjkWhite, &vrtDjkWhite);
	board.getDjikstraScore(-1, &vrtDjkBlack, &horDjkBlack);

	horDjkWhite = Board::SIZE - horDjkWhite;
	horDjkBlack = Board::SIZE - horDjkBlack;
	vrtDjkWhite = Board::SIZE - vrtDjkWhite;
	vrtDjkBlack = Board::SIZE - vrtDjkBlack;

	double djk = horDjkWhite - horDjkBlack + vrtDjkWhite - vrtDjkBlack;

	return djk * 4.0 + mat;
}

double MinmaxPlayer::scoreMaterial(const Board& board) {
	double score = 0;

	for (int y = 0; y < Board::SIZE; ++y) {
		for (int x = 0; x < Board::SIZE; ++x) {
			const Stack& st = board.stacks[INDEX_BOARD(x, y)];

			int8_t top = st.top();
			if (top == 0) continue;

			// value for the stack count and range and all that jazzzyness.
			const std::bitset<48> whitePieces = st.stack();
			const std::bitset<48> blackPieces = ~whitePieces;

			double stratValue = 0;

			int numWhitePieces = whitePieces.count() - (whitePieces >> 5).count();
			int numBlackPieces = blackPieces.count() - (blackPieces >> 5).count();

			if (top > 0) {
				stratValue += (numWhitePieces * 1.3 - numBlackPieces) * 0.3;
			} else {
				stratValue += (numBlackPieces * 1.3 - numWhitePieces) * 0.3;
			}

			// buff for having neighbors of the same color
			double castleBuff = 1.0;
			if (x > 0 && board.stacks[INDEX_BOARD(x - 1, y)].top() * top > 0) {
				castleBuff *= 2.0;
			}
			if (y > 0 && board.stacks[INDEX_BOARD(x, y - 1)].top() * top > 0) {
				castleBuff *= 2.0;
			}
			if (y > 0 && x > 0 && board.stacks[INDEX_BOARD(x - 1, y - 1)].top() * top > 0) {
				castleBuff *= 2.0;
			}
			if (castleBuff != 1.0)
				stratValue += castleBuff * 0.2;

			// buff for a hard cap if possible!
			if (top == PIECE_CAP) {
				if (whitePieces[st.size() - 2] == 1)
					stratValue += 2;
			} else if (top == -PIECE_CAP) {
				if (blackPieces[st.size() - 2] == 1)
					stratValue += 2;
			}

			// placement value
			stratValue -= (abs(x - 2) + abs(y - 2)) * 0.075;

			if (top > 0)
				score += stratValue + 1.0;
			else
				score -= stratValue + 1.0;
		}
	}

	return score;
}
