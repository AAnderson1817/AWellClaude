# Phase 4 — traversal report

## Static analysis — tools/traverse.py

Models walking, falling with air control, whole-tile jumps, dropping through one-way
platforms, load-signed buoyancy, and changing load at a rim. Does NOT model the gaff,
so it under-states what is reachable rather than over-stating it.

```
start room (2,2) tile (2,17)

ROOM        doors   load0 load1 load2 load3 load4   teaches
(0,0)       DR     ok    ok    ok    ok    ok      S2
(1,0)       LR     ok    ok    ok    ok    ok      S11
(2,0)       DLR    ok    ok    ok    ok    ok      S10
(3,0)       LR     ok    ok    ok    ok    ok      S8
(4,0)       DL     ok    ok    ok    ok    ok      S4
(0,1)       RU     ok    ok    ok    ok    ok      S1
(1,1)       DLR    ok    ok    ok    ok    ok      S5
(2,1)       LRU    ok    ok    ok    ok    ok      S9
(3,1)       DLR    ok    ok    ok    ok    ok      S11
(4,1)       LU     ok    ok    ok    ok    ok      S3
(0,2)       DR     ok    ok    ok    ok    ok      S6
(1,2)       LRU    ok    ok    ok    ok    ok      S1
(2,2)       DLR    ok    ok    ok    ok    ok      S3
(3,2)       LRU    ok    ok    ok    ok    ok      S10
(4,2)       DL     ok    ok    ok    ok    ok      S2
(0,3)       RU     ok    ok    ok    ok    ok      S7
(1,3)       DLR    ok    ok    ok    ok    ok      S8
(2,3)       LRU    ok    ok    ok    ok    ok      S9
(3,3)       DLR    ok    ok    ok    ok    ok      S5
(4,3)       LU     ok    ok    ok    ok    ok      S6
(0,4)       R      ok    ok    ok    ok    ok      PENDING
(1,4)       LRU    CUT   CUT   CUT   CUT   CUT     PENDING
(2,4)       LR     ok    ok    ok    ok    ok      PENDING
(3,4)       LRU    CUT   CUT   CUT   CUT   CUT     PENDING
(4,4)       L      ok    ok    ok    ok    ok      PENDING

deep brine (a load-4 body sinks and cannot swim up):
  (1,1) depth  3 tiles -> hook
  (4,1) depth  3 tiles -> rim
  (0,2) depth  4 tiles -> rim
  (0,3) depth  7 tiles -> rim
  (1,3) depth  6 tiles -> rim
  (2,3) depth  6 tiles -> rim
  (3,3) depth  5 tiles -> rim
  (4,3) depth 12 tiles -> rim

rooms reachable from the start, by doors alone: 25/25

------------------------------------------------------------
10 ISSUES
  ! (1,4) load 0: doors not mutually reachable
  ! (1,4) load 1: doors not mutually reachable
  ! (1,4) load 2: doors not mutually reachable
  ! (1,4) load 3: doors not mutually reachable
  ! (1,4) load 4: doors not mutually reachable
  ! (3,4) load 0: doors not mutually reachable
  ! (3,4) load 1: doors not mutually reachable
  ! (3,4) load 2: doors not mutually reachable
  ! (3,4) load 3: doors not mutually reachable
  ! (3,4) load 4: doors not mutually reachable
```

## Composition — tools/compose.py

The camera is locked, so all 22 rows are the composition. This measures how much of the
frame each room uses, detects the diagonal plank staircase, and compares every pair of
rooms as coarse silhouettes to catch two rooms that read as the same picture.

```
room    top(1-7)  mid(8-14)  bot(15-20)   fill%  gap  verdict
(0,0)     103        86         89      36.6    1  
(1,0)      96        27        166      38.0    1  
(2,0)      93        78        184      46.7    1  
(3,0)      84        62        181      43.0    0  
(4,0)      89        65        133      37.8    0  
(0,1)      93        54        131      36.6    0  
(1,1)      61        22        132      28.3    4  
(2,1)      75        21        109      27.0    1  
(3,1)      36        53        126      28.3    0  
(4,1)      62       106        186      46.6    1  
(0,2)      87        36        158      37.0    0  
(1,2)      60        43        107      27.6    1  
(2,2)      58        19         69      19.2    3  
(3,2)      67        45        102      28.2    1  
(4,2)      96        41         91      30.0    3  
(0,3)      94       112        228      57.1    0  
(1,3)     155        92        228      62.5    0  
(2,3)     174       128        228      69.7    0  
(3,3)      88        99        228      54.6    0  
(4,3)     104       266        228      78.7    0  
(0,4)       0         0         76      10.0   18  TOP-EMPTY DEAD-BAND(18) SPARSE 
(1,4)       0         0         76      10.0   18  TOP-EMPTY DEAD-BAND(18) SPARSE 
(2,4)       0         0         76      10.0   18  TOP-EMPTY DEAD-BAND(18) SPARSE 
(3,4)       0         0         76      10.0   18  TOP-EMPTY DEAD-BAND(18) SPARSE 
(4,4)       0         0         76      10.0   18  TOP-EMPTY DEAD-BAND(18) SPARSE 

rooms that read as the same picture:
  0.74  (2, 3) ~ (4, 3)
  0.73  (0, 3) ~ (3, 3)
  0.72  (0, 3) ~ (2, 3)

5 rooms need a composition pass:
  (0,4)  TOP-EMPTY DEAD-BAND(18) SPARSE
  (1,4)  TOP-EMPTY DEAD-BAND(18) SPARSE
  (2,4)  TOP-EMPTY DEAD-BAND(18) SPARSE
  (3,4)  TOP-EMPTY DEAD-BAND(18) SPARSE
  (4,4)  TOP-EMPTY DEAD-BAND(18) SPARSE
```
