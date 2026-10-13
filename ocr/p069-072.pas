            BEGIN
               CASE SPEC OF
   KONST:         BEGIN ALLOC(P,KONST,FORMAL);
                    WITH P^ DO
                    BEGIN CONTYPE := NIL;  CONKIND := FORMAL;
                      CADDR := LC;  CLEVEL := LEVEL;
                    END;
                  END;
   PROC:          BEGIN ALLOC(P,PROC);
                    WITH P^ DO
                    BEGIN PROCTYPE := NIL;  PROCKIND := FORMAL;
                      PROCADDR := LC;  PROCLEVEL := LEVEL;
                      FORMALS := NIL;
                    END;
                  END;
   VARS:          BEGIN ALLOC(P,VARS);
                    WITH P^ DO
                    BEGIN VTYPE := NIL;  VKIND := FORMAL;
                      VADDR := LC;  VLEVEL := LEVEL;
                    END;
                  END;
                  END (*CASE*);
                  P^.NAME := AVAL;  P^.NXTEL := NEXT;
                  P^.KLASS := SPEC;
                  LC := LC + 1;  NEXT := P;
               END (*CTPTR = NIL*);
               INSYMBOL;
               IF NO = 15 THEN
               BEGIN INSYMBOL;
                  IF NO = 19 THEN ERROR(11);
               END ELSE IF NO <> 19 THEN GOTO 4;
            UNTIL NO <> 1;
4:          IF NO <> 19 THEN ERROR(10) ELSE INSYMBOL;
            IF NO <> 1 THEN FORMERR;
            SEARCH;
            IF CTPTR = NIL THEN
            BEGIN ERROR(12);  GOTO 2 END;
            IF CTPTR^.KLASS <> TYPES THEN
            BEGIN ERROR(18) ;  GOTO 2 END ;
            N := NEXT;
            REPEAT
               CASE SPEC OF
   KONST:         IF N^.CONTYPE = NIL THEN
                  BEGIN IF NOT (CTPTR^.FORM IN
                        [NUMERIC,SYMBOLIC,POWER]) THEN
                     BEGIN N^.KLASS := VARS;
                        IF CTPTR^.FORM = POINTER THEN
                           N^.VKIND := ACTUAL ;
                     END;
                     N^.CONTYPE := CTPTR;  N := N^.NXTEL;
                  END ELSE N := NIL;
   PROC:          IF N^.PROCTYPE = NIL THEN
                     IF CTPTR^.FORM > POWER THEN
                     BEGIN ERROR(93);  N := NIL END ELSE
                     BEGIN N^.PROCTYPE := CTPTR;  N := N^.NXTEL END
                  ELSE N := NIL;
   VARS:          IF N^.VTYPE = NIL THEN
                  BEGIN N^.VTYPE := CTPTR;  N := N^.NXTEL;
                  END ELSE N := NIL;
                  END (*CASE*);
               UNTIL N = NIL;
2:             INSYMBOL;
         END (*NOT PROCEDURE*);
3:       REP := NO IN [1,41,43,44,45];
         IF NO = 16 THEN
         BEGIN INSYMBOL;  REP := NO <> 10  END;
      UNTIL NOT REP;
      IF NO <> 10 THEN
      BEGIN ERROR(9);  SKIP(10);
         IF NO IN [41,43,44,45] THEN GOTO 1;
      END;
      (*REVERSE POINTERS*)
      N := NEXT;  NEXT := NIL;
      WHILE N <> NIL DO
      BEGIN P := N;  N := P^.NXTEL;
         P^.NXTEL := NEXT;  NEXT := P;
      END;
   END (*FORMPARM*);

   PROCEDURE OUTPMD(SURRPTR : CTP);
      CONST DIRECT = 0, INDIRECT = 1;
      VAR N,P : CTP;  ADR,ACC : SHRTINT;  OUT : BOOLEAN;
   BEGIN
      WITH PMDFILE^ DO
      BEGIN IF SURRPTR = NIL THEN
         BEGIN PNAME := '(MAIN)';  SA := IC END ELSE
         BEGIN PNAME := SURRPTR^.NAME;  SA := SURRPTR^.PROCADDR END;
         PKIND := PROCOR;  LINK := LASTLINK;  COUNT := 0;
      END;
      PUT(PMDFILE);  PMCTR := PMCTR + 2;  LASTLINK := PMCTR;
      N := NEXT;
      WHILE N <> NIL DO
      WITH N^ DO
      BEGIN OUT := FALSE;  ACC := DIRECT;
         CASE KLASS OF
 KONST:     IF CONKIND = FORMAL THEN
            BEGIN OUT := TRUE;  P := CONTYPE;  ADR := CADDR END;
 VARS:      BEGIN OUT := TRUE;  P := VTYPE;  ADR := VADDR;
               IF VKIND = FORMAL THEN ACC := INDIRECT;
            END;
TYPES,PROC:
         END;
         IF OUT THEN IF P^.FORM < POWER THEN
         BEGIN WITH PMDFILE^ DO
            BEGIN PNAME := NAME;  PKIND := OBJ;
               IF P^.FORM = NUMERIC THEN TYP := 1 ELSE
               IF P = REALPTR THEN TYP := 2 ELSE
               IF P = ALFAPTR THEN TYP := 3 ELSE
               IF P = CHARPTR THEN TYP := 4 ELSE
               IF P = BOOLPTR THEN TYP := 5 ELSE
               IF P^.FORM = SYMBOLIC THEN TYP := 1 ELSE TYP := 6 ;
               RLA := ADR;  ACCESS := ACC;
            END;
            PUT(PMDFILE);  PMCTR := PMCTR + 2;
         END;
         N := NXTEL;
      END (*WITH, WHILE*);
   END (*OUTPMD*);

   PROCEDURE ENTERBODY ;
   (*GENERATES PROCEDURE PROLOG CODE AND PROCEDURE ENTRY CODE, CODE
    TO INITIALIZE EFETS AND TO OPEN LOCAL FILES, AND CODE TO INITIALIZE
    LOCAL CLASSPOINTERS*)
      CONST OPN = 104B ;
      VAR IT2 : SHRTINT;
   BEGIN CA := 0 ; CP := 4 ; LCX := 0 ;
      IF LEVEL <> 0 THEN
      BEGIN (*PROCEDURE ENTRY CODE*)
         GEN15(76B,0,5,0) ; GEN15(20B,0,0,18) ;
         GEN15(12B,7,7,0) ; GEN15(66B,5,6,0) ;
         GEN30(51B,7,5,1) ; GEN30(61B,6,6,0) ;
         LCA := CA ; LCP := CP ;
         IF LEVEL <> 1 THEN GEN15(56B,6,5,0) ;
         (*CODE TO INITIALIZE EFETS*)
         FOR IT := FILEV[LEVEL] TO FILTOP DO
         WITH FILPTS[IT]^ DO
         BEGIN
            UNPACK(NAME,LCH,1) ; IT2 := 7 ;
            WHILE LCH[IT2] = ' ' DO IT2 := IT2 - 1 ;
            RP := 0;  LDCST(INT(NAME));
            GEN15(43B,0,0,6*IT2);  GEN15(11B,6,0,1);
            GEN30(51B,6,5,VADDR+2);
            GEN30(71B,6,0,VTYPE^.FELTYPE^.SIZE);
            IF VTYPE^.FELTYPE <> CHARPTR THEN
            BEGIN GEN15(43B,0,0,1);
               GEN15(20B,0,0,56);  GEN15(12B,6,6,0);
            END;
            GEN30(51B,6,5,VADDR+1);
            GEN30(71B,6,5,VADDR+7);
            GEN30(51B,6,5,VADDR+3);
            GEN30(72B,6,6,VTYPE^.SIZE);
            GEN30(51B,6,5,VADDR+6);
         END;
      END ELSE
      BEGIN
         (*MAIN PROGRAM, SAVE X7,B5 SET B6*)
         GEN15(56B,7,5,0) ; GEN15(76B,7,5,0) ;
         GEN30(51B,7,0,PARMLIST.ICO) ; GEN30(61B,6,5,0) ;
         LCA := CA ; LCP := CP ; LC := 1 ;
      END ;
      IF STOFLCHECK THEN GEN30(06B,6,4,STOFLERR) ;
      (*CODE TO OPEN FILES*)
      IF LEVEL = 0 THEN IT2 := 0 ELSE IT2 := 5 ;
      FOR IT := FILEV[LEVEL] TO FILTOP DO
      BEGIN GEN30(61B,7,IT2,FILPTS[IT]^.VADDR+1);
         JUMPTO(OPN) ;
      END;
      (*CODE TO INITIALIZE CLASSPOINTERS*)
      FOR IT := PILEV[LEVEL] TO PFTOP DO
      BEGIN IT1 := PFL[IT] ;
         GEN30(71B,7,IT2,IT1+1) ; GEN30(51B,7,IT2,IT1) ;
      END ;
      PFTOP := PILEV[LEVEL] - 1 ; TCT := 0 ; TMAX := 0 ; CLABIX := 0 ;
   END (*ENTERBODY*) ;

   PROCEDURE LEAVEBODY ;
   (*CALLED AT PROCEDURE END. GOES THROUGH LABELTABLE LABTAB AND
     EXITTABLE EXTAB, UPDATING CODE, AND GENERATES PROCEDURE EXIT CODE*)
      CONST CLS = 105B ;
      VAR IT2,IT3 : INTEGER;
      PROCEDURE ERRMESSAGE(ALF:ALFA; VAL:SHRTINT) ;
      BEGIN ERROR(43) ; WRITE(EOL) ; PRTERR ;
         WRITE(' ',ALF:6,VAL:6,EOL) ;
         IF NOT EOLFLAG THEN WRITE(' ':CHCNT+8)
      END (*ERRMESSAGE*) ;
   BEGIN IF LEVEL = 0 THEN IT2 := 0 ELSE IT2 := 5 ;
      FOR IT := FILEV[LEVEL] TO FILTOP DO
      BEGIN GEN30(61B,7,IT2,FILPTS[IT]^.VADDR + 1) ; JUMPTO(CLS) ;
      END ;
      FILTOP := FILEV[LEVEL] - 1 ;
      IF LEVEL <> 0 THEN (*PROCEDURE EXIT CODE*)
      BEGIN GEN30(51B,1,5,1) ; GEN15(66B,6,5,0) ;
         GEN15(63B,7,1,0) ; GEN15(20B,1,0,42) ;
         GEN15(63B,5,1,0) ;
      END ELSE (*MAIN, RETURN TO MONITOR*)
      BEGIN GEN15(56B,1,5,0) ; GEN15(63B,7,1,0) END ;
      GEN30(02B,7,0,0) ; LC := LC + TMAX ; INS(LC,LCP,LCA) ;
      FOR IT := 1 TO CLABIX DO
      WITH LABTAB[IT] DO
      BEGIN IF FLD2 > 0 THEN (*UNDECLARED  LABEL*)
         BEGIN
            ERRMESSAGE('LABEL:',LABVAL) ;
            UNDLAB[FLD2].SUCC := CHNIX ; CHNIX := FLD3 ;
         END (*IF FLD2 > 0*) ;
      END (*WITH, FOR*) ;
      FOR IT := FSTIX TO CEXTABIX DO
      WITH EXTAB[IT] DO
      BEGIN FOR IT1 := 1 TO CLABIX DO
