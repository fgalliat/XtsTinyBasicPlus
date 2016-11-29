10 LCCLS
20 LCPRINT 1," --] Notepad [--"
30 LCPRINT 3,"Write Until ''"
40 INPUTSTR
50 L=STRLEN()
55 IF L = 0 GOTO 80
60 WRITE "notes.txt",$
70 GOTO 40
80 LCCLS
90 LCPRINT 1," --] Notepad [--"
100 LCPRINT 3,"Notes Saved"
200 ? "Bye"