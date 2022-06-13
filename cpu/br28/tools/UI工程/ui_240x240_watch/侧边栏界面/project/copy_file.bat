@echo off

..\..\..\UITools\style_table -stylefile project.bin -tablefile sidebar.tab -prj 0x2
copy .\project.bin	..\..\..\..\ui_resource\sidebar.sty
copy .\result.bin  	..\..\..\..\ui_resource\sidebar.res
copy .\result.str  	..\..\..\..\ui_resource\sidebar.str
copy .\sidebar.tab      ..\..\..\..\ui_resource\sidebar.tab
..\..\..\UITools\redefined -infile ename.h -prefix sidebar -outfile rename.h
if exist "..\..\..\..\..\..\..\apps\watch\include\ui\" (
    copy .\rename.h     ..\..\..\..\..\..\..\apps\watch\include\ui\style_sidebar.h
)

exit
