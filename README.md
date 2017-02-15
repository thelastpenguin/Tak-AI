# A Tak AI by Gareth George

# 3 Different AI's
 - https://github.com/Konijnendijk/C-MCTS/tree/master/src <- MCTS most promising
https://github.com/PetterS/monte-carlo-tree-search.git
https://github.com/Konijnendijk/C-MCTS <- really promising I think

# TBG Board Game Encoding
*TBG stands for tak board game encoding*

 - all whitespace in the file should be stripped or ignored
 - the file should be case in-sensative
 - the file is structured as a comma or semicolon separated list of values. It is recommmended to use comma's as the primary delimiter and a ';' to delimit the boundary between the metadata and the board state for readability.
 - the sequence of fields is as follows
 	- meta data:
		- turn number (integer base10)
		- current player turn ('w' for white 'b' for black). If black just placed a piece then it should be 'w' for white's turn
		- board width / height (integer base10)
		- white pieces left (integer base10)
		- black pieces left (integer base10)
		- white capstones left (integer base10)
		- black capstones left (integer base10)
	- board data
		- a list of the colors of the pieces in the stack listed as 'w' for white and 'b' for black going from bottom to top.
		- a single character denoting the type of the piece on the top of the stack
			- F for flat
			- S for standing stone
			- C for cap stone

example:			
```
Move #0   White turn
                A              B              C              D              E
1 |              |+++[bF]       |              |++[wW]        |              |
2 |              |              |              |              |              |
3 |              |              |              |              |              |
4 |              |              |              |              |              |
5 |              |              |              |              |              |
	TBGE: 5,w,5,17,18,1,1;,wbwbF,,wbwS,,,,,,,,,,,,,,,,,,,,,
```
