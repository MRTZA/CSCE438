README

Bradley Kost
UIN: 825007674
Murtaza Hakimi
UIN: 325003943

To compile our code you place the makefile in the same directory as the
crc.c, crsd.c, and interface.h. Then "make" while in that directory.
After you must run the "server" executable and then the "client"
executable in that order.

NOTE: Our server's main loop is iterative. So the loop that handles all
the commands will sometimes get stuck waiting on other clients to enter
their commands before it will allow the a given client to complete their
own command. Once all the clients are in the chatrooms there is no problem
with blocking and waiting, only when the clients are in "command" mode.