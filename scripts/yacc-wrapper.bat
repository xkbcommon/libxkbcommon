@echo on
rem Run yacc such that the working directory is the source root.
rem This is needed for various reasons (e.g. #line references).
rem Do not use directly.

setlocal

rem Use %%~ to strip existing quotes so we can re-quote them safely later.
set "YACC=%~1"
set "PARSER=%~2"
set "IMPLEMENTATION=%~3"
set "HEADER=%~4"
set "ABS_TOP_SRCDIR=%~5"

rem Check for exactly 5 arguments.
if "%~5"=="" (
    echo Usage: %~0 YACC PARSER IMPLEMENTATION HEADER ABS_TOP_SRCDIR 1>&2
    exit /b 1
)

cd /d "%ABS_TOP_SRCDIR%" || exit /b 1

rem Generate the parser
%YACC% --defines="%HEADER%" ^
       --output="%IMPLEMENTATION%" ^
       --name-prefix=_xkbcommon_ ^
       "%PARSER%"
