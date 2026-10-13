            TOP := TOP + 1 ; LTPCT := LTPCT + 1 ;
            IF TOP > DISPLIMIT THEN ERROR(82) ELSE
            WITH DISPLAY[TOP] DO
            BEGIN
               FNAME := TYPTR^.FSTFLD ;
               IF (ACCESS = DRCT) AND (BREG IN [0,LEVEL]) THEN
               BEGIN OCCUR := CWITH;
                  CDSPL := DPLMT;  CLEV := BREG;
               END ELSE
               BEGIN
                  LOADADR(GATTR,6) ; IT := LC + TCT ;
                  GEN30(51B,6,5,IT) ;
                  OCCUR := VWITH ; VDSPL := IT ;
                  TCT := TCT + 1 ; LB7CT := LB7CT + 1 ;
                  IF TCT > TMAX THEN TMAX := TCT ;
               END
            END
         END
      UNTIL NO <> 15 ; (* SY <> , *)
      IF NO <> 31 THEN
      BEGIN
         IF GATTR.TYPTR <> NIL THEN ERROR(59) ; SKIP(31) ;
         IF NO <> 31 THEN
         BEGIN
            IF GATTR.TYPTR = NIL THEN ERROR(59) ;
         IF ERRCL[NO] = BEGSY THEN GOTO 20 ; GOTO 10
         END
      END ;
      INSYMBOL ;
20:   STATEMENT ;
10:   TOP := TOP - LTPCT ; TCT := TCT - LB7CT ;
   END (* WITHSTAT *) ;

(* ------------------------------------------------------------ *)

PROCEDURE COMPSTAT ;
BEGIN REPEAT BEGIN
      INSYMBOL ;
1:    STATEMENT ;
      IF ERRCL[NO] = BEGSY THEN
   BEGIN ERROR(58) ; GOTO 1 END ;
   IF NO = 25 THEN (*ELSE*)
   BEGIN ERROR(54) ; INSYMBOL ; GOTO 1 END
   END
 UNTIL NO <> 16 (*;*) ;
 IF NO <> 22 THEN (*END*) ERROR(68) ELSE INSYMBOL ;
END (*COMPSTAT*) ;

PROCEDURE STATEMENT ;
  VAR LPSW : SHRTINT;
BEGIN
  IF (NO = 2) AND (CL = 1) THEN (*LABEL*)
  BEGIN NOOP ;
    IF IVAL >= TWOTO17 THEN
    BEGIN ERROR(100) ; GOTO 1 END ;
    FOR IT := 1 TO CLABIX DO
    WITH LABTAB[IT] DO
    IF LABVAL = IVAL THEN (*FOUND*)
    BEGIN IF FLD2 = 0 THEN (*MULTIDEF.*) ERROR(77) ELSE (*FIXUP*)
      BEGIN IT1 := FLD3 ;
        REPEAT WITH UNDLAB[IT1] DO
          BEGIN INS(IC,LFTSH,PLACE) ; IT1 := SUCC END
        UNTIL IT1 = 0 ;
        IT1 := FLD2 ; UNDLAB[IT1].SUCC := CHNIX ;
        CHNIX := FLD3 ; FLD2 := 0 ; FLD3 := IC ;
      END ;
      GOTO 1
    END (*IF, WITH, FOR*) ;
    (*NEW LABEL*)
    IF CLABIX = MAXLABS THEN ERROR(74) ELSE
    BEGIN CLABIX := CLABIX + 1 ;
      WITH LABTAB[CLABIX] DO
      BEGIN LABVAL := IVAL ; FLD2 := 0 ; FLD3 := IC END ;
    END ;
1:  INSYMBOL ;
    IF NO <> 19 THEN (*:*)
    BEGIN ERROR(64) ; SKIP(49) END ELSE INSYMBOL ;
  END (*IF (NO=2) AND (CL=1)*) ;
  RP := 0 ;
  CASE SPLITSTAT[NO] OF
  (*PASS  *) 1: (*ENDSY OR IRRELSY*) ;
  (*IDENT *) 2: BEGIN SEARCH ;
                  IF CTPTR = NIL THEN
                  BEGIN ERROR(31) ; CTPTR := UNDECPTR END ;
                  WITH CTPTR^ DO
                  IF KLASS <= KONST THEN ERROR(55) ELSE
                  IF (KLASS = PROC) AND
                  ((PROCTYPE = CTPTR) OR (PROCTYPE = NIL)) THEN
                  BEGIN (* PROCCALL *)
                    IF PROCTYPE = CTPTR THEN
                    BEGIN INSYMBOL ; IF CTPTR <= PREDEFP THEN
                      BEGIN LPSW := CTPTR^.SEGSIZE ;
                       CASE LPSW OF
                  0:      TITLE ;
          1,2,3,4:        GETPUT(LPSW) ;
                  5:      ALLC ;
                  6:      PCK ;
                  7:      UNPCK ;
                8,9:      INSAPP(LPSW) ;
                 10:      READIR ;
                 11:      WRITEIR ;
                       END ;
                      END ELSE  PASSPARAMS ;
                    END ELSE SKIP(49)
                  END (*PROCCALL*) ELSE  ASSIGN ;
                END ;
  (*BEGIN *) 3: COMPSTAT ;
  (*IF    *) 4: IFSTAT ;
  (*CASE  *) 5: CASESTAT ;
  (*REPEAT*) 6: REPEATSTAT ;
  (*WHILE *) 7: WHILESTAT ;
  (*FOR   *) 8: FORSTAT ;
  (*GOTO  *) 9: GOTOSTAT ;
  (*WITH  *) 10: WITHSTAT ;
  END ;
  IF ERRCL[NO] = IRRELSY THEN
  BEGIN ERROR(54) ; SKIP(49) END ;
  RP := 0
END (*STATEMENT*) ;

(* ------------------------------------------------------------ *)

PROCEDURE TYPEDECL(VAR TL:SHRTINT; P1:CTP);
VAR I,L,LL,J,CV,E1,E2,BDISPL : SHRTINT;
   OPT : OPTPWR;  DISPL : ADDRESS;  PACKFLAG,LERR : BOOLEAN;
   LASTFLD,PP,P,NXTF,NXTC,NXTA,RTYP : CTP;

   PROCEDURE SKIPT(FNO : SHRTINT) ;
   BEGIN
      WHILE (TERRCL[NO] = IRRELSY) AND (FNO <> NO) DO INSYMBOL;
   END (*SKIPT*);

   PROCEDURE TYPERR(I : SHRTINT);
   BEGIN TL := 0;  P1 := NIL;
      ERROR(I);  SKIPT(49);
   END (*TYPERR*);

   PROCEDURE SUBRANGE(VAR VAL1,VAL2 : INTEGER; N1 : CTP;
      CONST P : CTP) ;
      (*THE FIRST SYMBOL OF SUBRANGE HAS BEEN READ.
         THE PROCEDURE RETURNS THE TWO BOUND-VALUES IN VAL1,VAL2,
         AND RETURNS  N1 = POINTER TO TYPE OF CONSTANTS.
         ERRORS : TYPES DO NOT AGREE, TYPE IS NOT INTEGER,CHAR,
            OR SYMBOLIC,  VAL1 > VAL2.
         P INDICATES BEGINNING OF SEARCH FOR FIRST SYMBOL IF IT IS
         AN ID.  *)
      VAR N2 : CTP;
   BEGIN INCONST(VAL1,N1,P);
      IF (N1=NIL) OR (N1=REALPTR) OR (N1=ALFAPTR) THEN
         ERR := TRUE ELSE
      BEGIN IF NO <> 19 THEN ERROR(10) ELSE INSYMBOL ;
         INCONST(VAL2,N2,NEXT);
         IF N1 <> N2 THEN ERR := TRUE ELSE
         IF VAL1 > VAL2 THEN ERROR(25);
      END;
   END (*SUBRANGE*);

   PROCEDURE SUBTYPE(VAR I,J : SHRTINT; P : CTP);
      (*EITHER A TYPE-ID FOR A SUBRANGE OR AN EXPLICIT SUBRANGE
         ARE PROCESSED.
         RETURNS I = LOWBOUND, J = HIGHBOUND,
            P = POINTER TO TYPE OF CONSTANTS *)
   BEGIN
      IF NO = 1 THEN
      BEGIN SRCHREC(NEXT);
         IF CTPTR = NIL THEN SEARCH;
         IF CTPTR = NIL THEN ERROR(12) ELSE
         BEGIN IF CTPTR^.KLASS = TYPES THEN
            BEGIN IF CTPTR^.FORM > SYMBOLIC THEN ERROR(13) ELSE
               BEGIN
                  CASE CTPTR^.FORM OF
   NUMERIC:          BEGIN I := CTPTR^.MIN;  J := CTPTR^.MAX;
                        P := INTPTR;
                     END;
   SYMBOLIC:         BEGIN IF (CTPTR = REALPTR) OR (CTPTR = ALFAPTR) THEN
                        ERROR(6) ELSE
                        BEGIN I := 0 ; J := CTPTR^.FCONST^.VALUES ;
                           P := CTPTR ;
                        END
                     END;
                  END (*CASE*);
                  INSYMBOL;
               END;
            END (*TYPES*) ELSE
            IF CTPTR^.KLASS = KONST THEN
               SUBRANGE(I,J,P,CTPTR) ELSE ERROR(63) ;
         END;
      END (*ID*) ELSE
      IF NO IN [2,7] THEN SUBRANGE(I,J,P,NIL) ELSE
         ERROR(1) ;
   END (*SUBTYPE*);

   PROCEDURE SCALDECL(N : CTP);
   BEGIN SUBRANGE(I,J,PP,N);
      IF NOT ERR THEN
      IF PP^.FORM = SYMBOLIC THEN
      BEGIN ERROR(28);  P1 := NIL  END ELSE
      BEGIN ALLOC(P,TYPES,NUMERIC);
         WITH P^ DO
         BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
            SIZE := 1;  FORM := NUMERIC;
            MIN := I;  MAX := J;
            IF ABS(I) > ABS(J) THEN BITS := LOG2(ABS(I)) ELSE
                                    BITS := LOG2(ABS(J)) ;
         END;
         TL := 1;  P1 := P;
      END ELSE P1 := NIL
