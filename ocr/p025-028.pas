    IF GATTR.TYPTR <> NIL THEN
    IF GATTR.TYPTR^.FORM > SYMBOLIC THEN ERROR(35) ELSE
    IF (GATTR.TYPTR = REALPTR) OR (GATTR.TYPTR = ALFAPTR) THEN
    ERROR(62) ELSE
    BEGIN IF LATTR.TYPTR <> NIL THEN
      IF ((LATTR.TYPTR^.FORM <> NUMERIC) OR (GATTR.TYPTR^.FORM <>
      NUMERIC)) AND (LATTR.TYPTR <> GATTR.TYPTR) THEN ERROR(73) ;
      LATTR.TYPTR := GATTR.TYPTR ;
      IF GATTR.KIND = SVAL THEN INSERT(1,GATTR.VAL,LPSVAL) ELSE
      BEGIN TRANSFER(GATTR,RP) ; GEN15(63B,7,RP,0) ;
        IF LATTR.KIND = SVAL THEN
        BEGIN GEN30(71B,RP,0,1) ; GEN15(22B,RP,7,RP) ;
          LATTR.KIND := LVAL ; LATTR.CTERM := 0
        END ELSE
        BEGIN GEN30(71B,0,0,1) ; GEN15(22B,0,7,0) ;
          RP := RP - 1 ; GEN15(12B,RP,RP,0) ;
        END
      END
    END ;
  END (*ELEMENT*) ;

BEGIN IF NO = 1 THEN                                  (*IDENTIFIER*)
  BEGIN SEARCH ;
    IF CTPTR = NIL THEN
    BEGIN ERROR(31) ; CTPTR := UNDECPTR END ;
    CASE CTPTR^.KLASS OF
TYPES: BEGIN ERROR(45) ; GATTR.TYPTR := NIL ; INSYMBOL END ;
KONST: WITH GATTR, CTPTR^ DO
         BEGIN TYPTR := CONTYPE ;
           IF CONKIND = ACTUAL THEN
           BEGIN IF ABS(VALUES) >= TWOTO17 THEN
             BEGIN LDCST(VALUES) ; KIND := LVAL ; CTERM := 0 END ELSE
             BEGIN KIND := SVAL ; VAL := VALUES END
           END ELSE (*FORMAL*)
           BEGIN KIND := VARBL ; ACCESS := DRCT ;
             BREG := CLEV ; DPLMT := CADDR ; PCKD := FALSE ;
           END ;
           INSYMBOL
         END ;
PROC : BEGIN INSYMBOL ;
         IF (CTPTR^.PROCTYPE = NIL) OR (CTPTR^.PROCTYPE = CTPTR) THEN
         BEGIN ERROR(46) ; GATTR.TYPTR := NIL END ELSE
         IF NO <> 9 THEN (*(*)
         BEGIN ERROR(79) ; GATTR.TYPTR := NIL END ELSE
         IF CTPTR <= PREDEFP THEN                           (***INLINE FCT***)
         BEGIN LPTR := CTPTR ; INSYMBOL ; EXPRESSION ;
           IF GATTR.TYPTR <> NIL THEN
           CASE LPTR^.SEGSIZE OF
 (*ODD*) 1: BEGIN IF GATTR.TYPTR^.FORM <> NUMERIC THEN ERROR(47) ;
              TRANSFER(GATTR,RP) ;
              IF GATTR.TYPTR^.MIN >= 0 THEN GEN15(20B,RP,0,59) ELSE
              BEGIN GEN15(10B,0,RP,0) ; GEN15(20B,0,0,59) ;
                GEN15(13B,RP,RP,0) ;
              END ;
              WITH GATTR DO
              BEGIN TYPTR := BOOLPTR ; KIND := LCOND ; JMP := 2 ;
                ARITH := TRUE
              END
            END ;
 (*INT*) 2: BEGIN IF GATTR.TYPTR^.FORM > POWER THEN ERROR(44) ;
              GATTR.TYPTR := INTPTR
            END ;
 (*CHR*) 3: BEGIN IF GATTR.TYPTR^.FORM <> NUMERIC THEN ERROR(47) ;
              IF ASSCHECK THEN
              BEGIN TRANSFER(GATTR,RP) ;
                WITH GATTR DO
                BEGIN KIND := LVAL ; CTERM := 0 END ;
                CHECKBNDS(RP,0,63,ASSERR)
              END ;
              GATTR.TYPTR := CHARPTR
            END ;
 (*EOF*) 4: BEGIN IF GATTR.TYPTR^.FORM <> FILES THEN ERROR(44) ;
              WITH GATTR DO
              BEGIN DPLMT := DPLMT + 1 ; TRANSFER(GATTR,RP) ;
                TYPTR := BOOLPTR ; KIND := LCOND ; JMP := 2 ;
                ARITH := TRUE
              END
            END ;
 (*ABS*) 5: BEGIN IF (GATTR.TYPTR <> REALPTR) AND (GATTR.TYPTR^.FORM <>
              NUMERIC) THEN ERROR(44) ;
              TRANSFER(GATTR,RP) ; GEN15(10B,0,RP,0) ;
              GEN15(21B,0,0,73B) ; GEN15(13B,RP,0,RP) ;
              WITH GATTR DO
              BEGIN IF TYPTR <> REALPTR THEN TYPTR := INTPTR ;
                KIND := LVAL ; CTERM := 0
              END
            END ;
 (*SQR*) 6: BEGIN IF (GATTR.TYPTR <> REALPTR) AND (GATTR.TYPTR^.FORM <>
              NUMERIC) THEN ERROR(44) ;
              TRANSFER(GATTR,RP) ;
              WITH GATTR DO
              BEGIN IF TYPTR = REALPTR THEN MULTCODE(RP) ELSE
                BEGIN GEN15(42B,RP,RP,RP) ; TYPTR := INTPTR END ;
                KIND := LVAL ; CTERM := 0 ;
              END
            END ;
 (*TRC*) 7: BEGIN IF GATTR.TYPTR <> REALPTR THEN ERROR(44) ;
              TRANSFER(GATTR,RP) ;
              GEN15(26B,RP,7,RP) ; GEN15(22B,RP,7,RP) ;
              GEN15(13B,0,0,0) ; GEN15(36B,RP,RP,0) ;
              WITH GATTR DO
              BEGIN TYPTR := INTPTR ; KIND := LVAL ; CTERM := 0 END
            END ;
 (*PRE*) 8,
 (*SUC*) 9: IF (GATTR.TYPTR^.FORM > SYMBOLIC) OR (GATTR.TYPTR =
            REALPTR) OR (GATTR.TYPTR = ALFAPTR) THEN
            BEGIN ERROR(44) ; GATTR.TYPTR := NIL END ELSE
            BEGIN TRANSFER(GATTR,RP) ;
              WITH GATTR.TYPTR^ DO
              IF FORM = NUMERIC THEN
              BEGIN IT1 := MIN ; IT2 := MAX END ELSE
              BEGIN IT1 := 0 ; IT2 := FCONST^.VALUES END ;
              IT := LPTR^.SEGSIZE ;
              IF (ABS(IT1) >= TWOTO17) OR (ABS(IT2) >= TWOTO17) THEN
              BEGIN GEN30(71B,0,0,1) ;
                GEN15(47B-IT,RP,RP,0) ;   (*36B OR 37B*)
              END ELSE GEN30(72B,RP,RP,2*IT-17) ;  (*+1 OR -1*)
              IF ASSCHECK AND (GATTR.TYPTR <> INTPTR) THEN
              CHECKBNDS(RP,IT1,IT2,ASSERR) ;
              WITH GATTR DO
              BEGIN KIND := LVAL ; CTERM := 0 END
            END
          END (*CASE LPTR^.SEGSIZE OF*) ;
          IF NO <> 10 THEN (*)*)
          BEGIN ERROR(48) ; GATTR.TYPTR := NIL END ELSE INSYMBOL
        END (*IF CTPTR <= PREDEFP*) ELSE                  (***EXTERNAL FCT***)
        BEGIN FOR IT := 1 TO RP DO                        (***USERDEF. FCT***)
          BEGIN BXIXJ(6,IT) ; GEN30(51B,6,5,LC+TCT) ;
            TCT := TCT + 1 ; IF TCT > TMAX THEN TMAX := TCT ;
          END ;
          IF CTPTR <= EXTPTR THEN  (* SET BIT TO INFORM MONITOR *)
          WITH PARMLIST DO EXTFLAGS := EXTFLAGS+[CTPTR^.SEGSIZE] ;
          LRP := RP ; LB6 := B6DPL ; LPTR := CTPTR^.PROCTYPE ;
          IF LB6 <> 3 THEN GEN30(61B,6,6,LB6) ; RP := 0 ;
          PASSPARAMS ;
          IF LB6 <> 3 THEN GEN30(61B,6,6,-LB6) ;
          B6DPL := LB6 ;
          FOR IT := LRP DOWNTO 1 DO
          BEGIN TCT := TCT - 1 ; GEN30(51B,IT,5,LC+TCT) END ;
          IF LRP = 5 THEN ERROR(33) ELSE
          BEGIN RP := LRP + 1 ;
            IF LB6 = 3 THEN IT := 2 ELSE IT := LB6 + 2 ;
            GEN30(51B,RP,6,IT) ;
          END ;
          WITH GATTR DO
          BEGIN TYPTR := LPTR ; KIND := LVAL ; CTERM := 0 END
        END
      END ;
VARS ,
FIELD : VARIABLE ;
    END (*CASE CTPTR^.KLASS OF*) ;
  END (*IF NO = 1*) ELSE
  IF NO = 2 THEN                                                (*CONSTANT*)
  BEGIN WITH GATTR DO
    BEGIN IF ABS(IVAL) >= TWOTO17 THEN
      BEGIN LDCST(IVAL) ; KIND := LVAL ; CTERM := 0 END ELSE
      BEGIN KIND := SVAL ; VAL := IVAL END ;
      CASE CL OF
     1: TYPTR := INTPTR  ;
     2: TYPTR := REALPTR ;
     3: TYPTR := ALFAPTR ;
     4: TYPTR := CHARPTR ;
      END ;
    END ;
    INSYMBOL ;
  END (*IF NO = 2*) ELSE
  IF NO = 36 THEN                                                  (*NIL*)
  BEGIN WITH GATTR DO
    BEGIN TYPTR := NILPTR ; KIND := SVAL ; VAL := NILVAL END ;
    INSYMBOL
  END ELSE
  IF NO = 5 THEN                                                   (*NOT*)
  BEGIN INSYMBOL ; FACTOR ;
    WITH GATTR DO
    IF TYPTR <> NIL THEN
    IF TYPTR = BOOLPTR THEN
    BEGIN IF KIND = LCOND THEN
      BEGIN IF JMP <= 1 THEN JMP := 1 - JMP ELSE JMP := 5 - JMP ;
      END ELSE
      BEGIN TRANSFER(GATTR,RP) ; GEN15(43B,0,0,59) ;
        GEN15(17B,RP,0,RP) ; KIND := LVAL ; CTERM := 0
      END
    END ELSE ERROR(50)
  END (*IF NO = 5*) ELSE
  IF NO = 9 THEN                                                   (*(*)
  BEGIN INSYMBOL ; EXPRESSION ;
    IF NO <> 10 THEN (*)*)
    BEGIN ERROR(48) ; GATTR.TYPTR := NIL END ELSE INSYMBOL
  END ELSE
  IF NO = 11 THEN                                                  (*[*)
  BEGIN INSYMBOL ;
    IF NO = 12 THEN (*]*)
    WITH GATTR DO
    BEGIN TYPTR := LAMPTR ; KIND := SVAL ;
      VAL := 0 ; INSYMBOL
    END ELSE
    BEGIN WITH LATTR DO
      BEGIN TYPTR := NIL ; KIND := SVAL END ; LPSVAL := 0 ;
      ELEMENT ;
      WHILE NO = 15 DO (*,*)
      BEGIN INSYMBOL ; ELEMENT END ;
      IF LPSVAL <> 0 THEN
      IF LPSVAL >= TWOTO17 THEN
      BEGIN LDCST(LPSVAL) ;
        IF LATTR.KIND = LVAL THEN
        BEGIN RP := RP - 1 ; GEN15(12B,RP,RP,RP+1) END ELSE
        LATTR.KIND := LVAL ; LATTR.CTERM := 0
      END ELSE
