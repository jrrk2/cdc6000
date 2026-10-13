   END (*SCALDECL*);

PROCEDURE FIELDLIST(VAR MAXSIZE:SHRTINT; VARPTR,NXTF:CTP);
   VAR MXL,L,LL,MINSIZE,CASEBITS,I : SHRTINT;
      P,PP,PP1,PP2,NXTC,NXT,CPTR : CTP;
      TAGFLAG : BOOLEAN;

   PROCEDURE RECERR(I : SHRTINT);
   BEGIN ERROR(I);  SKIPT(49); END;

   PROCEDURE ADJUST;
      (* MOVE LAST FIELD TO RIGHT. IF IT IS THE ONLY FIELD,
         CHANGE TO NONPACKED.
         IF LAST FIELD IS TAGFIELD THEN DO NOT MOVE.
         INCREASE DISPL, RESET BDISPL  *)
   BEGIN IF NOT TAGFLAG THEN
      WITH LASTFLD^ DO
      BEGIN IF BITDISPL = 0 THEN (*ONLY ONE FIELD IN WORD*)
         BITWIDTH := 0 ELSE
         BITDISPL := WORDLENGTH - BITWIDTH;
      END;
      DISPL := DISPL + 1;  BDISPL := 0;
   END (*ADJUST*);

BEGIN
   TAGFLAG := TRUE;   NXT := NXTF;
   REPEAT
      IF NO = 26 THEN (*CASE*) GOTO 2 ;
      I := 0;
1:       IF NO <> 1 THEN
         BEGIN RECERR(11);
            IF TERRCL[NO] = BEGSY THEN GOTO 11;  GOTO 12;
         END;
         SRCHREC(NXT);
         IF CTPTR <> NIL THEN ERROR(5) ELSE
         BEGIN ALLOC(P,FIELD); I := I + 1;
            WITH P^ DO
            BEGIN NAME := AVAL;  NXTEL := NXT;  KLASS := FIELD;
               FLDTYPE := NIL;
            END;  NXT := P;
         END;
         INSYMBOL;
         IF NO = 15 THEN (*,*)
         BEGIN INSYMBOL;  GOTO 1 END;
         IF NO <> 19 THEN (*NOT :*) ERROR(10) ELSE INSYMBOL;
11:      TYPEDECL(L,P);
         IF P <> NIL THEN
         IF P^.FORM > RECORDS THEN ERROR(30) ELSE
         BEGIN IF PACKFLAG THEN
            BEGIN IF I > 1 THEN (*REVERSE POINTERS*)
               BEGIN PP := NXT;
                  FOR I := I DOWNTO 1 DO
                  BEGIN PP1 := PP^.NXTEL;
                    PP^.NXTEL := PP2;  PP2 := PP;
                    PP := PP1;
                  END;
                  NXT^.NXTEL := PP;  NXT := PP2;
               END (*REVERSE*) ELSE PP := NXT^.NXTEL;
               PP1 := NXT;
               IF P^.FORM <= POWER THEN
               BEGIN
                  CASE P^.FORM OF
   NUMERIC:         LL := P^.BITS;
   SYMBOLIC:        LL := P^.BITSIZE;
   POINTER:         LL := 18 ;
   POWER:           LL := P^.PWBITS;
                  END;
                  REPEAT
                     IF BDISPL + LL > WORDLENGTH THEN ADJUST;
                     IF LL = WORDLENGTH THEN
                     BEGIN PP1^.BITWIDTH := 0;
                       PP1^.FLDADDR := DISPL;  DISPL := DISPL + 1;
                     END ELSE
                     BEGIN PP1^.BITWIDTH := LL;
                       PP1^.BITDISPL := BDISPL;
                       PP1^.FLDADDR := DISPL;  BDISPL := BDISPL + LL;
                     END;
                     PP1^.FLDTYPE := P;
                     LASTFLD := PP1;  TAGFLAG := FALSE;
                     PP1 := PP1^.NXTEL;
                  UNTIL PP1 = PP;
               END (*FORM <= POINTER*) ELSE
               BEGIN IF BDISPL <> 0 THEN ADJUST;
                  TAGFLAG := FALSE;
                  REPEAT
                  PP1^.BITWIDTH := 0;  PP1^.FLDTYPE := P;
                     PP1^.FLDADDR := DISPL;
                     DISPL := DISPL + L;  LASTFLD := PP1;
                     PP1 := PP1^.NXTEL;
                  UNTIL PP1 = PP;
               END;
            END (*PACKFLAG*) ELSE
            BEGIN LL := DISPL + I*L;  DISPL := LL;
               PP := NXT;
               FOR I := I DOWNTO 1 DO
               BEGIN PP^.FLDTYPE := P;  LL := LL - L;
                  PP^.FLDADDR := LL;  PP^.BITWIDTH := 0;
                  PP := PP^.NXTEL;
               END;
            END;
         END (*FORM <= RECORDS*);
12:      IF NO = 16 THEN INSYMBOL;
   UNTIL (TERRCL[NO] = ENDSY) AND (NO <> 26);
   IF BDISPL <> 0 THEN ADJUST;
   MAXSIZE := DISPL;  VARPTR := NIL;
   GOTO 9;
2: (*CASE*)
   INSYMBOL;
   IF NO <> 1 THEN ERROR(11) ELSE
   BEGIN SRCHREC(NXT);
      IF CTPTR <> NIL THEN ERROR(5) ELSE
      BEGIN ALLOC(P,FIELD);
         WITH P^ DO
         BEGIN NAME := AVAL;  NXTEL := NXT;  KLASS := FIELD;
            FLDTYPE := NIL;
         END;  NXT := P;
      END;
      INSYMBOL;
   END (*NO = 1*);
      IF NO <> 19 THEN ERROR(10) ELSE INSYMBOL;
      IF NO <> 1 THEN ERROR(11) ELSE
      BEGIN SRCHREC(NEXT);
         IF CTPTR = NIL THEN SEARCH;
         IF CTPTR = NIL THEN ERROR(12) ELSE
         IF (CTPTR^.KLASS<>TYPES) OR (CTPTR^.FORM>SYMBOLIC) THEN
            ERROR(7);
         INSYMBOL;
            IF NO <> 27 THEN ERROR(14);
            CPTR := CTPTR;
            IF CPTR <> NIL THEN
            IF PACKFLAG THEN
            BEGIN
                CASE CPTR^.FORM OF
   NUMERIC:         LL := CPTR^.BITS;
   SYMBOLIC:        LL := CPTR^.BITSIZE;
                END;
                IF BDISPL + LL > WORDLENGTH THEN ADJUST;
                P^.BITDISPL := BDISPL;  P^.BITWIDTH := LL;
                P^.FLDADDR := DISPL;  BDISPL := BDISPL + LL;
               LASTFLD := P;  CASEBITS := BDISPL;
            END (*PACKFLAG*) ELSE
            BEGIN P^.FLDADDR := DISPL;
               P^.BITWIDTH := 0;
            END;
            P^.FLDTYPE := CPTR;  DISPL := DISPL + 1;
            MINSIZE := DISPL;  MAXSIZE := DISPL;
            NXTC := NIL;  INSYMBOL;
            REPEAT  I := 0;
               REPEAT
                  IF (NO>2) OR (NO=2) AND NOT (CL IN [1,4]) THEN
                  BEGIN RECERR(63);  GOTO 3 END ;
                  IF CPTR <> NIL THEN
                    IF NO = 1 THEN
                    BEGIN SRCHREC(NEXT);
                      IF CTPTR = NIL THEN SEARCH;
                      IF CTPTR = NIL THEN ERROR(12) ELSE
                      WITH CTPTR^ DO
                      IF KLASS <> KONST THEN ERROR(63) ELSE
                      IF (CPTR^.FORM = SYMBOLIC) AND
                          (CONTYPE <> CPTR)  OR
                        (CPTR^.FORM = NUMERIC) AND
                          (CONTYPE <> INTPTR) THEN ERROR(73) ELSE
                      IT := VALUES;
                    END (*NO = 1*) ELSE
                    IF (CL = 1) AND (CPTR^.FORM <> NUMERIC)
                      OR (CL = 4) AND (CPTR <> CHARPTR) THEN
                      ERROR(73) ELSE
                    IT := IVAL;  (*END CPTR <> NIL*)
                  ALLOC(P,TAGFIELD);
                  WITH P^ DO
                  BEGIN NAME := BLANK;  NXTEL := NXTC;
                    KLASS := TAGFIELD;
                    TAGVAL := TRUE;  CASEVAL := IT;
                  END;  NXTC := P;  I := I + 1;
                  INSYMBOL;
3:                IF NO <> 19 THEN ERROR(10) ELSE INSYMBOL;
               UNTIL NO > 2;
               IF NO = 9 THEN  (* ( FIELDLIST ) *)
               BEGIN IF PACKFLAG THEN
                  BEGIN DISPL := MINSIZE - 1;
                     BDISPL := CASEBITS;
                  END ELSE DISPL := MINSIZE;
                  INSYMBOL;  FIELDLIST(MXL,PP,NXT);
                  IF NO <> 10 THEN ERROR(9) ELSE INSYMBOL;
               END ELSE
               BEGIN PP := NIL;  MXL := MINSIZE END;
               P := NXTC;
               FOR I := I DOWNTO 1 DO
               WITH P^ DO
               BEGIN CASESIZE := MXL;  VARIANTS := PP;
                 P := NXTEL;
               END;  IF MAXSIZE < MXL THEN MAXSIZE := MXL;
               IF NO = 16 THEN INSYMBOL;
            UNTIL NO > 2;
            ALLOC(P,TAGFIELD);
            WITH P^ DO
            BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TAGFIELD;
               CASESIZE := MINSIZE;  VARIANTS := NXTC;
               TAGVAL := FALSE;  CASETYPE := CPTR;
            END;
            VARPTR := P;
      END (*NO = 1*);
9:    NXTF := NXT;
   END (* FIELDLIST *);

(*TYPEDECL*)
BEGIN PACKFLAG := FALSE;
   IF NO = 42 THEN (*PACKED*)
