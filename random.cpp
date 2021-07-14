#include <random>

std::random_device rd;
std::mt19937 mt(rd());

//生成一个0到max-1的随机数
int randomInt(int exclusiveMax)
{
	std::uniform_int_distribution<> dist(0, exclusiveMax - 1);
	return dist(mt);
}

//生成一个从min到max的随机数
int randomInt(int min, int max)
{
	std::uniform_int_distribution<> dist(0, max - min);
	return dist(mt) + min;
}

//按概率返回一个一个布尔值
bool randomBool(double probability = 0.5)
{
	std::bernoulli_distribution dist(probability);
	return dist(mt);
}