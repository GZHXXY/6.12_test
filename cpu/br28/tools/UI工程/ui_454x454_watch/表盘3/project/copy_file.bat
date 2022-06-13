@echo off

..\..\..\UITools\style_table -stylefile project.bin -prj 0x1
copy .\project.bin ..\..\..\..\ui_resource\watch3.sty
copy .\result.bin  ..\..\..\..\ui_resource\watch3.res
copy .\result.str  ..\..\..\..\ui_resource\watch3.str
copy .\version.txt  ..\..\..\..\ui_resource\watch3.json
..\..\..\UITools\redefined -infile ename.h -prefix watch3 -outfile rename.h
if exist "..\..\..\..\..\..\..\apps\watch\include\ui\" (
    copy .\rename.h     ..\..\..\..\..\..\..\apps\watch\include\ui\style_watch3.h
)

exit
