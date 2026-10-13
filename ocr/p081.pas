      WITH PT^ DO
      BEGIN NAME := INITNAM[IT] ; NXTEL := NEXT ;
         KLASS := PROC ; PROCTYPE := REALPTR ;
         PROCKIND := ACTUAL ; PROCLEVEL := 0 ;
         FORMALS := CTPTR; PROCADDR := IT + 58;
         IF IT <= 33 THEN SEGSIZE := 0 ELSE SEGSIZE := IT - 33 ;
      END ;
      NEXT := PT
   END ;
   EXTPTR := NEXT ;
   FILPTS[1]^.FELTYPE := CHARPTR;

   DIGITS := ['0','1','2','3','4','5','6','7','8','9'];
   FOR IT := 0 TO 3 DO PARMLIST.ERRNRS[IT] := [ ] ;
   ERRINX := 0;  EOLFLAG := TRUE;
   PARMLIST.ERRFLAG := FALSE ;
   DP := TRUE;  PRCODE := FALSE;
   PMCTR := 0;  LASTLINK := 0;  (*FOR PMDFILE*)
   ASSCHECK := TRUE ; INXCHECK := TRUE ; DIVCHECK := TRUE ;
   STOFLCHECK := TRUE ; ROUNDING := TRUE ;
   KK := ALFALENG;  FILTOP := -1;  JMPIX := 0;  CEXTABIX := 0;
   PTX := 0;  CHNIX := 1;  B6DPL := 3;
   FOR IT := 1 TO UNDMAX-1 DO UNDLAB[IT].SUCC := IT + 1 ;
   UNDLAB[UNDMAX].SUCC := 0 ;
   IC := PARMLIST.ICO + 1 ;
   MAXCTP := LAMPTR;
   PARMLIST.EXTFLAGS := [ ] ;
   WITH DISPLAY[0] DO
   BEGIN FNAME := EXTPTR ; OCCUR := BLOCK END ;
   NO := 0 ; LC := IC;
   NEXTCH;  INSYMBOL;
   REPEAT
      DISPLAY[1].FNAME := NIL;  DISPLAY[1].OCCUR := BLOCK;
      TOP := 1;  LEVEL := 0;  NEXT := NIL;
      ALLOC(PT,DUMMYCLASS);  LC := IC ;  PFTOP := -1;

      (*********************
       *                   *)
      BODY(NIL,PT);  (*  COMPILE USER PROGRAM
       *                   *
       *********************)

      IF (LEVEL = 0) AND (NO <> 17) THEN ERROR(54) ;
   UNTIL NO = 17 (* . *) ;
   WHILE NOT EOLFLAG DO NEXTCH;  IF ERRINX > 0 THEN PRTERR;
   IF PRCODE THEN PRJMPTAB;
   PMDFILE^.PKIND := PROCDR;  PMDFILE^.COUNT := LASTLINK;
   PUT(PMDFILE);  (*LINK TO MAIN PROGRAM ENTRY*)
   WRITE(EOL) ;
   PARMLIST.LIMCODE := IC ;
END .
