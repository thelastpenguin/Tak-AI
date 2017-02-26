#include <iostream>
#include <algorithm>
#include <sstream>
#include <set>

#include "player.h"
#include "helpers.h"


void clearLastLine(std::ostream& s) {
	s << "\033[F\033[2K" << std::flush;
}

int parsePosition(const std::string str) {
	if (str.size() < 2 || str[0] < 'A' || str[0] >= 'A' + Board::SIZE || str[1] < '1' || str[1] >= '1' + Board::SIZE)
		throw "bad position";
	return INDEX_BOARD(str[0] - 'A', str[1] - '1');
}

void clearBoard(const Board& board, std::ostream& s) {
	std::stringstream ss;
	ss << board;
	std::string str = ss.str();
	for (int i : range(0, std::count(str.begin(), str.end(), '\n'))) {
		clearLastLine(s);
	}
}

Board HumanPlayer::makeAMove(Board board) {
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
		} else if (command == "moves?") {
			std::cout << "Current board: " << std::endl;
			std::cout << board << std::endl;
			std::cout << "Move generation list?" << std::endl;
			for (auto m : board.get_moves()) {
				m.apply(board);
				std::cout << board << std::endl;
				m.revert(board);
			}
		}

		std::cout << boardCopy << std::endl;

		if  (boardCopy != board) {
			if (validStates.find(boardCopy.hash()) == validStates.end()) {
				std::cerr << "Invalid move!" << std::endl;
				continue ;
			}

			std::cout << boardCopy;

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
