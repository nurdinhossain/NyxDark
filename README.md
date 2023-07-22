# Nyx-CPP
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

Move Ordering Techniques:
- Most Valuable Victim/Least Valuable Attacker (MVV-LVA)
- Killer Moves
- History Heuristic

Eval Function:
- Piece-Square Tables
- Material
- King Safety
-   Pawn Shield/Storm
-   Enemy Attack Units
-   King Obstructing Rook 
- Pawn Structure (hashed)
-   Passed Pawns
-   Unstoppable Passed Pawns
-   Candidate Passed Pawns
-   Backward Pawns
-   Isolated Pawns
-   Obstructed Pawns
- Rook
-   Open File
-   Horizontal/Vertical Mobility
- Knight
-   Outpost (on/off hole in enemy pawn structure)
-   Mobility
- Bishop
  - Pair
  - Mobility

An optimization algorithm known as **particle swarm optimization (PSO)** was used to tune the parameters of this eval function due to its simple implementation and ability to tweak many parameters at once.

To run the engine, simply run the **3.exe** executable file.
