(* ---------------------------------------------------------------- the request *)
BEGIN
   IF NOT INITED THEN
   BEGIN
      INITED := TRUE;  XID := 5329152;  SPIN := 0;  CONSOLE := 0;  EDITED := 0;  LBASEN := 0;  INITTABLES;  INITCMDS;
      IF REG[9] MOD 128 <> 0 THEN SRVIP[3] := REG[9] MOD 128;   (* DIP switches 0-6: the server's host number *)
      NETOK := NETSTART(0);
      RDOK := REG[12] > 0;                        (* a ramdisk in the build: it is used when there is no network *)
      RAMMODE := RDOK AND NOT NETOK
   END;
   IF MB.OP = 5 THEN
   BEGIN
      MB.RES := 0;                                (* the shell sets it: 1 run the compiler, 2 halt *)
      SHELL
   END
   ELSE IF (CONSOLE = 1) AND ((MB.NAME = NOUTPUT) OR (MB.NAME = NINPUT)) THEN
   BEGIN                                       (* a program that is run: its INPUT and OUTPUT are the keyboard and the screen *)
      MB.RES := 0;
      IF MB.OP = 1 THEN MB.RES := CONGET(0)
      ELSE IF MB.OP = 2 THEN CONPUT(MB.ARG)
   END
   ELSE
   BEGIN
   MB.RES := -2;
   IF NETOK THEN
   BEGIN
      IF MB.OP = 1 THEN K := 1 ELSE K := 2;
      CS := FINDSLOT(K);
      IF CS >= 0 THEN
         CASE MB.OP OF
            1: MB.RES := GETCH(CS);
            2: IF PUTBYTE(CS, TOASC[MB.ARG]) THEN MB.RES := 0;
            3: IF PUTWORD(CS) THEN MB.RES := 0;
            4: IF SKIND[CS] = 1 THEN
               BEGIN
                  MB.RES := 0;  SKIND[CS] := 0;                         (* a file being read: just let go *)
                  IF (CONSOLE = 1) AND (MB.NAME = NPASCLGO) THEN BASEN := 0      (* the program has been read: its files are its own *)
               END
               ELSE IF WRITEPIECE(CS) THEN BEGIN MB.RES := 0;  SKIND[CS] := 0 END
         END
   END
   END
END .
