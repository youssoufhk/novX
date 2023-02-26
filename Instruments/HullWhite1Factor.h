#pragma once

#include "../MathTools.h"

template <typename T>
void hw1fCalibration(
	std::function<T(T)> zcPrice,
	T hw1fLambda,
	T hw1fEta)
{
	T h = 1E-6;
	auto forwardRate = [](T const& time)
	{
		return -log(zcPrice(time + h) / zcPrice(time - h)) / (2. * h); 
	};

	T initialRate = forwardRate(h);

	auto hw1fTheta = [](T const& time)
	{
		T temp = forwardRate(time) + (forwardRate(time + h) / forwardRate(time - h)) / (2. * h * hw1fLambda);
		temp += hw1fEta * hw1fEta * (1. - exp(-2. * hw1fLambda * time)) / (2. * hw1fLambda * hw1fLambda);
		return temp;
	};

	auto hw1fB = [](T const& time_t, T const& time_T)
	{
		return (exp(-hw1fLambda * (time_T - time_t)) - 1) / hw1fLambda;
	};

	auto hw1fA = [](T const& time_t, T const& time_T)
	{
		T temp1 = (time_T - time_t) + (exp(2 * hw1fLambda * (time_T - time_t)) * (4 * exp(hw1fLambda * (time_T - time_t)) - 1) - 3) / (2 * hw1fLambda);
		temp1 *= hw1fEta * hw1fEta;
		temp1 /= 2 * hw1fLambda * hw1fLambda;

		std::vector<T> time_grid = linspace<T>(time_t, time_T, (size_t)((time_t / time_T + 1) * time_T * 250));

		std::vector<T> integralValues;

		std::transform(
			time_grid.begin(), time_grid.end(),
			std::back_inserter(integralValues),
			[](T const& grid_time)
			{
				return hw1fTheta(grid_time) * hw1fB(grid_time, time_T);
			});

		T temp2;
		temp2 = std::accumulate(integralValues.begin(), integralValues.end(), 0.);
		temp2 *= hw1fLambda;
	};

	auto hw1fZeroCoupon = [](T const& time_t, T const& time_T,std T const& shortRate)
	{
		return exp(hw1fA(time_t, time_T) + hw1fB(time_t, time_T) * shortRate);
	}

}
