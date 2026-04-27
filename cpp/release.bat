@echo off

git status --porcelain | findstr . && set DIRTY=1 || set DIRTY=0
if %DIRTY% equ 1 (
pause
exit
)

set /p TAGVERSION="Version: "

git checkout --detach

git reset --hard
git clean -fdx :/

cmake -S . -B build -A x64

pushd build
cmake --build . --config RelWithDebInfo --parallel
cmake --install . --config RelWithDebInfo --prefix=..\..\
popd

rmdir build\ /s /q

git add -f ..\*.dll
git add -f ..\*.pdb

for /f "delims=" %%i in ('git show -s --format^="'%%s' (%%h)" HEAD') do set COMMITLINE=%%i

git commit -m "Build %COMMITLINE%"

git tag -a "v%TAGVERSION%" -m "%TAGVERSION% release"

git -C ..\ archive --prefix=randomize_item_picks\ -o randomize_item_picks-%TAGVERSION%.zip HEAD

pause
