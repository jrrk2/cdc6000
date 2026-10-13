   BEGIN PACKFLAG := TRUE;  INSYMBOL END;
   IF NO = 1 THEN
   BEGIN SRCHREC(NEXT);
      IF CTPTR = NIL THEN SEARCH;
      IF CTPTR = NIL THEN
      BEGIN ERROR(12);  P1 := NIL;  SKIPT(16) END ELSE
      BEGIN IF CTPTR^.KLASS = TYPES THEN (*TYPE-ID*)
         BEGIN TL := CTPTR^.SIZE;  P1 := CTPTR;  INSYMBOL END ELSE
         IF CTPTR^.KLASS = KONST THEN SCALDECL(CTPTR) ELSE
         TYPERR(11);
      END;
   END (*ID*) ELSE
   IF NO = 9 THEN  (* SYMBOLIC *)
   BEGIN CV := 0;  LERR := ERR ;  ERR := FALSE;ALLOC(P,TYPES,SYMBOLIC);
      WITH P^ DO
      BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
         FORM := SYMBOLIC;
      END;  RTYP := P;  NXTC := NIL;
      REPEAT INSYMBOL;
         IF NO <> 1 THEN
         BEGIN ERROR(11);  SKIPT(15);
            GOTO 2;
         END;
         SRCHREC(NEXT);
            IF CTPTR <> NIL THEN ERROR(8) ELSE
            BEGIN ALLOC(P,KONST,ACTUAL);
               WITH P^ DO
               BEGIN NAME := AVAL;  NXTEL := NEXT;  KLASS := KONST;
                  CONTYPE := RTYP;  CONKIND := ACTUAL;
                  VALUES := CV;  SUCC := NXTC;
               END;  CV := CV + 1;  NEXT := P;  NXTC := P;
            END;
            INSYMBOL;
2:       UNTIL NO <> 15;
            ALLOC(P,TYPES,POWER) ;
            WITH P^ DO
            BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
               SIZE := 1;  FORM := POWER;
               ELSET := RTYP;  PWBITS := CV + 1;
            END;
            WITH RTYP^ DO
            BEGIN FCONST := NEXT;  SIZE := 1;
               BITSIZE := LOG2(CV - 1);  PWSET := P;
            END;
         IF ERR THEN P1 := NIL ELSE
         BEGIN ERR := LERR ;  P1 := RTYP END ;
         TL := 1 ;
         IF NO <> 10 THEN TYPERR(9) ELSE INSYMBOL ;
   END (* SYMBOLIC *) ELSE
   IF NO IN [2,7] THEN (* SUBRANGE *)
   BEGIN LERR := ERR;  ERR := FALSE;
      SCALDECL(NEXT);
      IF ERR THEN ERROR(6) ELSE ERR := LERR
   END (*SUBRANGE*) ELSE
   IF NO = 38 THEN (* STRUCTURED TYPES *)
   CASE CL OF
(*ARRAY*) 1:
   BEGIN INSYMBOL;
      IF NO <> 11 THEN
      BEGIN TYPERR(98);
         IF TERRCL[NO] = BEGSY THEN TYPEDECL(I,CTPTR);  GOTO 19;
      END;
         NXTA := NIL;
         REPEAT ALLOC(P,TYPES,ARRAYS);
            WITH P^ DO
            BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
               FORM := ARRAYS;  AELTYPE := NXTA;
               (* AELTYPE TEMPORARILY LINKS SUBARRAYS *)
            END;  NXTA := P;
            INSYMBOL;
            LERR := ERR;  ERR := FALSE;
            SUBTYPE(I,J,P);
            IF ERR THEN
            BEGIN ERROR(6);  SKIPT(15);  I := 0;  J := 0;  P := NIL
            END ELSE
            ERR := LERR;
            WITH NXTA^ DO
            BEGIN LO := I;  HI := J;  INXTYPE := P END;
         UNTIL NO <> 15;
         IF NO <> 12 THEN
         BEGIN ERROR(37);  SKIPT(27);
            IF TERRCL[NO] = BEGSY THEN GOTO 11;
            IF NO = 27 THEN
            BEGIN INSYMBOL;  GOTO 11 END ;
            IF NO <> 12 THEN
            BEGIN P1 := NIL ;  TL := 0;  GOTO 19 END;
         END;
         INSYMBOL;
            IF NO <> 27 THEN ERROR(14) ELSE INSYMBOL;
11:         TYPEDECL(TL,CTPTR);
            IF CTPTR <> NIL THEN
            IF CTPTR^.FORM > RECORDS THEN
            BEGIN ERROR(30);  CTPTR := NIL END ELSE
            BEGIN
               REPEAT
                  WITH NXTA^ DO
                  BEGIN MULOPT(TL,E1,E2,OPT);
                     OPTTYP := OPT;  EXP1 := E1;  EXP2 := E2;
                     TL := TL*(HI - LO + 1);
                     SIZE := TL;
                     P := AELTYPE;  AELTYPE := CTPTR;
                  END;
                  CTPTR := NXTA;  NXTA := P;
               UNTIL NXTA = NIL;
               (* NOW TL = SIZE OF ARRAY, CTPTR POINTS TO IT *)
            END;  P1 := CTPTR;
19: END (*ARRAY*);
(*RECORD*) 2:
   BEGIN ALLOC(P,TYPES,RECORDS);
      WITH P^ DO
      BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
         FORM := RECORDS;
      END;  RTYP := P;
      INSYMBOL;  NXTF := NIL;
      DISPL := 0;  BDISPL := 0;  LERR := ERR;  ERR := FALSE;
      FIELDLIST(TL,P,NXTF);
      IF NO <> 22 THEN ERROR(17);
      IF ERR THEN TYPERR(18) ELSE
      WITH RTYP^ DO
      BEGIN SIZE := TL;  FSTFLD := NXTF;  RECVAR := P;
         P1 := RTYP;  ERR := LERR;
      END;
      IF NO = 22 THEN INSYMBOL;
   END (*RECORD*);
(*FILE*) 3:
   BEGIN INSYMBOL;
      IF NO = 27 THEN (*OF*)
      BEGIN I := 8 ;  INSYMBOL END ELSE
      BEGIN INCONST(I,PT,NEXT) ;
         IF (I<=0) OR (PT<>INTPTR) THEN
         BEGIN ERROR(95) ;  I := 0 END ;
         IF NO <> 27 THEN ERROR(14) ELSE INSYMBOL ;
      END ;
      TYPEDECL(TL,CTPTR);
      IF CTPTR <> NIL THEN
      IF CTPTR^.FORM > RECORDS THEN
      BEGIN ERROR(30);  P1 := NIL END ELSE
      BEGIN L := CTPTR^.SIZE;
         LL := L DIV 64 + 1 ;
         IF LL < I THEN LL := I ;
         LL := ((LL*64) DIV L + 1) * L ;
         ALLOC(P,TYPES,FILES);
         WITH P^ DO
         BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
            FORM := FILES;  SIZE := LL;  FELTYPE := CTPTR;
         END;
         TL := LL;  P1 := P;
      END ELSE P1 := NIL;
   END (*FILE*);
(*CLASS*) 4:
   BEGIN ALLOC(P,TYPES,CLASSS);
      WITH P^ DO
      BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
         FORM := CLASSS;
      END;
      INSYMBOL ;
      IF NO = 27 THEN
      BEGIN I := 100 ;  INSYMBOL END ELSE
      BEGIN INCONST(I,PT,NEXT) ;
         IF (I<=0) OR (PT<>INTPTR) THEN
         BEGIN ERROR(6) ;  I := 0 END ;
         IF NO <> 27 THEN ERROR(14) ELSE INSYMBOL ;
      END ;
      TYPEDECL(TL,CTPTR);
      IF CTPTR <> NIL THEN
      IF CTPTR^.FORM > RECORDS THEN
      BEGIN ERROR(30);  P1 := NIL END ELSE
      BEGIN TL := TL*I + 1;
         P^.SIZE := TL;  P^.PELTYPE := CTPTR;
         P1 := P;
      END ELSE P1 := NIL ;
   END (*CLASS*);
(*POWERSET*) 5:
   BEGIN INSYMBOL;  LERR := ERR;  ERR := FALSE;
      SUBTYPE(I,J,P);
      IF ERR THEN TYPERR(6) ELSE
      BEGIN ERR := LERR;
         CASE P^.FORM OF
 NUMERIC:   IF (I < 0) OR (J > WORDLENGTH - 2) THEN
            BEGIN ERROR(6);  P1 := NIL END ELSE
               P1 := PNUMPTR;
 SYMBOLIC:  BEGIN IF P = CHARPTR THEN J := WORDLENGTH - 2  ;
               IF J > WORDLENGTH - 2 THEN
               BEGIN ERROR(6);  P1 := NIL END ELSE P1 := P^.PWSET;
            END;
         END (*CASE*);
         TL := 1;
      END (*NOT ERR*);
   END (*POWERSET*);

   END (*STRUCTURED TYPES*) ELSE
   IF NO = 18 THEN (*POINTER*)
   BEGIN INSYMBOL;
      IF NO <> 1 THEN TYPERR(11) ELSE
      BEGIN TL := 1;  ALLOC(P,TYPES,POINTER);
         WITH P^ DO
         BEGIN NAME := BLANK;  NXTEL := NIL;  KLASS := TYPES;
            FORM := POINTER;  SIZE := 1;
         END;
         SRCHREC(NEXT);
         IF CTPTR = NIL THEN SEARCH;
         IF CTPTR <> NIL THEN
         IF CTPTR^.VTYPE = NIL THEN CTPTR := NIL;
         IF CTPTR = NIL THEN (*UNDECLARED CLASS*)
         IF PTX > PTLIMIT THEN
         BEGIN ERROR(92);  P1 := NIL;  INSYMBOL END ELSE
         WITH PTLIST[PTX] , P^ DO
         BEGIN HNAME := AVAL;  PPTR := P;
