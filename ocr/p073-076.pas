      WITH LABTAB[IT1] DO
      IF LABVAL = EXVAL THEN
      BEGIN IF FLD2 > 0 THEN ERRMESSAGE('EXIT:',EXVAL) ELSE
         BEGIN IT2 := JMPCST ; INSERT(LC,30,IT2) ;
            INSERT(FLD3,0,IT2) ; JMPTAB[JMPTABIX] := IT2
         END ;
         GOTO 2
      END (*IF, WITH, FOR*) ;
      ERRMESSAGE('EXIT:',EXVAL) ;
2:    END (*WITH, FOR*) ;
      CEXTABIX := FSTIX - 1 ;
      WRITOUT ;
      IF LEVEL = 0 THEN (* WRITE OUT JUMPTABLE *)
      FOR IT := 0 TO JMPMAX DO
      BEGIN PASCLGO^ := JMPTAB[IT] ; PUT(PASCLGO) END
   END (*LEAVEBODY*) ;

BEGIN (*BODY*)
   PILEV[LEVEL]:= PFTOP+1;  VLC := 0;
   FILEV[LEVEL] := FILTOP + 1;
   FSTIX := CEXTABIX + 1 ;
1: IF NO = 40 THEN (*LABEL*)
   BEGIN REPEAT INSYMBOL ;
         IF (NO = 2) AND (CL = 1) THEN
         BEGIN IF IVAL >= TWOTO17 THEN
            BEGIN ERROR(100) ; GOTO 2 END ;
            FOR IT := FSTIX TO CEXTABIX DO
            IF EXTAB[IT].EXVAL = IVAL THEN
            BEGIN ERROR(77) ; GOTO 2 END ;
            IF CEXTABIX = MAXEXLABS THEN ERROR(78) ELSE
            BEGIN CEXTABIX := CEXTABIX + 1 ;
               IF JMPIX > JMPMAX THEN ERROR(91) ELSE
               BEGIN WITH EXTAB[CEXTABIX] DO
                  BEGIN EXVAL := IVAL ; JMPTABIX := JMPIX END ;
                  JMPTAB[JMPIX] := LEVEL ; JMPIX := JMPIX + 1 ;
               END
            END ;
2:          INSYMBOL
         END (*IF (NO=2) AND (CL=1)*) ELSE
         BEGIN ERROR(61) ; GOTO 3 END
      UNTIL NO <> 15 (*,*) ;
3:    FINDSEMICOLON;
   END ;
   IF NO = 41 THEN (* CONST *)
   BEGIN INSYMBOL ;
         WHILE NO = 1 DO
         BEGIN REPEAT SRCHREC(NEXT) ; IF CTPTR <> NIL THEN ERROR(8);
         ALLOC(P,KONST,ACTUAL) ;
         WITH P^ DO
         BEGIN NAME := AVAL;  NXTEL := NEXT;  KLASS := KONST;
           CONKIND := ACTUAL;
         END;  NEXT := P;
         INSYMBOL;
         IF (NO <> 8) OR (CL <> 6) THEN ERROR(4) ELSE INSYMBOL;
         IF NO = 36 THEN (* NIL *)
         BEGIN P^.CONTYPE := NILPTR;
            P^.VALUES := NILVAL;  INSYMBOL;
         END ELSE
         BEGIN INCONST(I,N,NEXT^.NXTEL);
            P^.CONTYPE := N;  P^.VALUES := I;
         END;
            WHILE NO = 15 DO
            BEGIN INSYMBOL;
               IF NO <> 1 THEN
               BEGIN ERROR(11); SKIP(15) END;
            END;
         UNTIL NO <> 1;
         FINDSEMICOLON;
      END (* WHILE NO = 1 *) ;
   END (*CONST*);
   IF NO =37 THEN (*TYPE*)
   BEGIN INSYMBOL;
   WHILE NO = 1 DO
   BEGIN SRCHREC(NEXT); IF CTPTR <> NIL THEN ERROR(8);
      TYPID := AVAL;  INSYMBOL;
      IF (NO <> 8) OR (CL <> 6) THEN ERROR(4) ELSE INSYMBOL;
      ERR := FALSE;  TYPEDECL(TL,P);
      IF NOT ERR THEN
      IF P^.NAME <> BLANK THEN ERROR(96) ELSE
      BEGIN P^.NAME := TYPID;  P^.NXTEL := NEXT;
         NEXT := P;
      END;
      FINDSEMICOLON;
   END;
   END (*TYPE*) ;
   IF NO=43 THEN (*VAR*)
      VARDECL ELSE VLC := LC;
   IF PTX > 0 THEN
   BEGIN ERROR(12); WRITE(EOL); PRTERR;
      FOR PTX := PTX-1 DOWNTO 0 DO
         WRITE(' ','CLASS-ID',PTLIST[PTX].HNAME,EOL) ;
      IF NOT EOLFLAG THEN WRITE(' ':CHCNT+8);
   END;
   IF LEVEL = 0 THEN
   BEGIN
      FOR I := 0 TO FILTOP DO
      WITH FILPTS[I]^ DO
      BEGIN
         IF VTYPE^.FELTYPE = CHARPTR THEN
         BEGIN IT := 0;  LC := LC + ALFALENG;
            FOR J := 1 TO ALFALENG DO PUT(PASCLGO);  (*CHAR BUFFER*)
         END ELSE IT := 1;
         PUT(PASCLGO);  (* P-PTR *)
         J := 2*VLEVEL + IT;  (*SPECIFIER*)  VLEVEL := 0;
         UNPACK(NAME,LCH,1) ; IT1 := 7 ;
         WHILE LCH[IT1] = ' ' DO IT1 := IT1 - 1 ;
         IT1 := (ALFALENG - IT1)*6 ;
         APPEND(J,55,VTYPE^.FELTYPE^.SIZE);
         PASCLGO^ := J;  PUT(PASCLGO);  (* SPECIFIER + LRL *)
         J := INT(NAME);  APPEND(J,-IT1,0);  APPEND(J,IT1,0);
         PASCLGO^ := J;  PUT(PASCLGO);  (* LFN *)
         PASCLGO^ := VADDR;
         FOR J := 1 TO 3 DO PUT(PASCLGO);  (*FIRST,IN,OUT*)
         PASCLGO^ := VADDR + VTYPE^.SIZE;  PUT(PASCLGO);  (* LIMIT *)
         VADDR := LC;  LC := LC + 7;
      END;
      PARMLIST.LPJMPTAB := LC ;
      IF VLC <> LC THEN FOR J := 0 TO JMPMAX DO PUT(PASCLGO)
                   ELSE VLC := LC + (JMPMAX + 1) ;
      LC := LC + (JMPMAX + 1);
      IC := LC;  PARMLIST.LOADPT := VLC;  (*LOADPT*)
   END;
   DP := FALSE;
   IF NO IN [44,45] THEN (*FUNCTION OR PROCEDURE*)
   BEGIN IF SURRPTR <> NIL THEN
      IF SURRPTR^.PROCADDR = 0 THEN
      BEGIN IF JMPIX > JMPMAX THEN ERROR(91) ELSE
         BEGIN SURRPTR^.PROCADDR := PARMLIST.LPJMPTAB + JMPIX ;
            JMPIX := JMPIX + 1 ;
         END
      END;
   REPEAT
      LL := NO; ERR := FALSE;
      OLDLEV := LEVEL;
      IF LEVEL < MAXLEVEL THEN LEVEL := LEVEL + 1 ELSE ERROR(76);
      INSYMBOL;
      IF NO <> 1 THEN
      BEGIN ERROR(11);  LEVEL := OLDLEV;  GOTO 1 END;
      LC1 := LC;  SRCHREC(NEXT);
      IF CTPTR <> NIL THEN
      IF CTPTR^.KLASS <> PROC THEN
      BEGIN ERROR(8);  CTPTR := NIL END;
      IF CTPTR = NIL THEN (* UNDECLARED PROCEDURE *)
      BEGIN ALLOC(PROCPTR,PROC);
         WITH PROCPTR^ DO
         BEGIN NAME := AVAL;  NXTEL := NEXT;  KLASS := PROC;
            PROCTYPE := PROCPTR;  PROCKIND := ACTUAL;
            PROCLEVEL := LEVEL - 1;
         END;
         DISPLAY[TOP].FNAME := PROCPTR;
         NEXT := NIL ;
         INSYMBOL;
         IF LL = 44 THEN (* FUNCTION *)
         BEGIN IF NO <> 9 THEN ERROR(29) ELSE
            BEGIN LC := 3 ; INSYMBOL ; FORMPARM END ;
            IF ERR THEN PROCPTR^.PROCTYPE := NIL;
