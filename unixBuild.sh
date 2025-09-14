
if [ ! -d "build" ]; then
  mkdir build
fi

cd build

cmake ../CMakeLists.txt
make

./commieLAB1

cd ..