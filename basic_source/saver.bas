10 LCCLS
20 LCPRINT 1," --] Src Save [--"
25 INPUTSTR 1
30 LCPRINT 3,$1
40 INPUTSTR 2
50 L=STRLEN(2)
55 IF L = 0 GOTO 80
60 WRITE $1,$2
65 WRITE $1,"\n"
70 GOTO 40
80 LCCLS
90 LCPRINT 1," --] Src Save [--"
100 LCPRINT 3,"File Saved"
200 ? "Bye"