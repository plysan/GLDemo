GLDemo
======

This is a demo program for my opengl and c++ learning for
rendering terrain

This is only a demo for testing, and probably contains a lot of
ungraceful code, but it can run...

To run the demo, you currently need to solve related library
dependency yourself, and execute:
1. g++ -o draw draw.cpp tools/*.cpp -lGL -ltiff -lGLEW -lglfw -std=c++0x -lpthread
2. ./draw

**Preview of the rendeing for hawaii islands:**
![](https://raw.githubusercontent.com/plysan/GLDemo/master/demo.jpg)

