@echo off

..\..\..\UITools\style_table -stylefile project.bin -prj 0x0
copy .\project.bin ..\..\..\..\ui_resource\JL.sty
copy .\result.bin  ..\..\..\..\ui_resource\JL.res
copy .\result.str  ..\..\..\..\ui_resource\JL.str
if exist "..\..\..\..\..\..\..\apps\watch\include\ui\" (
    copy .\ename.h     ..\..\..\..\..\..\..\apps\watch\include\ui\style_JL.h
	copy .\result_pic_index.h     ..\..\..\..\..\..\..\apps\watch\include\ui\result_pic_index.h
	copy .\result_str_index.h     ..\..\..\..\..\..\..\apps\watch\include\ui\result_str_index.h
)

exit
