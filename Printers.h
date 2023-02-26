#pragma once

#include <iostream>
#include <iomanip>
#include <vector>

template <typename T>
void myPrinter(
	std::vector<T> const& inputVector,
	std::string title = "Vector values:",
	size_t preview = 10)
{
	size_t rowIndex = std::min(preview, inputVector.size());
	std::cout << "\n" << title << "\n";
	for (size_t i = 0; i < rowIndex; i++)
	{
		std::cout << std::setw(10) << inputVector[i] << " " << std::fixed;
	}
	std::cout << "\n";
}

template <typename T>
void myPrinter(
	std::vector<std::vector<T>> const& inputMatrix,
	std::string title = "Matrix values:",
	size_t preview = 10
)
{
	size_t rowIndex = std::min(preview, inputMatrix.size());
	std::cout << "\n" << title << "\n";
	for (size_t i = 0; i < rowIndex; i++)
	{
		std::cout << "\033[A\033[A";
		myPrinter(inputMatrix[i], "", preview);

	}
}