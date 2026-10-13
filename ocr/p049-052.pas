   BEGIN
      NOOP ; LJPADDR := IC ;
      INSYMBOL ; EXPRESSION ;
      IF GATTR.TYPTR <> NIL THEN GENJP(0) ;
      LCA := CA ; LCP := CP ;
      IF NO <> 31 THEN  (* SY <> DO *)
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
      GEN30(04B,0,0,LJPADDR) ;
10:   NOOP;  INS(IC,LCP,LCA);
   END (* WHILESTAT *) ;

   PROCEDURE FORSTAT ;
   VAR LATTR : ATTR ; LCLASS, LCA, LCP : SHRTINT ;
      LJPADDR : ADDRESS ; LOF : BOOLEAN ; LUPBND : SHRTINT ;

   PROCEDURE CHTYPE ;
   BEGIN
      WITH GATTR DO
      IF TYPTR <> NIL THEN
      IF (TYPTR^.FORM > SYMBOLIC) OR (TYPTR = REALPTR) OR
      (TYPTR = ALFAPTR) THEN
      BEGIN ERROR(62) ; TYPTR := NIL END
   END ;

   PROCEDURE CHTYPES ;
   BEGIN
      WITH GATTR DO
      IF (TYPTR <> NIL) AND (LATTR.TYPTR <> NIL) THEN
      IF((TYPTR^.FORM = SYMBOLIC) OR (LATTR.TYPTR^.FORM = SYMBOLIC)) AND
      (TYPTR <> LATTR.TYPTR) THEN
      BEGIN ERROR(73) ; TYPTR := NIL END
   END ;

   BEGIN LCA := 0 ;
      INSYMBOL ;
      IF NO <> 1 THEN
      BEGIN ERROR(49) ; GATTR.TYPTR := NIL END ELSE
      BEGIN SEARCH ;
         IF CTPTR = NIL THEN
         BEGIN ERROR(31) ; CTPTR := UNDECPTR END ;
         IF CTPTR^.KLASS <= PROC THEN
         BEGIN ERROR(32) ; INSYMBOL END ELSE VARIABLE
      END ;
      CHTYPE ;
      IF GATTR.TYPTR <> NIL THEN
      WITH GATTR DO
      IF ACCESS <> DRCT THEN
      BEGIN ERROR(69) ; TYPTR := NIL END ELSE
      IF NOT (BREG IN [0,LEVEL]) THEN ERROR(69) ;
      LATTR := GATTR ;
      IF NO <> 20 THEN  (* SY <> := *)
      BEGIN
         IF GATTR.TYPTR <> NIL THEN ERROR(52) ; SKIP(20);
         IF NO <> 20 THEN
         BEGIN
            IF GATTR.TYPTR = NIL THEN ERROR(52) ;
            IF ERRCL[NO] = BEGSY THEN GOTO 20 ; GOTO 10
         END
      END ;
      INSYMBOL ; EXPRESSION ;
      CHTYPE ; CHTYPES ;
      TRANSFER(GATTR,RP) ;
      IF NO <> 33 THEN  (* SY <> TO/DOWNTO *)
      BEGIN
         IF GATTR.TYPTR <> NIL THEN ERROR(70) ; SKIP(33) ;
         IF NO <> 33 THEN
         BEGIN
            IF GATTR.TYPTR = NIL THEN ERROR(70) ;
            IF ERRCL[NO] = BEGSY THEN GOTO 20 ; GOTO 10
         END
      END ;
      LCLASS := CL ;
      INSYMBOL ; EXPRESSION ;
      CHTYPE ; CHTYPES ;
      IF (GATTR.TYPTR <> NIL) AND (LATTR.TYPTR <> NIL) THEN
      BEGIN
         TRANSFER(GATTR,RP) ;
         IF GATTR.KIND = SVAL THEN
         BEGIN LOF := TRUE ; LUPBND := GATTR.VAL END ELSE
         BEGIN GEN15(10B,7,RP,0) ;
            GEN30(51B,7,5,LC+TCT) ; TCT := TCT + 1 ;
            IF TCT > TMAX THEN TMAX := TCT ; LOF := FALSE ;
         END ;
         GEN15(10B,6,1,0) ; NOOP ; LJPADDR := IC ;
         IF LCLASS = 1 THEN (*STEP +1*) GEN15(37B,1,2,6)
                       ELSE (*STEP -1*) GEN15(37B,1,6,2) ;
         GEN30(03B,3,1,0) ; LCA := CA ; LCP := CP ; RP := 0 ;
         IF ASSCHECK THEN
         WITH LATTR.TYPTR^ DO
         IF FORM = NUMERIC THEN
         BEGIN IF LATTR.TYPTR <> INTPTR THEN
            CHECKBNDS(6,MIN,MAX,ASSERR)
         END ELSE CHECKBNDS(6,0,FCONST^.VALUES,ASSERR) ;
         TRANSFER(LATTR,6) ;
      END ;
      IF NO <> 31 THEN  (* SY <> DO *)
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
      IF LATTR.TYPTR <> NIL THEN
      BEGIN TRANSFER(LATTR,RP) ;
         WITH LATTR.TYPTR^ DO
         IF (FORM = NUMERIC) AND ((ABS(MIN) >= TWOTO17) OR (ABS(MAX) >=
         TWOTO17)) THEN
         BEGIN GEN30(71B,0,0,1) ;
            GEN15(35B+LCLASS,6,1,0) ;  (* +1 OR -1*)
         END ELSE GEN30(72B,6,1,3-2*LCLASS) ;
         IF LOF THEN GEN30(71B,2,0,LUPBND) ELSE
         BEGIN TCT := TCT - 1 ; GEN30(51B,2,5,LC+TCT) END ;
         GEN30(04B,0,0,LJPADDR) ; NOOP ;
         INS(IC,LCP,LCA);
      END ;
10:   END (* FORSTAT *) ;

   PROCEDURE GOTOSTAT ;
   BEGIN INSYMBOL ;
         IF (NO = 1) AND (AVAL = 'EXIT') THEN
         BEGIN INSYMBOL ;
            IF (NO <> 2) OR (CL <> 1) THEN
            BEGIN ERROR(61) ; SKIP(49) END ELSE
            BEGIN FOR IT := CEXTABIX DOWNTO 1 DO
                  IF EXTAB[IT].EXVAL = IVAL THEN
                  BEGIN IT1 := EXTAB[IT].JMPTABIX ;
                     IF JMPTAB[IT1] <> 0 THEN
                     BEGIN IF JMPTAB[IT1] <> LEVEL THEN
                        BEGIN LOADBASE(1,JMPTAB[IT1]) ; GEN15(63B,5,1,0)
                        END
                     END ELSE
                     BEGIN GEN30(51B,1,0,PARMLIST.ICO) ;
                        GEN15(63B,5,1,0)
                     END ;
                     GEN30(04B,0,0,PARMLIST.LPJMPTAB+IT1) ; GOTO 1
                  END ;
                  ERROR(43) ;
1:                INSYMBOL
            END
         END ELSE
         IF (NO <> 2) OR (CL <> 1) THEN
         BEGIN ERROR(61) ; SKIP(49) END ELSE
         BEGIN IF IVAL >= TWOTO17 THEN
            BEGIN ERROR(100) ; GOTO 20 END ;
            (* SEARCH THROUGH LABELTABLE OF CURRENT BLOCK *)
            FOR IT := 1 TO CLABIX DO
            WITH LABTAB[IT] DO
            BEGIN
               IF LABVAL = IVAL THEN  (* LABEL ALREADY OCCURED *)
               BEGIN
                  (* IF DECL. OCC. GENERATE CODE ELSE CHAIN OCC. *)
                  IF FLD2 = 0 THEN GEN30(04B,0,0,FLD3) ELSE
                  BEGIN
                     GEN30(04B,0,0,0) ;
                     IF CHNIX = 0 THEN
                     BEGIN ERROR(75) ; GOTO 20 END ;
                     WITH UNDLAB[CHNIX] DO
                     BEGIN
                        IT1 := SUCC ; SUCC := FLD3 ;
                        FLD3 := CHNIX ; PLACE := CA ;
                        LFTSH := CP ;
                     END ;
                     CHNIX := IT1
                  END ;
                  GOTO 20
               END
            END ;
            (* LABEL NOT YET MET, ENTER IT INTO LABELTABLE *)
            GEN30(04B,0,0,0) ;
            IF CLABIX = MAXLABS THEN BEGIN ERROR(74) ; GOTO 20 END ;
            IF CHNIX = 0 THEN BEGIN ERROR(75) ; GOTO 20 END ;
            CLABIX := CLABIX + 1 ;
            WITH LABTAB[CLABIX], UNDLAB[CHNIX] DO
            BEGIN
               IT1 := SUCC ; LABVAL := IVAL ;
               SUCC := 0 ; FLD2 := CHNIX ;
               FLD3 := CHNIX ; PLACE := CA ;
               LFTSH := CP
            END ;
            CHNIX := IT1 ;
20:         INSYMBOL ;
         END ;
   END (* GOTOSTAT *) ;

   PROCEDURE WITHSTAT ;
   VAR LTPCT, LB7CT: SHRTINT ;
   BEGIN
      LTPCT := 0 ; LB7CT := 0 ;
      REPEAT
         INSYMBOL ;
         VARIAB ;
         WITH GATTR DO
         IF TYPTR <> NIL THEN
         IF TYPTR^.FORM <> RECORDS THEN ERROR(38) ELSE
         BEGIN
