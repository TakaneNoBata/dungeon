#include <random>

std::random_device rd;
std::mt19937 mt(rd());

//����һ��0��max-1�������
int randomInt(int exclusiveMax)
{
	std::uniform_int_distribution<> dist(0, exclusiveMax - 1);
	return dist(mt);
}

//����һ����min��max�������
int randomInt(int min, int max)
{
	std::uniform_int_distribution<> dist(0, max - min);
	return dist(mt) + min;
}

//�����ʷ���һ��һ������ֵ
bool randomBool(double probability = 0.5)
{
	std::bernoulli_distribution dist(probability);
	return dist(mt);
}