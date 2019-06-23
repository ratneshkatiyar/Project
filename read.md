// Install libjpeg first

//code usage

mkdir build
cd build
cmake ..
make

//code run 

./testQt ../test.jpg out.jpg


// Algorithm

1) Read the file, extract heght and width from header
2) use libjpeg library to convert jpgeg image to RGB24 data // can be optimized further to direct generate YUV4:2:0 data
3) convert RGB24 to YUV4:2:0 data
4) Belnding 
5) Write back to jpeg file


// Future cases (needs to be done)
// code can be optimized in lot of manner
// no error checking done for malloc
// no google test case(gtest)/ unit test case 
