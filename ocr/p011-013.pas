               FAC := SQR(FAC); SCALE := SCALE DIV 2
            UNTIL SCALE = 0; (*R = 10 ** SCALE*)
            RVAL := RVAL*R
         END;
         IF CL = 2 THEN IVAL := INT(RVAL)
      END (*NOT OCTAL*)
   END (*NUMBER*) ELSE
   BEGIN (* SPECIAL CHARACTER *)
      IF CH = '''' THEN (* ALFA OR CHAR CONSTANT *)
      BEGIN NO := 2; K := 0; BT1 := FALSE;
         REPEAT NEXTCH;
            IF CH = '''' THEN
            BEGIN NEXTCH; BT1 := CH <> '''' END;
            IF NOT BT1 THEN
               IF K = ALFALENG THEN
               BEGIN ERROR(3); BT1 := TRUE END ELSE
               BEGIN K := K + 1; A[K] := CH END;
         UNTIL BT1;
         IF K =1 THEN (* CHAR CONST *)
         BEGIN CL:=4; IVAL:= INT (A[1]) END
         ELSE (* ALFA CONST *)
         BEGIN IF K >= KK THEN KK := K ELSE
            REPEAT A[KK] := ' '; KK := KK - 1 UNTIL KK = K ;
            PACK (A,1,AA);
            CL:=3; IVAL:= INT(AA) ;
         END;
      END (* ' *) ELSE
      BEGIN NO:= SYMNO[CH]; CL:=SYMCL[CH]; IVAL:=0;
         (* TEST FOR TWO-CHARACTER SYMBOL *)
         IF CH = ':' THEN
         BEGIN NEXTCH;
            IF CH='=' THEN
            BEGIN NO:= 20; NEXTCH END;
         END ELSE
         IF CH ='.' THEN
         BEGIN NEXTCH;
            IF CH = '.' THEN (* : *)
            BEGIN NO:=19; NEXTCH END;
         END ELSE
         IF CH = '↗' THEN (* SKIP COMMENT *)
         BEGIN NEXTCH;
            IF CH = '$' THEN
            REPEAT NEXTCH ;
            IF CH <> '↓' THEN
            BEGIN CH1 := CH ; NEXTCH ;
               IF CH1 = 'A' THEN ASSCHECK := CH = '+' ELSE
               IF CH1 = 'C' THEN PRCODE := CH = '+' ELSE
               IF CH1 = 'D' THEN DIVCHECK := CH = '+' ELSE
               IF CH1 = 'O' THEN STOFLCHECK := CH = '+' ELSE
               IF CH1 = 'R' THEN ROUNDING := CH = '+' ELSE
               IF CH1 = 'T' THEN
               BEGIN ASSCHECK := CH = '+' ;
                  DIVCHECK := CH = '+' ; STOFLCHECK := CH = '+' ;
                  ROUNDING := CH = '+' ; INXCHECK := CH = '+'
               END ELSE
               IF CH1 = 'X' THEN INXCHECK := CH = '+' ;
               IF CH <> '↓' THEN NEXTCH
            END ;
            UNTIL CH <> ',' ;
            WHILE CH <> '↓' DO NEXTCH;
            NEXTCH; GOTO 1 ;
         END ELSE
         NEXTCH;
      END (* SPECIAL CHARACTER *) ;
   END;
END (* INSYMBOL *) ;

(* ------------------------------------------------------------------ *)

PROCEDURE SRCHREC(P:CTP);
   (* SEARCHES ONE BLOCK, RETURNS CTPTR *)
BEGIN CTPTR := P;
   WHILE CTPTR <> NIL DO
      IF CTPTR^.NAME = AVAL THEN GOTO 1
      ELSE CTPTR := CTPTR^.NXTEL;
1:END (* SRCHREC *) ;

PROCEDURE SEARCH;
   (* SEARCHES CONTEXTTABLE, RETURNS CTPTR AND DISX = INDEX TO
      DISPLAY *)
   VAR I : SHRTINT;
BEGIN FOR I := TOP DOWNTO 0 DO
   BEGIN CTPTR := DISPLAY[I].FNAME;
      WHILE CTPTR <> NIL DO
         IF CTPTR^.NAME = AVAL THEN GOTO 1
         ELSE CTPTR := CTPTR^.NXTEL;
   END;
1: DISX := I;
END (* SEARCH *) ;

PROCEDURE INCONST (VAR V : INTEGER; P : CTP; CONST NXT : CTP) ;
   (* INPUT PARAMETER : NXT. SEARCHING IS TO BEGIN AT NXT
      OUTPUT PARAMETER : V = VALUE
                         P = TYPE POINTER , P = NIL IF ERROR *)
   VAR SIGN : BOOLEAN; PT : CTP;
BEGIN SIGN := FALSE; P := NIL;
   IF NO = 7 THEN
   BEGIN SIGN := CL = 2;
      IF CL <= 2 THEN
      BEGIN INSYMBOL; IF NO = 1 THEN ERROR(3)  END;
   END;
   IF NO = 2 THEN
   BEGIN CASE CL OF
1:       P := INTPTR;
2:       P := REALPTR;
3:       P := ALFAPTR;
4:       P := CHARPTR;
      END;
      IF SIGN THEN V := -IVAL ELSE V := +IVAL;
      INSYMBOL;
   END ELSE
   IF NO = 1 THEN
   BEGIN PT := CTPTR;
      SRCHREC(NXT);
      IF CTPTR = NIL THEN SEARCH;
      IF CTPTR = NIL THEN ERROR(12) ELSE
      WITH CTPTR^ DO
      BEGIN IF (KLASS <> KONST)V(CONKIND = FORMAL) THEN ERROR(63)
         ELSE
         BEGIN P := CONTYPE;  V := VALUES END;
      END;
      CTPTR := PT;  INSYMBOL;
   END ELSE ERROR(3);
END (* INCONST *) ;

(* ------------------------------------------------------------------ *)

(* CODE GENERATING ROUTINES *)

PROCEDURE NOOP;
BEGIN WHILE CP < 4 DO
   BEGIN APPEND(BUF,15,NOP);
      CP := CP + 1;
   END ;
END (* NOOP *) ;

PROCEDURE GEN15(OP,I,J,K : SHRTINT);
BEGIN LASTOP := OP ; LASTI := I ;
   IF CP <> 4 THEN
   BEGIN CP := CP + 1; APPEND(BUF,6,OP)  END ELSE
   BEGIN CODE[CA] := BUF;  BUF := OP;  CP := 1;
      CA := CA + 1;  IC := IC + 1;
      IF CA > CODMAX THEN
      BEGIN ERROR(90); CA := 1;
      END;
   END;
   BUF := ((8*BUF + I)*8 + J)*8 + K ;
END (* GEN15 *) ;

PROCEDURE GEN30(OP,I,J,K : SHRTINT);
BEGIN LASTOP := OP ; LASTI := I ;
   IF CP = 3 THEN (*NOOP*)
   BEGIN APPEND(BUF,15,NOP); CP := 4 END;
   IF CP < 4 THEN
   BEGIN CP := CP + 2; APPEND(BUF,6,OP)  END ELSE
   BEGIN CODE[CA] := BUF; BUF := OP; CP := 2;
