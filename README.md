<div align="center">

# Cataphract

[![License][license-badge]][license-link]
[![Release][release-badge]][release-link]
[![Commits][commits-badge]][commits-link]

___

</div>

A UCI chess engine in modern C++.

## Features:
### Search:
* Principal variation search with iterative deepening
* Aspiration windows
* Futility pruning and reverse futility pruning
* Razoring
* Mate distance pruning
* Null move pruning
* Late move reductions
* Late move pruning
* Improving heuristic
* History pruning
* Internal iterative reduction
* Static exchange evaluation
* Probcut
* Singular extension
* Multicut
* Negative extensions
* Lazy SMP
#### History:
* Main history (Piece-to and butterfly)
* Killer moves
* Countermove history (1-ply), follow-up history (2-ply), and 4-ply history.
* Capture history
#### Correction history:
* Pawn correction history
* Non-pawn correction history
* Continuation correction history
* Major piece correction history
> Parameters are tuned automatically using SPSA.
### Evaluation
#### NNUE 
* Architecture: (768x16 → 1280)x2 → 1x8, horizontally mirrored.
* Trained using the [Bullet](https://github.com/jw1912/bullet) trainer.

[license-badge]: https://img.shields.io/github/license/masceron/Cataphract?style=for-the-badge
[license-link]: https://github.com/masceron/Cataphract/blob/main/LICENSE

[release-badge]: https://img.shields.io/github/v/release/masceron/Cataphract?style=for-the-badge
[release-link]: https://github.com/masceron/Cataphract/releases/latest

[commits-badge]: https://img.shields.io/github/commits-since/masceron/Cataphract/latest?style=for-the-badge
[commits-link]: https://github.com/masceron/Cataphract/commits/main