#include <iostream>
#include <unistd.h>
#include <set>
#include <cassert>
#include <stdint.h>
#include <time.h>
#include <sstream>

#include "board.h"
#include "mcts.h"
#include "helpers.h"

int cutoffs = 0;
double negamax(Board& board, int depth, Move* result, double alpha = -100000000, double beta = 100000000) {
	// int djikScore = std::min(board.getDjikstraScore(1), board.getDjikstraScore(1));
	// if ((djikScore > 1 && (depth == 0)) || (depth == -1)) {
    //     return board.getScore() * (10 + depth);
    // }
	// if (djikScore == 0 || board.piecesleft[0] == 0 || board.piecesleft[1] == 0) {
	// 	return board.getScore() * (10 + depth);
	// }

	if (depth == 0)
		return board.getScore();
	int djikScore = std::min(board.getDjikstraScore(1), board.getDjikstraScore(1));
	if (djikScore == 0 || board.piecesleft[0] == 0 || board.piecesleft[1] == 0)
		return board.getScore();

	std::vector<Move> moves = board.get_moves();
	if (depth >= 3) {
		for (Move& m : moves) {
			m.apply(board);
			m.temp = board.getScore();
			m.revert(board);
		}

		std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
			return a.temp < b.temp;
		});
	}


	if (board.playerTurn > 0) {
		double max = -std::numeric_limits<double>::max();
		for (Move& move : moves) {
	        move.apply(board);
	        double score = negamax(board, depth - 1, nullptr, alpha, beta);
	        move.revert(board);

			if (score > alpha) alpha = score;
			if (beta <= alpha) {
				cutoffs++;
				break;
			}

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
	        double score = negamax(board, depth - 1, nullptr, alpha, beta);
	        move.revert(board);

			if (score < beta) beta = score;
			if (beta <= alpha) {
				cutoffs ++;
				break ;
			}

	        if (score < min || (score == min && rand() % 2 == 0)) {
	            if (result)
	                *result = move;
	            min = score;
	        }
	    }
		return min;
	}
}

struct Player {
	virtual Board makeAMove(Board board) = 0;
};

struct AIPlayer : public Player {
	virtual Board makeAMove(Board board) {
		Move move;
		int lastCutoffs = cutoffs;
		double score = negamax(board, 6, &move);
		std::cout << "AI Player generated move with score: " << score << std::endl;
		std::cout << "\tcutoffs: " << cutoffs - lastCutoffs << std::endl;
		move.apply(board);
		return board;
	}
};

struct HumanPlayer : public Player {
	virtual void clearLastLine(std::ostream& s) {
		s << "\033[F\033[2K" << std::flush;
	}

	int parsePosition(const std::string str) {
		if (str.size() < 2 || str[0] < 'A' || str[0] >= 'A' + Board::SIZE || str[1] < '1' || str[1] >= '1' + Board::SIZE)
			throw "bad position";
		return INDEX_BOARD(str[0] - 'A', str[1] - '1');
	}

	virtual void clearBoard(const Board& board, std::ostream& s) {
		std::stringstream ss;
		ss << board;
		std::string str = ss.str();
		for (int i : range(0, std::count(str.begin(), str.end(), '\n'))) {
			clearLastLine(s);
		}
	}

	virtual Board makeAMove(Board board) {
		std::cout << "Human Move (" << (board.playerTurn > 0 ? "White" : "Black") << ") ..." << std::endl;
		std::string line;

		std::set<uint64_t> validStates;
		for (Move& m : board.get_moves()) {
			m.apply(board);
			validStates.insert(board.hash());
			m.revert(board);
		}

		while (true) {
			std::cout << " -> ";
			if (!std::getline(std::cin, line)) break ;

			std::istringstream iss(line);
			std::string command;
			iss >> command;

			Board boardCopy = board;

			if (command == "split") {
				std::string arg;
				iss >> arg;
				if (arg.length() < 4 || (arg[2] != 'u' && arg[2] != 'd' && arg[2] != 'l' && arg[2] != 'r')) {
					std::cout << "split must have format: [POS][udlr][stack sizes]*" << std::endl;
					continue ;
				}

				int origin;
				try {
					origin = parsePosition(arg.substr(0, 2));
				} catch (const char *error) {
					std::cerr << "error: " << error << std::endl;
					continue;
				}

				int count = boardCopy.stacks[origin].size();
				if (count > Board::SIZE) count = Board::SIZE;
				if (count == 0) {
					std::cerr << "error: No pieces in the specified stack" << std::endl;
					continue ;
				}

				int delta;
				if (arg[2] == 'u')
					delta = -Board::SIZE;
				else if (arg[2] == 'd')
					delta = Board::SIZE;
				else if (arg[2] == 'l')
					delta = -1;
				else
					delta = 1;

				int dest = origin;

				bool error = false;
				for (size_t i = 3; i < arg.length(); ++i) {
					int no = arg[i] - '0';
					if (no > count || no < 0) {
						std::cerr << "error: tried to move more pieces than there are in the stack! " << no << std::endl;
						error = true;
						break ;
					}
					dest += delta;
					if (dest < 0 || dest >= Board::SQUARES) {
						std::cerr << "error: walked off the board!" << std::endl;
						error = true;
						break ;
					}
					boardCopy.move(origin, dest, no);
					count -= no;
				}
				if (error) continue;

				boardCopy.moveno++;
				boardCopy.playerTurn = -boardCopy.playerTurn;
			} else if (command == "place") {
				std::string arg;
				iss >> arg;
				if (arg.length() != 3) {
					std::cerr << "error. Expected placement format [position][piece W F C]" << std::endl;
					continue ;
				}

				int origin;
				try {
					origin = parsePosition(arg.substr(0, 2));
				} catch (const char *error) {
					std::cerr << "error: " << error << std::endl;
					continue;
				}

				switch (arg[2]) {
				case 'F':
					boardCopy.piecesleft[boardCopy.placementColor() > 0 ? 0 : 1]--;
					boardCopy.place(origin, PIECE_FLAT * boardCopy.placementColor());
					break ;
				case 'W':
					boardCopy.piecesleft[boardCopy.placementColor() > 0 ? 0 : 1]--;
					boardCopy.place(origin, PIECE_WALL * boardCopy.placementColor());
					break ;
				case 'C':
					boardCopy.capstones[boardCopy.placementColor() > 0 ? 0 : 1]--;
					boardCopy.place(origin, PIECE_CAP * boardCopy.placementColor());
					break ;
				default:
					std::cerr << "Unknown piece flag " << arg[2] << std::endl;
					continue ;
				}

				boardCopy.moveno++;
				boardCopy.playerTurn = -boardCopy.playerTurn;
			}

			if  (boardCopy != board) {
				// TODO: prompt to confirm changes.
				std::cout << boardCopy;

				if (validStates.find(boardCopy.hash()) == validStates.end()) {
					std::cerr << "Invalid move!" << std::endl;
					continue ;
				}

				std::cout << "confirm board changes (y/n)? ";
				std::string confirm;
				std::cin >> confirm;
				if (confirm[0] == 'y') {
					board = boardCopy;
					clearBoard(boardCopy, std::cout);
					clearLastLine(std::cout);
					std::cout << "Moved successfully." << std::endl;
					break ;
				}
			}
		}

		return board;
	};
};



int main() {

	// TODO: if the first move place the opponent in a corner
	// NOTE: Adjacent corner is best, but mix in opposite corner to keep them on their toes

	Board board;

	// board.place(0, -PIECE_FLAT);
	// board.place(1, PIECE_FLAT);
	// board.moveno = 2;
	//
	// for (auto move : board.get_moves()) {
	// 	move.apply(board);
	// 	std::cout << board << std::endl;
	// 	move.revert(board);
	// }

	srand(time(0));

	Player *white = new HumanPlayer();
	Player *black = new AIPlayer();

	while (true) {
		Player *cur = board.playerTurn == 1 ? white : black;

		std::cout << "\n\n\n\n" << std::endl;
		board = cur->makeAMove(board);

		std::cout << board;
		std::cout << "\tboard score: " << board.getScore() << std::endl;
		std::cout << "\tdjikstra white: " << board.getDjikstraScore(1) << std::endl;
		std::cout << "\tdjikstra black: " << board.getDjikstraScore(-1) << std::endl;

		if (board.isTerminalState()) break ;
	}
	std::cout << "Done." << std::endl;
	std::cout << "\t" << (board.playerTurn < 0 ? "White" : "Black") << " Wins!" << std::endl;
}
