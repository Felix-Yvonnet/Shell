If not installed (but I guess you have) do "$>sudo apt-get install libreadline-dev"...
Also bison
Also flex



1) To my mind execvp is the best choice since it returns a vector which is easier to handle than chained lists and the suffix p helps to handle the path (so have access to all commands that it allows).

2) The sequence operator is ";". It behaves differently from the and ("&&") operator since it executes all actions if no error is raised. For example ```$false; echo sequence || echo and``` will print "sequence" while ```$false && echo sequence || echo and``` will print "and".
Expliquez le parsage !!! Passé trop de temps sur la compréhension de comment les séquences étaient implémentés...

3) With a normal parser it would have been one lined bu I already don't know why the program stops after calling the first ```false && ...```.

4) ```$true && rm /dev/null | echo b 2>/dev/null``` will raise an error but ```$(true && rm /dev/null | echo b) 2>/dev/null``` wont : "2>" is a unary operation and has a superior priority than "|".
What is C_VOID ? I guess it's parenthesed sequences...

5) Well if not corrected the problem is that ^C is the terminal signal to stop the process including our own bash... To avoid it I called signal(SIGINT,handler) which set the interpretation of an INTerruption SIGnal to the actions explained in the handler.

6) Since we haven't implemented the redirect function yet ```ls > dump``` call not only print the ls call but also doesn't do any backup since dump doesn't get any entrance.

WTF no ~??? add it to the lexer...

'-' must be precede by a '\' sign since it's a special character in lexer...

