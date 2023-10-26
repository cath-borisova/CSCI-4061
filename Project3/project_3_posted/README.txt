CSCI-4061 Fall 2022 - Project#3

Group Member #1: Ruth Mesfin mesfi020
- completed
    - Did worker function (Part C)
        - (C.I)
        - (C.II)
        - (C.III)
        - (C.IV)
        - (C.V)
    - debugged & completed readFromDisk
    - some of (B.IV)
    - a little bit of addIntoCache

Group Member #2: Alice Zhang zhan6698
- completed
    - getContentType
    - readFromDisk initial draft
    - most dispatch function (Part B)
        - (B.I)
        - (B.II)
        - (B.III)
        - (B.IV)
    - Some of main
        - (A.I)
        

Group Member #3: Katya Borisova boris040
- completed
    - addIntoCache
    - deleteCache
    - initCache
    - some of (B.IV)
    - some of main (Part A)
        - (A.II)
        - (A.III)
        - (A.IV)
        - (A.V)
        - (A.VI)
        - (A.VII)

Ruth & Alice worked on:
- error checking 
- debugging issues with accessing text and html files
- revising documentation
- readme
- fixing buffer overflow issues with addIntoCache




You can implement any cache replacement policy like random, FIFO,
LRU or LFU policy to choose an entry to evict when the cache is full. You must explain the policy you
implemented in the README file.

We chose to use FIFO because it makes the most sense to remove the item that's 
been in the cache for the longest time
(item that was first put in) first when the cache is full. 
We prioritize items by
how recently they were placed into the cache.
Since it's more likely for someone to want to accesss something they recently
accessed repeatedly, it's more efficient to prioitize those items. 