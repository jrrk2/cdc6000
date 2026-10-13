         0,5,0,3,3,0,0,1,4,2,3,1,0);
      JB1 = ('PE','RE','JE',' ','EE','NE','GE','LE');
      JB2 = ('SE','JE','PE',' ','QE','EE','EE','TE');
      JX1 = ('ZE','NE','PE','NE','IE','OE','DE','IE');
      JX2 = ('RE','ZE','LE','GE','RE','RE','FE','DE');
      REL = (' ','^','V','-','=','<>','>=','<=');
      FS  = ('FE','RE','DE',' ',' ','AE','BE','XE');
      R1  = ('AE','BE','XE','XE','AE','AE','BE','BE');
      SH  = ('FE','DE','RE','IE','LE','AE','LE','AE','NE','ZE','UE','PE');
      INITNAM = ('TEXT','WEOR','GET','PUT','RESET','ALLOC',
         'PACK','UNPACK','INSERT','APPEND',
         'READ','WRITE',
         'ODD','INT','CHR','EOF','ABS','SQR',
         'TRUNC','PRED','SUCC','EOL','ALFALENG',
         'INPUT','OUTPUT','ALFA','REAL','CHAR','BOOLEAN',
         'FALSE','TRUE','INTEGER',' ',
         'SIN','COS','EXP','LN','SQRT','ARCTAN') ;

   (*TWO STANDARD PROCEDURES  INSERT  AND  APPEND  ARE PROVIDED FOR
      CODE GENERATION.
    APPEND(A,B,C) SHIFTS THE CONTENTS OF A LEFT B BITS AND
      'OR'S C INTO IT. RESULT IN A. B,C UNCHANGED.
    INSERT(A,B,C) SHIFTS THE CONTENTS OF A LEFT B BITS AND
      'OR'S THEM INTO C. RESULT IN C. A,B UNCHANGED
    A,B,C ARE OF TYPE INTEGER  *)


PROCEDURE PRTERR ;
   VAR LASTPOS,FREEPOS,CURRPOS,CURRNMR,F,K : SHRTINT ;
BEGIN WRITE(' ****  ' : 7) ;
   LASTPOS := 0 ; FREEPOS := 1 ;
   FOR K := 1 TO ERRINX DO
   BEGIN WITH ERRLIST[K] DO
      BEGIN CURRPOS := POS ; CURRNMR := NMR END ;
      IF CURRPOS = LASTPOS THEN WRITE(',') ELSE
      BEGIN WHILE FREEPOS < CURRPOS DO
         BEGIN WRITE(' ') ;
            FREEPOS := FREEPOS + 1
         END ;
         WRITE('^') ;
         LASTPOS := CURRPOS
      END ;
      IF CURRNMR < 10 THEN F := 1 ELSE
      IF CURRNMR < 100 THEN F := 2 ELSE F := 3 ;
      WRITE(CURRNMR : F) ;
      FREEPOS := FREEPOS + F + 1 ;
   END ;
   WRITE(EOL) ; ERRINX := 0
END (*PRTERR*) ;

PROCEDURE NEXTCH;
   (*READS NEXT CHAR OF FILE INPUT,
    CONVERTS EOL TO BLANK,
    PRINTS ERROR SUMMARY AFTER EACH LINE (IF ERRORS PRESENT)
    AND PRINTS ADDRESS AT BEGINNING OF LINE *)
BEGIN
   IF EOLFLAG THEN
   BEGIN IF ERRINX > 0 THEN (*PRINT ERRORS*) PRTERR;
      EOLFLAG := FALSE;  CHCNT := 0;
      OUTPUT^ := ' '; PUT(OUTPUT);
      IF DP THEN WRITE(LC : 6 OCT) ELSE WRITE(IC : 6 OCT) ;
      OUTPUT^ := ' ';  PUT(OUTPUT);
   END (*EOLFLAG*);
   GET(INPUT); CH := INPUT^;
   OUTPUT^ := CH; PUT(OUTPUT); CHCNT := CHCNT + 1;
   IF CH = EOL THEN
   BEGIN CH := ' ';  EOLFLAG := TRUE  END;
END (*NEXTCH*);

PROCEDURE ERROR(I:SHRTINT);
BEGIN ERR := TRUE ;
   WITH PARMLIST DO
   BEGIN ERRFLAG := TRUE ;
      ERRNRS[I DIV 32] := ERRNRS[I DIV 32]V[I MOD 32]
   END ;
   IF ERRINX >= 9 THEN
   BEGIN ERRLIST[10].NMR := 104 ; ERRINX := 10 ;
      WITH PARMLIST DO
      ERRNRS[3] := ERRNRS[3]V[8] ;
   END ELSE
   BEGIN ERRINX := ERRINX + 1 ;
      ERRLIST[ERRINX].NMR := I
   END ;
   ERRLIST[ERRINX].POS := CHCNT
END (*ERROR*);

(* ------------------------------------------------------------------ *)

(* I N S Y M B O L          READS ONE PASCAL-SYMBOL  *)
(* LIST OF SYMBOL NUMBERS
 NO   CL   SYMBOL                  NO   CL   SYMBOL

  1        ID                      20        :=
  2    1   INTEGER CONST           21        BEGIN
  2    2   REAL    CONST           22        END
  2    3   ALFA    CONST           23        IF
  2    4   CHAR    CONST           24        THEN
                                   25        ELSE
                                   26        CASE
  5    1   NOT                     27        OF
  6    1   *                       28        REPEAT
  6    2   /                       29        UNTIL
  6    3   ^                       30        WHILE
  6    4   DIV                     31        DO
  6    5   MOD                     32        FOR
  7    1   +                       33   1    TO
  7    2   -                       33   2    DOWNTO
  7    3   V                       35        GOTO
  8    1   <                       36        NIL
  8    2   <=                      37        TYPE
  8    3   >=                      38   1    ARRAY
  8    4   >                       38   2    RECORD
  8    5   <>                      38   3    FILE
  8    6   =                       38   4    CLASS
  8    7   IN                      38   5    POWERSET
  9        (                       40        LABEL
 10        )                       41        CONST
 11        [                       42        PACKED
 12        ]                       43        VAR
 15        ,                       44        FUNCTION
 16        ;                       45        PROCEDURE
 17        .                       47        VALUE
 18        ^                       48        WITH
 19        :

 NOT PASCAL SYMBOLS : BLANK  '  (*  $  *)

PROCEDURE INSYMBOL;
   VAR SCALE,EXP : SHRTINT;  R,FAC,RVAL : REAL;
      DIGIT : ARRAY[0..19] OF SHRTINT;
            (*FOR WORDLENGTH DIV 3 = 20 DIGITS*)
      I,K : SHRTINT;  BT1,SIGN : BOOLEAN;
      AA : ALFA; CH1 : CHAR;

BEGIN
1: WHILE CH=' ' DO NEXTCH;
   IF CH <= 'Z' THEN
   BEGIN (* READ IDENTIFIER OR WORD DELIMITER *)
      K := 0;
      REPEAT IF K < ALFALENG THEN
         BEGIN K := K + 1;  A[K] := CH END;
         NEXTCH;
      UNTIL CH >'9';
      IF K >= KK THEN KK := K ELSE
      REPEAT A[KK] := ' ' ; KK := KK - 1 UNTIL KK = K ;
      PACK(A,1,AA);
      (* TEST FOR WORD DELIMITER *)
      FOR I := WL[K] TO WL[K+1]-1 DO
         IF AA = WD[I] THEN
         BEGIN NO := WNO[I];  CL := WCL[I];
            IVAL := 0;  GOTO 2;
         END;
      (* IDENTIFIER *)
      NO := 1;  CL := K;  AVAL := AA;
2: END (*LETTER*) ELSE
   IF CH <= '9' THEN
   BEGIN NO := 2; CL := 1; I := 0;
      WHILE CH IN DIGITS DO
      BEGIN IF I < 20 (*20 = WORDLENGTH DIV 3*) THEN
         DIGIT[I] := INT(CH) - INT('0');
         I := I + 1; NEXTCH
      END;
      IVAL := 0;
      IF CH = 'B' THEN (*OCTAL*)
      BEGIN IF I > 20 THEN ERROR(2) ELSE
         FOR K := 0 TO I-1 DO IVAL := 8*IVAL + DIGIT[K];
         NEXTCH
      END (*OCTAL*) ELSE
      BEGIN IF I > 17 THEN ERROR(2) ELSE
         FOR K := 0 TO I - 1 DO
            BEGIN IF IVAL <= MAX10 THEN
               IVAL := 10*IVAL + DIGIT[K] ELSE
               BEGIN ERROR(2); IVAL := 0; GOTO 40 END
            END;
40:         SCALE := 0;
            IF CH = '.' THEN
            BEGIN NEXTCH; IF CH = '.' THEN CH := ':' ELSE
               BEGIN RVAL := IVAL; CL := 2;
                  IF NOT(CH IN DIGITS) THEN ERROR(3) ELSE
                  REPEAT R := INT(CH) - INT('0');
                     RVAL := 10.0*RVAL + R;
                     SCALE := SCALE - 1; NEXTCH
                  UNTIL NOT(CH IN DIGITS);
               END;
            END;
            IF CH = 'E' THEN
            BEGIN IF SCALE = 0 THEN
               BEGIN RVAL := IVAL; CL := 2
               END;
               SIGN := FALSE; NEXTCH;
               IF CH = '+' THEN NEXTCH ELSE
               IF CH = '-' THEN
                  BEGIN NEXTCH; SIGN := TRUE
                  END;
               EXP := 0;
               WHILE CH IN DIGITS DO
               BEGIN EXP := 10*EXP + (INT(CH) - INT('0'));
                  NEXTCH
               END;
               IF SIGN THEN EXP := -EXP;
               SCALE := SCALE + EXP
            END;
            IF SCALE <> 0 THEN
            BEGIN R := 1.0;
               IF SCALE < 0 THEN BEGIN FAC := 0.1; SCALE := -SCALE END
                              ELSE FAC := 10.0;
               REPEAT IF ODD(SCALE) THEN R := R*FAC;
