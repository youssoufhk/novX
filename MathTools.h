#pragma once

#include <mkl.h>
#include <vector>
#include <functional>  // std::function, std::multiplies
#include <numeric>    // std::accumulate, std::adjacent_difference
#include <algorithm> // std::lower_bound
#include <iterator> // std::back_inserter
#include <unordered_map>
#include <variant>
#include <string>
#include <chrono>

template <typename T>
std::vector<T> flatten(
	std::vector<std::vector<T>> const& inputMatrix
)
{
	std::vector<T> outputVector;

	for (std::vector<T> const& inputVectorElement : inputMatrix)
	{
		outputVector.insert(outputVector.end(), inputVectorElement.begin(), inputVectorElement.end());
	}

	return outputVector;
}

template <typename T>
std::vector<std::vector<T>> unflatten(
	std::vector<T> const& inputVector,
	size_t const matrixColumnSize
)
{
	size_t const matrixRowSize = (size_t)(inputVector.size() / matrixColumnSize);
	
	std::vector<std::vector<T>> outputMatrix(matrixRowSize, std::vector<T>(matrixColumnSize));

	for (size_t i = 0; i < matrixRowSize; i++)
	{
		for (size_t j = 0; j < matrixColumnSize; j++)
		{
			outputMatrix[i][j] = inputVector[i * matrixColumnSize + j];
		}
	}

	return outputMatrix;
}

template <typename T>
std::vector<T> linspace(
    T const& start,
    T const& end,
    size_t const size = 1
){

    std::vector<T> linspaced_vector(size, start);
    if (size == 1 && start == end)
    {
        return linspaced_vector;
    }
    else if (size == 1 && start != end)
    {
        linspaced_vector.push_back(end);
        return linspaced_vector;
    }

    T increment = (end - start) / static_cast<T>(size - 1);
    typename std::vector<T>::iterator step;
    T value;

    for (step = linspaced_vector.begin(), value = start;
        step != linspaced_vector.end();
        ++step, value += increment)
        *step = value;
    
    return linspaced_vector;
}

template <typename T, typename U = T>
T linearInterpolation(
    U const& value,
    std::vector<U> const& xAxis,
    std::vector<T> const& yAxis,
    bool loglinear = false
)
{
    U x1{}, x2{};
    T y1{}, y2{};

    auto finder = std::lower_bound(xAxis.begin(), xAxis.end(), value);
    int index = std::distance(xAxis.begin(), finder);

    if (finder != xAxis.end() && finder != xAxis.begin())
    {
        x1 = xAxis[index - 1];
        x2 = xAxis[index];

        y1 = yAxis[index - 1];
        y2 = yAxis[index];
    }

    if (value <= xAxis.front())
    {
        return yAxis.front();
    }

    if (value >= xAxis.back())
    {
        return yAxis.back();
    }

    if (loglinear)
    {
        return pow(y1, (x2 - value) / (x2 - x1)) * pow(y2, (value - x1) / (x2 - x1));
    }

    return y1 + (value - x1) * (y2 - y1) / (x2 - x1);
}

enum InterpolationType
{
    LINEAR_ON_Y,               //= linear on zc rates if Y is interest rates
    LOGLINEAR_ON_Y,            //= loglinear on zc rates if Y is interest rates
    LINEAR_ON_EXP_X_TIMES_Y,   //= linear on zc price if Y is interest rates
    LOGLINEAR_ON_EXP_X_TIMES_Y //= loglinear on zc price if Y is interest rates
};

template <typename T, typename U = T>
T interpolate(
    U const& xToInterpolate,
    std::vector<U> const& xAxis,
    std::vector<T> const& yAxis,
    InterpolationType m_interpolationMethod = InterpolationType::LINEAR_ON_Y
)
{
    if (m_interpolationMethod == LINEAR_ON_Y)
    {
        return linearInterpolation<T, U>(xToInterpolate, xAxis, yAxis);
    }
    if (m_interpolationMethod == LOGLINEAR_ON_Y)
    {
        return linearInterpolation<T, U>(xToInterpolate, xAxis, yAxis, true); // applying the log linear interpolation by using loglinear = true as last parameter
    }
    if (m_interpolationMethod == LINEAR_ON_EXP_X_TIMES_Y)
    {
        std::vector<U> vdExpXY = yAxis;
        std::transform(xAxis.begin(), xAxis.end(), yAxis.begin(), vdExpXY.begin(),
            [](U const& x, T const& y)
            {
                return exp(-x * y);
            }); // in order to do something like vdExpXY = exp(-xAxis * yAxis); before applying the linear interpolation to the ExpXY

        T interpolated_price = linearInterpolation<T, U>(xToInterpolate, xAxis, vdExpXY);

        if (xToInterpolate > 0)
        {
            return -log(interpolated_price) / xToInterpolate;
        }
        else
        {
            return 0;
        }
    }
    if (m_interpolationMethod == LOGLINEAR_ON_EXP_X_TIMES_Y)
    {
        std::vector<T> vdExpXY = yAxis;
        std::transform(xAxis.begin(), xAxis.end(), yAxis.begin(), vdExpXY.begin(),
            [](U const& x,T const& y)
            {
                return exp(-x * y);
            }); // in order to do something like vdExpXY = exp(-xAxis * yAxis); before applying the log linear interpolation to the ExpXY

        T interpolated_price = linearInterpolation<T, U>(xToInterpolate, xAxis, vdExpXY, true);

        if (xToInterpolate > 0)
        {
            return -log(interpolated_price) / xToInterpolate;
        }
        else
        {
            return 0;
        }
    }

    return 0;
}

template <typename T>
std::vector<T> mklSystemSolver(
    std::vector<std::vector<T>> const& inputMatrix,
    std::vector<T> const& inputVector
)
{
    int const m = (int)inputMatrix.size();
    int const n = (int)inputVector.size();
    int const nrhs = 1;
    int info;
    std::vector<int> ipiv(m * n);
    std::vector<T> inputMatrixFlattened = flatten<T>(inputMatrix);
    std::vector<T> outputVector = inputVector;

    dgetrf(&m, &n, inputMatrixFlattened.data(), &m, ipiv.data(), &info);

    dgetrs("N", &n, &nrhs, inputMatrixFlattened.data(), &m, ipiv.data(), outputVector.data(), &n, &info);

    return outputVector;
}

template <typename T>
std::vector<std::vector<T>> mklCholesky(
    std::vector<std::vector<T>> const& inputMatrix
)
{
    int const m = (int)inputMatrix.size();
    int const n = (int)inputMatrix[0].size();
    int info;
    std::vector<int> ipiv(m * n);
    std::vector<T> inputMatrixFlattened = flatten<T>(inputMatrix);
    
    dpotrf("U", &n, inputMatrixFlattened.data(), &m, &info);
    
    return unflatten(inputMatrixFlattened, n);
}

template <typename T>
std::vector<std::vector<T>> computeJacobian(
    std::vector<T>& xVariable, 
    std::function<std::vector<T>(std::vector<T>)> objectiveFunction
)
{
    double h = 1E-8;
    size_t xSize = xVariable.size();
    std::vector<std::vector<T>> jacobian(xSize);

    std::vector<T> function = objectiveFunction(xVariable);
    std::vector<T> shockedFunction(xSize);
    std::vector<T> derivative(xSize);
    std::vector<T> shockedVariable = xVariable;

    for (size_t i = 0; i < xVariable.size(); i++)
    {
        shockedVariable[i] = xVariable[i] + h; // we will be shocking only the index i
        shockedFunction = objectiveFunction(shockedVariable);
        shockedVariable[i] = xVariable[i]; // back to normal in order not to affect next iteration

        vdSub(xSize, shockedFunction.data(), function.data(), derivative.data());
        cblas_dscal(xSize, 1 / h, derivative.data(), 1);

        jacobian[i] = derivative;
    }

    return jacobian;
}

template <typename T>
void multivariateNewtonRaphson(
    std::vector<T>& xVariable,
    std::function<std::vector<T>(std::vector<T>)> objectiveFunction,
    double tolerance = 1E-16, int maxIterations = 100
)
{
    double error = 1E10;
    size_t xSize = xVariable.size();
    std::vector<T> vTarget;
    std::vector<std::vector<T>> mJacobian;
    std::vector<T> vError(xSize);

    for (int i = 0; i < maxIterations && error > tolerance; i++)
    {
        vTarget = objectiveFunction(xVariable);
        mJacobian = computeJacobian<T>(xVariable, objectiveFunction);
        vError = mklSystemSolver<T>(mJacobian, vTarget);

        vdSub(xSize, xVariable.data(), vError.data(), xVariable.data());
        // xVariable -= vError

        error = std::accumulate(vError.begin(), vError.end(), 0.,
            [](const auto& a, const auto& b) { return a + b * b; });

        std::cout << "iteration " << i << ", error value = " << error << "\n";
    }

}


template <typename T, class F>
T integral(F f, T a, T b, int n = 1E3, bool trapezoidal = false)
{
    T x = a;
    T dx = (b - a) / n;
    T integral = 0;

    if (trapezoidal)
    {
        for (size_t i = 0; i < n; i++)
        {
            integral += (f(x) + f(x + dx)) / 2;
            x += dx;
        }
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            integral += f(x);
            x += dx;
        }
    }

    return integral * dx;

}

