10 IF BTN() = -1 GOTO 2000
12 A = 2
15 LCCLS
20 LCPRINT 1,"  -=[ User Menu ]=-"
30 LCPRINT 2,"  Start server"
40 LCPRINT 3,"  Swap Serial Ports"
50 LCPRINT 4,"  Exit"
70 LCPRINT A,">"
100 K = BTN()
110 IF K = 0 GOTO 100
120 IF K = 1 A = A - 1
130 IF K = 2 A = A + 1
140 IF K = 4 GOTO 170
150 IF BTN() <> 0 GOTO 150
153 IF A<2 A=4
155 IF A > 4 A = 2
160 GOTO 15
170 C = A - 1
180 IF C = 1 WTEST
190 IF C = 2 CINV
200 IF C = 3 GOTO 1900
210 GOTO 15
1900 ? "bye":END
2000 ? "no buttons, error"