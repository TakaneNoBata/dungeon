#pragma once
#include "random.cpp"
#include "list.h"
#include <iostream>
#include <graphics.h>
#include <conio.h>
#include <mysql.h>
#include <utility>
#include <map>
#include <string>

//road
#define Road		std::pair<int,int>(0,0)
//topo_tb
#define ClosedDoor	std::pair<int,int>(1,0)
#define OpenDoor	std::pair<int,int>(1,1)
#define Wall		std::pair<int,int>(1,2)
#define StairsDown	std::pair<int,int>(1,3)
#define StairsUp	std::pair<int,int>(1,4)
//mon_tb
#define Mon0		std::pair<int,int>(2,0)
#define Mon1		std::pair<int,int>(2,1)
//player
#define Player0		std::pair<int,int>(3,0)
//chest_tb
#define Pakellas0	std::pair<int,int>(4,0)
#define Pakellas1	std::pair<int,int>(4,1)
#define Pakellas2	std::pair<int,int>(4,2)
#define Pakellas3	std::pair<int,int>(4,3)

//keyboard defines
#define UP (char)24
#define DOWN 25
#define RIGHT 26
#define LEFT 27

//extern random func
extern int randomInt(int exclusiveMax);
extern int randomInt(int min, int max);
extern bool randomBool(double probability = 0.5);

//tile to image path
std::map<std::pair<int, int>, std::string> imageMap;
void loadImage();



