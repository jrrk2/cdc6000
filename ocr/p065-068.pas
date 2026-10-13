            DOMAIN := P;  ELTYPE := P;
            PTX := PTX + 1;  P1 := P;  INSYMBOL;
         END ELSE
         WITH CTPTR^ DO
         IF (KLASS = VARS) AND (VTYPE^.FORM = CLASSS) THEN
         BEGIN P^.DOMAIN := CTPTR;
            P^.ELTYPE := VTYPE^.PELTYPE;
            P1 := P;  INSYMBOL;
         END ELSE TYPERR(15);
      END;
   END (*POINTER*) ELSE TYPERR(18);
END (*TYPEDECL*);

(* ------------------------------------------------------------ *)

PROCEDURE BODY(SURRPTR,FIRSTENTRY:CTP);

LABEL 1;
VAR I,J : INTEGER;
   OLDLEV : RG3;
   LC1,TL,LL,VLC,FSTIX,LCA,LCP : SHRTINT ;
   N,P,PROCPTR,LFIRSTENTRY : CTP;
   TYPID : ALFA ; LCH : CWORD ;

   PROCEDURE FINDSEMICOLON;
   BEGIN IF NO <> 16 THEN
      BEGIN ERROR(58);  SKIP(16);
         IF NO <> 16 THEN GOTO EXIT 1;
      END;  INSYMBOL;
   END (*FINDSEMICOLON*);

PROCEDURE VARDECL;
   LABEL 10;
   VAR AT : ADDRESS; I,J : SHRTINT;

   PROCEDURE FILERR;
   BEGIN ERROR(95);  SKIP(12);  GOTO EXIT 10 END;

BEGIN INSYMBOL;
   WHILE NO = 1 DO
   BEGIN I := 0;
      REPEAT
         SRCHREC(NEXT);
         IF CTPTR <> NIL THEN ERROR(8) ELSE
         BEGIN ALLOC(P,VARS);
            WITH P^ DO
            BEGIN NAME := AVAL;  NXTEL := NEXT;  KLASS := VARS;
               VKIND := ACTUAL;  VTYPE := NIL;
               VLEVEL := 0 ;
            END;
            NEXT := P;  I := I + 1;
         END;
         INSYMBOL;
         IF NO = 11 THEN (* [ IN,OUT,PRINT,PUNCH  ] *)
         BEGIN INSYMBOL;
            IF (NO = 8) AND (CL = 7) THEN (*IN*)
               P^.VLEVEL := 4  ELSE
            IF NO = 1 THEN
               IF AVAL = 'OUT' THEN
                 P^.VLEVEL := 1  ELSE
               IF AVAL = 'PRINT' THEN
                 P^.VLEVEL := 2 ELSE
               IF AVAL = 'PUNCH' THEN
                 P^.VLEVEL := 3 ELSE FILERR ELSE FILERR ;
            INSYMBOL;  IF NO <> 12 THEN FILERR;
            INSYMBOL;
         END;
         ERR := FALSE;
         IF NO = 15 THEN
         BEGIN INSYMBOL;
            IF NO <> 1 THEN
            BEGIN ERROR(11);  GOTO 10 END;
         END ELSE IF NO <> 19 THEN ERROR(10);
      UNTIL NO <> 1 ;
      IF NO = 19 THEN INSYMBOL
         ELSE IF NOT ERR THEN ERROR(10) ;
      N := NEXT;  ERR := FALSE;
      TYPEDECL(TL,CTPTR);
      IF ERR THEN GOTO 10;
      LC := LC + I*TL;  LL := LC;
      FOR I := I DOWNTO 1 DO
      WITH N^ DO
      BEGIN LL := LL - TL;
         VTYPE := CTPTR;
         IF CTPTR <> NIL THEN
         IF CTPTR^.FORM = CLASSS THEN
         BEGIN (* CHECK FOR PREDECLARED CLASS *)
            TYPID := NAME;  P := CTPTR^.PELTYPE;
            IF PFTOP = FILLIMIT THEN ERROR(92) ELSE
            BEGIN PFTOP := PFTOP + 1;  PFL[PFTOP] := LL END;
            FOR J := PTX - 1 DOWNTO 0 DO
            WITH PTLIST[J] DO
               IF HNAME = TYPID THEN
               BEGIN PPTR^.DOMAIN := N;
                  PPTR^.ELTYPE := P;
                  PTX := PTX - 1;
                  HNAME := PTLIST[PTX].HNAME;
                  PPTR := PTLIST[PTX].PPTR;
               END (* WITH, FOR *) ;
            VLEVEL := LEVEL;  VADDR := LL;
         END (* CHECK CLASS *) ELSE
         IF CTPTR^.FORM = FILES THEN
            IF FILTCP = FILLIMIT THEN ERROR(92) ELSE
            BEGIN FILTOP := FILTOP + 1;  FILPTS[FILTOP] := N;
               VADDR := LL;
               IF LEVEL > 0 THEN
               (* TAKE EFET INTO ACCOUNT FOR ADDRESS *)
               BEGIN IF CTPTR^.FELTYPE = CHARPTR THEN
                  J := ALFALENG + 7 ELSE J := 7;
                  LC := LC + J;  VADDR := VADDR + I*J - 7;
                  VLEVEL := LEVEL ;
               END;
            END (*FORM = FILES*) ELSE
         BEGIN VLEVEL := LEVEL;  VADDR := LL END;
         N := NXTEL;
      END (* FOR I *) ;
10:   FINDSEMICOLON;
   END (*WHILE NO = 1*);
   IF NO = 47 THEN (* VALUE *)
      BEGIN IF LEVEL <> 0 THEN ERROR(22);
         INSYMBOL;
         WHILE NO = 1 DO
         BEGIN SRCHREC(NEXT);
            IF CTPTR = NIL THEN SEARCH;
            IF CTPTR = NIL THEN
            BEGIN ERROR(12);  FINDSEMICOLON;  GOTO 20 END;
            IF CTPTR^.KLASS <> VARS THEN
            BEGIN ERROR(26);  FINDSEMICOLON;  GOTO 20 END;
            WITH CTPTR^ DO
            BEGIN IT := VADDR;
               IF VLC = 0 THEN
               BEGIN VLC := IT;  LL := IT;  AT := IT;  END;
               IF IT < LL THEN ERROR(27) ELSE
               WHILE IT > AT DO
               BEGIN PUT(PASCLGO);  AT := AT + 1  END;
               INSYMBOL;
               IF (NO <> 8) OR (CL <> 6) THEN ERROR(4) ELSE INSYMBOL;
               IF NO = 9 THEN (* LIST *)
               BEGIN
                  REPEAT INSYMBOL;  INCONST(I,PT,NEXT);  IT1 := 1;
                     IF (NO = 6) AND (CL = 1) THEN (* * *)
                     BEGIN IT1 := I;  INSYMBOL;
                        INCONST(I,PT,NEXT);
                     END;
                     IF NOT ERR THEN
                    BEGIN
                     FOR J := 1 TO IT1 DO
                     BEGIN PASCLGO^ := I;  PUT(PASCLGO)  END;
                     AT := AT + IT1;
                    END (*NOT ERR*);
                     IF NOT (NO IN [15,10]) THEN (* NOT , NOR ) *)
                     BEGIN ERROR(20);  FINDSEMICOLON;  GOTO 20 END;
                  UNTIL NO = 10 ;  INSYMBOL;
               END ELSE
               BEGIN INCONST(I,PT,NEXT);
                  IF NOT ERR THEN
                  BEGIN PASCLGO^ := I;  PUT(PASCLGO);
                     AT := AT + 1;
                  END;
               END;
               LL := IT + VTYPE^.SIZE;
               IF LL < AT THEN ERROR(20) ;
               FINDSEMICOLON;
            END (*WITH*);
20:         END (*WHILE NO = 1*);
            FOR AT := AT + 1 TO LC DO PUT(PASCLGO) ;
      END (*VALUE*) ELSE VLC := LC;
END (*VARDECL*) ;

PROCEDURE FORMPARM;
   (* ( HAS BEEN VERIFIED AND NEXT SYMBOL READ *)
   LABEL 3;
   VAR SPEC : IDCLASS;  REP : BOOLEAN;

   PROCEDURE FORMERR;
   BEGIN ERROR(11);  SKIP(10);  GOTO EXIT 3 END;

BEGIN SPEC := KONST ;
1: REPEAT
      IF NO = 45 THEN (*PROCEDURE*)
      REPEAT INSYMBOL;
         IF NO <> 1 THEN ERROR(11) ELSE
         BEGIN SRCHREC(NEXT);
            IF CTPTR <> NIL THEN ERROR(8) ELSE
            BEGIN ALLOC(P,PROC);
               WITH P^ DO
               BEGIN NAME := AVAL;  NXTEL := NEXT;  KLASS := PROC;
                  PROCTYPE := P;  PROCKIND := FORMAL;
                  PROCADDR := LC;  PROCLEVEL := LEVEL;
                  FORMALS := NIL;
               END;  LC := LC + 1;  NEXT := P;
            END (*CTPTR = NIL*);
            INSYMBOL;
         END;
      UNTIL NO <> 15
      ELSE (*NOT PROCEDURE*)
      BEGIN
         IF NO <> 1 THEN
         BEGIN IF NO = 43 THEN SPEC := VARS ELSE
            IF NO = 41 THEN SPEC := KONST ELSE
            IF NO = 44 THEN SPEC := PROC ELSE
               FORMERR;
            INSYMBOL;
         END ELSE IF SPEC <> VARS THEN SPEC := KONST ;
         IF NO <> 1 THEN FORMERR;
         REPEAT
            SRCHREC(NEXT);
            IF CTPTR <> NIL THEN ERROR(8) ELSE
