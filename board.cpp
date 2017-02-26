#include <sstream>
#include <istream>

#include <algorithm>
#include <functional>
#include <queue>

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
	piecesleft[0] = PIECES_PER_SIDE;
	piecesleft[1] = PIECES_PER_SIDE;
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

int Board::getWinner() const {
	int djikstraWhite = this->getDjikstraScore(1);
	if (djikstraWhite == 0) return 1;
	int djikstraBlack = this->getDjikstraScore(-1);
	if (djikstraBlack == 0) return -1;
	if (piecesleft[0] == 0 || piecesleft[1] == 0) {
		int count = 0;
		for (int i : range(0, Board::SQUARES)) {
			if (stacks[i].top() != 0)
				count += stacks[i].top() > 0 ? 1 : -1;
		}
		if (count == 0) return playerTurn;
		return count > 0 ? 1 : -1;
	}
	return 0;
}

/**
	djikstra's algorithm based scoring function
*/
struct djknode {
	const static int16_t max_dist = INT16_MAX;
	bool visited = false;
	int8_t x = -1;
	int8_t y = -1;
	int16_t cost = max_dist;
	int16_t distance = max_dist;

	template<int8_t dx, int8_t dy, typename T>
	void visit_neighbor(djknode* grid, T& queue) {
		if (x + dx >= 0 && x + dx < Board::SIZE && y + dy >= 0 && y + dy < Board::SIZE) {
			int ind = INDEX_BOARD(x + dx, y + dy);
			if (!grid[ind].visited) {
				int8_t distTentative = grid[ind].cost + distance;
				if (distTentative < grid[ind].distance) {
					grid[ind].distance = distTentative;
				}
				queue.push(&grid[ind]);
			}
		}
	}

	template<typename T>
	void visit_neighbors(djknode* grid, T& queue) {
		visited = true;
		visit_neighbor<1, 0>(grid, queue);
		visit_neighbor<0, 1>(grid, queue);
		visit_neighbor<-1, 0>(grid, queue);
		visit_neighbor<0, -1>(grid, queue);
	}
};

int getShortestPath(int16_t costs[Board::SQUARES]) {
	djknode graph[Board::SQUARES];

	auto compareFunc = [](djknode* a, djknode* b) {
		return a->distance > b->distance;
	};
	std::priority_queue<djknode*, std::vector<djknode*>, decltype(compareFunc)> queue(compareFunc);

	for (int i = 0; i < Board::SQUARES; ++i) {
		graph[i].x = i % Board::SIZE;
		graph[i].y = i / Board::SIZE;
		graph[i].cost = costs[i];
		graph[i].distance = INT16_MAX;
		graph[i].visited = false;
	}

	for (int i = 0; i < Board::SIZE; ++i) {
		graph[i].distance = graph[i].cost;
		graph[i].visit_neighbors(graph, queue);
	}

	while (!queue.empty()) {
		djknode* top = queue.top();
		queue.pop();

		top->visit_neighbors(graph, queue);
	}

	int16_t minDist = INT16_MAX;
	for (int i = Board::SQUARES - Board::SIZE; i < Board::SQUARES; ++i) {
		if (graph[i].distance < minDist)
			minDist = graph[i].distance;
	}

	return minDist;
}

int Board::getDjikstraScore(int player, int *horizontalDistance, int *verticalDistance) const {
	int horDist;
	int vertDist;

	int16_t costsTopBottom[Board::SQUARES];
	int16_t costsLeftRight[Board::SQUARES];
	for (int y = 0; y < Board::SIZE; ++y) {
		for (int x = 0; x < Board::SIZE; ++x) {
			int8_t top = stacks[INDEX_BOARD(x, y)].top();
			// TODO: update
			int16_t cost = (top * player == PIECE_FLAT || top * player == PIECE_CAP) ? 0 : 1; // ((top == PIECE_WALL || top == -PIECE_WALL) ? 1 : 1);
			costsTopBottom[INDEX_BOARD(x, y)] = cost;
			costsLeftRight[INDEX_BOARD(y, x)] = cost;
		}
	}

	horDist = getShortestPath(costsLeftRight);
	vertDist = getShortestPath(costsTopBottom);

	if (horizontalDistance)
		*horizontalDistance = horDist;
	if (verticalDistance)
		*verticalDistance = vertDist;

	return std::min(horDist, vertDist);
};
