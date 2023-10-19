# Shell
Reimplement a "simple" shell in c. To be used on Linux.

To use it, get the code, type `make` on your command line and then `./shell`. Now you can play!

It behave like a real shell so worry not `$ rm -rf /` or you'll regret...

To leave the shell click `<Ctrl>+D`. If you are using a Mac then I don't know, struggle I guess.

I've implemented my own `ls` and `cd` commands, so arguments and behaviors may change for the first one. Look into the code to make it back to normal --- I don't know in which position it is now...

Finally, I've implemented a very basic variable system that -- unfortunatly -- doesn't handle arithmetic operations like a real shell would. However it has two more operations that can manipulate your own bash variables (done with double equal "==" sign for assignement and a forgetting function whose name I have ironically forgotten).
