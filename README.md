# Character-Device-Driver-Module-Playing-the-Master-Mind-Game
In this two-player game, one of the players (A) chooses a secret 4-digit number and the other player (B) tries to guess what it is.
For each guess, the first player (player A) reports the number of digits correctly in-place (represented by a +) and the number of digits 
which are out-of-place (represented by a -).
Each write operation on the device is interpreted as a guess by player B. It is  evaluated by the driver and the report is stored
in an internal buffer in the form "xxxx m+ n- abcd\n" , 
where "xxxx" is the guess, "m" is the number of in-place digits, "n" is the number of out-of-place digits and "abcd" is the number 
of guesses so far. 
Each read operation on the device is a regular read from the internal buffer.
