# NyxDark
UCI-compatible chess engine built from scratch in C++

Search Techniques:
- Principal Variation Search (PVS)
- Quiescence Search
- Transposition Table
- Mate Distance Pruning
- One-Reply Extension
- Razoring
- Reverse Futility Pruning
- Null Move Reductions
- Internal Iterative Deepening (IID)
- Late Move Pruning
- Futility Pruning
- Singular Extension Search (inspired by Stockfish)
- Multi-Cut Pruning
- Late Move Reductions
- Opening Book
- LazySMP Multi-Threading

Move Ordering Techniques:
- Most Valuable Victim/Least Valuable Attacker (MVV-LVA)
- Killer Moves
- History Heuristic

The above techniques were chosen at the recommendation of the Chess Programming Wiki (https://www.chessprogramming.org/) and tested with the Cute Chess Cli.

Eval Function:
- Piece-Square Tables
- Material
- King Safety
  - Pawn Shield/Storm
  - Enemy Attack Units
  - King Obstructing Rook 
- Pawn Structure (hashed)
  - Passed Pawns
  - Unstoppable Passed Pawns
  - Candidate Passed Pawns
  - Backward Pawns
  - Isolated Pawns
  - Obstructed Pawns
- Rook
  - Open File
  - Horizontal/Vertical Mobility
- Knight
  - Outpost (on/off hole in enemy pawn structure)
  - Mobility
- Bishop
  - Pair
  - Mobility

An optimization algorithm known as **particle swarm optimization (PSO)** was used to tune the parameters of this eval function due to its simple implementation and ability to tweak many parameters at once. The parameters were "trained" on publicly-available engine games online in hopes of achieving similar strength and consistency. The engine has not yet been gauntlet-tested against other engines to estimate its ELO, so that would be a reasonable next step to truly assess its playing strength. 

To run the engine, simply run the **3.exe** executable file. Make sure the file "best_parameters.txt" remains in the same directory as the executable, as the engine requires these parameters for the eval function.
