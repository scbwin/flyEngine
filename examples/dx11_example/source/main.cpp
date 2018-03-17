#include <DX11App.h>
#include <iostream>

int main()
{
  /* auto gauss = [](float x, float sigma) {
    return exp(-x * x / (2.f * sigma));
  };

  float sum = 0.f;
  std::vector<float> weights;
  int num = 4;
  float sigma = 4.f;
  for (int x = -num; x <= num; x++) {
    weights.push_back(gauss(x, sigma));
    sum += weights.back();
  }

  for (auto& w : weights) {
    w /= sum;
  }

  for (const auto& w : weights) {
    OutputDebugStringA((std::to_string(w) + ",").c_str());
  }
  OutputDebugStringA("\n");*/

  DX11App app;
	return app.execute();
}