/VERSION_NUM/ { 
   split ($3, a, ".");
   a[2] = a[2] + 1;
   
   if ( a[2] > 255 )
      a[2] = 0;
}

END {
   printf ("// VERSION_NUM %d.%d\n", a[1], a[2] ) > "version.h";
   printf ("#define VER_MAJ %d\n", a[1] ) >> "version.h";
   printf ("#define VER_MIN %d\n", a[2] ) >> "version.h";
   printf ("#define VER_FORK 0\n") >> "version.h";
   version = "NIMO_VERSION="a[1] "." a[2]
   cmd = "echo " version ">> $GITHUB_ENV"
   system (cmd)
}