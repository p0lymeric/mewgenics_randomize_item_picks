cmake -S . -B build -A x64

pushd build
cmake --build . --config RelWithDebInfo --parallel
cmake --install . --config RelWithDebInfo --prefix=..\..\
popd

pause
