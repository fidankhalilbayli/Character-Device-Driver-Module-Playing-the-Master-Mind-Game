# Character-Device-Driver-Module-Playing-the-Master-Mind-Game
In this two-player game, one of the players (A) chooses a secret 4-digit number and the other player (B) tries to guess what it is.
For each guess, the first player (player A) reports the number of digits correctly in-place (represented by a +) and the number of digits 
which are out-of-place (represented by a -).
Each write operation on the device is interpreted as a guess by player B. It is  evaluated by the driver and the report is stored
in an internal buffer in the form "xxxx m+ n- abcd\n" , 
where "xxxx" is the guess, "m" is the number of in-place digits, "n" is the number of out-of-place digits and "abcd" is the number 
of guesses so far. 
Each read operation on the device is a regular read from the internal buffer. <br>
Required commands: <br>
make -> compile <br>
sudo insmod ./mastermind.ko mmind_number="1234"  mmind_max_guesses=5 -> set the maximum guess number and the secret number (module parameters)
grep mastermind /proc/devices -> getting the major number <br>
mknod dev/mastermind c <major_number> 0 -> creating the device node with the major number

echo "1234" > /dev/mastermind -> writing to device <br>
cat /dev/mastermind -> reading from device

Required commands for ioctl commands:
sudo gcc -o <test> <test.c> -> compiling the file <br> 
./mmind_remaining -> to see remaining tries <br>
./mmind_newgame <new_mmind_number> -> to start new session with new mmind_number <br>
./mmind_endgame -> to end the game <br>
