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

bool Board::isTerminalState(int8_t team) const {
	assert(team == 1 || team == -1);
	return getDjikstraScore(team) == 0 || piecesleft[team > 0 ? 0 : 1] == 0;
}

double Board::getScore() const {
	// NOTE: this should shift from early, to mid, to end game score weightings

	int whiteHorDist;
	int whiteVertDist;
	int blackHorDist;
	int blackVertDist;

	// determine the winner from running out of pieces
	if (piecesleft[1] == 0 || piecesleft[0] == 0) {
		int count = 0;
		for (int i : range(0, Board::SQUARES)) {
			if (stacks[i].top() != 0)
				count += stacks[i].top() > 0 ? 1 : -1;
		}
		return count == 0 ? 0 : (count > 0 ? 100000.0 : -100000.0);
	}

	// determine the djikstra score
	getDjikstraScore(1, &whiteHorDist, &whiteVertDist);
	getDjikstraScore(-1, &blackHorDist, &blackVertDist);

	if (whiteHorDist == 0 || whiteVertDist == 0) return 100000.0;
	if (blackHorDist == 0 || blackHorDist == 0) return -100000.0;

	// determine the djkistra' score of the board
	whiteHorDist = Board::SIZE - whiteHorDist;
	whiteVertDist = Board::SIZE - whiteVertDist;
	blackHorDist = Board::SIZE - blackHorDist;
	blackVertDist = Board::SIZE - blackVertDist;
	// whiteHorDist *= whiteHorDist;
	// whiteVertDist *= whiteVertDist;
	// blackHorDist *= blackHorDist;
	// blackVertDist *= blackVertDist;

	double djikstraScore = (whiteHorDist + whiteVertDist) - (blackHorDist + blackVertDist);
	double materialScore = getMaterialScore();

	if (isLateGame()) {
		return djikstraScore + materialScore * 4;
	} else {
		return djikstraScore + materialScore;
	}

}

/**
	material scoring algorithm
*/

const int CAP_MOBILITY_WEIGHT = 0;

// NOTE: towards late game material score becomes MUCH more important... yeah.
//       high value in having more material than the opponent / having fewer pieces.
double Board::getMaterialScore() const {
	const double COVERAGE_VALUE = 1.0;
	const double NEGIHBORS_BUFF = 3.0;

	double score = 0;

	for (int y = 0; y < Board::SIZE; ++y) {
		for (int x = 0; x < Board::SIZE; ++x) {
			const Stack& st = stacks[INDEX_BOARD(x, y)];

			int8_t top = st.top();
			if (top == 0) continue;

			double stratValue = 0;

			const double maxStrategicValue = 5 * 1.5 + 5 + NEGIHBORS_BUFF * 2;

			// value for the stack count and range and all that jazzzyness.
			const std::bitset<48> whitePieces = st.stack();
			const std::bitset<48> blackPieces = ~whitePieces;

			int numWhitePieces = whitePieces.count() - (whitePieces >> 5).count();
			int numBlackPieces = blackPieces.count() - (blackPieces >> 5).count();


			if (top > 0) {
				stratValue += numWhitePieces * 1.5 - numBlackPieces;
			} else {
				stratValue += numBlackPieces * 1.5 - numWhitePieces;
			}

			// buff for having neighbors of the same color
			if (x > 0 && stacks[INDEX_BOARD(x - 1, y)].top() * top > 0) {
				stratValue += 3;
			}
			if (y > 0 && stacks[INDEX_BOARD(x, y - 1)].top() * top > 0) {
				stratValue += 3;
			}

			// buff for a hard cap if possible!
			if (top == PIECE_CAP) {
				if (whitePieces[st.size() - 2] == 1)
					stratValue += 5;
			} else if (top == -PIECE_CAP) {
				if (blackPieces[st.size() - 2] == 1)
					stratValue += 5;
			}

			// placement value
			stratValue -= (abs(x - 2) + abs(y - 2)) * 0.5;

			// material value
			stratValue *= 1.0 / maxStrategicValue;

			if (top > 0) {
				score += stratValue + COVERAGE_VALUE;
			}
			else {
				score -= stratValue + COVERAGE_VALUE;
			}
		}
	}
	return score;
}

/**
	djikstra's algorithm based scoring function
*/
struct djknode {
	const static int16_t max_dist = 10000;
	bool visited = false;
	int8_t x = -1;
	int8_t y = -1;
	int16_t cost = max_dist;
	int16_t distance = max_dist;
	int16_t heuristic = Board::SIZE;

	template<typename T, int dx, int dy>
	void visit_neighbor(djknode* grid, T& queue) {
		if (x + dx >= 0 && x + dx < Board::SIZE && y + dy >= 0 && y + dy < Board::SIZE) {
			int ind = INDEX_BOARD(x + dx, y + dy);
			int8_t distTentative = grid[ind].cost + distance;
			if (distTentative < grid[ind].distance) grid[ind].distance = distTentative;

			if (!grid[ind].visited) {
				// enqueue if not yet vistied
				queue.push(&grid[ind]);
			}
		}
	}

	template<typename T>
	void visit_neighbors(djknode* grid, T& queue) {
		visited = true;
		visit_neighbor<T, 1, 0>(grid, queue);
		visit_neighbor<T, 0, 1>(grid, queue);
		visit_neighbor<T, -1, 0>(grid, queue);
		visit_neighbor<T, 0, -1>(grid, queue);
	}
};

bool operator < (const djknode& a, const djknode& b) {
	// return a.distance + a.heuristic < b.distance + b.heuristic;
	return a.distance < b.distance;
}

int Board::getDjikstraScore(int player, int *horizontalDistance, int *verticalDistance) const {
	assert(player == 1 || player == -1);

	int vertDist = djknode::max_dist;
	int horDist = djknode::max_dist;

	djknode graph[Board::SQUARES];
	std::priority_queue<djknode*, std::vector<djknode*>, std::greater<djknode*>> queue;

	// setup the graph
	for (int y = 0; y < Board::SIZE; ++y) {
		for (int x = 0; x < Board::SIZE; ++x) {
			int i = INDEX_BOARD(x, y);
			graph[i].x = x;
			graph[i].y = y;
			graph[i].visited = false;
			graph[i].distance = djknode::max_dist;

			int8_t top = stacks[i].top() * player;
			if (top == PIECE_WALL || top == -PIECE_WALL) {
				// TODO: tweek the distance penalty assigned to walls
				graph[i].cost = 2;
			} else if (top > 0) {
				graph[i].cost = 0;
			} else {
				graph[i].cost = 1;
			}
		}
	}

	// initialize the queue for a top -> bottom scan
	for (int i = 0; i < Board::SIZE; ++i) {
		int p = INDEX_BOARD(i, 0); // the top row.
		graph[p].distance = graph[p].cost;
		// graph[p].heuristic = Board::SIZE - i / Board::SIZE;
		graph[p].visited = true;
		queue.push(&graph[p]); // push the node into the queue...
	}

	while (!queue.empty()) {
		djknode* cur = queue.top();
		queue.pop();
		cur->visit_neighbors(graph, queue);
	}

	for (int i = 0; i < Board::SIZE; ++i) {
		int p = INDEX_BOARD(i, Board::SIZE - 1); // the bottom row
		if (graph[p].distance < vertDist)
			vertDist = graph[p].distance;
	}

	// initialize the queue for a left -> right scan
	for (int i = 0; i < Board::SQUARES; ++i) {
		// clear the state information from the graph that was set in previous run
		graph[i].visited = false;
		graph[i].distance = djknode::max_dist;
	}

	for (int i = 0; i < Board::SIZE; ++i) {
		int p = INDEX_BOARD(0, i); // the top row.
		graph[p].distance = graph[p].cost;
		// graph[p].heuristic = Board::SIZE - i % Board::SIZE;
		graph[p].visited = true;
		queue.push(&graph[p]); // push the node into the queue...
	}

	while (!queue.empty()) {
		djknode* cur = queue.top();
		queue.pop();
		cur->visit_neighbors(graph, queue);
	}

	for (int i = 0; i < Board::SIZE; ++i) {
		int p = INDEX_BOARD(Board::SIZE - 1, i); // the bottom row
		if (graph[p].distance < horDist)
			horDist = graph[p].distance;
	}

	// pass back results
	if (horizontalDistance)
		*horizontalDistance = horDist;
	if (verticalDistance)
		*verticalDistance = vertDist;

	return vertDist < horDist ? vertDist : horDist;
};
