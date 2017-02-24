#include <sstream>
#include <istream>

#include <algorithm>

#include "board.h"
#include "termcolor.h"
#include "helpers.h"

const char* piece_to_string(int8_t piece) {
	switch (piece) {
	case PIECE_FLAT: return "wF";
	case -PIECE_FLAT: return "bF";
	case PIECE_WALL: return "wW";
	case -PIECE_WALL: return "bW";
	case PIECE_CAP: return "wC";
	case -PIECE_CAP: return "bC";
	case 0: return " ";
	default: return "unknown";
	}
}

Board::Board() {
	bzero(this, sizeof(Board));
	moveno = 0;
	playerTurn = 1;

	capstones[0] = 1;
	capstones[1] = 1;
	piecesleft[0] = 21;
	piecesleft[1] = 21;
}

Board::Board(const std::string& tbgEncoding) : Board() {
	std::string encoding(tbgEncoding);
	std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::tolower);
	std::replace(encoding.begin(), encoding.end(), ';', ',');

	std::string segment;
	std::istringstream ss(encoding);

	int i = 0;

	while (std::getline(ss, segment, ',')) {
		i++;
		if (i == 1) {
			this->moveno = std::stoi(segment);
		} else if (i == 2) {
			if (segment == "w") {
				this->playerTurn = 1;
			} else if (segment == "b") {
				this->playerTurn = -1;
			} else {
				std::cerr << "invalid current player turn '" << segment << std::endl;
				exit(1); // TODO: learn about throwing exceptions
			}
		} else if (i == 3) {
			assert(std::stoi(segment) == Board::SIZE);
		} else if (i == 4) {
			this->piecesleft[0] = std::stoi(segment);
		} else if (i == 5) {
			this->piecesleft[1] = std::stoi(segment);
		} else if (i == 6) {
			this->capstones[0] = std::stoi(segment);
		} else if (i == 7) {
			this->capstones[1] = std::stoi(segment);
		} else {
			int stack = i - 8;
			if (segment.length() >= 2) {
				for (size_t i = 0; i < segment.length() - 2; ++i) {
					if (segment[i] == 'b')
						place(stack, -PIECE_FLAT);
					else if (segment[i] == 'w')
						place(stack, PIECE_FLAT);
				}
				int8_t team = segment[segment.length() - 2] == 'w' ? 1 : -1;
				switch (segment[segment.length() - 1]) {
				case 'f': place(stack, team * PIECE_FLAT); break ;
				case 's': place(stack, team * PIECE_WALL); break ;
				case 'c': place(stack, team * PIECE_CAP); break ;
				default:
					std::cerr << "invalid piece type on top of stack. error" << std::endl;
					exit(1);
				}
			}
		}
	}
}

int Board::isTerminalState(int8_t team) const {
	uint8_t num_pieces = 0;
	int8_t piece_positions[Board::SQUARES];
	int8_t top[Board::SQUARES];

	// NOTE: todo finish code to check if the terminal state has been reached...

	for (int i = 0; i < Board::SQUARES; ++i) {
		top[i] = stacks[i].top();
		if (top[i] * team > 0) {
			piece_positions[num_pieces++] = i;
		}
	}

	for (int i = 0; i < Board::SQUARES; ++i) {

	}

	return 0;
}

std::string Board::toTBGEncoding() const {
	std::stringstream ss;

	ss << moveno << "," << (playerTurn > 0 ? "w" : "b") << "," << SIZE << ",";
	ss << piecesleft[0] << "," << piecesleft[1] << "," << capstones[0] << "," << capstones[1];

	bool isFirst = true;

	for (int y = 0; y < 5; ++y) {
		for (int x = 0; x < 5; ++x) {
			ss << (isFirst ? ";" : ",");
			isFirst = false;
			const Stack& stack = stacks[x + y * Board::SIZE];
			for (int i = 0; i < stack.size(); ++i) {
				if (stack.stack()[i])
					ss << "w";
				else
					ss << "b";
			}
			if (stack.size() >= 1) {
				switch (stack.top()) {
				case PIECE_FLAT:
				case -PIECE_FLAT: ss << "F"; break ;
				case PIECE_WALL:
				case -PIECE_WALL: ss << "S"; break ;
				case PIECE_CAP:
				case -PIECE_CAP: ss << "C"; break ;
				}
			}
		}
	}

	return ss.str();
}

std::ostream& operator << (std::ostream& out, const Board& board) {
	out << "Move #" << board.moveno << "   " << (board.playerTurn > 0 ? "Black moved. White turn." : "White moved. Black turn.") << std::endl;

	bool useColors = &out == &std::cout;

	char buffer[80];
	out << "  ";
	for (int i = 0; i < Board::SIZE; ++i) {
		sprintf(buffer, "%15c", 'A' + i);
		out << buffer;
	}
	out << std::endl;

	for (int y = 0; y < Board::SIZE; ++y) {
		out << y + 1 << " ";
		for (int x = 0; x < Board::SIZE; ++x) {
			out << "|";
			const Stack& stack = board.stacks[x + y * Board::SIZE];
			for (int i = 0; i < stack.size() - 1; ++i) {
				if (stack.stack()[i]) {
					if (useColors) out << termcolor::white << "+";
					else out << "w";
				}
				else {
					if (useColors) out << termcolor::red << "+";
					else  out << "b";
				}
			}
			if (stack.size() >= 1) {
				if (useColors) {
					if (stack.top() < 0) out << termcolor::red; else out << termcolor::white;
				}
				out << "[" << piece_to_string(stack.top()) << "]";
			} else
				out << "   ";
			for (int i = 10 - stack.size(); i >= 0; --i) {
				out << " ";
			}
			if (useColors)
				out << termcolor::reset;
		}
		out << "|" << std::endl;
	}

	out << "\t" << "TBGE: " << board.toTBGEncoding() << std::endl;

	return out;
};

/**
	move generation
*/

struct MoveInternal {
	const static int8_t TYPE_PLACE = 1;
	const static int8_t TYPE_SPLIT = 2;
	const static int8_t TYPE_SPLIT_SQUASH = 3;

	MoveInternal() { bzero(this, sizeof(MoveInternal)); };

	uint32_t moveid;

	int8_t type;

	int8_t position;
	int8_t piece;

	int8_t split_count;
	int8_t split_positions[Board::SIZE];
	int8_t split_sizes[Board::SIZE];

	inline static int8_t piece_color(const Board& board) {
		if (board.moveno < 2)
			return -board.playerTurn;
		else
			return board.playerTurn;
	}

	bool can_move(const Board& board) {
		if (type == TYPE_PLACE) {
			switch (piece) {
			case PIECE_CAP: return board.capstones[0] > 0;
			case -PIECE_CAP: return board.capstones[1] > 0;
			case PIECE_WALL:
			case PIECE_FLAT: return board.piecesleft[0] > 0;
			case -PIECE_WALL:
			case -PIECE_FLAT: return board.piecesleft[1] > 0;
			default:
				std::cerr << "invalid piece being placed. " << (int) piece << " Exiting" << std::endl;
				exit(0);
			}
		} else if(type == TYPE_SPLIT) {
			const int8_t landingOn = board.stacks[split_positions[split_count - 1]].top();
			return landingOn == 0 || landingOn == -PIECE_FLAT || landingOn == PIECE_FLAT;
		} else if (type == TYPE_SPLIT_SQUASH) {
			const int8_t landingOn = std::abs(board.stacks[split_positions[split_count - 1]].top());
			return landingOn == PIECE_WALL;
		}
		return false;
	}

	void apply(Board& board) const {
		const int8_t piece_color = MoveInternal::piece_color(board);
		board.playerTurn = -board.playerTurn;
		board.moveno++;
		if (type == TYPE_PLACE) {
			board.place(position, piece * piece_color);
			switch (piece * piece_color) {
			case PIECE_CAP: board.capstones[0]--; break ;
			case -PIECE_CAP: board.capstones[1]--; break ;
			case PIECE_FLAT:
			case PIECE_WALL: board.piecesleft[0]--; break ;
			case -PIECE_FLAT:
			case -PIECE_WALL: board.piecesleft[1]--; break ;
			}
			return ;
		}

		for (int8_t i = 0; i < split_count; ++i) {
			board.move(position, split_positions[i], split_sizes[i]);
		}
	}

	void revert(Board& board) const {
		board.moveno--;
		board.playerTurn = -board.playerTurn;
		if (type == TYPE_PLACE) {
			const int8_t piece_color = MoveInternal::piece_color(board);
			board.remove(position);
			switch (piece * piece_color) {
			case PIECE_CAP: board.capstones[0]++; break ;
			case -PIECE_CAP: board.capstones[1]++; break ;
			case PIECE_FLAT:
			case PIECE_WALL: board.piecesleft[0]++; break ;
			case -PIECE_FLAT:
			case -PIECE_WALL: board.piecesleft[1]++; break ;
			}
			return ;
		}

		for (int8_t i = split_count - 1; i >= 0; --i) {
			board.move(split_positions[i], position, split_sizes[i]);
		}

		if (type == TYPE_SPLIT_SQUASH) {
			const int8_t last = split_positions[split_count - 1];
			const int8_t wall = board.stacks[last].top() > 0 ? PIECE_WALL : -PIECE_WALL;
			board.remove(last);
			board.place(last, wall);
		}
	}
};

namespace movegen {
	std::vector<MoveInternal> all_moves;

	MoveInternal placements[Board::SQUARES][3];

	std::vector<MoveInternal> cuts[Board::SQUARES][Board::SIZE + 1]; // 0 index isn't used
	std::vector<MoveInternal> cuts_flatten[Board::SQUARES][Board::SIZE + 1]; // 0 index isn't used


	// generate all sequences of integers that reach the sum 'n'
	void target_sum(int n, std::vector<int> current, std::vector<std::vector<int>>& results) {
		for (int i = 1; i < n; ++i) {
			std::vector<int> copy = current;
			copy.push_back(i);
			target_sum(n - i, copy, results);
		}
		std::vector<int> copy = current;
		copy.push_back(n);
		results.push_back(copy);
	}

	// generate all moves that reach length given
	std::vector<MoveInternal> generate_moves(int x, int y, int dx, int dy, int distance, int piecesUsed) {
		std::vector<MoveInternal> result;

		int d = dx + dy * Board::SIZE;
		std::vector<std::vector<int>> vectors;
		target_sum(piecesUsed, std::vector<int>(), vectors);

		std::sort(vectors.begin(), vectors.end(), [](std::vector<int>& a, std::vector<int>& b) {
			return a.size() < b.size();
		});

		for (std::vector<int>& vec : vectors) {
			if (vec.size() > (size_t)distance) continue;
			MoveInternal move;
			move.position = x + y * Board::SIZE;
			move.split_count = vec.size();
			for (int i : range(0, vec.size())) {
				move.split_positions[i] = move.position + d * (i + 1);
				move.split_sizes[i] = vec[i];
			}
			result.push_back(move);
		}

		return result;
	}

	struct _InitializeMoveCache {
		void generate_cuts_for_position(int x, int y) {
			const int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

			for (const int* d : directions) {
				int dx = d[0];
				int dy = d[1];
				int maxrange = Board::SIZE;

				if (dx < 0)
					maxrange = std::min(maxrange, x);
				if (dx > 0)
					maxrange = std::min(maxrange, Board::SIZE - x - 1);
				if (dy < 0)
					maxrange = std::min(maxrange, y);
				if (dy > 0)
					maxrange = std::min(maxrange, Board::SIZE - y - 1);

				if (maxrange == 0) continue ;
				for (int pieceCount : range(1, Board::SIZE + 1)) {
					std::vector<MoveInternal> moves = generate_moves(x, y, dx, dy, std::min(maxrange, pieceCount), pieceCount);

					// NOTE: adds normal moves
					for (auto& move : moves) {
						move.type = MoveInternal::TYPE_SPLIT;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts[INDEX_BOARD(x, y)][pieceCount].push_back(move);
					}

					// NOTE: adds flatten moves
					for (auto& move : moves) {
						if (move.split_sizes[move.split_count - 1] != 1) continue ;
						move.type = MoveInternal::TYPE_SPLIT_SQUASH;
						move.moveid = all_moves.size();
						all_moves.push_back(move);
						cuts_flatten[INDEX_BOARD(x, y)][pieceCount].push_back(move);
					}
				}
			}
		}

		void generate_placements_for_position(int x, int y) {
			int i = x + y * Board::SIZE;

			MoveInternal move;

			move.type = MoveInternal::TYPE_PLACE;
			move.position = i;

			move.piece = PIECE_FLAT;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i][0] = move;

			move.piece = PIECE_WALL;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i][1] = move;

			move.piece = PIECE_CAP;
			move.moveid = all_moves.size();
			all_moves.push_back(move);
			placements[i][2] = move;
		}

		void generate_moves_for_position(int x, int y) {
			generate_placements_for_position(x, y);
			generate_cuts_for_position(x, y);
		}

		_InitializeMoveCache() {
			for (int y : range(0, Board::SIZE)) {
				for (int x : range(0, Board::SIZE)) {
					generate_moves_for_position(x, y);
				}
			}
		}
	};

	_InitializeMoveCache _movecache;

}


std::vector<Move> Board::get_moves(int8_t team) const {
	uint64_t boardhash = this->hash();

	std::vector<Move> moves;
	for (int i = 0; i < Board::SQUARES; ++i) {
		const Stack& stack = stacks[i];
		int8_t top = stack.top();

		if (top == 0) {
			// NOTE: these moves are for the top of the stack
			for (int j = 0; j < 3; ++j) {
				MoveInternal& move = movegen::placements[i][j];
				if (move.can_move(*this))
					moves.push_back(Move(boardhash, move.moveid));
			}
		} else if (top * team > 0 && moveno >= 2) {
			bool stop = false;
			for (int j = 1; j <= stack.size() && !stop; ++j) {
				for (MoveInternal& move : movegen::cuts[i][j]) {
					if (move.can_move(*this)) {
						moves.push_back(Move(boardhash, move.moveid));
					} else
						stop = true;
				}
				if (stop)
					for (MoveInternal& move : movegen::cuts_flatten[i][j]) {
						if (move.can_move(*this)) {
							moves.push_back(Move(boardhash, move.moveid));
						}
					}
			}
		}
	}

	return moves;
};

void Move::apply(Board& board) const {
	movegen::all_moves[moveid].apply(board);
}

void Move::revert(Board& board) const {
	movegen::all_moves[moveid].revert(board);
}
