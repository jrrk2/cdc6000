      CA := CA + 1;  IC := IC + 1;
      IF CA > CODMAX THEN
      BEGIN ERROR(90);  CA := 1;
      END;
   END;
   BUF := (8*BUF + I)*8 + J ;
   IF K >= 0 THEN APPEND(BUF,18,K) ELSE APPEND(BUF,18,777777B+K) ;
END (* GEN30 *) ;

PROCEDURE BXIXJ(I, J : SHRTINT) ;
(*TO AVOID B XI XJ INSTRUCTIONS WHENEVER POSSIBLE*)
BEGIN IF (J = LASTI)^(LASTOP >= 10B)^(LASTOP <= 45B)
   ^NOT(LASTOP IN [20B,21B,43B]) THEN
   BEGIN IF BUF >= 0 THEN BUF := BUF - LASTI*64 + I*64 ELSE
      BEGIN BUF := - BUF ;
         BUF := BUF - (7 - LASTI)*64 + (7 - I)*64 ;
         BUF := - BUF
      END ;
      LASTI := I
   END ELSE GEN15(10B,I,J,0)
END (*BXIXJ*) ;

PROCEDURE INS(FADR : ADDRESS ; FCP, FCA : SHRTINT) ;
BEGIN IF CA <> FCA THEN INSERT(FADR,(4-FCP)*15,CODE[FCA])
                   ELSE INSERT(FADR,(CP-FCP)*15,BUF)
END ;

(* ------------------------------------------------------------------ *)

(* ROUTINES TO PRINT CODE AND JUMPTABLE AND TO OUTPUT CODE *)

PROCEDURE PRTCOMP;
   (* GLOBAL ARRAYS : JB1,JB2,JX1,JX2,REL,FS,R1,SH *)
   VAR II,JJ,KK : CHAR;
      C,CC,S,IT3, OP,I,J,K : SHRTINT;
      M,IT4 : SHRTINT;

   PROCEDURE OUTCH(C : CHAR) ;
   BEGIN WRITE(C) END ;

   PROCEDURE SPACE ;
   BEGIN WRITE(' ' : 3) END ;

BEGIN
   IC := IC - CA - 1;
   M := -77B;  IT4 := -7B;
   FOR IT3 := 1 TO CA DO
   BEGIN BUF := -CODE[IT3];  OUTCH(EOL);
      OUTCH(' ') ; OUTCH(' ') ; WRITE(IC+IT3 : 6 OCT);  S := 0;
      REPEAT IT := BUF; APPEND(IT,S+6,M); OP := -IT;
         C := OP MOD 8;
         IT := BUF;  APPEND(IT,S+9,IT4); I := -IT;
         IT := BUF;  APPEND(IT,S+12,IT4); J := -IT;
         IT := BUF;
         IF (OP < 8) V ((OP >= 40) ^ (C <= 2)) THEN
         BEGIN S := S+30;  APPEND(IT,S,MASK) END ELSE
         BEGIN S := S+15;  APPEND(IT,S,IT4) END;
         K := -IT;

      CC := OP DIV 8;
      II :=CHR(I+INT('0')); JJ := CHR(J+INT('0')); KK :=CHR(K+INT('0'));
      OUTCH(EOL); WRITE(BLANK);
      IF CC < 5 THEN
      CASE CC OF
0:       IF OP = 3 THEN
         BEGIN OUTCH(JX1[I]);  OUTCH(JX2[I]);  OUTCH(' ');
            SPACE;  OUTCH('X');  OUTCH(JJ);  OUTCH(',');
            WRITE(K : 6 OCT);
         END ELSE
         BEGIN OUTCH(JB1[OP]);  OUTCH(JB2[OP]);  OUTCH(' ');
            SPACE;
            IF OP > 3 THEN
            BEGIN OUTCH('B');  OUTCH(II);  OUTCH(REL[OP]);
               OUTCH('B');  OUTCH(JJ);  OUTCH(',');  WRITE(K : 6 OCT);
            END ELSE
            BEGIN IF OP = 2 THEN
               BEGIN OUTCH('B') ; OUTCH(II) ; OUTCH('+') END ;
               WRITE(K : 6 OCT)
            END;
         END (* 0 *) ;
1:       BEGIN OUTCH('B');  OUTCH('X');  OUTCH(II);  SPACE;
            IF OP > 11 THEN
            BEGIN OUTCH('¬');  II := JJ; JJ := KK; KK := II  END;
            OUTCH('X');  OUTCH(JJ);
            C := OP MOD 4;
            IF C <> 0 THEN
            BEGIN OUTCH(REL[C]);  OUTCH('X');  OUTCH(KK)  END;
         END (* 1 *) ;
2:       BEGIN OUTCH(SH[OP]);  OUTCH('X');  OUTCH(II);  SPACE;
            IF OP < 18 THEN
            BEGIN OUTCH(JJ);  OUTCH(KK)  END ELSE
            BEGIN OUTCH('B');  OUTCH(JJ);  OUTCH(',');
               OUTCH('X');  OUTCH(KK);
            END;
         END (* 2 *) ;
3:       BEGIN OUTCH(SH[OP DIV 2]);  OUTCH('X');  OUTCH(II);
            SPACE;  OUTCH('X');  OUTCH(JJ);
            IF ODD(OP) THEN OUTCH('-') ELSE OUTCH('+');
            OUTCH('X');  OUTCH(KK);
         END (* 3 *) ;
4:       BEGIN C := OP MOD 4;
            IF OP = 38 THEN
            BEGIN OUTCH('N');  OUTCH('O')  END ELSE
            IF C = 3 THEN
            BEGIN IF OP = 35 THEN OUTCH('M') ELSE OUTCH('C');
               OUTCH('X');  OUTCH(II);  SPACE;
               IF OP = 35 THEN OUTCH(JJ) ELSE OUTCH('X') ;
               OUTCH(KK);
            END ELSE
            BEGIN OUTCH(FS[C]);  OUTCH('X');  OUTCH(II);
               SPACE;  OUTCH('X');  OUTCH(JJ);
               IF OP < 36 THEN OUTCH('*') ELSE OUTCH('/');
               OUTCH('X');  OUTCH(KK);
            END;
         END (* 4 *) ;
      END (* CASE *) ELSE
      BEGIN
         OUTCH('S');  OUTCH(FS[CC]);  OUTCH(II);
         SPACE;  OUTCH(R1[C]);  OUTCH(JJ);
         IF C IN [5,7] THEN OUTCH('-') ELSE OUTCH('+') ;
         IF C <= 2 THEN WRITE(K : 6 OCT) ELSE
         BEGIN OUTCH('B');  OUTCH(KK)  END;
      END;

      UNTIL S >= 60;
   END;
   IC := IC + CA + 1;  OUTCH(EOL);
END (* PRTCOMP *) ;

PROCEDURE PRJMPTAB ;
 (* PRINTS OUT JMPTAB.
   LOADPOINT OF JMPTAB IN PARMLIST.LPJMPTAB *)
   VAR I : SHRTINT;
BEGIN IF JMPIX > 0 THEN WRITE(EOL,'1',EOL) ;
   FOR I := 0 TO JMPIX - 1 DO
   WRITE(' ' : 2,PARMLIST.LPJMPTAB+I:6 OCT,' ',JMPTAB[I]:20 OCT,EOL)
END (*PRJMPTAB*) ;

PROCEDURE WRITOUT ;
 (*CALLED AT PROCEDURE END, UPDATING ITS CODE (INSERTING ADDRESSES
   OF CONSTANTS) AND WRITING OUT CODE TOGETHER WITH THE CONSTANTS
   USED IN THIS PROCEDURE*)
   VAR IT2 : SHRTINT;
BEGIN
   NOOP ; CODE[CA] := BUF ;
   FOR IT := 1 TO LCX DO
   WITH CSTTB[IT] DO
   BEGIN
      CA := CA + 1 ;
      IF CA > CODMAX THEN
      BEGIN ERROR(90) ; CA := 1 END ELSE
      BEGIN
         CODE[CA] := VALU ; IT1 := INX ;
         REPEAT
            WITH UNDLAB[IT1] DO
            BEGIN INS(IC,LFTSH,PLACE) ; IT2 := IT1 ; IT1 := SUCC END
